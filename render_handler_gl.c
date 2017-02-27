#include "ecef.h"

#define GLERR do { on_error(api, __FUNCTION__, __LINE__); } while (0)

static void
on_error(Evas_GL_API *api, const char *func, int line)
{
   int _e = api->glGetError();
   if (_e)
     {
        fprintf(stderr, "%s:%d: GL error 0x%04x\n", func, line, _e);
        //abort();
     }
   return;
}

static float texture_vertices[] =
{
   1.0,  1.0,
   0.0,  0.0,
   1.0,  0.0,
   1.0,  1.0,
   0.0,  1.0,
   0.0,  0.0,
};

static float rectangle_fullscreen_vertices[] =
{
    1.0,  1.0,  0.0,
   -1.0, -1.0,  0.0,
    1.0, -1.0,  0.0,
    1.0,  1.0,  0.0,
   -1.0,  1.0,  0.0,
   -1.0, -1.0,  0.0
};

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


static GLuint
shader_compile(Evas_GL_API *api, GLenum type, const char *str)
{
   GLint status;
   GLuint shader;

   shader = api->glCreateShader(type);GLERR;
   api->glShaderSource(shader, 1, &str, NULL);GLERR;
   api->glCompileShader(shader);GLERR;
   api->glGetShaderiv(shader, GL_COMPILE_STATUS, &status);GLERR;
   if (status == GL_FALSE)
     {
        int size;
        char *buf;

        fprintf(stderr, "SHADER COMPILE ERROR: ");
        api->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        buf = malloc(size + 1);
        api->glGetShaderInfoLog(shader, size + 1, NULL, buf);
        fprintf(stderr, "%s\n", buf);
        free(buf);
     }
   return shader;
}

static void
render_image_gl_clone_update_cb(Browser *b, Evas_Object *o)
{
   Evas_Native_Surface ns;
   Evas_Object *img;

   ns.version = EVAS_NATIVE_SURFACE_VERSION;
   ns.type = EVAS_NATIVE_SURFACE_OPENGL;
   ns.data.opengl.texture_id = b->tex;
   ns.data.opengl.framebuffer_id = b->fbo;
   ns.data.opengl.internal_format = GL_RGBA;
   ns.data.opengl.format = GL_BGRA_EXT;
   ns.data.opengl.x = ns.data.opengl.y = 0;
   ns.data.opengl.w = b->w;
   ns.data.opengl.h = b->h;
   img = elm_image_object_get(o);

   evas_object_image_native_surface_set(img, &ns);
   evas_object_image_size_set(img, b->w, b->h);
   evas_object_image_pixels_dirty_set(img, 1);
}

static void
render_image_gl_clones_update(Browser *b)
{
   Evas_Object *o;
   Eina_List *l;

   EINA_LIST_FOREACH(b->clones, l, o)
     render_image_gl_clone_update_cb(b, o);
}

static void
render_image_gl_init(Evas_Object *obj)
{
   Browser *b;
   Evas_GL_API *api;
   GLuint vertexShader, fragmentShader, program;
   cef_browser_host_t *host;
   ECef_Client *ec;

   b = evas_object_data_get(obj, "Browser");
   ec = browser_get_client(b->browser);
   ec->clone_update_cb = render_image_gl_clone_update_cb;
   host = browser_get_host(b->browser);
   b->gl = elm_glview_evas_gl_get(obj);
   api = evas_gl_api_get(b->gl);
   b->glcfg = evas_gl_config_new();
   b->glcfg->color_format = EVAS_GL_RGB_888;
   b->glcfg->depth_bits = EVAS_GL_DEPTH_BIT_24;
   b->glcfg->stencil_bits = EVAS_GL_STENCIL_BIT_8;

   vertexShader = shader_compile(api, GL_VERTEX_SHADER, vert_shader);
   fragmentShader = shader_compile(api, GL_FRAGMENT_SHADER, frag_shader);
   api->glClearColor(0.0, 0.0, 0.0, 0.0);GLERR;

   b->program = program = api->glCreateProgram();GLERR;
   api->glAttachShader(program, vertexShader);GLERR;
   api->glAttachShader(program, fragmentShader);GLERR;
   api->glLinkProgram(program);GLERR;
   api->glDetachShader(program, vertexShader);GLERR;
   api->glDetachShader(program, fragmentShader);GLERR;

   api->glGenBuffers(1, &b->vbo);GLERR;
   api->glBindBuffer(GL_ARRAY_BUFFER, b->vbo);GLERR;
   api->glBufferData(GL_ARRAY_BUFFER, sizeof(texture_vertices), texture_vertices, GL_STATIC_DRAW);GLERR;
   api->glEnableVertexAttribArray(1);GLERR;
   api->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);GLERR;
   api->glBindBuffer(GL_ARRAY_BUFFER, 0);GLERR;

   api->glGenBuffers(1, &b->vbo2);GLERR;
   api->glBindBuffer(GL_ARRAY_BUFFER, b->vbo2);GLERR;
   api->glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_fullscreen_vertices), rectangle_fullscreen_vertices, GL_STATIC_DRAW);GLERR;
   api->glEnableVertexAttribArray(1);GLERR;
   api->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);GLERR;
   api->glBindBuffer(GL_ARRAY_BUFFER, 0);GLERR;

   api->glGenTextures(1, &b->tex);GLERR;

   api->glBindTexture(GL_TEXTURE_2D, b->tex);GLERR;
   api->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);GLERR;
   api->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);GLERR;
   api->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);GLERR;
   api->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);GLERR;
   api->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, b->w, b->h, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);GLERR;

   api->glGenFramebuffers(1, &b->fbo);GLERR;
   api->glBindFramebuffer(GL_FRAMEBUFFER, b->fbo);GLERR;
   api->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, b->tex, 0);GLERR;

