#include "ecef.h"

#ifdef HAVE_SERVO
static void
render_image_servo_init_job(Browser *b)
{
   Evas_GL_API *api = evas_gl_api_get(b->gl);
   cef_browser_host_t *host;

   host = browser_get_host(b->browser);
   evas_gl_make_current(b->gl, b->glsfc, b->glctx);
   api->glGenTextures(1, &b->tex);

   api->glBindTexture(GL_TEXTURE_2D, b->tex);
   api->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   api->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   api->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   api->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   api->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, b->w, b->h, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);

   api->glGenFramebuffers(1, &b->fbo);
   api->glBindFramebuffer(GL_FRAMEBUFFER, b->fbo);
   api->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, b->tex, 0);

   host->initialize_compositing(host);
   api->glBindFramebuffer(GL_FRAMEBUFFER, 0);
   api->glBindTexture(GL_TEXTURE_2D, 0);
   b->gljob = NULL;
}

static void
render_image_servo_init(Evas_Object *obj)
{
   Browser *b;
   int attr = EVAS_GL_NONE;

   b = evas_object_data_get(obj, "Browser");
   b->gl = elm_glview_evas_gl_get(obj);
   b->glcfg = evas_gl_config_new();
   b->glcfg->color_format = EVAS_GL_RGB_888;
   b->glcfg->depth_bits = EVAS_GL_DEPTH_BIT_24;
   b->glcfg->stencil_bits = EVAS_GL_STENCIL_BIT_8;
   b->glctx = evas_gl_context_create(b->gl, evas_gl_current_context_get(b->gl));
   b->glsfc = evas_gl_pbuffer_surface_create(b->gl, b->glcfg, 1, 1, &attr);
   b->gljob = ecore_job_add((Ecore_Cb)render_image_servo_init_job, b);
}

void
render_image_servo_present(cef_render_handler_t *handler EINA_UNUSED, cef_browser_t *browser)
{
   Browser *b = browser_get(browser_get_client(browser), browser);

   elm_glview_size_set(b->img, b->w, b->h);
   elm_glview_changed_set(b->img);
}

static void
render_image_servo_render(Evas_Object *obj)
{
   Browser *b;
   Evas_GL_API *api;

   b = evas_object_data_get(obj, "Browser");
   api = evas_gl_api_get(b->gl);
   api->glBindTexture(GL_TEXTURE_2D, b->tex);
   api->glDrawArrays(GL_TRIANGLES, 0, 6);
}

void
render_image_servo_paint(Browser *b)
{
   cef_browser_host_t *host;
   Evas_GL_API *api = evas_gl_api_get(b->gl);

   host = browser_get_host(b->browser);
   evas_gl_make_current(b->gl, b->glsfc, b->glctx);
   if ((b->w != b->pw) || (b->h != b->ph))
     {
        api->glBindTexture(GL_TEXTURE_2D, b->tex);
        api->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, b->w, b->h, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
        api->glBindTexture(GL_TEXTURE_2D, 0);
     }
   api->glBindFramebuffer(GL_FRAMEBUFFER, b->fbo);
   host->composite(host);
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
