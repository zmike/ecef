#define CEF_NEW(TYPE) cef_new(sizeof(TYPE), NULL)
#define CEF_NEW_PTR(TYPE, PTR) cef_new(sizeof(TYPE), (void**)PTR)

//#define CEF_INCLUDE_INTERNAL_CEF_EXPORT_H_
//#define CEF_CALLBACK
//#define CEF_EXPORT __attribute__((weak))

#include "include/capi/cef_base_capi.h"
#include "include/capi/cef_browser_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_render_handler_capi.h"
#include "include/capi/cef_render_process_handler_capi.h"
#include "include/capi/cef_display_handler_capi.h"
#include <assert.h>

#ifndef TEST_APP

// From ui/events/keycodes/keyboard_codes_posix.h.
typedef enum {
  VKEY_BACK = 0x08,
  VKEY_TAB = 0x09,
  VKEY_BACKTAB = 0x0A,
  VKEY_CLEAR = 0x0C,
  VKEY_RETURN = 0x0D,
  VKEY_SHIFT = 0x10,
  VKEY_CONTROL = 0x11,
  VKEY_MENU = 0x12,
  VKEY_PAUSE = 0x13,
  VKEY_CAPITAL = 0x14,
  VKEY_KANA = 0x15,
  VKEY_HANGUL = 0x15,
  VKEY_JUNJA = 0x17,
  VKEY_FINAL = 0x18,
  VKEY_HANJA = 0x19,
  VKEY_KANJI = 0x19,
  VKEY_ESCAPE = 0x1B,
  VKEY_CONVERT = 0x1C,
  VKEY_NONCONVERT = 0x1D,
  VKEY_ACCEPT = 0x1E,
  VKEY_MODECHANGE = 0x1F,
  VKEY_SPACE = 0x20,
  VKEY_PRIOR = 0x21,
  VKEY_NEXT = 0x22,
  VKEY_END = 0x23,
  VKEY_HOME = 0x24,
  VKEY_LEFT = 0x25,
  VKEY_UP = 0x26,
  VKEY_RIGHT = 0x27,
  VKEY_DOWN = 0x28,
  VKEY_SELECT = 0x29,
  VKEY_PRINT = 0x2A,
  VKEY_EXECUTE = 0x2B,
  VKEY_SNAPSHOT = 0x2C,
  VKEY_INSERT = 0x2D,
  VKEY_DELETE = 0x2E,
  VKEY_HELP = 0x2F,
  VKEY_0 = 0x30,
  VKEY_1 = 0x31,
  VKEY_2 = 0x32,
  VKEY_3 = 0x33,
  VKEY_4 = 0x34,
  VKEY_5 = 0x35,
  VKEY_6 = 0x36,
  VKEY_7 = 0x37,
  VKEY_8 = 0x38,
  VKEY_9 = 0x39,
  VKEY_A = 0x41,
  VKEY_B = 0x42,
  VKEY_C = 0x43,
  VKEY_D = 0x44,
  VKEY_E = 0x45,
  VKEY_F = 0x46,
  VKEY_G = 0x47,
  VKEY_H = 0x48,
  VKEY_I = 0x49,
  VKEY_J = 0x4A,
  VKEY_K = 0x4B,
  VKEY_L = 0x4C,
  VKEY_M = 0x4D,
  VKEY_N = 0x4E,
  VKEY_O = 0x4F,
  VKEY_P = 0x50,
  VKEY_Q = 0x51,
  VKEY_R = 0x52,
  VKEY_S = 0x53,
  VKEY_T = 0x54,
  VKEY_U = 0x55,
  VKEY_V = 0x56,
  VKEY_W = 0x57,
  VKEY_X = 0x58,
  VKEY_Y = 0x59,
  VKEY_Z = 0x5A,
  VKEY_LWIN = 0x5B,
  VKEY_COMMAND = VKEY_LWIN,  // Provide the Mac name for convenience.
  VKEY_RWIN = 0x5C,
  VKEY_APPS = 0x5D,
  VKEY_SLEEP = 0x5F,
  VKEY_NUMPAD0 = 0x60,
  VKEY_NUMPAD1 = 0x61,
  VKEY_NUMPAD2 = 0x62,
  VKEY_NUMPAD3 = 0x63,
  VKEY_NUMPAD4 = 0x64,
  VKEY_NUMPAD5 = 0x65,
  VKEY_NUMPAD6 = 0x66,
  VKEY_NUMPAD7 = 0x67,
  VKEY_NUMPAD8 = 0x68,
  VKEY_NUMPAD9 = 0x69,
  VKEY_MULTIPLY = 0x6A,
  VKEY_ADD = 0x6B,
  VKEY_SEPARATOR = 0x6C,
  VKEY_SUBTRACT = 0x6D,
  VKEY_DECIMAL = 0x6E,
  VKEY_DIVIDE = 0x6F,
  VKEY_F1 = 0x70,
  VKEY_F2 = 0x71,
  VKEY_F3 = 0x72,
  VKEY_F4 = 0x73,
  VKEY_F5 = 0x74,
  VKEY_F6 = 0x75,
  VKEY_F7 = 0x76,
  VKEY_F8 = 0x77,
  VKEY_F9 = 0x78,
  VKEY_F10 = 0x79,
  VKEY_F11 = 0x7A,
  VKEY_F12 = 0x7B,
  VKEY_F13 = 0x7C,
  VKEY_F14 = 0x7D,
  VKEY_F15 = 0x7E,
  VKEY_F16 = 0x7F,
  VKEY_F17 = 0x80,
  VKEY_F18 = 0x81,
  VKEY_F19 = 0x82,
  VKEY_F20 = 0x83,
  VKEY_F21 = 0x84,
  VKEY_F22 = 0x85,
  VKEY_F23 = 0x86,
  VKEY_F24 = 0x87,
  VKEY_NUMLOCK = 0x90,
  VKEY_SCROLL = 0x91,
  VKEY_LSHIFT = 0xA0,
  VKEY_RSHIFT = 0xA1,
  VKEY_LCONTROL = 0xA2,
  VKEY_RCONTROL = 0xA3,
  VKEY_LMENU = 0xA4,
  VKEY_RMENU = 0xA5,
  VKEY_BROWSER_BACK = 0xA6,
  VKEY_BROWSER_FORWARD = 0xA7,
  VKEY_BROWSER_REFRESH = 0xA8,
  VKEY_BROWSER_STOP = 0xA9,
  VKEY_BROWSER_SEARCH = 0xAA,
  VKEY_BROWSER_FAVORITES = 0xAB,
  VKEY_BROWSER_HOME = 0xAC,
  VKEY_VOLUME_MUTE = 0xAD,
  VKEY_VOLUME_DOWN = 0xAE,
  VKEY_VOLUME_UP = 0xAF,
  VKEY_MEDIA_NEXT_TRACK = 0xB0,
  VKEY_MEDIA_PREV_TRACK = 0xB1,
  VKEY_MEDIA_STOP = 0xB2,
  VKEY_MEDIA_PLAY_PAUSE = 0xB3,
  VKEY_MEDIA_LAUNCH_MAIL = 0xB4,
  VKEY_MEDIA_LAUNCH_MEDIA_SELECT = 0xB5,
  VKEY_MEDIA_LAUNCH_APP1 = 0xB6,
  VKEY_MEDIA_LAUNCH_APP2 = 0xB7,
  VKEY_OEM_1 = 0xBA,
  VKEY_OEM_PLUS = 0xBB,
  VKEY_OEM_COMMA = 0xBC,
  VKEY_OEM_MINUS = 0xBD,
  VKEY_OEM_PERIOD = 0xBE,
  VKEY_OEM_2 = 0xBF,
  VKEY_OEM_3 = 0xC0,
  VKEY_OEM_4 = 0xDB,
  VKEY_OEM_5 = 0xDC,
  VKEY_OEM_6 = 0xDD,
  VKEY_OEM_7 = 0xDE,
  VKEY_OEM_8 = 0xDF,
  VKEY_OEM_102 = 0xE2,
  VKEY_OEM_103 = 0xE3,  // GTV KEYCODE_MEDIA_REWIND
  VKEY_OEM_104 = 0xE4,  // GTV KEYCODE_MEDIA_FAST_FORWARD
  VKEY_PROCESSKEY = 0xE5,
  VKEY_PACKET = 0xE7,
  VKEY_DBE_SBCSCHAR = 0xF3,
  VKEY_DBE_DBCSCHAR = 0xF4,
  VKEY_ATTN = 0xF6,
  VKEY_CRSEL = 0xF7,
  VKEY_EXSEL = 0xF8,
  VKEY_EREOF = 0xF9,
  VKEY_PLAY = 0xFA,
  VKEY_ZOOM = 0xFB,
  VKEY_NONAME = 0xFC,
  VKEY_PA1 = 0xFD,
  VKEY_OEM_CLEAR = 0xFE,
  VKEY_UNKNOWN = 0,

  // POSIX specific VKEYs. Note that as of Windows SDK 7.1, 0x97-9F, 0xD8-DA,
  // and 0xE8 are unassigned.
  VKEY_WLAN = 0x97,
  VKEY_POWER = 0x98,
  VKEY_BRIGHTNESS_DOWN = 0xD8,
  VKEY_BRIGHTNESS_UP = 0xD9,
  VKEY_KBD_BRIGHTNESS_DOWN = 0xDA,
  VKEY_KBD_BRIGHTNESS_UP = 0xE8,

  // Windows does not have a specific key code for AltGr. We use the unused 0xE1
  // (VK_OEM_AX) code to represent AltGr, matching the behaviour of Firefox on
  // Linux.
  VKEY_ALTGR = 0xE1,
  // Windows does not have a specific key code for Compose. We use the unused
  // 0xE6 (VK_ICO_CLEAR) code to represent Compose.
  VKEY_COMPOSE = 0xE6,
} KeyboardCode ;

