#include "ecef.h"
#include <dlfcn.h>
#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_client_capi.h"
#include <Ecore_X.h>

#define CEF_STRING(VAR, STRING) \
   (VAR) = cef_string_userfree_utf8_alloc(); \
   cef_string_utf8_set(STRING, (STRING) ? strlen(STRING) : 0, (VAR), 1)

Eina_Bool servo;
Eina_Bool gl_avail;
static Eina_List *handlers;

Eina_List *clients;

static Eina_Bool
timer(void *d EINA_UNUSED)
{
   //fprintf(stderr, "TIMER\n");
   cef_do_message_loop_work();
   return EINA_TRUE;
}


static cef_life_span_handler_t *
client_life_span_handler_get(cef_client_t *client EINA_UNUSED)
{
   cef_life_span_handler_t *lsh;

   lsh = CEF_NEW(cef_life_span_handler_t);
   lsh->on_after_created = on_after_browser_created;
   return lsh;
}

#ifdef HAVE_SERVO
static void
work_cb()
{
   //fprintf(stderr, "WORK %.10f\n", ecore_time_unix_get());
   cef_do_message_loop_work();
}

static void
work_available(cef_browser_process_handler_t *self EINA_UNUSED)
{
   ecore_main_loop_thread_safe_call_async(work_cb, NULL);
}

static cef_browser_process_handler_t *
browser_process_handler_get(cef_app_t *self EINA_UNUSED)
{
   cef_browser_process_handler_t *bph;

   bph = CEF_NEW(cef_browser_process_handler_t);
   bph->on_work_available = work_available;
   return bph;
}
#endif

static Eina_Bool
paste_url(void *data, Evas_Object *obj EINA_UNUSED, Elm_Selection_Data *ev)
{
   ECef_Client *ec = data;
   char *url;

   url = (char*)eina_memdup(ev->data, ev->len, 1);
   browser_urlbar_set(ec, url);
   free(url);
   return EINA_TRUE;
}

