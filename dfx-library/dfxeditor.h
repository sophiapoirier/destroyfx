
/* This is the interface you must implement in order to create an editor
   for a DFXPlugin. A GUI for VST and for Audiounit will be created from
   this. */

#include <dfxplugin.h>
#include "keys.h"

class DfxEditor {
public:
  DfxEditor (DfxPlugin *effect) = 0;
  virtual ~DfxEditor() = 0;

  /* This instructs the plugin to open its 
     window. It's given the parent window under
     VST and the the view under AU.

     close will never be called for AU, so don't
     do any special destruction here.
  */
  virtual long open (surface gui_window) = 0;
  virtual void close () = 0;

  /* idle is called periodically and can be used to
     update secondary displays (ie, a volume meter)
     or do special gui effects. */

  virtual void idle () = 0;

  /* redraw should redraw the entire window */

  virtual void redraw () = 0;

  /* These are called whenever the user moves his mouse/clicks/releases
     his mouse in the window. (If DfxGUI::capture is called, then
     mouse events will appear even if the mouse isn't in the window). */

  virtual void uponmousemove (int x, int y, kmod m);
  virtual void uponmousedown (int x, int y, bool left, kmod m);
  virtual void uponmouseup (int x, int y, bool left, kmod m);

  /* These are called whenever the user presses a key with focus on the
     plugin. See keys.h for the definition of dfxkey. */

  virtual void uponkeydown (dfxkey k) = 0;
  virtual void uponkeyup (dfxkey k) = 0;

  /* You don't need to worry about these accessors for the private variables,
     as long as you set the corresponding private members in the constructor. */

  virtual long getheight () { return height; }
  virtual long getwidth () { return width; }

private:
  long height, width;
  
};
