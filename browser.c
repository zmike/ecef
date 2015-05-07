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
   ECef_Client *ec;

   ec = browser_get_client(b->browser);
   id = b->browser->get_identifier(b->browser);
   eina_hash_del_by_key(ec->browsers, &id);
   browser_get_host(b->browser)->close_browser(browser_get_host(b->browser), 0);
   evas_object_del(b->img);
   free(b);
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
   int id;
   Browser *b;

   id = browser->get_identifier(browser);
   b = calloc(1, sizeof(Browser));
   b->browser = browser;
   eina_hash_add(ec->browsers, &id, b);
   b->it = elm_genlist_item_append(ec->pagelist, &browser_itc, b, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
   if (!ec->current_page)
     browser_set(ec, b);
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
   cef_string_utf16_t *u;

   u = cef_string_userfree_utf16_alloc();
   cef_string_utf8_to_utf16(url, strlen(url), u);
   cef_browser_host_create_browser(ec->window_info, &ec->client, u, ec->browser_settings, NULL);
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
        host->set_focus(host, 0);
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
   host = browser_get_host(ec->current_page->browser);
   host->set_focus(host, 1);
   if (gl_avail) return;
   edje_object_part_geometry_get(elm_layout_edje_get(ec->layout), "ecef.swallow.browser", &x, &y, &w, &h);
   ecore_x_window_move(host->get_window_handle(host), x, y);
   ecore_x_window_show(host->get_window_handle(host));
   ecore_x_netwm_window_state_set(host->get_window_handle(host), state, 0);
}