#include <Elementary.h>
#include <Evas.h>
#define WEIGHT evas_object_size_hint_weight_set
#define ALIGN evas_object_size_hint_align_set
#define EXPAND(X) WEIGHT((X), EVAS_HINT_EXPAND, EVAS_HINT_EXPAND)
#define FILL(X) ALIGN((X), EVAS_HINT_FILL, EVAS_HINT_FILL)
#endif
typedef struct Ref
{
   unsigned int count;
   void **ptr;
} Ref;

static inline void *
getref(cef_base_t *self)
{
   return (char*)self + self->size;
}

static inline void
cef_addref(void *self)
{
   Ref *ref = getref(self);
   ++ref->count;
   //fprintf(stderr, "REF(%p) %d\n", self, ref->count);
}

static inline int
cef_delref(void *self)
{
   Ref *ref = getref(self);
   //fprintf(stderr, "UNREF(%p) %d\n", self, ref->count - 1);
   if (!(--ref->count))
     {
        if (ref->ptr)
          *ref->ptr = NULL;
        free(self);
        return 1;
     }
   return 0;
}

static inline int
cef_has_one_ref(void *self)
{
   Ref *ref = getref(self);
   return ref->count == 1;
}

static inline void
cef_init(cef_base_t *base)
{
   base->add_ref = (void*)cef_addref;
   base->release = (void*)cef_delref;
   base->has_one_ref = (void*)cef_has_one_ref;
   base->add_ref(base);
}

