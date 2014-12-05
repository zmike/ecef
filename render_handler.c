#include "ecef.h"
#include <Ecore_X.h>
#include <Evas_GL.h>

#ifndef GL_POLYGON_SMOOTH_HINT
# define GL_POLYGON_SMOOTH_HINT 0x0C53
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
# define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif
#define GLERR() do { on_error(__FUNCTION__, __LINE__); } while (0)
static Eina_Bool render_init_done = EINA_FALSE;

static float texture_vertices[] =
{
   1.0,  1.0,
   0.0,  0.0,
   1.0,  0.0,
   1.0,  1.0,
   0.0,  1.0,
   0.0,  0.0,
};

static float screen_vertices[] =
{
    1.0,  1.0,  0.0,
   -1.0, -1.0,  0.0,
    1.0, -1.0,  0.0,
    1.0,  1.0,  0.0,
   -1.0,  1.0,  0.0,
   -1.0, -1.0,  0.0
};

static const char vertex_texture[] =
      "attribute vec2 vTexCoord;\n"
      "attribute vec3 vPosition;\n"
      "varying vec2 texcoord;\n"
      "void main()\n"
      "{\n"
      "   texcoord = vTexCoord;\n"
      "   gl_Position = vec4(vPosition, 0);\n"
      "}\n";

static const char fragment_texture[] =
   "varying vec2 texcoord;\n"
   "uniform sampler2D tex;\n"
   "void main()\n"
   "{\n"
   "gl_FragColor = texture2D(tex, texcoord.xy);"
   "}\n";

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
_shader_verify(Evas_GL_API *gl, GLuint shader)
{
   GLint status;
   GLsizei len;
   char buf[4096];

   gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
   if (!status)
     {
        gl->glGetShaderInfoLog(shader, sizeof(buf), &len, buf);
        fprintf(stderr, "Shader compilation failed:\nFailed with: %s\n", buf);
     }
}

