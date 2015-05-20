#include "ecef.h"

#ifdef HAVE_SERVO
static void
render_image_servo_init(Evas_Object *obj)
{
   Browser *b;
   cef_browser_host_t *host;

   b = evas_object_data_get(obj, "Browser");
   host = browser_get_host(b->browser);
   host->initialize_compositing(host);
}

void
render_image_servo_present(cef_render_handler_t *handler, cef_browser_t *browser)
{
   Browser *b = browser_get(browser_get_client(browser), browser);
   elm_glview_changed_set(b->img);
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

static void
render_image_servo_render(Evas_Object *obj)
{
   Browser *b;
   cef_browser_host_t *host;

   b = evas_object_data_get(obj, "Browser");
   host = browser_get_host(b->browser);
   host->composite(host);
}

void
render_image_servo_paint(Browser *b)
{
   elm_glview_size_set(b->img, b->w, b->h);
   elm_glview_changed_set(b->img);
}

void
render_image_servo_setup(Browser *b, int w, int h)
{
   elm_glview_mode_set(b->img, ELM_GLVIEW_DEPTH | ELM_GLVIEW_STENCIL | ELM_GLVIEW_DIRECT);
   elm_glview_init_func_set(b->img, render_image_servo_init);
   //elm_glview_resize_policy_set(b->img, ELM_GLVIEW_RESIZE_POLICY_SCALE);
   /* force setup of internal render callbacks because glview is a stupid widget */
   elm_glview_render_func_set(b->img, render_image_servo_render);
   elm_glview_size_set(b->img, w, h);
   elm_glview_changed_set(b->img);
}

#endif
