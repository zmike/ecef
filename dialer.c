#include "ecef.h"

/* pages:
 * 
 * broscience
 * gold's gym
 * ON powder
 */

static const char *dialers[] =
{
   "http://en.wikipedia.org",
   "http://broscience.com/",
   "http://www.goldsgym.com/",
   "http://www.optimumnutrition.com/",
};

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

static Evas_Object *
dialer_item_content_get(Browser *b, Evas_Object *obj, const char *part)
{
   if (!b) return NULL;
   return b->it_clone = render_image_clone(b);
}

static void
dialer_browser_cb(void *d EINA_UNUSED, Browser *b)
{
   static Elm_Gengrid_Item_Class dialer_itc = {
      .item_style = "default",
      .func = {
           .content_get = (Elm_Gengrid_Item_Content_Get_Cb)dialer_item_content_get,
           .text_get = (Elm_Gengrid_Item_Text_Get_Cb)dialer_item_text_get,
           .del = (Elm_Gengrid_Item_Del_Cb)dialer_item_del
      },
      .version = ELM_GENGRID_ITEM_CLASS_VERSION
   };
   ECef_Client *ec;

   ec = browser_get_client(b->browser);
   b->it = elm_gengrid_item_append(ec->dialer, &dialer_itc, b, NULL, NULL);
   if (ec->dialing && is_glview())
     elm_glview_render_policy_set(b->img, ELM_GLVIEW_RENDER_POLICY_ALWAYS);
   evas_object_layer_set(b->img, -100);
}

static void
dialer_replace_cb(void *d, Browser *b)
{
   elm_object_item_data_set(d, b);
   b->it = d;
   evas_object_layer_set(b->img, -100);
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
   evas_object_hide(b->img);
   browser_page_item_add(ec, b);
   browser_swap(ec, b, b->it_clone);
   evas_object_del(b->it_clone);
   elm_object_item_data_set(it, NULL);
   elm_gengrid_item_update(it);
}

static void
dialer_unrealized(ECef_Client *ec EINA_UNUSED, Evas_Object *obj EINA_UNUSED, Elm_Object_Item *it)
{
   Browser *b;

   b = elm_object_item_data_get(it);
   b->it_clone = NULL;
}

static void
dialer_deactivate(ECef_Client *ec, ...)
{
   Elm_Object_Item *it;

   ec->dialing = 0;
   evas_object_hide(elm_object_part_content_unset(ec->layout, "ecef.swallow.dialier"));
   elm_layout_signal_emit(ec->layout, "ecef,dialer,reset", "ecef");
   for (it = elm_gengrid_first_item_get(ec->dialer); it; it = elm_gengrid_item_next_get(it))
     {
        Browser *b;

        b = elm_object_item_data_get(it);
        if (b && is_glview())
          elm_glview_render_policy_set(b->img, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
     }
}

void
dialer_populate(ECef_Client *ec)
{
   unsigned int i;

   ec->dialer = elm_gengrid_add(ec->win);
   elm_gengrid_item_size_set(ec->dialer, 100, 120);
   elm_object_style_set(ec->dialer, "dialer");
   elm_scroller_bounce_set(ec->dialer, 0, 0);
   elm_scroller_policy_set(ec->dialer, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_AUTO);
   evas_object_smart_callback_add(ec->dialer, "unrealized", (Evas_Smart_Cb)dialer_unrealized, ec);
   evas_object_smart_callback_add(ec->dialer, "activated", (Evas_Smart_Cb)dialer_activated, ec);
   elm_layout_signal_callback_add(ec->layout, "ecef,browser,swapped", "ecef", (Edje_Signal_Cb)dialer_deactivate, ec);

   for (i = 0; i < EINA_C_ARRAY_LENGTH(dialers); i++)
     browser_new(ec, dialers[i], 0, dialer_browser_cb, NULL);
}

void
dialer_use(ECef_Client *ec)
{
   Elm_Object_Item *it;

   if (ec->dialing) return;
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
}

void
dialer_unuse(ECef_Client *ec)
{
   if (ec->dialing)
     dialer_deactivate(ec);
}
