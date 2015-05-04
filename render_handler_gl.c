#define RENDER_HANDLER_GL
#include <GL/gl.h>
#include "ecef.h"

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
render_gl_init(void)
{
   if (render_init_done) return;
   glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST); GLERR();

   glClearColor(0.0f, 0.0f, 0.0f, 1.0f); GLERR();

   // Necessary for non-power-of-2 textures to render correctly.
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1); GLERR();

   render_init_done = EINA_TRUE;
}

static void
render_image_gl_surface_update(Browser *b, Evas_Object *o, int w, int h)
{
   Evas_Native_Surface ns = {0};

   ns.version = EVAS_NATIVE_SURFACE_VERSION;
   ns.type = EVAS_NATIVE_SURFACE_OPENGL;
   ns.data.opengl.texture_id = b->texture_id;
   ns.data.opengl.format = GL_BGRA;
   ns.data.opengl.w = w;
   ns.data.opengl.h = h;

   evas_object_image_native_surface_set(o, &ns);
}

static void
render_gl(Browser *b, int w, int h)
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

void
paint_gl(ECef_Client *ec, Browser *b, Evas_Object *o, cef_paint_element_type_t type,
      size_t dirtyRectsCount, cef_rect_t const *dirtyRects, const void *buffer, int width, int height)
{
   Evas_Native_Surface *ns;
   int ww, wh;
   size_t r;

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
        render_image_gl_surface_update(b, o, width, height);
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


static void
render_image_gl_pixels_get(void *d, Evas_Object *obj)
{
   int w, h;

   evas_object_image_size_get(obj, &w, &h);
   render_gl(d, w, h);
}

static void
render_image_gl_del(Browser *b, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   if (b->texture_id)
     glDeleteTextures(1, &b->texture_id);
}

void
render_image_gl_init(Browser *b, Evas_Object *o, int w, int h)
{
   if (!render_init_done)
     render_gl_init();
   // Create the texture.
   glGenTextures(1, &b->texture_id); GLERR();

   glBindTexture(GL_TEXTURE_2D, b->texture_id); GLERR();
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                   GL_NEAREST); GLERR();
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                   GL_NEAREST); GLERR();
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); GLERR();

   render_image_gl_surface_update(b, o, w, h);
   evas_object_image_pixels_get_callback_set(o, render_image_gl_pixels_get, b);
   evas_object_event_callback_add(b->img, EVAS_CALLBACK_DEL, (Evas_Object_Event_Cb)render_image_gl_del, b);
}
