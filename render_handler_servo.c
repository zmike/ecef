#include "ecef.h"

#ifdef HAVE_SERVO
static void
render_image_servo_compositing_initialize(void *d)
{
   Browser *b = d;
   cef_browser_host_t *host;

   assert(evas_gl_make_current(b->gl, NULL, NULL));
   assert(evas_gl_make_current(b->gl, b->gl_surf, b->gl_ctx));
   host = browser_get_host(b->browser);
   host->initialize_compositing(host);
   b->gl_init = NULL;
}

static void
render_image_servo_init(Evas_Object *obj)
{
   Evas_GL *gl;
   Browser *b;

   b = evas_object_data_get(obj, "Browser");
   b->gl = gl = elm_glview_evas_gl_get(b->img);
   b->gl_cfg = evas_gl_config_new();
   b->gl_cfg->color_format = EVAS_GL_RGB_888;
   b->gl_surf = evas_gl_surface_create(gl, b->gl_cfg, b->w, b->h);
   b->gl_ctx = evas_gl_context_create(gl, evas_gl_current_context_get(gl));
   b->gl_init = ecore_job_add(render_image_servo_compositing_initialize, b);
}

void
render_image_servo_present(cef_render_handler_t *handler, cef_browser_t *browser)
{
   Browser *b = browser_get(browser_get_client(browser), browser);
   elm_glview_changed_set(b->img);
   evas_gl_make_current(b->gl, NULL, NULL);
   fprintf(stderr, "PRESENT\n");
   //void *buffer = malloc(b->w * b->h * 4);
   //static Evas_Object *win, *img;
   //elm_glview_gl_api_get(b->img)->glReadPixels(0, 0, b->w, b->h, GL_RGB, GL_UNSIGNED_BYTE, buffer);
   //if (!win)
     //{
        //win = elm_win_util_standard_add("test", "preview");
        //img = elm_image_add(win);
     //}
   //evas_object_show(win);
   //evas_object_resize(win, b->w, b->h);
   //evas_object_resize(img, b->w, b->h);
   //evas_object_image_size_set(elm_image_object_get(img), b->w, b->h);
   //evas_object_image_data_set(elm_image_object_get(img), buffer);
   //free(buffer);
   //evas_object_show(img);
}

void
render_image_servo_paint(Browser *b)
{
   evas_gl_make_current(b->gl, NULL, NULL);
   if ((b->w != b->pw) || (b->h != b->ph))
     {
        evas_gl_surface_destroy(b->gl, b->gl_surf);
        b->gl_surf = evas_gl_surface_create(b->gl, b->gl_cfg, b->w, b->h);
        elm_glview_size_set(b->img, b->w, b->h);
     }
   evas_gl_make_current(b->gl, b->gl_surf, b->gl_ctx);
}

void
render_image_servo_setup(Browser *b, int w, int h)
{
   
   elm_glview_mode_set(b->img, ELM_GLVIEW_DEPTH);
   elm_glview_init_func_set(b->img, render_image_servo_init);
   /* force setup of internal render callbacks because glview is a stupid widget */
   elm_glview_render_func_set(b->img, NULL);
   elm_glview_size_set(b->img, w, h);
   elm_glview_changed_set(b->img);
}

#endif
