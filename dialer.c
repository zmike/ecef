#include "ecef.h"


static const char *dialers[] =
{
   "http://en.wikipedia.org/wiki/Doughnut",
   "http://broscience.com/",
   "http://cnn.com",
   "http://www.goldsgym.com/",
   "http://pinterest.com",
   "http://reddit.com",
   "http://www.optimumnutrition.com/",
   "http://yahoo.com",
   "https://twitter.com/schwarzenegger",
};

#define DIALER_MINW 100
#define DIALER_MINH 120

static char *
dialer_item_text_get(Browser *b, Evas_Object *obj EINA_UNUSED, const char *part)
{
   if (!b) return NULL;
   return strdup(browser_page_string_get(b));
}

static void
dialer_item_del(Browser *b, Evas_Object *obj EINA_UNUSED)
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
dialer_browser_cb(void *d EINA_UNUSED, Browser *b)
{
   static Elm_Gengrid_Item_Class dialer_itc = {
      .item_style = "default",
      .func = {
           .content_get = (Elm_Gengrid_Item_Content_Get_Cb)browser_page_content_get,
           .text_get = (Elm_Gengrid_Item_Text_Get_Cb)dialer_item_text_get,
           .del = (Elm_Gengrid_Item_Del_Cb)dialer_item_del
      },
      .version = ELM_GENGRID_ITEM_CLASS_VERSION
   };
   ECef_Client *ec;

   ec = browser_get_client(b->browser);
   b->it = elm_gengrid_item_append(ec->dialer, &dialer_itc, b, NULL, NULL);
   elm_object_item_tooltip_content_cb_set(b->it, (Elm_Tooltip_Item_Content_Cb)browser_tooltip_cb, b, NULL);
   elm_object_item_tooltip_style_set(b->it, "browser");
   elm_object_item_tooltip_window_mode_set(b->it, 1);
   if (ec->dialing && is_glview())
     elm_glview_render_policy_set(b->img, ELM_GLVIEW_RENDER_POLICY_ALWAYS);
   evas_object_layer_set(b->img, -100);
   evas_object_pass_events_set(b->img, 1);
}

static void
dialer_replace_cb(void *d, Browser *b)
{
   elm_object_item_data_set(d, b);
   b->it = d;
   evas_object_layer_set(b->img, -100);
   evas_object_pass_events_set(b->img, 1);
   elm_object_item_tooltip_content_cb_set(b->it, (Elm_Tooltip_Item_Content_Cb)browser_tooltip_cb, b, NULL);
   elm_object_item_tooltip_style_set(b->it, "browser");
   elm_object_item_tooltip_window_mode_set(b->it, 1);
   elm_gengrid_item_update(d);
}

