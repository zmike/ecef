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

static void
browser_buttons_add(Evas_Object *layout, cef_browser_t *browser)
{
   const char *swallows[] =
   {
      "ecef.swallow.back",
      "ecef.swallow.forward",
      "ecef.swallow.refresh",
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
      (Evas_Smart_Cb)browser_refresh,
   };
   int i;

   for (i = 0; i < EINA_C_ARRAY_LENGTH(swallows); i++)
     {
        Evas_Object *o;

        o = button_add(layout, icons[i], NULL, NULL, callbacks[i], browser);
        elm_object_tooltip_text_set(o, tooltips[i]);
        //elm_object_tooltip_style_set(o, "ecef");
        elm_object_style_set(o, "browser_navigation");
        elm_object_part_content_set(layout, swallows[i], o);
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
urlbar_activate(ECef_Client *ec, Evas_Object *obj, void *ev EINA_UNUSED)
{
   cef_frame_t *fr;
   cef_string_t str = {0};
   Eina_Stringshare *url;

   url = elm_entry_entry_get(obj);
   cef_string_from_utf8(url, strlen(url), &str);
   fr = ec->current_page->browser->get_main_frame(ec->current_page->browser);
   fr->load_url(fr, &str);
   cef_string_clear(&str);
}

void
on_after_browser_created(cef_life_span_handler_t *self EINA_UNUSED, cef_browser_t *browser)
{
   static Elm_Genlist_Item_Class browser_itc = {
      .item_style = "default",
      .func = {
           .text_get = (Elm_Genlist_Item_Text_Get_Cb)browser_page_text_get,
           .del = (Elm_Genlist_Item_Del_Cb)browser_page_del
      },
      .version = ELM_GENLIST_ITEM_CLASS_VERSION
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
   b->it = elm_genlist_item_append(ec->pagelist, &browser_itc, b, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
   if (ec->current_page) return;
   /* first browser creation: set up callbacks */
   browser_set(ec, b);
   browser_buttons_add(ec->layout, browser);
   elm_layout_signal_callback_add(ec->layout, "ecef,urlbar,visible", "ecef", urlbar_visible, ec);
   elm_layout_signal_callback_add(ec->layout, "ecef,urlbar,hidden", "ecef", urlbar_hidden, ec);
   evas_object_smart_callback_add(ec->urlbar, "activated", (Evas_Smart_Cb)urlbar_activate, ec);
   evas_object_smart_callback_add(ec->urlbar, "changed,user", (Evas_Smart_Cb)urlbar_changed, ec);
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
browser_refresh(cef_browser_t *browser, ...)
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
   if (b->img)
     elm_object_part_content_set(ec->layout, "ecef.swallow.browser", b->img);
   elm_win_title_set(ec->win, b->title);
   elm_entry_entry_set(ec->urlbar, b->url);
   host = browser_get_host(ec->current_page->browser);
   //host->set_focus(host, 1);
   if (gl_avail) return;
   edje_object_part_geometry_get(elm_layout_edje_get(ec->layout), "ecef.swallow.browser", &x, &y, &w, &h);
   ecore_x_window_move(host->get_window_handle(host), x, y);
   ecore_x_window_show(host->get_window_handle(host));
   ecore_x_netwm_window_state_set(host->get_window_handle(host), state, 0);
}
