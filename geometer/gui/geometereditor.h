
#ifndef __DFX_GEOMETEREDITOR_H
#define __DFX_GEOMETEREDITOR_H

#include "dfxgui.h"

#include "dfxguislider.h"
#include "dfxguibutton.h"
#include "dfxguidisplay.h"


//--------------------------------------------------------------------------
class GeometerHelpBox : public DGTextDisplay {
public:
  GeometerHelpBox(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground);
  virtual ~GeometerHelpBox();

  virtual void draw(CGContextRef inContext, long inPortHeight);

  virtual void mouseDown(float, float, unsigned long, unsigned long) { }
  virtual void mouseTrack(float, float, unsigned long, unsigned long) { }
  virtual void mouseUp(float, float, unsigned long) { }

  void setDisplayItem(long inHelpCategory, long inItemNum);

private:
  long helpCategory;
  long itemNum;
};


//--------------------------------------------------------------------------
class GeometerEditor : public DfxGuiEditor {
public:
  GeometerEditor(AudioUnitCarbonView inInstance);
  virtual ~GeometerEditor();

  virtual long open();
  virtual void mouseovercontrolchanged(DGControl * currentControlUnderMouse);

  long choose_multiparam(long baseParamID) {
    return getparameter_i(baseParamID) + baseParamID + 1;
  }
  void changehelp(DGControl * currentControlUnderMouse);

private:
  AUParameterListenerRef parameterListener;
  AudioUnitParameter * sliderAUPs;
  DGSlider ** sliders;
  DGTextDisplay ** displays;
  DGFineTuneButton ** finedownbuttons;
  DGFineTuneButton ** fineupbuttons;

  DGControl ** genhelpitemcontrols;
  DGImage ** g_helpicons;
  DGButton * helpicon;
  GeometerHelpBox * helpbox;
};

#endif