static inline void *
cef_new(size_t size, void **ptr)
{
   cef_base_t *base = calloc(1, size + sizeof(Ref));
   Ref *ref;

   base->size = size;
   cef_init(base);
   ref = getref(base);
   ref->ptr = ptr;
   if (ptr)
     *ptr = base;

   return base;
}
#ifndef TEST_APP
typedef struct Browser
{
   cef_browser_t *browser;
   Evas_Object *img;
   Eina_Stringshare *title;
   Eina_Stringshare *url;
   Elm_Object_Item *it;
   GLuint program;
   GLuint vao;
   GLuint vbo;
   GLuint vbo2;
   GLuint texture_id;
   void *buffer;
#ifdef HAVE_SERVO
   Evas_GL_Context *glctx;
   Evas_GL *gl;
   Evas_GL_Surface *glsfc;
   Evas_GL_Config *glcfg;
   Ecore_Job *gljob;
   GLuint fbo;
   GLuint tex;
   int pw, ph;
   Eina_List *clones;
#endif
   int w, h;
} Browser;

typedef struct ECef_Client
{
   cef_client_t client;
   Evas_Object *win;
   Evas_Object *layout;
   Evas_Object *pagelist;
   Evas_Object *urlbar;
   Browser *current_page;
   Eina_Hash *browsers;
   cef_render_handler_t *render_handler;
   cef_display_handler_t *display_handler;
   cef_window_info_t *window_info;
   cef_browser_settings_t *browser_settings;
   Eina_Bool urlbar_changed : 1;
} ECef_Client;


