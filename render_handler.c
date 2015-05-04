#include "ecef.h"
#include <Ecore_X.h>


void render_image_gl_init(Browser *b, Evas_Object *o, int w, int h);
void paint_gl(ECef_Client *ec, Browser *b, Evas_Object *o, cef_paint_element_type_t type, size_t dirtyRectsCount, cef_rect_t const *dirtyRects, const void *buffer, int width, int height);

static void render_image_new(ECef_Client *ec, cef_browser_host_t *host, int w, int h);

static int
get_root_screen_rect(cef_render_handler_t *self, cef_browser_t *browser, cef_rect_t *rect)
{
   ecore_x_window_size_get(ecore_x_window_root_first_get(), &rect->width, &rect->height);
   return 1;
}

static int
get_view_rect(cef_render_handler_t *self, cef_browser_t *browser, cef_rect_t *rect)
{
   ECef_Client *client;

   client = browser_get_client(browser);
   evas_object_geometry_get(client->win, &rect->x, &rect->y, &rect->width, &rect->height);
   fprintf(stderr, "VIEW RECT: %dx%d\n", rect->width, rect->height);
   return 1;
}

static int
get_screen_point(cef_render_handler_t *self, cef_browser_t *browser, int viewX, int viewY, int *screenX, int *screenY)
{
   ECef_Client *client;
   int x, y;

   client = browser_get_client(browser);
   evas_object_geometry_get(client->win, &x, &y, NULL, NULL);
   *screenX = viewX + x;
   *screenY = viewY + y;
   return 1;
}

static void
paint(cef_render_handler_t *self, cef_browser_t *browser, cef_paint_element_type_t type,
      size_t dirtyRectsCount, cef_rect_t const *dirtyRects, const void *buffer,
      int width, int height)
{
   ECef_Client *ec;
   Evas_Object *img, *o;
   size_t r;
   Browser *b;

   ec = browser_get_client(browser);
   b = browser_get(ec, browser);
   if (!b->img)
     render_image_new(ec, browser->get_host(browser), width, height);
   img = b->img;
   o = elm_image_object_get(img);
   evas_object_image_size_set(o, width, height);
   if (ec->gl_avail)
     paint_gl(ec, b, o, type, dirtyRectsCount, dirtyRects, buffer, width, height);
   else
     {
        evas_object_image_data_set(o, (void*)buffer);
        evas_object_image_size_set(o, width, height);
        for (r = 0; r < dirtyRectsCount; r++)
          evas_object_image_data_update_add(o, dirtyRects[r].x, dirtyRects[r].y, dirtyRects[r].width, dirtyRects[r].height);
     }
   fprintf(stderr, "PAINT %dx%d\n", width, height);
}

static void
init_handler(ECef_Client *ec)
{
   cef_render_handler_t *render_handler;

   render_handler = CEF_NEW_PTR(cef_render_handler_t, &ec->render_handler);
   render_handler->on_paint = paint;
   render_handler->get_root_screen_rect = get_root_screen_rect;
   render_handler->get_view_rect = get_view_rect;
   render_handler->get_screen_point = get_screen_point;
}

cef_render_handler_t *
client_render_handler_get(cef_client_t *client)
{
   ECef_Client *ec = (void*)client;
   if (!ec->render_handler)
     init_handler(ec);
   return ec->render_handler;
}


static void
render_image_mouse(ECef_Client *ec, Evas *e, Evas_Object *obj, Evas_Event_Mouse_Down *ev, int up)
{
   int x, y, mod, count = 1;
   cef_browser_host_t *host;
   cef_mouse_event_t event;

   host = evas_object_data_get(obj, "browser_host");
   if (!host) return;
   if (ev->flags & EVAS_BUTTON_TRIPLE_CLICK)
     count = 3;
   else if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     count = 2;

   mod = modifiers_get(ev->modifiers);
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   event.x = ev->canvas.x - x, event.y = ev->canvas.y - y;
   event.modifiers = mod;
   if (evas_pointer_button_down_mask_get(e) & (1 << 0))
     event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
   if (evas_pointer_button_down_mask_get(e) & (1 << 1))
     event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
   if (evas_pointer_button_down_mask_get(e) & (1 << 2))
     event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
   host->send_mouse_click_event(host, &event, ev->button - 1, up, count);
}

