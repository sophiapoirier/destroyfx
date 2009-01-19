
#include "fireflyeditor.hpp"

#include "firefly.hpp"

#include <stdio.h>
#include <math.h>


/* graphics */
enum {
  background,

  NUM_GRAPHICS,

#if 0

  /* parameter tags for the buttons that are not real parameters */
  tag_midilearn = id_midilearnbutton + 30000,
  tag_midireset = id_midiresetbutton + 30000,
  tag_destroyfxlink = id_destroyfxlink + 30000,
  tag_smartelectronixlink = id_smartelectronixlink + 30000
#endif

};

struct graphicentry {
  int resource;
  CBitmap * bmp;
  graphicentry(int r) : resource(r), bmp(0) {}
  load() { bmp = new CBitmap(resource); }
  forget() { if (bmp) bmp->forget(); bmp = 0; }
  width() { return bmp->getWidth (); }
  height() { return bmp->getHeight (); }
};

graphicentry graphics[NUM_GRAPHICS] = {
  graphicentry(background),
};

//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void genericDisplayConvert(float value, char *string);
void genericDisplayConvert(float value, char *string) {
  sprintf(string, "%.7f", value);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
FireflyEditor::FireflyEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) {
  frame = 0;

  // load the background bitmap
  // we don't need to load all bitmaps, this could be done when open is called
  g_background = new CBitmap(id_background);

  // init the size of the plugin
  rect.left   = 0;
  rect.top    = 0;
  rect.right  = (short)g_background->getWidth();
  rect.bottom = (short)g_background->getHeight();

  chunk = ((PLUGIN*)effect)->chunk;     // this just simplifies pointing
}

//-----------------------------------------------------------------------------
FireflyEditor::~FireflyEditor() {
  // free background bitmap
  if (g_background) g_background->forget();
  g_background = 0;
}

//-----------------------------------------------------------------------------
long FireflyEditor::getRect(ERect **erect) {
  *erect = &rect;
  return true;
}

//-----------------------------------------------------------------------------
long FireflyEditor::open(void *ptr) {
  /* !!! always call this !!! */
  AEffGUIEditor::open(ptr);

  /* resets the state of MIDI learning and the learner */
  chunk->resetLearning();

  /* background frame */
  frame = new CFrame (CRect(0, 0, g_background->getWidth(), g_background->getHeight()), ptr, this);
  frame->setBackground(g_background);

  return true;
}

//-----------------------------------------------------------------------------
void FireflyEditor::close() {
  if (frame) delete frame;
  frame = 0;

  chunk->resetLearning();

}

//-----------------------------------------------------------------------------
// called from FireflyEdit
void FireflyEditor::setParameter(long index, float value) {
  if (!frame) return;


  postUpdate();
}

//-----------------------------------------------------------------------------
void FireflyEditor::valueChanged(CDrawContext* context, CControl* control) {
  long tag = control->getTag();

#if 0
  if (tag == tag_midilearn)
    chunk->setParameterMidiLearn(control->getValue());
  else if (tag == tag_midireset)
    chunk->setParameterMidiReset(control->getValue());
#endif


  control->update(context);
}

//-----------------------------------------------------------------------------
void FireflyEditor::idle() {

  /* post update if something has changed */

  // this is called so that idle() actually happens
  AEffGUIEditor::idle();
}


//-----------------------------------------------------------------------------
bool FireflyEditor::setGlowing(long index, bool glow) {

  return false;
}