static Eina_Bool
key_down(void *d EINA_UNUSED, int t EINA_UNUSED, Ecore_Event_Key *ev)
{
   ECef_Client *ec;
   Eina_List *l;

   EINA_LIST_FOREACH(clients, l, ec)
     if (elm_win_window_id_get(ec->win) == ev->window) break;
   if (!ec) return ECORE_CALLBACK_RENEW;
   if ((!strcmp(ev->key, "q")) && (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     ecore_main_loop_quit();
   else if ((!strcmp(ev->key, "Escape")) && (!ev->modifiers))
     {
        if (elm_object_focus_get(ec->urlbar))
          browser_urlbar_hide(ec);
        else if (ec->pagelist_visible)
          browser_pagelist_hide(ec);
        else if (ec->urlbar_visible)
          browser_urlbar_hide(ec);
        else
          return ECORE_CALLBACK_RENEW;
     }
   else if ((!strcmp(ev->key, "F8")) && (!ev->modifiers))
     browser_urlbar_show(ec, 0);
   else if ((!strcmp(ev->key, "F4")) && (!ev->modifiers))
     browser_pagelist_show(ec);
   else if ((!strcasecmp(ev->key, "v")) && (ev->modifiers & (ECORE_EVENT_MODIFIER_CTRL | ECORE_EVENT_MODIFIER_SHIFT)))
     elm_cnp_selection_get(ec->win, ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT, paste_url, ec);
   else if ((!strcmp(ev->key, "F5")) || ((!strcmp(ev->key, "r")) && (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)))
     browser_reload(ec->current_page->browser);
   else if ((!strcmp(ev->key, "Left")) && (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     {
        if (ec->current_page->can_back)
          browser_back(ec->current_page->browser);
     }
   else if ((!strcmp(ev->key, "Right")) && (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     {
        if (ec->current_page->can_forward)
          browser_forward(ec->current_page->browser);
     }
   else
     return ECORE_CALLBACK_RENEW;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
mouse_down()
{
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
mouse_up()
{
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
mouse_wheel()
{
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
mouse_move()
{
   return ECORE_CALLBACK_RENEW;
}




Evas_Object *
button_add(Evas_Object *parent, const char *icon, const char *text, const char *style, Evas_Smart_Cb cb, void *data)
{
   Evas_Object *o, *ic;

   o = elm_button_add(parent);
   if (style)
     elm_object_style_set(o, style);
   elm_object_focus_allow_set(o, EINA_FALSE);
   EXPAND(o);
   FILL(o);
   ic = elm_icon_add(parent);
   elm_image_resizable_set(ic, 0, 0);
   elm_icon_order_lookup_set(ic, ELM_ICON_LOOKUP_THEME_FDO);
   elm_icon_standard_set(ic, icon);
   elm_object_part_content_set(o, "icon", ic);
   if (text)
     elm_object_text_set(o, text);
   evas_object_show(ic);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "clicked", cb, data);
   return o;
}

int
main(int argc, char *argv[])
{
   cef_main_args_t args = { .argc = argc, .argv = argv };
   cef_app_t *app;
   cef_settings_t settings;
   cef_window_info_t window_info = {0};
   cef_client_t *client;
   cef_browser_settings_t browser_settings;
   int ex;
   Evas_Object *win;
   ECef_Client *ec;

   eina_init();
   ecore_event_init();
   E_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_KEY_DOWN, key_down, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_MOUSE_BUTTON_DOWN, mouse_down, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_MOUSE_BUTTON_UP, mouse_up, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_MOUSE_WHEEL, mouse_wheel, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_MOUSE_MOVE, mouse_move, NULL);
   app = CEF_NEW(cef_app_t);
   cef_addref(app);
   ecore_app_no_system_modules();
#ifdef HAVE_SERVO
   app->get_browser_process_handler = browser_process_handler_get;
#endif
   ex = cef_execute_process(&args, app, NULL);
   if (ex >= 0)
     return ex;

   eldbus_init();
   ecore_main_loop_glib_integrate();

   gl_avail = !getenv("ECEF_WINDOWED");
   memset(&settings, 0, sizeof(cef_settings_t));
   memset(&browser_settings, 0, sizeof(cef_browser_settings_t));
   settings.size = sizeof(cef_settings_t);
   settings.windowless_rendering_enabled = gl_avail;
   browser_settings.size = sizeof(cef_browser_settings_t);
   if (!cef_initialize(&args, &settings, app, NULL))
     return -1;
   window_info.width = 640;
   window_info.height = 480;
   client = CEF_NEW(ECef_Client);
   ec = (void*)client;
   clients = eina_list_append(clients, ec);
   if (gl_avail)
     client->get_render_handler = client_render_handler_get;
   client->get_display_handler = client_display_handler_get;
   client->get_load_handler = client_load_handler_get;
   client->get_life_span_handler = client_life_span_handler_get;
   ec->browsers = eina_hash_int32_new(NULL);
   ec->browser_settings = &browser_settings;
   ec->window_info = &window_info;

   if (gl_avail)
     ecore_x_init_from_display(cef_get_xdisplay());

   elm_init(argc, (char**)argv);
   elm_theme_overlay_add(NULL, "./ecef.edj");
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   //if (gl_avail)
     //elm_config_accel_preference_set("opengl:depth24:stencil8");
   win = elm_win_util_standard_add("ecef", "Loading");
   evas_object_resize(win, 640, 480);
   elm_win_autodel_set(win, 1);

   servo = !!dlsym(NULL, "servo_test");

   ec->win = win;

   eina_log_domain_level_set("evas_main", EINA_LOG_LEVEL_CRITICAL);
   ec->layout = elm_layout_add(win);
   EXPAND(ec->layout);
   FILL(ec->layout);
   evas_object_resize(ec->layout, 640, 480);
   elm_win_resize_object_add(win, ec->layout);
   elm_layout_theme_set(ec->layout, "layout", "ecef", "base");
   evas_object_show(ec->layout);

   ec->pagelist = elm_gengrid_add(win);
   elm_gengrid_item_size_set(ec->pagelist, 100, 120);
   elm_object_style_set(ec->pagelist, "pagelist");
   elm_scroller_bounce_set(ec->pagelist, 0, 0);
   elm_scroller_policy_set(ec->pagelist, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_AUTO);
   elm_object_part_content_set(ec->layout, "ecef.swallow.pagelist", ec->pagelist);

   ec->urlbar = elm_entry_add(win);
   elm_entry_single_line_set(ec->urlbar, 1);
   elm_object_part_content_set(ec->layout, "ecef.swallow.urlbar", ec->urlbar);

   evas_object_show(win);
   window_info.windowless_rendering_enabled = gl_avail;
   if (!gl_avail)
     {
        int x, y, w, h;

        elm_layout_sizing_eval(ec->layout);
        evas_object_smart_calculate(ec->layout);
        edje_object_part_geometry_get(elm_layout_edje_get(ec->layout), "ecef.swallow.browser", &x, &y, &w, &h);
        evas_obscured_rectangle_add(evas_object_evas_get(ec->win), x, y, w, h);
     }

   window_info.parent_window = elm_win_window_id_get(win);
   browser_new(ec, "www.mozilla.org");

   if (!servo)
     ecore_timer_add(0.01, timer, NULL);
   ecore_main_loop_begin();

   return 0;
}