static void
render_image_mouse_down(ECef_Client *ec, Evas *e, Evas_Object *obj, Evas_Event_Mouse_Down *ev)
{
   render_image_mouse(ec, e, obj, ev, 0);
}

static void
render_image_mouse_up(ECef_Client *ec, Evas *e, Evas_Object *obj, Evas_Event_Mouse_Down *ev)
{
   render_image_mouse(ec, e, obj, ev, 1);
}

static void
render_image_mouse_move(ECef_Client *ec, Evas *e, Evas_Object *obj, Evas_Event_Mouse_Move *ev)
{
   int x, y, mod;
   cef_browser_host_t *host;
   cef_mouse_event_t event;

   host = evas_object_data_get(obj, "browser_host");
   mod = modifiers_get(ev->modifiers);
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   event.x = ev->cur.canvas.x - x, event.y = ev->cur.canvas.y - y;
   event.modifiers = mod;
   if (ev->buttons & (1 << 0))
     event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
   if (ev->buttons & (1 << 1))
     event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
   if (ev->buttons & (1 << 2))
     event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
   host->send_mouse_move_event(host, &event, 0);
}

static void
render_image_mouse_wheel(ECef_Client *ec, Evas *e, Evas_Object *obj, Evas_Event_Mouse_Wheel *ev)
{
   int x, y, mod, dx, dy;
   cef_browser_host_t *host;
   cef_mouse_event_t event;

   host = evas_object_data_get(obj, "browser_host");
   mod = modifiers_get(ev->modifiers);
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   event.x = ev->canvas.x - x, event.y = ev->canvas.y - y;
   event.modifiers = mod;
   if (evas_pointer_button_down_mask_get(e) & (1 << 0))
     event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
   if (evas_pointer_button_down_mask_get(e) & (1 << 1))
     event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
   if (evas_pointer_button_down_mask_get(e) & (1 << 2))
     event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
   dx = 0;
   if (ev->z < 0)
     dy = 40;
   if (ev->z > 0)
     dy = -40;
   host->send_mouse_wheel_event(host, &event, dx, dy);
}

static void
render_image_mouse_move_out(ECef_Client *ec, Evas *e, Evas_Object *obj, Evas_Event_Mouse_Out *ev)
{
   int x, y, mod;
   cef_browser_host_t *host;
   cef_mouse_event_t event;

   host = evas_object_data_get(obj, "browser_host");
   mod = modifiers_get(ev->modifiers);
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   event.x = ev->canvas.x - x, event.y = ev->canvas.y - y;
   event.modifiers = mod;
   if (evas_pointer_button_down_mask_get(e) & (1 << 0))
     event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
   if (evas_pointer_button_down_mask_get(e) & (1 << 1))
     event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
   if (evas_pointer_button_down_mask_get(e) & (1 << 2))
     event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
   host->send_mouse_move_event(host, &event, 1);
}

static void
render_image_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   cef_browser_host_t *host = data;

   host->was_resized(host);
}

static void
render_image_new(ECef_Client *ec, cef_browser_host_t *host, int w, int h)
{
   Evas_Object *i, *o;
   Browser *b;

   b = browser_get(ec, host->get_browser(host));
   b->img = i = elm_image_add(ec->win);
   elm_image_no_scale_set(i, 1);
   elm_image_resizable_set(i, 0, 0);
   elm_image_smooth_set(i, 0);
   evas_object_resize(i, w, h);
   if (ec->current_page == b)
     elm_object_part_content_set(ec->layout, "ecef.swallow.browser", i);
   o = elm_image_object_get(i);
   if (ec->gl_avail)
     render_image_gl_init(b, o, w, h);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)render_image_mouse_down, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)render_image_mouse_up, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_MOVE, (Evas_Object_Event_Cb)render_image_mouse_move, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_OUT, (Evas_Object_Event_Cb)render_image_mouse_move_out, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_WHEEL, (Evas_Object_Event_Cb)render_image_mouse_wheel, ec);
   evas_object_show(i);
   evas_object_data_set(i, "browser_host", host);
}
