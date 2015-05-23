#include "ecef.h"
#include <Ecore_X.h>

int GetControlCharacter(KeyboardCode windows_key_code, int shift);
KeyboardCode GdkEventToWindowsKeyCode(const Evas_Event_Key_Down* event);
KeyboardCode GetWindowsKeyCodeWithoutLocation(KeyboardCode key_code);
KeyboardCode KeyboardCodeFromXKeysym(unsigned int keysym);

void render_image_gl_setup(Browser *b, int w, int h);
void paint_gl(ECef_Client *ec, Browser *b, cef_paint_element_type_t type, size_t dirtyRectsCount, cef_rect_t const *dirtyRects, const void *buffer, int width, int height);
#ifdef HAVE_SERVO
void render_image_servo_paint(Browser *b);
void render_image_servo_present(cef_render_handler_t *handler, cef_browser_t *browser);
void render_image_servo_setup(Browser *b, int w, int h);
#endif

static int
get_root_screen_rect(cef_render_handler_t *self, cef_browser_t *browser, cef_rect_t *rect)
{
   ecore_x_window_size_get(ecore_x_window_root_first_get(), &rect->width, &rect->height);
   return 1;
}

static int
get_view_rect(cef_render_handler_t *self, cef_browser_t *browser, cef_rect_t *rect)
{
   ECef_Client *ec;

   ec = browser_get_client(browser);
   edje_object_part_geometry_get(elm_layout_edje_get(ec->layout), "ecef.swallow.browser", &rect->x, &rect->y, &rect->width, &rect->height);
   //fprintf(stderr, "VIEW RECT: %dx%d\n", rect->width, rect->height);
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
   Eina_List *l;

   ec = browser_get_client(browser);
   b = browser_get(ec, browser);
   b->pw = b->w, b->ph = b->h;
   b->w = width, b->h = height;
   img = b->img;
   if (ec->current_page != b)
     evas_object_size_hint_aspect_set(img, EVAS_ASPECT_CONTROL_HORIZONTAL, b->w, b->h);
   if (gl_avail)
#ifdef HAVE_SERVO
     render_image_servo_paint(b);
#else
     paint_gl(ec, b, type, dirtyRectsCount, dirtyRects, buffer, width, height);
#endif
   else
     {
        o = elm_image_object_get(img);
        evas_object_image_size_set(o, width, height);
        evas_object_image_data_set(o, (void*)buffer);
        evas_object_image_size_set(o, width, height);
        for (r = 0; r < dirtyRectsCount; r++)
          evas_object_image_data_update_add(o, dirtyRects[r].x, dirtyRects[r].y, dirtyRects[r].width, dirtyRects[r].height);
     }
   EINA_LIST_FOREACH(b->clones, l, o)
     evas_object_size_hint_aspect_set(o, EVAS_ASPECT_CONTROL_HORIZONTAL, b->w, b->h);
   //fprintf(stderr, "PAINT %dx%d\n", width, height);
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
#ifdef HAVE_SERVO
   render_handler->on_present = render_image_servo_present;
#endif   
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
render_image_key(ECef_Client *ec, Evas *e, Evas_Object *obj, Evas_Event_Key_Down *ev, int up)
{
   cef_browser_host_t *host;
   cef_key_event_t event;
   KeyboardCode windows_key_code;
   Eina_Unicode *str;

   host = evas_object_data_get(obj, "browser_host");
   if (!host) return;

   windows_key_code = GdkEventToWindowsKeyCode(ev);
   event.windows_key_code = GetWindowsKeyCodeWithoutLocation(windows_key_code);

   event.native_key_code = ev->keycode;
   event.modifiers = modifiers_get(ev->modifiers);
   /* ecore, y u no have keycode defines??? */
   if ((ev->keysym >= 0xff80 /* space */) && (ev->keysym <= 0xffb9 /* kp_9 */))
     event.modifiers |= EVENTFLAG_IS_KEY_PAD;
   if (event.modifiers & EVENTFLAG_ALT_DOWN)
     event.is_system_key = 1;

   if (windows_key_code == VKEY_RETURN)
      // We need to treat the enter key as a key press of character \r.  This
      // is apparently just how webkit handles it and what it expects.
      event.unmodified_character = '\r';
   else if (ev->string)
     {
        // FIXME: fix for non BMP chars
        str = eina_unicode_utf8_to_unicode(ev->string, NULL);
        if (str)
          event.unmodified_character = *str;
        free(str);
     }

   // If ctrl key is pressed down, then control character shall be input.
   if (event.modifiers & EVENTFLAG_CONTROL_DOWN)
     event.character = GetControlCharacter(windows_key_code, event.modifiers & EVENTFLAG_SHIFT_DOWN);
   else
     event.character = event.unmodified_character;

   if (up)
     {
        event.type = KEYEVENT_KEYUP;
        host->send_key_event(host, &event);
        event.type = KEYEVENT_CHAR;
     }
   else
     event.type = KEYEVENT_RAWKEYDOWN;
   host->send_key_event(host, &event);
}

static void
render_image_key_down(ECef_Client *ec, Evas *e, Evas_Object *obj, Evas_Event_Key_Down *ev)
{
   render_image_key(ec, e, obj, ev, 0);
}

static void
render_image_key_up(ECef_Client *ec, Evas *e, Evas_Object *obj, Evas_Event_Key_Down *ev)
{
   render_image_key(ec, e, obj, ev, 1);
}

static void
render_image_geom(Browser *b, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   int x, y, w, h;

   evas_object_geometry_get(b->img, &x, &y, &w, &h);
   //fprintf(stderr, "IMG GEOM: %d,%d %dx%d\n", x, y, w, h);
}

void
render_image_new(ECef_Client *ec, Browser *b, cef_browser_host_t *host, int w, int h)
{
   Evas_Object *i;

   if (gl_avail)
     {
        b->img = i = elm_glview_version_add(ec->win,
#ifdef HAVE_SERVO
        EVAS_GL_GLES_2_X
#else
        EVAS_GL_GLES_3_X
#endif
        );
        if (!i) gl_avail = 0;
     }
   b->pw = b->w = w, b->ph = b->h = h;
   if (!b->img)
     b->img = i = elm_image_add(ec->win);
   evas_object_resize(i, w, h);
   evas_object_data_set(i, "browser_host", host);
   evas_object_data_set(i, "Browser", b);
   if (ec->current_page == b)
     elm_object_part_content_set(ec->layout, "ecef.swallow.browser", i);
   if (gl_avail)
#ifdef HAVE_SERVO
     render_image_servo_setup(b, w, h);
#else
     render_image_gl_setup(b, w, h);
#endif
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)render_image_mouse_down, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)render_image_mouse_up, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_MOVE, (Evas_Object_Event_Cb)render_image_mouse_move, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_OUT, (Evas_Object_Event_Cb)render_image_mouse_move_out, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_WHEEL, (Evas_Object_Event_Cb)render_image_mouse_wheel, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_KEY_DOWN, (Evas_Object_Event_Cb)render_image_key_down, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_KEY_UP, (Evas_Object_Event_Cb)render_image_key_up, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOVE, (Evas_Object_Event_Cb)render_image_geom, b);
   evas_object_event_callback_add(i, EVAS_CALLBACK_RESIZE, (Evas_Object_Event_Cb)render_image_geom, b);
   evas_object_show(i);
}

static void
render_image_clone_del(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Browser *b = data;

   b->clones = eina_list_remove(b->clones, obj);
}

Evas_Object *
render_image_clone(Browser *b)
{
   Evas_Object *img;
   ECef_Client *ec;

   if (!servo) return NULL; //temp

   ec = browser_get_client(b->browser);
   img = elm_image_add(ec->win);
   evas_object_size_hint_aspect_set(img, EVAS_ASPECT_CONTROL_HORIZONTAL, b->w, b->h);
   evas_object_event_callback_add(img, EVAS_CALLBACK_DEL, render_image_clone_del, b);
   if (ec->clone_update_cb)
     ec->clone_update_cb(b, img);
   b->clones = eina_list_append(b->clones, img);
   return img;
}
