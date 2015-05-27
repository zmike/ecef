#include "ecef.h"

static int
on_console_message(cef_display_handler_t *self EINA_UNUSED, cef_browser_t *browser, const cef_string_t *message, const cef_string_t *source, int line)
{
   ECef_Client *ec;

   ec = browser_get_client(browser);
   cef_string_utf8_t u8 = {0};
   if (message)
     cef_string_to_utf8(message->str, message->length, &u8);
   cef_string_utf8_clear(&u8);
   return 0;
}

static void
tooltip_update(ECef_Client *ec)
{
   Eina_Strbuf *buf;

   if ((!ec->tooltip) && (!ec->status))
     {
        if (ec->tooltip_visible)
          elm_object_tooltip_hide(ec->win);
        ec->tooltip_visible = 0;
        return;
     }
   buf = eina_strbuf_new();
   if (ec->tooltip)
     eina_strbuf_append(buf, ec->tooltip);
   if (ec->tooltip && ec->status)
     eina_strbuf_append(buf, "<ps/>");
   if (ec->status)
     eina_strbuf_append_printf(buf, "<link>%s</link>", ec->status);
   elm_object_tooltip_text_set(ec->win, eina_strbuf_string_get(buf));
   elm_object_tooltip_style_set(ec->win, "browser");
   elm_object_tooltip_show(ec->win);
   eina_strbuf_free(buf);
   ec->tooltip_visible = 1;
}

static void
on_status_message(cef_display_handler_t *self EINA_UNUSED, cef_browser_t *browser, const cef_string_t *text)
{
   ECef_Client *ec;
   cef_string_utf8_t u8 = {0};

   ec = browser_get_client(browser);
   if (text && text->str)
     cef_string_to_utf8(text->str, text->length, &u8);
   eina_stringshare_replace(&ec->status, u8.str);
   tooltip_update(ec);
   cef_string_utf8_clear(&u8);
}

static int
on_tooltip(cef_display_handler_t *self EINA_UNUSED, cef_browser_t *browser, cef_string_t *text)
{
   ECef_Client *ec;
   cef_string_utf8_t u8 = {0};

   ec = browser_get_client(browser);
   if (text && text->str)
     cef_string_to_utf8(text->str, text->length, &u8);
   eina_stringshare_replace(&ec->tooltip, u8.str);
   tooltip_update(ec);
   cef_string_utf8_clear(&u8);

   return 1;
}

static void
on_address_change(cef_display_handler_t *self, cef_browser_t *browser, cef_frame_t *frame, const cef_string_t *url)
{
   ECef_Client *ec;
   cef_string_utf8_t u8 = {0};
   Browser *b;

   ec = browser_get_client(browser);
   b = browser_get(ec, browser);
   if (url)
     cef_string_to_utf8(url->str, url->length, &u8);
   eina_stringshare_replace(&b->url, u8.str);
   cef_string_utf8_clear(&u8);
   if (ec->current_page == b)
     {
        elm_entry_entry_set(ec->urlbar, b->url);
        browser_urlbar_show(ec, 1);
     }
}

static void
on_title_change(cef_display_handler_t *self, cef_browser_t *browser, const cef_string_t *title)
{
   ECef_Client *ec;
   cef_string_utf8_t u8 = {0};
   Browser *b;

   ec = browser_get_client(browser);
   b = browser_get(ec, browser);
   if (title)
     cef_string_to_utf8(title->str, title->length, &u8);
   eina_stringshare_replace(&b->title, u8.str);
   cef_string_utf8_clear(&u8);
   if (ec->current_page == b)
     browser_window_title_update(ec);
}

static void
init_handler(ECef_Client *ec)
{
   cef_display_handler_t *display_handler;

   display_handler = CEF_NEW_PTR(cef_display_handler_t, &ec->display_handler);
   display_handler->on_title_change = on_title_change;
   display_handler->on_address_change = on_address_change;
   display_handler->on_tooltip = on_tooltip;
   display_handler->on_status_message = on_status_message;
   display_handler->on_console_message = on_console_message;
}

cef_display_handler_t *
client_display_handler_get(cef_client_t *client)
{
   ECef_Client *ec = (void*)client;
   if (!ec->display_handler)
     init_handler(ec);
   return ec->display_handler;
}
