
#ifndef __KEYS_H
#define __KEYS_H

/* These are keys for the abstract GUI layer. */

enum kmod {
  KMOD_SHIFT     = 1<<0,
  KMOD_ACCEL     = 1<<1, // PC: Ctrl, Mac: Command
  KMOD_ALT       = 1<<2, // PC: Alt, Mac: Opt
  KMOD_MAC_CTRL  = 1<<3  // PC: No!, Mac: Ctrl
};

/* if key <= 255, then it is an ascii key.
   if > 255, it should be one of the DKEY_ listed
   below. */

/* modifier is a bitmask of the above. */

struct {
  long key;
  long modifier;
} dfxkey;

enum {
  DKEY_BACK = 256,
  DKEY_CLEAR, 
  DKEY_PAUSE, 
  DKEY_NEXT, 
  DKEY_END, 
  DKEY_HOME, 

  DKEY_LEFT, 
  DKEY_UP, 
  DKEY_RIGHT, 
  DKEY_DOWN, 
  DKEY_PAGEUP, 
  DKEY_PAGEDOWN, 

  DKEY_SELECT, 
  DKEY_PRINT, 
  DKEY_ENTER, 
  DKEY_INSERT, 
  DKEY_DELETE, 
  DKEY_HELP, 
  DKEY_NUMPAD0, 
  DKEY_NUMPAD1, 
  DKEY_NUMPAD2, 
  DKEY_NUMPAD3, 
  DKEY_NUMPAD4, 
  DKEY_NUMPAD5, 
  DKEY_NUMPAD6, 
  DKEY_NUMPAD7, 
  DKEY_NUMPAD8, 
  DKEY_NUMPAD9, 
  DKEY_MULTIPLY, 
  DKEY_ADD, 
  DKEY_SEPARATOR, 
  DKEY_SUBTRACT, 
  DKEY_DECIMAL, 
  DKEY_DIVIDE, 
  DKEY_EQUALS,
  DKEY_F1, 
  DKEY_F2, 
  DKEY_F3, 
  DKEY_F4, 
  DKEY_F5, 
  DKEY_F6, 
  DKEY_F7, 
  DKEY_F8, 
  DKEY_F9, 
  DKEY_F10, 
  DKEY_F11, 
  DKEY_F12, 
  DKEY_NUMLOCK, 
  DKEY_SCROLL,

  DKEY_SHIFT,
  DKEY_CONTROL,
  DKEY_ALT,
};

#endif
