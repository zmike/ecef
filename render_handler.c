#include "ecef.h"
#include <Ecore_X.h>

#ifndef GL_POLYGON_SMOOTH_HINT
# define GL_POLYGON_SMOOTH_HINT 0x0C53
#endif


#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

#define GLERR() do { on_error(__FUNCTION__, __LINE__); } while (0)
static Eina_Bool render_init_done = EINA_FALSE;

static void render_image_new(ECef_Client *ec, cef_browser_host_t *host, int w, int h);

void
on_error(const char *func, int line)
{
   int _e = glGetError();
   if (_e)
     {
        fprintf(stderr, "%s:%d: GL error 0x%04x\n", func, line, _e);
        //abort();
     }
   return;
}

static void
surface_update(ECef_Client *ec, Browser *b, int w, int h)
{
   Evas_Native_Surface ns = {0};

   ns.version = EVAS_NATIVE_SURFACE_VERSION;
   ns.type = EVAS_NATIVE_SURFACE_OPENGL;
   ns.data.opengl.texture_id = b->texture_id;
   ns.data.opengl.format = GL_BGRA;
   ns.data.opengl.w = w;
   ns.data.opengl.h = h;

   evas_object_image_native_surface_set(elm_image_object_get(b->img), &ns);
}

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
   Evas_Object *img;

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
render(Browser *b)
{
   struct {
     float tu, tv;
     float x, y, z;
   } static vertices[] = {
     {0.0f, 1.0f, -1.0f, -1.0f, 0.0f},
     {1.0f, 1.0f,  1.0f, -1.0f, 0.0f},
     {1.0f, 0.0f,  1.0f,  1.0f, 0.0f},
     {0.0f, 0.0f, -1.0f,  1.0f, 0.0f}
   };
   int w, h;

   elm_image_object_size_get(b->img, &w, &h);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GLERR();

   glMatrixMode(GL_MODELVIEW); GLERR();
   glLoadIdentity(); GLERR();

   // Match GL units to screen coordinates.
   glViewport(0, 0, w, h); GLERR();
   glMatrixMode(GL_PROJECTION); GLERR();
   glLoadIdentity(); GLERR();

   // Draw the background gradient.
   glPushAttrib(GL_ALL_ATTRIB_BITS); GLERR();
   // Don't check for errors until glEnd().
   glBegin(GL_QUADS);
   glColor4f(1.0, 0.0, 0.0, 1.0);  // red
   glVertex2f(-1.0, -1.0);
   glVertex2f(1.0, -1.0);
   glColor4f(0.0, 0.0, 1.0, 1.0);  // blue
   glVertex2f(1.0, 1.0);
   glVertex2f(-1.0, 1.0);
   glEnd(); GLERR();
   glPopAttrib(); GLERR();

   // Rotate the view based on the mouse spin.
   //if (spin_x_ != 0) {
     //glRotatef(-spin_x_, 1.0f, 0.0f, 0.0f); GLERR();
   //}
   //if (spin_y_ != 0) {
     //glRotatef(-spin_y_, 0.0f, 1.0f, 0.0f); GLERR();
   //}
#if 0
   if (transparent_) {
     // Alpha blending style. Texture values have premultiplied alpha.
     glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); GLERR();

     // Enable alpha blending.
     glEnable(GL_BLEND); GLERR();
   }
#endif
   // Enable 2D textures.
   glEnable(GL_TEXTURE_2D); GLERR();

   // Draw the facets with the texture.
   glBindTexture(GL_TEXTURE_2D, b->texture_id); GLERR();
   glInterleavedArrays(GL_T2F_V3F, 0, vertices); GLERR();
   glDrawArrays(GL_QUADS, 0, 4); GLERR();

   // Disable 2D textures.
   glDisable(GL_TEXTURE_2D); GLERR();
#if 0
   if (transparent_) {
     // Disable alpha blending.
     glDisable(GL_BLEND); GLERR();
   }
#endif
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
   int id;

   ec = browser_get_client(browser);
   b = browser_get(ec, browser);
   if (!b->img)
     render_image_new(ec, browser->get_host(browser), width, height);
   img = b->img;
   o = elm_image_object_get(img);
   evas_object_image_size_set(o, width, height);
   if (ec->gl_avail)
     {
        Evas_Native_Surface *ns;
        int ww, wh;

#if 0
        if (transparent_) {
          // Enable alpha blending.
          glEnable(GL_BLEND); GLERR();
        }
#endif
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, b->texture_id);
        GLERR();
        ns = evas_object_image_native_surface_get(o);GLERR();

        evas_object_geometry_get(ec->win, NULL, NULL, &ww, &wh);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, ww);GLERR();

        if ((width != ns->data.opengl.w) || (height != ns->data.opengl.h) ||
            ((dirtyRectsCount == 1) && (dirtyRects[0].width == ww) && (dirtyRects[0].height == wh)))
          {
             glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);GLERR();
             glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);GLERR();
             surface_update(ec, b, width, height);
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);GLERR();
             fprintf(stderr, "NEW ");
          }
        else
          {
             for (r = 0; r < dirtyRectsCount; r++)
               {
                  glPixelStorei(GL_UNPACK_SKIP_PIXELS, dirtyRects[r].x);GLERR();
                  glPixelStorei(GL_UNPACK_SKIP_ROWS, dirtyRects[r].y);GLERR();
                  glTexSubImage2D(GL_TEXTURE_2D, 0, dirtyRects[r].x, dirtyRects[r].y, dirtyRects[r].width,
                                  dirtyRects[r].height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                                  buffer);GLERR();
               }
             fprintf(stderr, "UPDATE ");
          }
        glDisable(GL_TEXTURE_2D);
        evas_object_image_pixels_dirty_set(o, 1);
     }
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
render_image_del(Browser *b, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   ECef_Client *ec;

   ec = browser_get_client(b->browser);
   if (b->texture_id)
     glDeleteTextures(1, &b->texture_id);
}

static void
render_image_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   cef_browser_host_t *host = data;

   host->was_resized(host);
}

static void
render_init(ECef_Client *ec)
{
   if (render_init_done) return;
   glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST); GLERR();

   glClearColor(0.0f, 0.0f, 0.0f, 1.0f); GLERR();

   // Necessary for non-power-of-2 textures to render correctly.
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1); GLERR();

   render_init_done = EINA_TRUE;
}

static void
render_image_pixels_get(void *d, Evas_Object *obj EINA_UNUSED)
{
   render(d);
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
     {
        GLERR();
        if (!render_init_done)
          render_init(ec);
        // Create the texture.
        glGenTextures(1, &b->texture_id); GLERR();

        glBindTexture(GL_TEXTURE_2D, b->texture_id); GLERR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_NEAREST); GLERR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        GL_NEAREST); GLERR();
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); GLERR();

        surface_update(ec, b, w, h);
        evas_object_image_pixels_get_callback_set(o, render_image_pixels_get, b);
     }
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)render_image_mouse_down, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)render_image_mouse_up, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_MOVE, (Evas_Object_Event_Cb)render_image_mouse_move, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_OUT, (Evas_Object_Event_Cb)render_image_mouse_move_out, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_WHEEL, (Evas_Object_Event_Cb)render_image_mouse_wheel, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_DEL, (Evas_Object_Event_Cb)render_image_del, b);
   evas_object_show(i);
   evas_object_data_set(i, "browser_host", host);
}
