#include "ecef.h"
#include <Ecore_X.h>

static char *
browser_page_text_get(Browser *b, Evas_Object *obj EINA_UNUSED, const char *part)
{
   return strdup(b->title ?: "");
}

static void
browser_page_del(Browser *b, Evas_Object *obj EINA_UNUSED)
{
   int id;
   Evas_Object *o;
   ECef_Client *ec;

   ec = browser_get_client(b->browser);
   id = b->browser->get_identifier(b->browser);
   eina_hash_del_by_key(ec->browsers, &id);
   browser_get_host(b->browser)->close_browser(browser_get_host(b->browser), 0);
   EINA_LIST_FREE(b->clones, o)
     evas_object_del(o);
   evas_object_del(b->img);
   free(b);
}

static Evas_Object *
browser_page_content_get(Browser *b, Evas_Object *obj, const char *part)
{
   Evas_Object *ly, *img;

   ly = elm_layout_add(obj);
   elm_layout_theme_set(ly, "layout", "ecef", "dummy");
   if (browser_get_client(b->browser)->current_page == b)
     img = render_image_clone(b);
   else
     img = b->img;
   elm_object_content_set(ly, img);
   return ly;
}

static void
browser_buttons_add(ECef_Client *ec, cef_browser_t *browser)
{
   const char *swallows[] =
   {
      "ecef.swallow.back",
      "ecef.swallow.forward",
      "ecef.swallow.reload",
   };
   const char *icons[] =
   {
      "back",
      "forward",
      "reload"
   };
   const char *tooltips[] =
   {
      "Back",
      "Forward",
      "Reload",
   };
   Evas_Smart_Cb callbacks[] =
   {
      (Evas_Smart_Cb)browser_back,
      (Evas_Smart_Cb)browser_forward,
      (Evas_Smart_Cb)browser_reload,
   };
   Evas_Object **objs[] =
   {
      &ec->back,
      &ec->forward,
      &ec->reload,
   };
   int i;

   for (i = 0; i < EINA_C_ARRAY_LENGTH(swallows); i++)
     {
        Evas_Object *o;

        *objs[i] = o = button_add(ec->win, icons[i], NULL, NULL, callbacks[i], browser);
        elm_object_tooltip_text_set(o, tooltips[i]);
        //elm_object_tooltip_style_set(o, "ecef");
        elm_object_style_set(o, "browser_navigation");
        elm_object_part_content_set(ec->layout, swallows[i], o);
     }
}

static void
urlbar_visible(void *d, Evas_Object *obj EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   ECef_Client *ec = d;

   if (!ec->urlbar_changed)
     elm_object_focus_set(ec->urlbar, 1);
}

static void
urlbar_hidden(void *d, Evas_Object *obj EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   ECef_Client *ec = d;

   ec->urlbar_changed = 0;
}

static void
urlbar_changed(ECef_Client *ec, Evas_Object *obj, void *ev EINA_UNUSED)
{
   /* ensure urlbar does not hide while user is typing */
   if (ec->urlbar_changed)
     browser_urlbar_show(ec, 0);
}

static void
urlbar_activate(ECef_Client *ec, ...)
{
   cef_frame_t *fr;
   cef_string_t str = {0};
   Eina_Stringshare *url;

   url = elm_entry_entry_get(ec->urlbar);
   cef_string_from_utf8(url, strlen(url), &str);
   fr = ec->current_page->browser->get_main_frame(ec->current_page->browser);
   fr->load_url(fr, &str);
   cef_string_clear(&str);
}

static void
pagelist_unrealized(ECef_Client *ec, Evas_Object *obj EINA_UNUSED, Elm_Object_Item *it)
{
   Evas_Object *ly;

   /* only save the content if it's not the current page's clone
    * nobody cares about clones
    */
   if (elm_object_item_data_get(it) != ec->current_page) return;
   ly = elm_object_item_part_content_get(it, "ecef.swallow.view");
   elm_object_content_unset(ly);
}

void
on_after_browser_created(cef_life_span_handler_t *self EINA_UNUSED, cef_browser_t *browser)
{
   static Elm_Gengrid_Item_Class browser_itc = {
      .item_style = "default",
      .func = {
           .content_get = (Elm_Gengrid_Item_Content_Get_Cb)browser_page_content_get,
           .text_get = (Elm_Gengrid_Item_Text_Get_Cb)browser_page_text_get,
           .del = (Elm_Gengrid_Item_Del_Cb)browser_page_del
      },
      .version = ELM_GENGRID_ITEM_CLASS_VERSION
   };
   ECef_Client *ec = browser_get_client(browser);
   int id, w, h;
   Browser *b;
   cef_browser_host_t *host;

   id = browser->get_identifier(browser);
   host = browser_get_host(browser);
   b = calloc(1, sizeof(Browser));
   b->browser = browser;
   edje_object_part_geometry_get(elm_layout_edje_get(ec->layout), "ecef.swallow.browser", NULL, NULL, &w, &h);
   render_image_new(ec, b, host, w, h);
   eina_hash_add(ec->browsers, &id, b);
   b->it = elm_gengrid_item_append(ec->pagelist, &browser_itc, b, NULL, NULL);
   if (ec->current_page) return;
   /* first browser creation: set up callbacks */
   browser_set(ec, b);
   browser_buttons_add(ec, browser);
   elm_layout_signal_callback_add(ec->layout, "ecef,urlbar,visible", "ecef", urlbar_visible, ec);
   elm_layout_signal_callback_add(ec->layout, "ecef,urlbar,hidden", "ecef", urlbar_hidden, ec);
   evas_object_smart_callback_add(ec->urlbar, "activated", (Evas_Smart_Cb)urlbar_activate, ec);
   evas_object_smart_callback_add(ec->urlbar, "changed,user", (Evas_Smart_Cb)urlbar_changed, ec);
   evas_object_smart_callback_add(ec->pagelist, "unrealized", (Evas_Smart_Cb)pagelist_unrealized, ec);
}

