#include "ecef.h"

/* references some code from https://github.com/andmcgregor/cefgui
 * http://creativecommons.org/licenses/by/4.0/
 */

static const char *frag_shader = \
"#version 300 es\n" \
"precision mediump float;\n" \
"in vec2 Texcoord;\n" \
"out vec4 fragColor;\n" \
"uniform sampler2D tex;\n" \
"void main() {\n" \
"  fragColor = texture2D(tex, Texcoord);\n" \
"  if (fragColor.a < 0.1)\n" \
"  {\n" \
"    discard;\n" \
"  }\n" \
"}";

static const char *vert_shader = \
"#version 300 es\n" \
"in vec2 position;\n" \
"out vec2 Texcoord;\n" \
"void main() {\n" \
"  Texcoord = (vec2(position.x + 1.0f, position.y - 1.0f) * 0.5);\n" \
"  Texcoord.y *= -1.0f;\n" \
"  gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);\n" \
"}";

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif
#define GLERR() do { on_error(__FUNCTION__, __LINE__); } while (0)

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
render_gl(Browser *b, int w, int h)
{
   Evas_GL_API *gl;

GLERR();
   gl = elm_glview_gl_api_get(b->img);GLERR();

   gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);GLERR();
   gl->glUseProgram(b->program);GLERR();
   gl->glBindVertexArray(b->vao);GLERR();

   gl->glBindBuffer(GL_ARRAY_BUFFER, b->vbo);GLERR();
   gl->glDrawArrays(GL_TRIANGLES, 0, 6);GLERR();
   gl->glBindBuffer(GL_ARRAY_BUFFER, 0);GLERR();

   gl->glBindVertexArray(0);GLERR();
   gl->glUseProgram(0);GLERR();
}

void
paint_gl(ECef_Client *ec, Browser *b, cef_paint_element_type_t type,
      size_t dirtyRectsCount, cef_rect_t const *dirtyRects, const void *buffer, int width, int height)
{
   int ww, wh, gw, gh;
   size_t r;
   Evas_GL_API *gl;

   //elm_glview_current_set(b->img, EINA_TRUE);GLERR();
   gl = elm_glview_gl_api_get(b->img);GLERR();
   elm_glview_size_get(b->img, &gw, &gh);GLERR();
   gl->glBindTexture(GL_TEXTURE_2D, b->texture_id);GLERR();

   evas_object_geometry_get(ec->win, NULL, NULL, &ww, &wh);
   elm_glview_size_set(b->img, width, height);GLERR();
   if ((width != gw) || (height != gh) ||
       ((dirtyRectsCount == 1) && (dirtyRects[0].width == ww) && (dirtyRects[0].height == wh)))
     {
        //elm_glview_current_set(b->img, EINA_TRUE);GLERR();
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);
        GLERR();
        fprintf(stderr, "NEW ");
     }
   //else
     //{
        //for (r = 0; r < dirtyRectsCount; r++)
          //{
             //gl->glPixelStorei(GL_UNPACK_SKIP_PIXELS, dirtyRects[r].x);GLERR();
             //gl->glPixelStorei(GL_UNPACK_SKIP_ROWS, dirtyRects[r].y);GLERR();
             //gl->glTexSubImage2D(GL_TEXTURE_2D, 0, dirtyRects[r].x, dirtyRects[r].y, dirtyRects[r].width,
                             //dirtyRects[r].height, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV,
                             //buffer);GLERR();
          //}
        //fprintf(stderr, "UPDATE ");
     //}

   gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);GLERR();
   gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);GLERR();
   gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);GLERR();
   gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);GLERR();


   //elm_glview_current_set(b->img, EINA_FALSE);
   elm_glview_changed_set(b->img);
}


static void
render_image_gl_pixels_get(Evas_Object *obj)
{
   int w, h;

   elm_glview_size_get(obj, &w, &h);
   render_gl(evas_object_data_get(obj, "Browser"), w, h);
}

static void
render_image_gl_del(Browser *b, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   //if (b->texture_id)
     //glDeleteTextures(1, &b->texture_id);
}

static GLuint
shader_compile(Evas_GL_API *gl, GLenum type, const char *str)
{
   GLint status;
   GLuint shader;

   shader = gl->glCreateShader(type);GLERR();
   gl->glShaderSource(shader, 1, &str, NULL);GLERR();
   gl->glCompileShader(shader);GLERR();
   gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &status);GLERR();
   if (status == GL_FALSE)
     {
        int size;
        char *buf;

        fprintf(stderr, "SHADER COMPILE ERROR: ");
        gl->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        buf = malloc(size + 1);
        gl->glGetShaderInfoLog(shader, size + 1, NULL, buf);
        fprintf(stderr, "%s\n", buf);
        free(buf);
     }
   return shader;
}

static void
render_image_gl_init(Evas_Object *obj)
{
   Evas_GL_API *gl;
   Browser *b;
   GLuint vertexShader, fragmentShader, program;
   GLint positionLoc;
   float coords[] = {-1.0,-1.0,-1.0,1.0,1.0,-1.0,1.0,-1.0,-1.0,1.0,1.0,1.0};

   b = evas_object_data_get(obj, "Browser");
   gl = elm_glview_gl_api_get(b->img);

   vertexShader = shader_compile(gl, GL_VERTEX_SHADER, vert_shader);
   fragmentShader = shader_compile(gl, GL_FRAGMENT_SHADER, frag_shader);
   gl->glGenTextures(1, &b->texture_id); GLERR();
   gl->glClearColor(0.0, 0.0, 0.0, 0.0);GLERR();

   b->program = program = gl->glCreateProgram();GLERR();

   gl->glAttachShader(program, vertexShader);GLERR();
   gl->glAttachShader(program, fragmentShader);GLERR();
   gl->glBindAttribLocation(program, 1, "position");GLERR();
   gl->glLinkProgram(program);GLERR();
   gl->glDetachShader(program, vertexShader);GLERR();
   gl->glDetachShader(program, fragmentShader);GLERR();

   gl->glGenVertexArrays(1, &b->vao);GLERR();
   gl->glBindVertexArray(b->vao);GLERR();
   gl->glGenBuffers(1, &b->vbo);GLERR();
   gl->glBindBuffer(GL_ARRAY_BUFFER, b->vbo);GLERR();
   gl->glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW);GLERR();
   gl->glEnableVertexAttribArray(1);GLERR();
   gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);GLERR();
   gl->glBindBuffer(GL_ARRAY_BUFFER, 0);GLERR();
   gl->glBindVertexArray(0);GLERR();
}

void
render_image_gl_setup(Browser *b, int w, int h)
{
   elm_glview_mode_set(b->img, ELM_GLVIEW_ALPHA | ELM_GLVIEW_DEPTH | ELM_GLVIEW_DIRECT);
   elm_glview_render_func_set(b->img, render_image_gl_pixels_get);
   elm_glview_init_func_set(b->img, render_image_gl_init);
   elm_glview_size_set(b->img, w, h);
   //elm_glview_current_set(b->img, 1);
   //render_image_gl_init(b->img);
   //elm_glview_current_set(b->img, 0);
   
   //evas_object_event_callback_add(b->img, EVAS_CALLBACK_DEL, (Evas_Object_Event_Cb)render_image_gl_del, b);
}