static void
surface_update(ECef_Client *ec, Evas_Object *i, int w, int h)
{
   static Evas_GL_Config cfg =
   {
       EVAS_GL_RGBA_8888,
       EVAS_GL_DEPTH_BIT_8,
       EVAS_GL_STENCIL_NONE,
       EVAS_GL_OPTIONS_DIRECT,
       EVAS_GL_MULTISAMPLE_NONE
   };
   Evas_GL_Surface *s, *sprev;
   Evas_Native_Surface ns;
   Evas_Object *o;

   o = elm_image_object_get(i);
   evas_object_image_native_surface_set(o, NULL);
   evas_gl_make_current(ec->gl, NULL, NULL);
   s = evas_gl_surface_create(ec->gl, &cfg, w, h);
   sprev = eina_hash_set(ec->surfaces, &i, s);
   if (sprev)
     evas_gl_surface_destroy(ec->gl, sprev);
   evas_gl_native_surface_get(ec->gl, s, &ns);
   evas_object_image_native_surface_set(o, &ns);
   evas_object_image_pixels_dirty_set(o, 1);

   evas_gl_make_current(ec->gl, s, ec->glctx);
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
paint(cef_render_handler_t *self, cef_browser_t *browser, cef_paint_element_type_t type,
      size_t dirtyRectsCount, cef_rect_t const *dirtyRects, const void *buffer,
      int width, int height)
{
   ECef_Client *ec;
   Evas_Object *img, *o;
   size_t r;
   int id;

   ec = browser_get_client(browser);
   id = browser->get_identifier(browser);
   img = eina_hash_find(ec->browsers, &id);
   if (!img)
     {GLERR();
        img = render_image_new(ec, browser->get_host(browser), width, height);
        eina_hash_add(ec->browsers, &id, img);
     }
   o = elm_image_object_get(img);
   evas_object_image_size_set(o, width, height);
   evas_object_resize(img, width, height);
   if (ec->gl)
     {
        Evas_GL_API *gl = evas_gl_api_get(ec->gl);
        Evas_Native_Surface *ns;
        GLuint u;
GLERR();
        ns = evas_object_image_native_surface_get(o);GLERR();
        gl->glViewport(0, 0, width, height);GLERR();
        gl->glClearColor(0, 0, 0, 1);GLERR();
        gl->glClear(GL_COLOR_BUFFER_BIT);GLERR();

        gl->glEnable(GL_BLEND);GLERR();
        gl->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);GLERR();
        gl->glEnable(GL_TEXTURE_2D);GLERR();
        gl->glBindTexture(GL_TEXTURE_2D, ns->data.opengl.texture_id);GLERR();
        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width);GLERR();

        if ((width != ns->data.opengl.w) || (height != ns->data.opengl.h))
          {
             surface_update(ec, img, width, height);
             gl->glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);GLERR();
             gl->glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);GLERR();
             gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                GL_BGRA_IMG, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);GLERR();
             fprintf(stderr, "NEW ");
          }
        else
          {
             for (r = 0; r < dirtyRectsCount; r++)
               {
                  gl->glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, dirtyRects[r].x);GLERR();
                  gl->glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, dirtyRects[r].y);GLERR();
                  gl->glTexSubImage2D(GL_TEXTURE_2D, 0, dirtyRects[r].x, dirtyRects[r].y, dirtyRects[r].width,
                                  dirtyRects[r].height, GL_BGRA_IMG, GL_UNSIGNED_INT_8_8_8_8_REV,
                                  buffer);GLERR();
               }
             fprintf(stderr, "UPDATE ");
          }
        gl->glUseProgram(ec->prog);GLERR();
        gl->glBindBuffer(GL_ARRAY_BUFFER, ec->vbo[0]);GLERR();
        gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);GLERR();
        gl->glEnableVertexAttribArray(0);GLERR();

        gl->glBindBuffer(GL_ARRAY_BUFFER, ec->vbo[1]);GLERR();
        gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);GLERR();
        gl->glEnableVertexAttribArray(1);GLERR();
        gl->glDrawElements(GL_TRIANGLES, 4, GL_FLOAT, 0);GLERR();
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
render_init(ECef_Client *ec)
{
   Evas_GL_API *glapi;
   const char *p;

   if (render_init_done) return;
   glapi = evas_gl_api_get(ec->gl);
   glapi->glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);GLERR();

   glapi->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);GLERR();

   glapi->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);GLERR();

   glapi->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);GLERR();
   glapi->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);GLERR();
   glapi->glGenBuffers(2, ec->vbo);GLERR();
   glapi->glBindBuffer(GL_ARRAY_BUFFER, ec->vbo[0]);GLERR();
   glapi->glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_STATIC_DRAW);GLERR();
   glapi->glBindBuffer(GL_ARRAY_BUFFER, ec->vbo[1]);GLERR();
   glapi->glBufferData(GL_ARRAY_BUFFER, sizeof(texture_vertices), texture_vertices, GL_STATIC_DRAW);GLERR();

   p = vertex_texture;
   ec->vshader = glapi->glCreateShader(GL_VERTEX_SHADER);GLERR();
   glapi->glShaderSource(ec->vshader, 1, &p, NULL);GLERR();
   glapi->glCompileShader(ec->vshader);
   _shader_verify(glapi, ec->vshader);

   p = fragment_texture;
   ec->fshader = glapi->glCreateShader(GL_FRAGMENT_SHADER);GLERR();
   glapi->glShaderSource(ec->fshader, 1, &p, NULL);GLERR();
   glapi->glCompileShader(ec->fshader);
   _shader_verify(glapi, ec->fshader);
   

   ec->prog = glapi->glCreateProgram();GLERR();
   glapi->glAttachShader(ec->prog, ec->vshader);GLERR();
   glapi->glAttachShader(ec->prog, ec->fshader);GLERR();
   glapi->glBindAttribLocation(ec->prog, 0, "vPosition");GLERR();
   glapi->glBindAttribLocation(ec->prog, 1, "vTexCoord");GLERR();
   glapi->glLinkProgram(ec->prog);
   _shader_verify(glapi, ec->prog);
   if (glapi->glTexEnvf)
     glapi->glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   render_init_done = EINA_TRUE;
}

Evas_Object *
render_image_new(ECef_Client *ec, cef_browser_host_t *host, int w, int h)
{
   Evas_Object *i, *o;
   Evas_GL *gl = ec->gl;

   i = elm_image_add(ec->win);
   elm_image_no_scale_set(i, 1);
   elm_image_resizable_set(i, 0, 0);
   elm_image_smooth_set(i, 0);
   o = elm_image_object_get(i);
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   if (!gl)
     ec->gl = gl = evas_gl_new(evas_object_evas_get(ec->win));
   if (!ec->glctx)
     ec->glctx = evas_gl_context_create(gl, NULL);
   if (gl)
     {GLERR();
        surface_update(ec, i, w, h);
        if (!render_init_done)
          render_init(ec);
     }
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)render_image_mouse_down, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)render_image_mouse_up, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_MOVE, (Evas_Object_Event_Cb)render_image_mouse_move, ec);
   evas_object_event_callback_add(i, EVAS_CALLBACK_MOUSE_OUT, (Evas_Object_Event_Cb)render_image_mouse_move_out, ec);
   evas_object_event_callback_add(ec->win, EVAS_CALLBACK_RESIZE, (Evas_Object_Event_Cb)render_image_resize, host);
   EXPAND(i);
   FILL(i);
   evas_object_show(i);
   evas_object_data_set(i, "browser_host", host);
   return i;
}