#ifdef HAVE_SERVO
   if (servo)
     {
        host->initialize_compositing(host);GLERR;
     }
#endif
   api->glBindTexture(GL_TEXTURE_2D, 0);GLERR;
   api->glBindFramebuffer(GL_FRAMEBUFFER, 0);GLERR;
}

void
render_image_gl_present(cef_render_handler_t *handler EINA_UNUSED, cef_browser_t *browser EINA_UNUSED)
{
   //Browser *b = browser_get(browser_get_client(browser), browser);

//fprintf(stderr, "PRESENT\n");

}

static void
init_matrix(float matrix[16])
{
   matrix[0] = 1.0f;
   matrix[1] = 0.0f;
   matrix[2] = 0.0f;
   matrix[3] = 0.0f;

   matrix[4] = 0.0f;
   matrix[5] = 1.0f;
   matrix[6] = 0.0f;
   matrix[7] = 0.0f;

   matrix[8] = 0.0f;
   matrix[9] = 0.0f;
   matrix[10] = 1.0f;
   matrix[11] = 0.0f;

   matrix[12] = 0.0f;
   matrix[13] = 0.0f;
   matrix[14] = 0.0f;
   matrix[15] = 1.0f;
}

static void
render_image_gl_render(Evas_Object *obj)
{
   Browser *b;
   GLuint u;
   float mvp[16];
   Evas_GL_API *api;
   cef_browser_host_t *host;
//fprintf(stderr, "RENDER\n");
   b = evas_object_data_get(obj, "Browser");
   host = browser_get_host(b->browser);
   api = evas_gl_api_get(b->gl);
   api->glClearColor(0.0, 1.0, 0.0, 1.0);GLERR;
   if (servo && ((b->w != b->pw) || (b->h != b->ph)))
     {
        api->glBindTexture(GL_TEXTURE_2D, b->tex);GLERR;
        api->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, b->w, b->h, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);GLERR;
     }
   else if (!servo)
     {
        api->glBindTexture(GL_TEXTURE_2D, b->tex);GLERR;
     }
   else
     {
        api->glBindTexture(GL_TEXTURE_2D, 0);GLERR;
     }
   api->glBindFramebuffer(GL_FRAMEBUFFER, b->fbo);GLERR;
#ifdef HAVE_SERVO
   if (servo)
     {
        host->composite(host); api->glGetError();
     }
   else
#endif
     {
        api->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, b->w, b->h, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, b->buffer);GLERR;
     }
   api->glBindFramebuffer(GL_FRAMEBUFFER, 0);GLERR;

   api->glViewport(0, 0, b->w, b->h); GLERR;
   api->glClearColor(0.0, 0.0, 1.0, 1.0); GLERR;
   api->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);GLERR;

   api->glEnable(GL_BLEND); GLERR;
   api->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); GLERR;
   api->glUseProgram(b->program);GLERR;

   api->glBindBuffer(GL_ARRAY_BUFFER, b->vbo2); GLERR;
   api->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); GLERR;
   api->glEnableVertexAttribArray(0); GLERR;
   api->glBindBuffer(GL_ARRAY_BUFFER, b->vbo);GLERR;
   api->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0); GLERR;
   api->glEnableVertexAttribArray(1); GLERR;

   init_matrix(mvp);
   u = api->glGetUniformLocation(b->program, "mvpMatrix"); GLERR;
   api->glUniformMatrix4fv(u, 1, GL_FALSE, mvp); GLERR;

   u = api->glGetUniformLocation(b->program, "tex"); GLERR;
   api->glUniform1i(u, 0); GLERR;

   api->glActiveTexture(GL_TEXTURE0);
   api->glBindTexture(GL_TEXTURE_2D, b->tex);

   api->glDrawArrays(GL_TRIANGLES, 0, 6);GLERR;
   api->glBindBuffer(GL_ARRAY_BUFFER, 0);GLERR;
}

void
render_image_gl_paint(Browser *b)
{
   elm_glview_size_set(b->img, b->w, b->h);
   elm_glview_changed_set(b->img);
   render_image_gl_clones_update(b);
}

void
render_image_gl_setup(Browser *b, int w, int h)
{
   if (!getenv("WAYLAND_DISPLAY"))
     elm_glview_mode_set(b->img, ELM_GLVIEW_DEPTH | ELM_GLVIEW_STENCIL /*| ELM_GLVIEW_DIRECT */);
   elm_glview_init_func_set(b->img, render_image_gl_init);
   //elm_glview_resize_policy_set(b->img, ELM_GLVIEW_RESIZE_POLICY_SCALE);
   elm_glview_render_func_set(b->img, render_image_gl_render);
   elm_glview_size_set(b->img, w, h);
   elm_glview_changed_set(b->img);
}
