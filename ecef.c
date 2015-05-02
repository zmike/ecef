#include "ecef.h"
#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_client_capi.h"

#define CEF_STRING(VAR, STRING) \
   (VAR) = cef_string_userfree_utf8_alloc(); \
   cef_string_utf8_set(STRING, (STRING) ? strlen(STRING) : 0, (VAR), 1)

static Eina_Bool
timer(void *d EINA_UNUSED)
{
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
   app = CEF_NEW(cef_app_t);
   cef_addref(app);
   ex = cef_execute_process(&args, app, NULL);
   if (ex >= 0)
     return ex;
   memset(&settings, 0, sizeof(cef_settings_t));
   memset(&browser_settings, 0, sizeof(cef_browser_settings_t));
   settings.size = sizeof(cef_settings_t);
   settings.windowless_rendering_enabled = 1;
   browser_settings.size = sizeof(cef_browser_settings_t);
   if (!cef_initialize(&args, &settings, app, NULL))
     return -1;
   window_info.width = 640;
   window_info.height = 480;
   client = CEF_NEW(ECef_Client);
   ec = (void*)client;
   client->get_render_handler = client_render_handler_get;
   client->get_display_handler = client_display_handler_get;
   client->get_life_span_handler = client_life_span_handler_get;
   ec->browsers = eina_hash_int32_new(NULL);
   ec->browser_settings = &browser_settings;
   ec->window_info = &window_info;

   elm_init(argc, (char**)argv);
   elm_theme_overlay_add(NULL, "./ecef.edj");
   ecore_main_loop_glib_integrate();
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   ec->win = win = elm_win_util_standard_add("ecef", "Loading");
   elm_win_autodel_set(win, 1);

   ec->layout = elm_layout_add(win);
   EXPAND(ec->layout);
   FILL(ec->layout);
   elm_win_resize_object_add(win, ec->layout);
   elm_layout_theme_set(ec->layout, "layout", "ecef", "base");
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

   evas_object_show(win);
   evas_object_resize(win, 640, 480);
   ec->gl_avail = !!strstr(ecore_evas_engine_name_get(ecore_evas_ecore_evas_get(evas_object_evas_get(win))), "gl");
   window_info.windowless_rendering_enabled = 1;
   if (ec->gl_avail)
     ec->surfaces = eina_hash_pointer_new(NULL);

   window_info.parent_window = elm_win_window_id_get(win);
   browser_new(ec, "www.mozilla.org");

   ecore_timer_add(0.3, timer, NULL);
   ecore_main_loop_begin();

   return 0;
}
