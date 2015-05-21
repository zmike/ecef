#define TEST_APP
#include <stdlib.h>
#include <string.h>
#include "ecef.h"
#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_client_capi.h"

#define CEF_STRING(VAR, STRING) \
   (VAR) = cef_string_userfree_utf8_alloc(); \
   cef_string_utf8_set(STRING, (STRING) ? strlen(STRING) : 0, (VAR), 1)

static void
on_after_browser_created(cef_life_span_handler_t *self, cef_browser_t *browser)
{
   cef_browser_host_t *host;
   cef_browser_t *b;

   host = browser->get_host(browser);
   b = host->get_browser(host);
   if (b){}
}

static cef_life_span_handler_t *
client_life_span_handler_get()
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

   app = CEF_NEW(cef_app_t);
   cef_addref(app);

   ex = cef_execute_process(&args, app, NULL);
   if (ex >= 0)
     return ex;

   memset(&settings, 0, sizeof(cef_settings_t));
   memset(&browser_settings, 0, sizeof(cef_browser_settings_t));
   settings.size = sizeof(cef_settings_t);
   settings.windowless_rendering_enabled = 0;
   browser_settings.size = sizeof(cef_browser_settings_t);
   if (!cef_initialize(&args, &settings, app, NULL))
     return -1;
   window_info.width = 640;
   window_info.height = 480;
   client = CEF_NEW(cef_client_t);
   client->get_life_span_handler = client_life_span_handler_get;
   window_info.windowless_rendering_enabled = 0;

   {
      cef_string_utf16_t u = {0};

      cef_string_utf8_to_utf16("www.mozilla.org", strlen("www.mozilla.org"), &u);
      cef_browser_host_create_browser_sync(&window_info, client, &u, &browser_settings, NULL);
      cef_string_utf16_clear(&u);
   }

   return 0;
}
