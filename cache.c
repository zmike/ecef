#include "ecef.h"
#include <Eio.h>

static Eet_File *favicons;


static void
idcb()
{
}

void
cache_init(void)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "%s/ecef", efreet_cache_home_get());
   ecore_file_mkpath(buf);
   snprintf(buf, sizeof(buf), "%s/ecef/favicons.eet", efreet_cache_home_get());
   favicons = eet_open(buf, EET_FILE_MODE_READ_WRITE);
}

Eina_Bool
cache_favicon_set(Evas_Object *img, const char *favicon)
{
   int num = 0;
   free(eet_list(favicons, favicon, &num));
   if (!num) return EINA_FALSE;
   return elm_image_file_set(img, eet_file_get(favicons), favicon);
}

void
cache_favicon_add(Evas_Object *img, const char *favicon)
{
   int w, h;
   void *px;

   px = evas_object_image_data_get(img, EINA_FALSE);
   evas_object_image_size_get(img, &w, &h);
   eet_data_image_write(favicons, favicon, px, w, h, 1, 1, 100, 0);
   eio_eet_sync(favicons, idcb, idcb, NULL);
}
