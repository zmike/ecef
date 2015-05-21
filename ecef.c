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
        else
          return ECORE_CALLBACK_RENEW;
     }
   else if ((!strcmp(ev->key, "F8")) && (!ev->modifiers))
     browser_urlbar_show(ec, 0);
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

static void
urlbar_visible(void *d, Evas_Object *obj EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   ECef_Client *ec = d;

   elm_object_focus_set(ec->urlbar, 1);
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
   client->get_life_span_handler = client_life_span_handler_get;
   ec->browsers = eina_hash_int32_new(NULL);
   ec->browser_settings = &browser_settings;
   ec->window_info = &window_info;

   if (gl_avail)
     ecore_x_init_from_display(cef_get_xdisplay());

   elm_init(argc, (char**)argv);
   elm_theme_overlay_add(NULL, "./ecef.edj");
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   if (gl_avail)
     elm_config_accel_preference_set("opengl:depth24:stencil8");
   win = elm_win_util_standard_add("ecef", "Loading");
   evas_object_resize(win, 640, 480);
   elm_win_autodel_set(win, 1);

   servo = !!dlsym(NULL, "servo_test");

   ec->win = win;

   ec->layout = elm_layout_add(win);
   EXPAND(ec->layout);
   FILL(ec->layout);
   evas_object_resize(ec->layout, 640, 480);
   elm_win_resize_object_add(win, ec->layout);
   elm_layout_theme_set(ec->layout, "layout", "ecef", "base");
   elm_layout_signal_callback_add(ec->layout, "ecef,urlbar,visible", "ecef", urlbar_visible, ec);
   evas_object_show(ec->layout);

   ec->pagelist = elm_genlist_add(win);
   //evas_object_smart_callback_add(o, "longpressed", queue_list_item_longpress, NULL);
   //evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, queue_list_item_click, NULL);
   //evas_object_smart_callback_add(o, "realized", queue_list_item_realize, NULL);
   //evas_object_smart_callback_add(o, "unrealized", queue_list_item_unrealize, NULL);
   //evas_object_smart_callback_add(o, "unselected", queue_list_item_unselect, NULL);
   //evas_object_smart_callback_add(o, "clicked,double", queue_list_double_click, NULL);
   //evas_object_smart_callback_add(o, "scroll,anim,stop", queue_list_scroll_stop, NULL);
   //evas_object_event_callback_add(o, EVAS_CALLBACK_KEY_DOWN, queue_list_key_down, NULL);
   elm_genlist_multi_select_mode_set(ec->pagelist, ELM_OBJECT_MULTI_SELECT_MODE_DEFAULT);
   elm_genlist_homogeneous_set(ec->pagelist, EINA_TRUE);
   elm_genlist_multi_select_set(ec->pagelist, 1);
   elm_scroller_bounce_set(ec->pagelist, 0, 0);
   elm_scroller_policy_set(ec->pagelist, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_AUTO);
   elm_genlist_mode_set(ec->pagelist, ELM_LIST_COMPRESS);
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
