
/* given an implementation (subclass) of DfxEditor called
   EDITOR, create an AEffEditor for VST. */

/* in order to send mouse events, we need to create the
   win32 event handler, etc. This isn't done yet! */

#include "dfxeditor.h"
#include "aeffeditor.h"

class DfxAEffEditor : public AEffEditor {
private:
  EDITOR * ed;

public:
  DfxAEffEditor(AudioEffect * eff) {
    effect = eff;
    updateFlag = 0;
    ed = new EDITOR((DfxPlugin*)eff);
  }

  ~DfxAEffEditor() {
    delete ed;
  }

  /* XXX does this pointer come with
     space allocated? */
     
  virtual long getRect(ERect **r) {
    ERect * a = *r;
    a->top = 0;
    a->left = 0;
    a->bottom = ed->getheight ();
    a->right = ed->getwidth ();
  }
  
  virtual long open (void * ptr) {
    systemWindow = ptr; 
    ed->open(ptr);
    return 0;
  }
  
  virtual void close () {
    ed->close();
  }

  virtual void idle () {
    if (updateFlag) {
      updateFlag = 0;
      update();
    }
    ed->idle();
  }

  virtual void update () {
    ed->redraw();
  }

  virtual void postUpdate() {
    updateFlag = 1;
  }

  virtual long onKeyDown (VstKeyCode & kc) {
    ed->uponkeydown(translatekey(kc));
  }

  virtual long onKeyUp (VstKeyCode & kc) {
    ed->uponkeyup(translatekey(kc));
  }

  /* we don't use knobs, ever!! */
  virtual long setKnobMode(int) { return 0; }

  /* we don't support mouse wheel yet. */
  virtual long onWheel (float distance) {
    return false;
  }

}

class DfxEditor {
public:
  DfxEditor (DfxPlugin *effect) = 0;
  virtual ~AEffEditor() = 0;

  /* This instructs the plugin to open its 
     window. It's given the parent window under
     VST and the the view under AU.

     close will never be called for AU, so don't
     do any special destruction here.
  */
  virtual long open (TARGET_API_GUI_WINDOW ptr) = 0;
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