void
on_browser_destroyed(cef_render_process_handler_t *self, cef_browser_t *browser)
{
   ECef_Client *ec = browser_get_client(browser);
}

Browser *
browser_get(ECef_Client *ec, cef_browser_t *browser)
{
   int id;

   id = browser->get_identifier(browser);
   return eina_hash_find(ec->browsers, &id);
}

void
browser_new(ECef_Client *ec, const char *url)
{
   cef_string_t u = {0};

   cef_string_from_utf8(url, strlen(url), &u);
   cef_browser_host_create_browser(ec->window_info, &ec->client, &u, ec->browser_settings, NULL);
   cef_string_clear(&u);
}

void
browser_back(cef_browser_t *browser, ...)
{
   browser->go_back(browser);
}

void
browser_forward(cef_browser_t *browser, ...)
{
   browser->go_forward(browser);
}

void
browser_reload(cef_browser_t *browser, ...)
{
   browser->reload(browser);
}

void
browser_urlbar_show(ECef_Client *ec, Eina_Bool changed)
{
   if (changed)
     {
        if (evas_object_visible_get(ec->urlbar)) return;
          elm_layout_signal_emit(ec->layout, "ecef,urlbar,change", "ecef");
     }
   else
     elm_layout_signal_emit(ec->layout, "ecef,urlbar,show", "ecef");
   ec->urlbar_changed = changed;
}

void
browser_urlbar_hide(ECef_Client *ec)
{
   elm_layout_signal_emit(ec->layout, "ecef,urlbar,hide", "ecef");
}

void
browser_urlbar_set(ECef_Client *ec, const char *url)
{
   elm_entry_entry_set(ec->urlbar, url);
   urlbar_activate(ec);
}

void
browser_pagelist_show(ECef_Client *ec)
{
   elm_layout_signal_emit(ec->layout, "ecef,pagelist,show", "ecef");
}

void
browser_pagelist_hide(ECef_Client *ec)
{
   elm_layout_signal_emit(ec->layout, "ecef,pagelist,hide", "ecef");
}

void
browser_window_title_update(ECef_Client *ec)
{
   char buf[4096] = {0};

   if (ec->current_page->title)
     snprintf(buf, sizeof(buf), "%s - Servo", ec->current_page->title);
   else if (ec->current_page->url)
     {
        char *p;

        p = strrchr(ec->current_page->url, '/');
        /* probably image */
        if (p)
          snprintf(buf, sizeof(buf), "%s - Servo", p + 1);
        else
          snprintf(buf, sizeof(buf), "%s - Servo", ec->current_page->url);
     }
   else
     strcpy(buf, "Blank - Servo");
   elm_win_title_set(ec->win, buf);
}

void
browser_set(ECef_Client *ec, Browser *b)
{
   cef_browser_host_t *host;
   int x, y, w, h;
   Ecore_X_Window_State state[1];

   elm_object_part_content_unset(ec->layout, "ecef.swallow.browser");
   if (ec->current_page)
     {
        state[0] = ECORE_X_WINDOW_STATE_HIDDEN;
        if (ec->current_page->img)
          evas_object_hide(ec->current_page->img);
        host = browser_get_host(ec->current_page->browser);
        //host->set_focus(host, 0);
        if (!gl_avail)
          {
             ecore_x_window_hide(host->get_window_handle(host));
             ecore_x_netwm_window_state_set(host->get_window_handle(host), state, 1);
          }
     }
   ec->current_page = b;
   elm_object_part_content_set(ec->layout, "ecef.swallow.browser", b->img);
   evas_object_size_hint_aspect_set(b->img, EVAS_ASPECT_CONTROL_NONE, -1, -1);
   browser_window_title_update(ec);
   elm_entry_entry_set(ec->urlbar, b->url);
   host = browser_get_host(ec->current_page->browser);
   //host->set_focus(host, 1);
   if (gl_avail) return;
   edje_object_part_geometry_get(elm_layout_edje_get(ec->layout), "ecef.swallow.browser", &x, &y, &w, &h);
   ecore_x_window_move(host->get_window_handle(host), x, y);
   ecore_x_window_show(host->get_window_handle(host));
   ecore_x_netwm_window_state_set(host->get_window_handle(host), state, 0);
}
