#include "ecef.h"

#ifdef HAVE_SERVO
GLuint shader_compile(Evas_GL_API *gl, GLenum type, const char *str);

#define GLERR do { on_error(api, __FUNCTION__, __LINE__); } while (0)

static void
on_error(Evas_GL_API *api, const char *func, int line)
{
   int _e = api->glGetError();
   if (_e)
     {
        fprintf(stderr, "%s:%d: GL error 0x%04x\n", func, line, _e);
        abort();
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

/* Vertext Shader Source */
static const char vertex_shader[] =
      "attribute vec4 vPosition;\n"
      "attribute vec3 inColor;\n"
      "uniform mat4 mvpMatrix;"
      "varying vec3 outColor;\n"
      "void main()\n"
      "{\n"
      "   outColor = inColor;\n"
      "   gl_Position = mvpMatrix * vPosition;\n"
      "}\n";

/* Fragment Shader Source */
static const char fragment_shader[] =
      "#ifdef GL_ES\n"
      "precision mediump float;\n"
      "#endif\n"
      "varying vec3 outColor;\n"
      "uniform float alpha;\n"
      "void main()\n"
      "{\n"
      "   gl_FragColor = alpha * vec4 (outColor, 1.0);\n"
      "}\n";

static void
render_image_servo_init(Evas_Object *obj)
{
   Browser *b;
   Evas_GL_API *api;
   GLuint vertexShader, fragmentShader, program;
   cef_browser_host_t *host;

   b = evas_object_data_get(obj, "Browser");
   host = browser_get_host(b->browser);
   b->gl = elm_glview_evas_gl_get(obj);
   api = evas_gl_api_get(b->gl);
   b->glcfg = evas_gl_config_new();
   b->glcfg->color_format = EVAS_GL_RGB_888;
   b->glcfg->depth_bits = EVAS_GL_DEPTH_BIT_24;
   b->glcfg->stencil_bits = EVAS_GL_STENCIL_BIT_8;

   vertexShader = shader_compile(api, GL_VERTEX_SHADER, vertex_shader);
   fragmentShader = shader_compile(api, GL_FRAGMENT_SHADER, fragment_shader);
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

   host->initialize_compositing(host);GLERR;
   api->glBindTexture(GL_TEXTURE_2D, 0);GLERR;
   api->glBindFramebuffer(GL_FRAMEBUFFER, 0);GLERR;
}

void
render_image_servo_present(cef_render_handler_t *handler EINA_UNUSED, cef_browser_t *browser)
{
   Browser *b = browser_get(browser_get_client(browser), browser);

fprintf(stderr, "PRESENT\n");

}

static void
render_image_servo_render(Evas_Object *obj)
{
   Browser *b;
   Evas_GL_API *api;
   cef_browser_host_t *host;
fprintf(stderr, "RENDER\n");
   b = evas_object_data_get(obj, "Browser");
   host = browser_get_host(b->browser);
   api = evas_gl_api_get(b->gl);
   api->glClearColor(0.0, 1.0, 0.0, 1.0);GLERR;
   if ((b->w != b->pw) || (b->h != b->ph))
     {
        api->glBindTexture(GL_TEXTURE_2D, b->tex);GLERR;
        api->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, b->w, b->h, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);GLERR;
        api->glBindTexture(GL_TEXTURE_2D, 0);GLERR;
     }
   api->glBindFramebuffer(GL_FRAMEBUFFER, b->fbo);GLERR;
   host->composite(host);
   api->glBindFramebuffer(GL_FRAMEBUFFER, 0);GLERR;

   api->glViewport(0, 0, b->w, b->h); GLERR;
   api->glClearColor(0.0, 0.0, 1.0, 1.0); GLERR;
   api->glClear(GL_COLOR_BUFFER_BIT); GLERR;

   api->glEnable(GL_BLEND); GLERR;
   api->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); GLERR;
   api->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);GLERR;
   api->glUseProgram(b->program);GLERR;

   api->glBindBuffer(GL_ARRAY_BUFFER, b->vbo2); GLERR;
   api->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); GLERR;
   api->glEnableVertexAttribArray(0); GLERR;
   api->glBindBuffer(GL_ARRAY_BUFFER, b->vbo);GLERR;
   api->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0); GLERR;
   api->glEnableVertexAttribArray(1); GLERR;
   api->glDrawArrays(GL_TRIANGLES, 0, 6);GLERR;
   api->glBindBuffer(GL_ARRAY_BUFFER, 0);GLERR;
}

void
render_image_servo_paint(Browser *b)
{
fprintf(stderr, "PAINT\n");
   elm_glview_size_set(b->img, b->w, b->h);
   elm_glview_changed_set(b->img);
}

void
render_image_servo_setup(Browser *b, int w, int h)
{
   elm_glview_mode_set(b->img, ELM_GLVIEW_DEPTH | ELM_GLVIEW_STENCIL /*| ELM_GLVIEW_DIRECT */);
   elm_glview_init_func_set(b->img, render_image_servo_init);
   //elm_glview_resize_policy_set(b->img, ELM_GLVIEW_RESIZE_POLICY_SCALE);
   /* force setup of internal render callbacks because glview is a stupid widget */
   elm_glview_render_func_set(b->img, render_image_servo_render);
   elm_glview_size_set(b->img, w, h);
   elm_glview_changed_set(b->img);
}

#endif
