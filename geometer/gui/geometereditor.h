#ifndef __GeometerEditor
#define __GeometerEditor

#include "dfxgui.h"
#include "MultiKick.hpp"
#include "VstChunk.h"
#include "geometerview.hpp"


#define NUM_SLIDERS   5
#define MKHELP(f)   ( paramSteppedScaled((f), NUM_HELPFRAMES) )
#define UNMKHELP(i)   ( paramSteppedUnscaled((i), NUM_HELPFRAMES) )

const CColor kGreenTextCColor = {75, 151, 71, 0};

#define CHOOSE_LANDMARK_PARAM   ( MKPOINTSTYLE(effect->getParameter(P_POINTSTYLE)) + P_POINTSTYLE + 1 )
#define CHOOSE_RECREATE_PARAM   ( MKPOINTSTYLE(effect->getParameter(P_INTERPSTYLE)) + P_INTERPSTYLE + 1 )
#define CHOOSE_OP1_PARAM   ( MKPOINTSTYLE(effect->getParameter(P_POINTOP1)) + P_POINTOP1 + 1 )
#define CHOOSE_OP2_PARAM   ( MKPOINTSTYLE(effect->getParameter(P_POINTOP2)) + P_POINTOP2 + 1 )
#define CHOOSE_OP3_PARAM   ( MKPOINTSTYLE(effect->getParameter(P_POINTOP3)) + P_POINTOP3 + 1 )

enum { HELP_MIDILEARN,
       HELP_WINDOWSHAPE,
       HELP_WINDOWSIZE,
       HELP_LANDMARKS,
       HELP_OPS,
       HELP_RECREATE,
       HELP_EMPTY1,
       HELP_EMPTY2,
       NUM_HELPFRAMES
};

//--------------------------------------------------------------------------
class GeometerEditor : public AEffGUIEditor, public CControlListener {
public:
  GeometerEditor(AudioEffect *effect);
  virtual ~GeometerEditor();

protected:
  virtual long getRect(ERect **rect);
  virtual long open(void *ptr);
  virtual void close();

  virtual void setParameter(long index, float value);
  virtual void valueChanged(CDrawContext* context, CControl* control);

  virtual void idle();

private:
  /* ---controls--- */
  // sliders
  CHorizontalSlider *landmarkscontrolslider;
  CHorizontalSlider *recreatecontrolslider;
  CHorizontalSlider *op1controlslider;
  CHorizontalSlider *op2controlslider;
  CHorizontalSlider *op3controlslider;
  // options menus
  MultiKick *windowshapemenu;
  MultiKick *windowsizemenu;
  MultiKick *landmarksmenu;
  MultiKick *op1menu;
  MultiKick *op2menu;
  MultiKick *op3menu;
  MultiKick *recreatemenu;
  // buttons
  CFineTuneButton *landmarkscontrolfinedownbutton;
  CFineTuneButton *landmarkscontrolfineupbutton;
  CFineTuneButton *recreatecontrolfinedownbutton;
  CFineTuneButton *recreatecontrolfineupbutton;
  CFineTuneButton *op1controlfinedownbutton;
  CFineTuneButton *op1controlfineupbutton;
  CFineTuneButton *op2controlfinedownbutton;
  CFineTuneButton *op2controlfineupbutton;
  CFineTuneButton *op3controlfinedownbutton;
  CFineTuneButton *op3controlfineupbutton;
  MultiKick *midilearnbutton;
  CWebLink *destroyfxlink;
  CWebLink *smartelectronixlink;

  /* ---information displays--- */
  // parameter value display boxes
  CParamDisplay *landmarkscontroldisplay;
  CParamDisplay *recreatecontroldisplay;
  CParamDisplay *op1controldisplay;
  CParamDisplay *op2controldisplay;
  CParamDisplay *op3controldisplay;
  // slider labels
  CMovieBitmap *landmarkscontrollabels;
  CMovieBitmap *recreatecontrollabels;
  CMovieBitmap *op1controllabel;
  CMovieBitmap *op2controllabel;
  CMovieBitmap *op3controllabel;
  // help display
  CMovieBitmap *helpbox;

  /* ---graphics--- */
  CBitmap *g_background;
  CBitmap *g_sliderhandle;
  CBitmap *g_glowingsliderhandle;
  CBitmap *g_finedownbutton;
  CBitmap *g_fineupbutton;
  CBitmap *g_windowshapemenu;
  CBitmap *g_windowsizemenu;
  CBitmap *g_landmarksmenu;
  CBitmap *g_opsmenu;
  CBitmap *g_recreatemenu;
  CBitmap *g_landmarkscontrollabels;
  CBitmap *g_recreatecontrollabels;
  CBitmap *g_op1controllabels;
  CBitmap *g_op2controllabels;
  CBitmap *g_op3controllabels;
  CBitmap *g_generalhelp;
  CBitmap *g_windowshapehelp;
  CBitmap *g_landmarkshelp;
  CBitmap *g_opshelp;
  CBitmap *g_recreatehelp;
  CBitmap *g_midilearnbutton;
  CBitmap *g_destroyfxlink;
  CBitmap *g_smartelectronixlink;

  GeometerView * gview;

  VstChunk *chunk;
  CHorizontalSlider **sliders;
  bool setGlowing(long index, bool glow = true);
};

#endif
