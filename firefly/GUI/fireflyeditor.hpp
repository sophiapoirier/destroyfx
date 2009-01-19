
#ifndef __DFX_FIREFLYEDITOR_H
#define __DFX_FIREFLYEDITOR_H

#include "dfxgui.h"
#include "multikick.hpp"
#include "vstchunk.h"
#include "indexbitmap.hpp"

#define FIRST_RESOURCE 128

const CColor kGreenTextCColor = {75, 151, 71, 0};

//--------------------------------------------------------------------------
class FireflyEditor : public AEffGUIEditor, public CControlListener {
public:
  FireflyEditor(AudioEffect *effect);
  virtual ~FireflyEditor();

protected:

  virtual long getRect(ERect **rect);
  virtual long open(void *ptr);
  virtual void close();

  virtual void setParameter(long index, float value);
  virtual void valueChanged(CDrawContext* context, CControl* control);

  virtual void idle();

private:

  /* ---graphics--- */
  CBitmap *g_background;

  VstChunk *chunk;
  bool setGlowing(long index, bool glow = true);
};

#endif
