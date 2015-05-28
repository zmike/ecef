#include "ecef.h"
#include <Ecore_X.h>

static void
browser_resize(ECef_Client *ec, ...)
{
   cef_browser_host_t *host;

   if (!ec->current_page) return;
   host = browser_get_host(ec->current_page->browser);
   if (!windowed)
     host->was_resized(host);
   else
     {
        int x, y, w, h;

        if (ec->current_page->swapping)
          {
             ec->need_resize = 1;
             return;
          }
        edje_object_part_geometry_get(elm_layout_edje_get(ec->layout), "ecef.swallow.browser", &x, &y, &w, &h);
        ecore_x_window_move_resize(host->get_window_handle(host), x, y, w, h);
     }
}

static char *
browser_page_text_get(Browser *b, Evas_Object *obj EINA_UNUSED, const char *part)
{
   return strdup(browser_page_string_get(b));
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
   if (!elm_gengrid_items_count(ec->pagelist))
     elm_layout_signal_emit(ec->layout, "ecef,pages,unavailable", "ecef");
   browser_get_host(b->browser)->close_browser(browser_get_host(b->browser), 0);
   EINA_LIST_FREE(b->clones, o)
     evas_object_del(o);
   evas_object_del(b->img);
   free(b);
}

static Evas_Object *
browser_page_content_get(Browser *b, Evas_Object *obj, const char *part)
{
   return b->it_clone = render_image_clone(b);
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
urlbar_visible(void *d, ...)
{
   ECef_Client *ec = d;

   if (!ec->urlbar_changed)
     {
        elm_object_focus_set(ec->urlbar, 1);
        elm_entry_select_all(ec->urlbar);
     }
   ec->urlbar_visible = 1;
}

static void
urlbar_hidden(void *d, Evas_Object *obj EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   ECef_Client *ec = d;

   ec->urlbar_visible = ec->urlbar_changed = 0;
}

static void
urlbar_changed(ECef_Client *ec, Evas_Object *obj, void *ev EINA_UNUSED)
{
   /* ensure urlbar does not hide while user is typing */
   if (ec->urlbar_changed)
     browser_urlbar_show(ec, 0);
}


//ref http://mxr.mozilla.org/mozilla-central/source/netwerk/base/nsURLHelper.cpp#487
static size_t
urlbar_scheme_parse(const char *url)
{
   const char *p;
   size_t len = 0;

   for (p = url; p[0]; p++)
     {
        if (!len)
          {
             if (isalpha(p[0]))
               len++;
             else
               return 0;
          }
        else
          {
             if (isalnum(p[0]) || (p[0] == '+') || (p[0] == '.') || (p[0] == '-'))
               len++;
             else if (p[0] == ':')
               return len;
             else
               return 0;
          }
     }
   return 0;
}

static void
urlbar_activate(ECef_Client *ec, ...)
{
   cef_frame_t *fr;
   cef_string_t str = {0};
   Eina_Stringshare *url;
   Eina_Strbuf *buf;
   char *s, *p, *r;
   size_t len;

   url = elm_entry_entry_get(ec->urlbar);
   buf = eina_strbuf_new();
   eina_strbuf_append(buf, url);

   //ref http://mxr.mozilla.org/mozilla-central/source/docshell/base/nsDefaultURIFixup.cpp
   eina_strbuf_trim(buf);

   /* remove embedded newlines */
   len = eina_strbuf_length_get(buf);
   s = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   for (r = p = s; r[0]; r++, p++)
     {
        while (r[0] && ((r[0] == '\r') || (r[0] == '\n')))
          r++, len--;
        p[0] = r[0];
     }
   if (!len) return;
   buf = eina_strbuf_manage_new_length(s, len);
   len = urlbar_scheme_parse(s);
   if (len)
     {
        //fixups for scheme typos
        switch (len)
          {
           case 2:
             if (!strncmp(s, "le", 2)) // "file"
               eina_strbuf_prepend(buf, "fi");
             if (!strncmp(s, "ps", 2)) // "https"
               eina_strbuf_prepend(buf, "htt");
             break;
           case 3:
             if (!strncmp(s, "ttp", 3)) // "http"
               eina_strbuf_prepend_char(buf, 'h');
             else if (!strncmp(s, "tps", 3)) // "https"
               eina_strbuf_prepend(buf, "ht");
             else if (!strncmp(s, "ile", 3)) // "file"
               eina_strbuf_prepend_char(buf, 'f');
             break;
           case 4:
             if (!strncmp(s, "ttps", 4)) // "https"
               eina_strbuf_prepend_char(buf, 'h');
          }
     }
   else //invalid scheme
     {
#ifdef WINDOWS //not a real define
        if (strchr(s, '\\') || (
#else
        if (s[0] == '/')
#endif
          eina_strbuf_prepend(buf, "file://");
     }
   if (!strncmp(eina_strbuf_string_get(buf), "://", 3))
     eina_strbuf_remove(buf, 3, 0);
   else if (!strncmp(eina_strbuf_string_get(buf), "//", 2))
     eina_strbuf_remove(buf, 2, 0);
   if (!urlbar_scheme_parse(eina_strbuf_string_get(buf)))
     eina_strbuf_prepend(buf, "http://");
   len = eina_strbuf_length_get(buf);
   s = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   if (urlbar_scheme_parse(s))
     {
        //yay a valid scheme!
        if (ec->current_page)
          {
             cef_string_from_utf8(s, len, &str);
             fr = ec->current_page->browser->get_main_frame(ec->current_page->browser);
             fr->load_url(fr, &str);
             cef_string_clear(&str);
          }
        else
          browser_new(ec, s, 1, NULL, NULL);
     }
   free(s);
}

static void
pagelist_activated(ECef_Client *ec, Evas_Object *obj EINA_UNUSED, Elm_Object_Item *it)
{
   Browser *b;

   b = elm_object_item_data_get(it);
   browser_swap(ec, b, b->it_clone);
}

static void
pagelist_unrealized(ECef_Client *ec, Evas_Object *obj EINA_UNUSED, Elm_Object_Item *it)
{
   Browser *b;

   b = elm_object_item_data_get(it);
   b->it_clone = NULL;
}

static void
pagelist_visible(void *d, Evas_Object *obj EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   ECef_Client *ec = d;
   Eina_Iterator *it;
   Browser *b;

   ec->pagelist_visible = 1;
   elm_object_focus_set(ec->pagelist, 1);
   if ((!gl_avail) || windowed) return;
   it = eina_hash_iterator_data_new(ec->browsers);
   EINA_ITERATOR_FOREACH(it, b)
     elm_glview_render_policy_set(b->img, ELM_GLVIEW_RENDER_POLICY_ALWAYS);
   eina_iterator_free(it);
}

static void
pagelist_hidden(void *d, Evas_Object *obj EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   ECef_Client *ec = d;
   Eina_Iterator *it;
   Browser *b;

   ec->pagelist_visible = 0;
   if ((!gl_avail) || windowed) return;
   it = eina_hash_iterator_data_new(ec->browsers);
   EINA_ITERATOR_FOREACH(it, b)
     elm_glview_render_policy_set(b->img, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
   eina_iterator_free(it);
}

static void
page_swapped(void *d, Evas_Object *obj EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   ECef_Client *ec = d;

   if (ec->need_resize)
     browser_resize(ec);
   ec->current_page->swapping = 0;
   evas_object_del(elm_object_part_content_unset(ec->layout, "ecef.swallow.swap"));
   elm_object_part_content_set(ec->layout, "ecef.swallow.browser", ec->current_page->img);
}

void
browser_page_item_add(ECef_Client *ec, Browser *b)
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
   b->it = elm_gengrid_item_append(ec->pagelist, &browser_itc, b, NULL, NULL);
}

void
on_after_browser_created(cef_life_span_handler_t *self EINA_UNUSED, cef_browser_t *browser)
{
   ECef_Client *ec = browser_get_client(browser);
   int id, w, h;
   Browser *b;
   cef_browser_host_t *host;
   Eina_Bool first;
   Browser_Created_Cb cb;
   void *cbdata;

   id = browser->get_identifier(browser);
   host = browser_get_host(browser);
   b = calloc(1, sizeof(Browser));
   b->browser = browser;
   edje_object_part_geometry_get(elm_layout_edje_get(ec->layout), "ecef.swallow.browser", NULL, NULL, &w, &h);
   render_image_new(ec, b, host, w, h);
   eina_hash_add(ec->browsers, &id, b);
   first = (eina_hash_population(ec->browsers) == 1) && (!ec->current_page);
   if (eina_list_data_get(ec->pending_pages))
     {
        browser_page_item_add(ec, b);
        browser_set(ec, b);
     }
   ec->pending_pages = eina_list_remove_list(ec->pending_pages, ec->pending_pages);
   cb = eina_list_data_get(ec->create_cbs);
   ec->create_cbs = eina_list_remove_list(ec->create_cbs, ec->create_cbs);
   cbdata = eina_list_data_get(ec->create_datas);
   ec->create_datas = eina_list_remove_list(ec->create_datas, ec->create_datas);
   if (cb) cb(cbdata, b);
   if (!first) return;
   /* first browser creation: set up callbacks */
   eina_log_domain_level_set("evas_main", EINA_LOG_LEVEL_ERR);
   browser_buttons_add(ec, browser);
   evas_object_event_callback_add(ec->layout, EVAS_CALLBACK_RESIZE, (Evas_Object_Event_Cb)browser_resize, ec);
   elm_layout_signal_callback_add(ec->layout, "ecef,urlbar,visible", "ecef", (Edje_Signal_Cb)urlbar_visible, ec);
   elm_layout_signal_callback_add(ec->layout, "ecef,urlbar,hidden", "ecef", urlbar_hidden, ec);
   elm_layout_signal_callback_add(ec->layout, "ecef,pagelist,visible", "ecef", pagelist_visible, ec);
   elm_layout_signal_callback_add(ec->layout, "ecef,pagelist,hidden", "ecef", pagelist_hidden, ec);
   elm_layout_signal_callback_add(ec->layout, "ecef,browser,swapped", "ecef", page_swapped, ec);
   evas_object_smart_callback_add(ec->urlbar, "activated", (Evas_Smart_Cb)urlbar_activate, ec);
   evas_object_smart_callback_add(ec->urlbar, "changed,user", (Evas_Smart_Cb)urlbar_changed, ec);
   evas_object_smart_callback_add(ec->pagelist, "unrealized", (Evas_Smart_Cb)pagelist_unrealized, ec);
   evas_object_smart_callback_add(ec->pagelist, "activated", (Evas_Smart_Cb)pagelist_activated, ec);
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
browser_new(ECef_Client *ec, const char *url, Eina_Bool pending, Browser_Created_Cb cb, void *data)
{
   cef_string_t u = {0};

   cef_string_from_utf8(url, strlen(url), &u);
   ec->pending_pages = eina_list_append(ec->pending_pages, (void*)(size_t)pending);
   ec->create_cbs = eina_list_append(ec->create_cbs, cb);
   ec->create_datas = eina_list_append(ec->create_datas, data);
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
     {
        if (ec->urlbar_visible)
          urlbar_visible(ec);
        else
          elm_layout_signal_emit(ec->layout, "ecef,urlbar,show", "ecef");
     }
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
   if (elm_gengrid_items_count(ec->pagelist))
     elm_layout_signal_emit(ec->layout, "ecef,pagelist,show", "ecef");
}

void
browser_pagelist_hide(ECef_Client *ec)
{
   elm_layout_signal_emit(ec->layout, "ecef,pagelist,hide", "ecef");
}

char *
browser_page_string_get(Browser *b)
{
   static char buf[4096];

   if (b->title)
     snprintf(buf, sizeof(buf), "%s", b->title);
   else if (b->url)
     {
        char *p;

        p = strrchr(b->url, '/');
        /* probably image */
        if (p)
          snprintf(buf, sizeof(buf), "%s", p + 1);
        else
          snprintf(buf, sizeof(buf), "%s", b->url);
     }
   else
     strcpy(buf, "Blank");
   return &buf[0];
}

void
browser_window_title_update(ECef_Client *ec)
{
   char buf[4096] = {0};
   const char *browser[] =
   {
      "Chromium Embedded",
      "Servo"
   };
   const char *str;

   if (ec->dialing && ((!ec->current_page) && ec->current_page->swapping))
     str = "Dialer";
   else
     str = browser_page_string_get(ec->current_page);
   snprintf(buf, sizeof(buf), "%s - %s", str, browser[servo]);
   elm_win_title_set(ec->win, buf);
}

void
browser_set(ECef_Client *ec, Browser *b)
{
   cef_browser_host_t *host;
   int x, y, w, h;
   Evas_Object *pobj;
   Ecore_X_Window_State state[1];

   pobj = elm_object_part_content_unset(ec->layout, "ecef.swallow.browser");
   if (ec->current_page)
     {
        state[0] = ECORE_X_WINDOW_STATE_HIDDEN;
        if (ec->current_page->img)
          evas_object_hide(ec->current_page->img);
        host = browser_get_host(ec->current_page->browser);
        //host->set_focus(host, 0);
        if (windowed)
          {
             ecore_x_window_hide(host->get_window_handle(host));
             ecore_x_netwm_window_state_set(host->get_window_handle(host), state, 1);
          }
        if (b->swapping)
          elm_object_part_content_set(ec->layout, "ecef.swallow.swap", render_image_clone(ec->current_page));
     }
   else
     evas_object_del(pobj);
   ec->current_page = b;
   if (b->swapping)
     elm_object_part_content_set(ec->layout, "ecef.swallow.browser", render_image_clone(b));
   else
     {
        elm_object_part_content_set(ec->layout, "ecef.swallow.browser", b->img);
        dialer_unuse(ec);
     }
   elm_layout_signal_emit(ec->layout, "ecef,pages,available", "ecef");
   evas_object_size_hint_aspect_set(b->img, EVAS_ASPECT_CONTROL_NONE, -1, -1);
   browser_window_title_update(ec);
   elm_entry_entry_set(ec->urlbar, b->url);
   host = browser_get_host(ec->current_page->browser);
   //host->set_focus(host, 1);
   if (b->swapping)
     elm_object_signal_emit(ec->layout, "ecef,browser,swap", "ecef");
   if (!windowed) return;
   edje_object_part_geometry_get(elm_layout_edje_get(ec->layout), "ecef.swallow.browser", &x, &y, &w, &h);
   ecore_x_window_move(host->get_window_handle(host), x, y);
   ecore_x_window_show(host->get_window_handle(host));
   ecore_x_netwm_window_state_set(host->get_window_handle(host), state, 0);
}

void
browser_swap(ECef_Client *ec, Browser *b, Evas_Object *clone)
{
   int x, y, w, h, bx, by, bw, bh;
   Edje_Message_Int_Set *msg;

   if (ec->current_page == b) return;

   if (windowed)
     {
        browser_set(ec, b);
        return;
     }
   evas_object_geometry_get(clone, &x, &y, &w, &h);
   msg = alloca(sizeof(Edje_Message_Int_Set) + (sizeof(int) * 3));
   msg->count = 4;
   msg->val[0] = x;
   msg->val[1] = y;
   if (ec->dialing)
     evas_object_geometry_get(ec->dialer, &bx, &by, &bw, &bh);
   else
     {
        evas_object_geometry_get(ec->current_page->img, &bx, &by, NULL, NULL);
        bw = ec->current_page->w, bh = ec->current_page->h;
     }
   msg->val[2] = bw - ((x - bx) + w);
   msg->val[3] = bh - ((y - by) + h);
   edje_object_message_send(elm_layout_edje_get(ec->layout), EDJE_MESSAGE_INT_SET, 0, msg);
   b->swapping = 1;
   browser_set(ec, b);
}
