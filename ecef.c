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

static void
on_browser_created(cef_render_process_handler_t *self, cef_browser_t *browser)
{
   ECef_Client *ec = browser_get_client(browser);
   int id;

   id = browser->get_identifier(browser);
   //eina_hash_add(ec->browsers, &id, render_image_new(ec, browser->get_host(browser)));
}

static void
on_browser_destroyed(cef_render_process_handler_t *self, cef_browser_t *browser)
{
   ECef_Client *ec = browser_get_client(browser);
   Eina_Iterator *it;
   Evas_Object *i;
   int id;

   id = browser->get_identifier(browser);
   eina_hash_del_by_key(ec->browsers, &id);
   if (eina_hash_population(ec->browsers) != 1) return;
   it = eina_hash_iterator_data_new(ec->browsers);
   EINA_ITERATOR_FOREACH(it, i)
     {
        elm_win_resize_object_del(ec->win, i);
        elm_win_resize_object_add(ec->win, i);
     }
   eina_iterator_free(it);
}

static cef_render_process_handler_t *
get_render_process_handler(cef_app_t *app)
{
   cef_render_process_handler_t *rph;

   rph = CEF_NEW(cef_render_process_handler_t);
   //rph->on_browser_created = on_browser_created;
   rph->on_browser_destroyed = on_browser_destroyed;
   return rph;
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
   cef_string_utf16_t *url;
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
   app->get_render_process_handler = get_render_process_handler;
   if (!cef_initialize(&args, &settings, app, NULL))
     return -1;
   window_info.windowless_rendering_enabled = 1;
   window_info.width = 640;
   window_info.height = 480;
   client = CEF_NEW(ECef_Client);
   ec = (void*)client;
   client->get_render_handler = client_render_handler_get;
   client->get_display_handler = client_display_handler_get;
   ec->browsers = eina_hash_int32_new((Eina_Free_Cb)evas_object_del);
   ec->surfaces = eina_hash_pointer_new(NULL);

   elm_init(argc, (char**)argv);
   ecore_main_loop_glib_integrate();
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   ec->win = win = elm_win_util_standard_add("ecef", "Loading");
   elm_win_autodel_set(win, 1);
   evas_object_show(win);
   evas_object_resize(win, 640, 480);
   elm_win_render(win);

   window_info.parent_window = elm_win_window_id_get(win);

   url = cef_string_userfree_utf16_alloc();
   cef_string_utf8_to_utf16("www.mozilla.org", sizeof("www.mozilla.org") - 1, url);
   cef_browser_host_create_browser(&window_info, client, url, &browser_settings, NULL);

   ecore_timer_add(0.3, timer, NULL);
   ecore_main_loop_begin();

   return 0;
}
