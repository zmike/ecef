#include "ecef.h"

static void
on_loading_state_change(cef_load_handler_t *self, cef_browser_t *browser, int isLoading, int canGoBack, int canGoForward)
{
   ECef_Client *ec;
   Browser *b;
   Eina_Bool load_change;
   const char *sig[] =
   {
      "ecef,state,loaded",
      "ecef,state,loading"
   };

   ec = browser_get_client(browser);
   b = browser_get(ec, browser);
   b->can_back = !!canGoBack;
   b->can_forward = !!canGoForward;
   load_change = b->loading != !!isLoading;
   b->loading = !!isLoading;
   if (ec->current_page != b) return;
   elm_object_disabled_set(ec->back, !b->can_back);
   elm_object_disabled_set(ec->forward, !b->can_forward);
   if (load_change)
     elm_layout_signal_emit(ec->layout, sig[b->loading], "ecef");
}

static void
on_load_start(cef_load_handler_t *self, cef_browser_t *browser, cef_frame_t *frame)
{
}

static void
on_load_end(cef_load_handler_t *self, cef_browser_t *browser, cef_frame_t *frame, int httpStatusCode)
{
}

static void
on_load_error(cef_load_handler_t *self, cef_browser_t *browser, cef_frame_t *frame, cef_errorcode_t errorCode, const cef_string_t *errorText, const cef_string_t *failedUrl)
{
}

static void
init_handler(ECef_Client *ec)
{
   cef_load_handler_t *load_handler;

   load_handler = CEF_NEW_PTR(cef_load_handler_t, &ec->load_handler);
   load_handler->on_loading_state_change = on_loading_state_change;
   load_handler->on_load_start = on_load_start;
   load_handler->on_load_end = on_load_end;
   load_handler->on_load_error = on_load_error;
}

cef_load_handler_t *
client_load_handler_get(cef_client_t *client)
{
   ECef_Client *ec = (void*)client;
   if (!ec->load_handler)
     init_handler(ec);
   return ec->load_handler;
}