cef_render_handler_t *client_render_handler_get(cef_client_t *client);
cef_display_handler_t *client_display_handler_get(cef_client_t *client);

static inline cef_browser_host_t *
browser_get_host(cef_browser_t *browser)
{
   return browser->get_host(browser);
}
static inline ECef_Client *
browser_get_client(cef_browser_t *browser)
{
   return (void*)browser_get_host(browser)->get_client(browser_get_host(browser));
}

static inline int
modifiers_get(Evas_Modifier *m) {
  int modifiers = 0;
  if (evas_key_modifier_is_set(m, "Shift_L") || evas_key_modifier_is_set(m, "Shift_R"))
    modifiers |= EVENTFLAG_SHIFT_DOWN;
  if (evas_key_modifier_is_set(m, "Caps_Lock"))
    modifiers |= EVENTFLAG_CAPS_LOCK_ON;
  if (evas_key_modifier_is_set(m, "Control_L") || evas_key_modifier_is_set(m, "Control_R"))
    modifiers |= EVENTFLAG_CONTROL_DOWN;
  if (evas_key_modifier_is_set(m, "Alt_L") || evas_key_modifier_is_set(m, "Alt_R"))
    modifiers |= EVENTFLAG_ALT_DOWN;
  return modifiers;
}

Evas_Object *button_add(Evas_Object *parent, const char *icon, const char *text, const char *style, Evas_Smart_Cb cb, void *data);

void on_after_browser_created(cef_life_span_handler_t *self EINA_UNUSED, cef_browser_t *browser);
void browser_new(ECef_Client *ec, const char *url);
Browser *browser_get(ECef_Client *ec, cef_browser_t *browser);
void browser_set(ECef_Client *ec, Browser *b);
void browser_back(cef_browser_t *browser, ...);
void browser_forward(cef_browser_t *browser, ...);
void browser_refresh(cef_browser_t *browser, ...);
void browser_urlbar_show(ECef_Client *ec, Eina_Bool changed);
void browser_urlbar_hide(ECef_Client *ec);
void browser_urlbar_set(ECef_Client *ec, const char *url);
void browser_pagelist_show(ECef_Client *ec);
void browser_pagelist_hide(ECef_Client *ec);

void render_image_new(ECef_Client *ec, Browser *b, cef_browser_host_t *host, int w, int h);

extern Eina_Bool servo;
extern Eina_Bool gl_avail;
extern Eina_List *clients;

# define E_LIST_HANDLER_APPEND(list, type, callback, data) \
  do \
    { \
       Ecore_Event_Handler *_eh; \
       _eh = ecore_event_handler_add(type, (Ecore_Event_Handler_Cb)callback, data); \
       assert(_eh); \
       list = eina_list_append(list, _eh); \
    } \
  while (0)
#endif