static void
dialer_activated(ECef_Client *ec, Evas_Object *obj EINA_UNUSED, Elm_Object_Item *it)
{
   Browser *b;

   b = elm_object_item_data_get(it);
   browser_new(ec, b->url, 0, dialer_replace_cb, it);
   if (is_glview())
     elm_glview_render_policy_set(b->img, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
   evas_object_layer_set(b->img, 0);
   evas_object_pass_events_set(b->img, 0);
   evas_object_hide(b->img);
   browser_page_item_add(ec, b);
   ec->focus_stack = eina_inlist_prepend(ec->focus_stack, EINA_INLIST_GET(b));
   browser_swap(ec, b, b->it_clone);
   evas_object_del(b->it_clone);
   elm_object_item_data_set(it, NULL);
   elm_gengrid_item_update(it);
   elm_gengrid_item_selected_set(it, 0);
}

static void
dialer_unrealized(ECef_Client *ec EINA_UNUSED, Evas_Object *obj EINA_UNUSED, Elm_Object_Item *it)
{
   Browser *b;

   b = elm_object_item_data_get(it);
   if (b)
     b->it_clone = NULL;
}

static void
dialer_deactivate(ECef_Client *ec, ...)
{
   Elm_Object_Item *it;

   ec->dialing = 0;
   browser_urlbar_hide(ec);
   evas_object_hide(elm_object_part_content_unset(ec->layout, "ecef.swallow.dialier"));
   elm_layout_signal_emit(ec->layout, "ecef,dialer,reset", "ecef");
   for (it = elm_gengrid_first_item_get(ec->dialer); it; it = elm_gengrid_item_next_get(it))
     {
        Browser *b;

        b = elm_object_item_data_get(it);
        if (b && is_glview())
          elm_glview_render_policy_set(b->img, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
     }
   if (ec->current_page)
     elm_entry_entry_set(ec->urlbar, ec->current_page->url);
}

static Eina_Bool
dialer_resize(ECef_Client *ec)
{
   int w, h, mw, mh, rows;

   evas_object_geometry_get(ec->win, NULL, NULL, &w, &h);

   rows = lround(floor(sqrt(EINA_C_ARRAY_LENGTH(dialers))));
   mh = (h - MAX((double)h * 0.1, 50)) / rows;
   mh = MAX(mh, DIALER_MINH);
   mw = (mh * 100) / 120;
   elm_gengrid_item_size_set(ec->dialer, mw, mh);
   ec->dialer_resize_timer = NULL;
   return EINA_FALSE;
}

static void
dialer_win_resize(ECef_Client *ec, ...)
{
   if (!ec->dialer_resize_timer)
     ec->dialer_resize_timer = ecore_timer_add(0.1, (Ecore_Task_Cb)dialer_resize, ec);
}

void
dialer_populate(ECef_Client *ec)
{
   unsigned int i;
   Evas_Object *o;

   ec->dialer = elm_gengrid_add(ec->win);
   elm_object_focus_allow_set(ec->dialer, 0);
   dialer_resize(ec);
   evas_object_event_callback_add(ec->win, EVAS_CALLBACK_RESIZE, (Evas_Object_Event_Cb)dialer_win_resize, ec);
   elm_object_style_set(ec->dialer, "dialer");
        o = elm_bg_add(ec->dialer);
        if (servo)
          elm_bg_file_set(o, "servomeme.jpg", NULL);
        else
          elm_bg_file_set(o, "donut.gif", NULL);
        elm_object_part_content_set(ec->dialer, "elm.swallow.background", o);
   elm_scroller_bounce_set(ec->dialer, 0, 0);
   elm_scroller_policy_set(ec->dialer, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_AUTO);
   evas_object_smart_callback_add(ec->dialer, "unrealized", (Evas_Smart_Cb)dialer_unrealized, ec);
   evas_object_smart_callback_add(ec->dialer, "selected", (Evas_Smart_Cb)dialer_activated, ec);
   elm_layout_signal_callback_add(ec->layout, "ecef,browser,swapped", "ecef", (Edje_Signal_Cb)dialer_deactivate, ec);

   for (i = 0; i < EINA_C_ARRAY_LENGTH(dialers); i++)
     browser_new(ec, dialers[i], 0, dialer_browser_cb, NULL);
}

void
dialer_use(ECef_Client *ec)
{
   Elm_Object_Item *it;

   if (ec->dialing) return;
   if (ec->tooltip_visible)
     {
        elm_object_tooltip_unset(ec->layout);
        ec->tooltip_visible = 0;
     }
   elm_entry_entry_set(ec->urlbar, NULL);
   elm_object_part_content_set(ec->layout, "ecef.swallow.dialer", ec->dialer);
   elm_layout_signal_emit(ec->layout, "ecef,dialer,activate", "ecef");
   ec->dialing = 1;
   for (it = elm_gengrid_first_item_get(ec->dialer); it; it = elm_gengrid_item_next_get(it))
     {
        Browser *b;

        b = elm_object_item_data_get(it);
        if (b && is_glview())
          elm_glview_render_policy_set(b->img, ELM_GLVIEW_RENDER_POLICY_ALWAYS);
     }
   browser_window_title_update(ec);
   browser_urlbar_show(ec, 0);
}

void
dialer_unuse(ECef_Client *ec)
{
   if (ec->dialing)
     dialer_deactivate(ec);
}

void
dialer_activate_num(ECef_Client *ec, unsigned int n)
{
   Elm_Object_Item *it;
   unsigned int i;

   for (i = 0, it = elm_gengrid_first_item_get(ec->dialer); it && (i <= n); i++, it = elm_gengrid_item_next_get(it))
     {
        if (i != n) continue;
        dialer_activated(ec, NULL, it);
        break;
     }
}
