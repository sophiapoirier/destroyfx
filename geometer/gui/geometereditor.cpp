#ifndef __GeometerEditor
#include "GeometerEditor.hpp"
#endif

#ifndef __geometer
#include "geometer.hpp"
#endif

#include <stdio.h>
#include <math.h>


//-----------------------------------------------------------------------------
enum {
  // resource IDs
  id_background = 128,
  id_sliderhandle,
  id_glowingsliderhandle,
  id_finedownbutton,
  id_fineupbutton,
  id_windowshapemenu,
  id_windowsizemenu,
  id_landmarksmenu,
  id_opsmenu,
  id_recreatemenu,
  id_landmarkscontrollabels,
  id_recreatecontrollabels,
  id_op1controllabels,
  id_op2controllabels,
  id_op3controllabels,
  id_generalhelp,
  id_windowshapehelp,
  id_landmarkshelp,
  id_opshelp,
  id_recreatehelp,
  id_midilearnbutton,
  id_destroyfxlink,
  id_smartelectronixlink,

  // positions
  pos_sliderX = 60,
  pos_sliderY = 255,
  pos_sliderwidth = 194,
  pos_sliderheight = 16,
  pos_sliderincX = 245,
  pos_sliderincY = 35,

  pos_finedownX = 27,
  pos_finedownY = 263,
  pos_fineupX = pos_finedownX + 9,
  pos_fineupY = pos_finedownY,
  pos_finebuttonincX = 240,
  pos_finebuttonincY = pos_sliderincY,

  pos_displayX = 180,
  pos_displayY = pos_sliderY - 11,
  pos_displaywidth = pos_sliderX + pos_sliderwidth + 1 - pos_displayX,
  pos_displayheight = 10,

  pos_windowshapemenuX = 19,
  pos_windowshapemenuY = 155,
  pos_windowsizemenuX = 258,
  pos_windowsizemenuY = 155,
  pos_landmarksmenuX = 19,
  pos_landmarksmenuY = 208,
  pos_recreatemenuX = 19,
  pos_recreatemenuY = 322,
  pos_op1menuX = 258,
  pos_op1menuY = 208,
  pos_opmenuinc = 44,

  pos_landmarkscontrollabelX = 19,
  pos_landmarkscontrollabelY = 250,
  pos_recreatecontrollabelX = pos_landmarkscontrollabelX,
  pos_recreatecontrollabelY = pos_landmarkscontrollabelY + pos_sliderincY,
  pos_op1controllabelX = 258,
  pos_op1controllabelY = pos_landmarkscontrollabelY,

  pos_helpboxX = 19,
  pos_helpboxY = 365,
  pos_helpboxwidth = 478,
  pos_helpboxheight = 96,
        
  pos_midilearnbuttonX = 228,
  pos_midilearnbuttonY = 324,

  pos_destroyfxlinkX = 269,
  pos_destroyfxlinkY = 500,
  pos_smartelectronixlinkX = 407,
  pos_smartelectronixlinkY = 500,

  pos_geometerviewx = 20,
  pos_geometerviewy = 14,
  pos_geometervieww = 476,
  pos_geometerviewh = 133

};

//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void genericDisplayConvert(float value, char *string);
void genericDisplayConvert(float value, char *string) {
  sprintf(string, "%.7f", value);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
GeometerEditor::GeometerEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) {
  frame = 0;

  // initialize the graphics pointers
  g_background = 0;
  g_sliderhandle = 0;
  g_glowingsliderhandle = 0;
  g_finedownbutton = 0;
  g_fineupbutton = 0;
  g_windowshapemenu = 0;
  g_windowsizemenu = 0;
  g_landmarksmenu = 0;
  g_opsmenu = 0;
  g_recreatemenu = 0;
  g_landmarkscontrollabels = 0;
  g_recreatecontrollabels = 0;
  g_op1controllabels = 0;
  g_op2controllabels = 0;
  g_op3controllabels = 0;
  g_generalhelp = 0;
  g_windowshapehelp = 0;
  g_landmarkshelp = 0;
  g_opshelp = 0;
  g_recreatehelp = 0;
  g_midilearnbutton = 0;
  g_destroyfxlink = 0;
  g_smartelectronixlink = 0;

  gview = 0;

  // initialize the controls pointers
  // sliders
  landmarkscontrolslider = 0;
  recreatecontrolslider = 0;
  op1controlslider = 0;
  op2controlslider = 0;
  op3controlslider = 0;
  // options menus
  windowshapemenu = 0;
  windowsizemenu = 0;
  landmarksmenu = 0;
  op1menu = 0;
  op2menu = 0;
  op3menu = 0;
  recreatemenu = 0;
  // buttons
  landmarkscontrolfinedownbutton = 0;
  landmarkscontrolfineupbutton = 0;
  recreatecontrolfinedownbutton = 0;
  recreatecontrolfineupbutton = 0;
  op1controlfinedownbutton = 0;
  op1controlfineupbutton = 0;
  op2controlfinedownbutton = 0;
  op2controlfineupbutton = 0;
  op3controlfinedownbutton = 0;
  op3controlfineupbutton = 0;
  midilearnbutton = 0;
  destroyfxlink = 0;
  smartelectronixlink = 0;

  // initialize the information displays
  // parameter value display boxes
  landmarkscontroldisplay = 0;
  recreatecontroldisplay = 0;
  op1controldisplay = 0;
  op2controldisplay = 0;
  op3controldisplay = 0;
  // slider labels
  landmarkscontrollabels = 0;
  recreatecontrollabels = 0;
  op1controllabel = 0;
  op2controllabel = 0;
  op3controllabel = 0;
  // help display
  helpbox = 0;

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
GeometerEditor::~GeometerEditor() {
  // free background bitmap
  if (g_background)
    g_background->forget();
  g_background = 0;
}

//-----------------------------------------------------------------------------
long GeometerEditor::getRect(ERect **erect) {
  *erect = &rect;
  return true;
}

//-----------------------------------------------------------------------------
long GeometerEditor::open(void *ptr) {
  CPoint displayoffset; // for positioning the background graphic behind display boxes


  // !!! always call this !!!
  AEffGUIEditor::open(ptr);

  /* ---load some bitmaps--- */
  // slider & fine tune controls
  if (!g_sliderhandle)
    g_sliderhandle = new CBitmap (id_sliderhandle);
  if (!g_glowingsliderhandle)
    g_glowingsliderhandle = new CBitmap (id_glowingsliderhandle);
  if (!g_finedownbutton)
    g_finedownbutton = new CBitmap (id_finedownbutton);
  if (!g_fineupbutton)
    g_fineupbutton = new CBitmap (id_fineupbutton);
  // option menus
  if (!g_windowshapemenu)
    g_windowshapemenu = new CBitmap (id_windowshapemenu);
  if (!g_windowsizemenu)
    g_windowsizemenu = new CBitmap (id_windowsizemenu);
  if (!g_landmarksmenu)
    g_landmarksmenu = new CBitmap (id_landmarksmenu);
  if (!g_opsmenu)
    g_opsmenu = new CBitmap (id_opsmenu);
  if (!g_recreatemenu)
    g_recreatemenu = new CBitmap (id_recreatemenu);
  // parameter labels
  if (!g_landmarkscontrollabels)
    g_landmarkscontrollabels = new CBitmap (id_landmarkscontrollabels);
  if (!g_recreatecontrollabels)
    g_recreatecontrollabels = new CBitmap (id_recreatecontrollabels);
  if (!g_op1controllabels)
    g_op1controllabels = new CBitmap (id_op1controllabels);
  if (!g_op2controllabels)
    g_op2controllabels = new CBitmap (id_op2controllabels);
  if (!g_op3controllabels)
    g_op3controllabels = new CBitmap (id_op3controllabels);
  // help displays
  if (!g_generalhelp)
    g_generalhelp = new CBitmap (id_generalhelp);
  if (!g_windowshapehelp)
    g_windowshapehelp = new CBitmap (id_windowshapehelp);
  if (!g_landmarkshelp)
    g_landmarkshelp = new CBitmap (id_landmarkshelp);
  if (!g_opshelp)
    g_opshelp = new CBitmap (id_opshelp);
  if (!g_recreatehelp)
    g_recreatehelp = new CBitmap (id_recreatehelp);
  // MIDI learn button
  if (!g_midilearnbutton)
    g_midilearnbutton = new CBitmap (id_midilearnbutton);
  // web links
  if (!g_destroyfxlink)
    g_destroyfxlink = new CBitmap (id_destroyfxlink);
  if (!g_smartelectronixlink)
    g_smartelectronixlink = new CBitmap (id_smartelectronixlink);


  chunk->resetLearning();       // resets the state of MIDI learning & the learner

  //--initialize the background frame--------------------------------------
  CRect size (0, 0, g_background->getWidth(), g_background->getHeight());
  frame = new CFrame (size, ptr, this);
  frame->setBackground(g_background);



  //--initialize the options menus----------------------------------------
  CPoint point (0, 0);


  /* geometer view */
  size(pos_geometerviewx, pos_geometerviewy, pos_geometerviewx + pos_geometervieww, pos_geometerviewy + pos_geometerviewh);
  gview = new GeometerView(size, (Geometer*)effect);
  gview->setTransparency(false);
  frame->addView(gview);
  gview->init();
  

  // window shape menu
  size (pos_windowshapemenuX, pos_windowshapemenuY, pos_windowshapemenuX + (g_windowshapemenu->getWidth()/2), pos_windowshapemenuY + (g_windowshapemenu->getHeight())/NUM_WINDOWSHAPES);
  windowshapemenu = new MultiKick (size, this, P_SHAPE, NUM_WINDOWSHAPES, g_windowshapemenu->getHeight()/NUM_WINDOWSHAPES, g_windowshapemenu, point, kKickPairs);
  windowshapemenu->setValue(effect->getParameter(P_SHAPE));
  frame->addView(windowshapemenu);

  // window size menu
  size (pos_windowsizemenuX, pos_windowsizemenuY, pos_windowsizemenuX + (g_windowsizemenu->getWidth()/2), pos_windowsizemenuY + (g_windowsizemenu->getHeight())/BUFFERSIZESSIZE);
  windowsizemenu = new MultiKick (size, this, P_BUFSIZE, BUFFERSIZESSIZE, g_windowsizemenu->getHeight()/BUFFERSIZESSIZE, g_windowsizemenu, point, kKickPairs);
  windowsizemenu->setValue(effect->getParameter(P_BUFSIZE));
  frame->addView(windowsizemenu);

  // how to generate landmarks menu
  size (pos_landmarksmenuX, pos_landmarksmenuY, pos_landmarksmenuX + (g_landmarksmenu->getWidth()/2), pos_landmarksmenuY + (g_landmarksmenu->getHeight())/NUM_POINTSTYLES);
  landmarksmenu = new MultiKick (size, this, P_POINTSTYLE, NUM_POINTSTYLES, g_landmarksmenu->getHeight()/NUM_POINTSTYLES, g_landmarksmenu, point, kKickPairs);
  landmarksmenu->setValue(effect->getParameter(P_POINTSTYLE));
  frame->addView(landmarksmenu);

  // how to recreate them menu
  size (pos_recreatemenuX, pos_recreatemenuY, pos_recreatemenuX + (g_recreatemenu->getWidth()/2), pos_recreatemenuY + (g_recreatemenu->getHeight())/NUM_INTERPSTYLES);
  recreatemenu = new MultiKick (size, this, P_INTERPSTYLE, NUM_INTERPSTYLES, g_recreatemenu->getHeight()/NUM_INTERPSTYLES, g_recreatemenu, point, kKickPairs);
  recreatemenu->setValue(effect->getParameter(P_INTERPSTYLE));
  frame->addView(recreatemenu);

  // op 1 menu
  size (pos_op1menuX, pos_op1menuY, pos_op1menuX + (g_opsmenu->getWidth()/2), pos_op1menuY + (g_opsmenu->getHeight())/NUM_OPS);
  op1menu = new MultiKick (size, this, P_POINTOP1, NUM_OPS, g_opsmenu->getHeight()/NUM_OPS, g_opsmenu, point, kKickPairs);
  op1menu->setValue(effect->getParameter(P_POINTOP1));
  frame->addView(op1menu);

  // op 2 menu
  size.offset (pos_opmenuinc, 0);
  op2menu = new MultiKick (size, this, P_POINTOP2, NUM_OPS, g_opsmenu->getHeight()/NUM_OPS, g_opsmenu, point, kKickPairs);
  op2menu->setValue(effect->getParameter(P_POINTOP2));
  frame->addView(op2menu);

  // op 3 menu
  size.offset (pos_opmenuinc, 0);
  op3menu = new MultiKick (size, this, P_POINTOP3, NUM_OPS, g_opsmenu->getHeight()/NUM_OPS, g_opsmenu, point, kKickPairs);
  op3menu->setValue(effect->getParameter(P_POINTOP3));
  frame->addView(op3menu);


  //--initialize the sliders-----------------------------------------------
  int minpos = pos_sliderX;
  int maxpos = pos_sliderX + pos_sliderwidth - g_sliderhandle->getWidth();
  long tag;

  // "how to generate landmarks" control slider
  tag = CHOOSE_LANDMARK_PARAM;
  size (pos_sliderX, pos_sliderY, pos_sliderX + pos_sliderwidth, pos_sliderY + pos_sliderheight);
  displayoffset (pos_sliderX, pos_sliderY);
  landmarkscontrolslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, g_sliderhandle, g_background, displayoffset, kLeft);
  landmarkscontrolslider->setValue(effect->getParameter(tag));
  //    landmarkscontrolslider->setDefaultValue(.f);
  frame->addView(landmarkscontrolslider);

  // "how to recreate the waveform" control slider
  tag = CHOOSE_RECREATE_PARAM;
  size.offset (0, pos_sliderincY);
  displayoffset.offset (0, pos_sliderincY);
  recreatecontrolslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, g_sliderhandle, g_background, displayoffset, kLeft);
  recreatecontrolslider->setValue(effect->getParameter(tag));
  //    recreatecontrolslider->setDefaultValue(.f);
  frame->addView(recreatecontrolslider);

  minpos += pos_sliderincX;
  maxpos += pos_sliderincX;
  // op 1 slider
  tag = CHOOSE_OP1_PARAM;
  size.offset (pos_sliderincX, -pos_sliderincY);
  displayoffset.offset (pos_sliderincX, -pos_sliderincY);
  op1controlslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, g_sliderhandle, g_background, displayoffset, kLeft);
  op1controlslider->setValue(effect->getParameter(tag));
  //    op1controlslider->setDefaultValue(.f);
  frame->addView(op1controlslider);

  // op 2 slider
  tag = CHOOSE_OP2_PARAM;
  size.offset (0, pos_sliderincY);
  displayoffset.offset (0, pos_sliderincY);
  op2controlslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, g_sliderhandle, g_background, displayoffset, kLeft);
  op2controlslider->setValue(effect->getParameter(tag));
  //    op2controlslider->setDefaultValue(.f);
  frame->addView(op2controlslider);

  // op 3 slider
  tag = CHOOSE_OP3_PARAM;
  size.offset (0, pos_sliderincY);
  displayoffset.offset (0, pos_sliderincY);
  op3controlslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, g_sliderhandle, g_background, displayoffset, kLeft);
  op3controlslider->setValue(effect->getParameter(tag));
  //    op3controlslider->setDefaultValue(.f);
  frame->addView(op3controlslider);


  //--initialize the buttons----------------------------------------------

  // fine tune down buttons
  tag = CHOOSE_LANDMARK_PARAM;
  size (pos_finedownX, pos_finedownY, pos_finedownX + g_finedownbutton->getWidth(), pos_finedownY + (g_finedownbutton->getHeight())/2);
  landmarkscontrolfinedownbutton = new CFineTuneButton(size, this, tag, (g_finedownbutton->getHeight())/2, g_finedownbutton, point, kFineDown);
  landmarkscontrolfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(landmarkscontrolfinedownbutton);
  //
  tag = CHOOSE_RECREATE_PARAM;
  size.offset (0, pos_finebuttonincY);
  recreatecontrolfinedownbutton = new CFineTuneButton(size, this, tag, (g_finedownbutton->getHeight())/2, g_finedownbutton, point, kFineDown);
  recreatecontrolfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(recreatecontrolfinedownbutton);
  //
  tag = CHOOSE_OP1_PARAM;
  size.offset (pos_finebuttonincX, -pos_finebuttonincY);
  op1controlfinedownbutton = new CFineTuneButton(size, this, tag, (g_finedownbutton->getHeight())/2, g_finedownbutton, point, kFineDown);
  op1controlfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(op1controlfinedownbutton);
  //
  tag = CHOOSE_OP2_PARAM;
  size.offset (0, pos_finebuttonincY);
  op2controlfinedownbutton = new CFineTuneButton(size, this, tag, (g_finedownbutton->getHeight())/2, g_finedownbutton, point, kFineDown);
  op2controlfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(op2controlfinedownbutton);
  //
  tag = CHOOSE_OP3_PARAM;
  size.offset (0, pos_finebuttonincY);
  op3controlfinedownbutton = new CFineTuneButton(size, this, tag, (g_finedownbutton->getHeight())/2, g_finedownbutton, point, kFineDown);
  op3controlfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(op3controlfinedownbutton);

  // fine tune up buttons
  tag = CHOOSE_LANDMARK_PARAM;
  size (pos_fineupX, pos_fineupY, pos_fineupX + g_fineupbutton->getWidth(), pos_fineupY + (g_fineupbutton->getHeight())/2);
  landmarkscontrolfineupbutton = new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, point, kFineUp);
  landmarkscontrolfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(landmarkscontrolfineupbutton);
  //
  tag = CHOOSE_RECREATE_PARAM;
  size.offset (0, pos_finebuttonincY);
  recreatecontrolfineupbutton = new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, point, kFineUp);
  recreatecontrolfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(recreatecontrolfineupbutton);
  //
  tag = CHOOSE_OP1_PARAM;
  size.offset (pos_finebuttonincX, -pos_finebuttonincY);
  op1controlfineupbutton = new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, point, kFineUp);
  op1controlfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(op1controlfineupbutton);
  //
  tag = CHOOSE_OP2_PARAM;
  size.offset (0, pos_finebuttonincY);
  op2controlfineupbutton = new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, point, kFineUp);
  op2controlfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(op2controlfineupbutton);
  //
  tag = CHOOSE_OP3_PARAM;
  size.offset (0, pos_finebuttonincY);
  op3controlfineupbutton = new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, point, kFineUp);
  op3controlfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(op3controlfineupbutton);


  // MIDI learn button
  size (pos_midilearnbuttonX, pos_midilearnbuttonY, pos_midilearnbuttonX + g_midilearnbutton->getWidth(), pos_midilearnbuttonY + (g_midilearnbutton->getHeight())/4);
  midilearnbutton = new MultiKick (size, this, id_midilearnbutton, 2, (g_midilearnbutton->getHeight())/4, g_midilearnbutton, point);
  midilearnbutton->setValue(0.0f);
  frame->addView(midilearnbutton);

  // Destroy FX web page link
  size (pos_destroyfxlinkX, pos_destroyfxlinkY, pos_destroyfxlinkX + g_destroyfxlink->getWidth(), pos_destroyfxlinkY + (g_destroyfxlink->getHeight())/2);
  destroyfxlink = new CWebLink (size, this, id_destroyfxlink, "http://www.smartelectronix.com/~destroyfx/", g_destroyfxlink);
  frame->addView(destroyfxlink);

  // Smart Electronix web page link
  size (pos_smartelectronixlinkX, pos_smartelectronixlinkY, pos_smartelectronixlinkX + g_smartelectronixlink->getWidth(), pos_smartelectronixlinkY + (g_smartelectronixlink->getHeight())/2);
  smartelectronixlink = new CWebLink (size, this, id_smartelectronixlink, "http://www.smartelectronix.com/", g_smartelectronixlink);
  frame->addView(smartelectronixlink);


  //--initialize the parameter value displays-----------------------------

  // landmarkscontrol value display
  tag = CHOOSE_LANDMARK_PARAM;
  size (pos_displayX, pos_displayY, pos_displayX + pos_displaywidth, pos_displayY + pos_displayheight);
  landmarkscontroldisplay = new CParamDisplay (size, g_background);
  displayoffset (pos_displayX, pos_displayY);
  landmarkscontroldisplay->setBackOffset(displayoffset);
  landmarkscontroldisplay->setHoriAlign(kRightText);
  landmarkscontroldisplay->setFont(kNormalFontSmall);
  landmarkscontroldisplay->setFontColor(kGreenTextCColor);
  landmarkscontroldisplay->setStringConvert(genericDisplayConvert);
  landmarkscontroldisplay->setTag(tag);
  landmarkscontroldisplay->setValue(effect->getParameter(tag));
  frame->addView(landmarkscontroldisplay);

  // recreatecontrol value display
  tag = CHOOSE_RECREATE_PARAM;
  size.offset (0, pos_sliderincY);
  recreatecontroldisplay = new CParamDisplay (size, g_background);
  displayoffset.offset (0, pos_sliderincY);
  recreatecontroldisplay->setBackOffset(displayoffset);
  recreatecontroldisplay->setHoriAlign(kRightText);
  recreatecontroldisplay->setFont(kNormalFontSmall);
  recreatecontroldisplay->setFontColor(kGreenTextCColor);
  recreatecontroldisplay->setStringConvert(genericDisplayConvert);
  recreatecontroldisplay->setTag(tag);
  recreatecontroldisplay->setValue(effect->getParameter(tag));
  frame->addView(recreatecontroldisplay);

  // op 1 value display
  tag = CHOOSE_OP1_PARAM;
  size.offset (pos_sliderincX, -pos_sliderincY);
  op1controldisplay = new CParamDisplay (size, g_background);
  displayoffset.offset (pos_sliderincX, -pos_sliderincY);
  op1controldisplay->setBackOffset(displayoffset);
  op1controldisplay->setHoriAlign(kRightText);
  op1controldisplay->setFont(kNormalFontSmall);
  op1controldisplay->setFontColor(kGreenTextCColor);
  op1controldisplay->setTag(tag);
  op1controldisplay->setValue(effect->getParameter(tag));
  op1controldisplay->setStringConvert(genericDisplayConvert);
  frame->addView(op1controldisplay);

  // op 2 value display
  tag = CHOOSE_OP2_PARAM;
  size.offset (0, pos_sliderincY);
  op2controldisplay = new CParamDisplay (size, g_background);
  displayoffset.offset (0, pos_sliderincY);
  op2controldisplay->setBackOffset(displayoffset);
  op2controldisplay->setHoriAlign(kRightText);
  op2controldisplay->setFont(kNormalFontSmall);
  op2controldisplay->setFontColor(kGreenTextCColor);
  op2controldisplay->setStringConvert(genericDisplayConvert);
  op2controldisplay->setTag(tag);
  op2controldisplay->setValue(effect->getParameter(tag));
  frame->addView(op2controldisplay);

  // op 3 value display
  tag = CHOOSE_OP3_PARAM;
  size.offset (0, pos_sliderincY);
  op3controldisplay = new CParamDisplay (size, g_background);
  displayoffset.offset (0, pos_sliderincY);
  op3controldisplay->setBackOffset(displayoffset);
  op3controldisplay->setHoriAlign(kRightText);
  op3controldisplay->setFont(kNormalFontSmall);
  op3controldisplay->setFontColor(kGreenTextCColor);
  op3controldisplay->setStringConvert(genericDisplayConvert);
  op3controldisplay->setTag(tag);
  op3controldisplay->setValue(effect->getParameter(tag));
  frame->addView(op3controldisplay);


  //--initialize the slider labels-----------------------------------------

  // landmarkscontrol parameter label
  size (pos_landmarkscontrollabelX, pos_landmarkscontrollabelY, pos_landmarkscontrollabelX + g_landmarkscontrollabels->getWidth(), pos_landmarkscontrollabelY + (g_landmarkscontrollabels->getHeight()/NUM_POINTSTYLES));
  landmarkscontrollabels = new CMovieBitmap (size, this, P_POINTSTYLE, NUM_POINTSTYLES, g_landmarkscontrollabels->getHeight()/NUM_POINTSTYLES, g_landmarkscontrollabels, point);
  landmarkscontrollabels->setValue(effect->getParameter(P_POINTSTYLE));
  frame->addView(landmarkscontrollabels);

  // recreatecontrol parameter label
  size (pos_recreatecontrollabelX, pos_recreatecontrollabelY, pos_recreatecontrollabelX + g_recreatecontrollabels->getWidth(), pos_recreatecontrollabelY + (g_recreatecontrollabels->getHeight()/NUM_INTERPSTYLES));
  recreatecontrollabels = new CMovieBitmap (size, this, P_INTERPSTYLE, NUM_INTERPSTYLES, g_recreatecontrollabels->getHeight()/NUM_INTERPSTYLES, g_recreatecontrollabels, point);
  recreatecontrollabels->setValue(effect->getParameter(P_INTERPSTYLE));
  frame->addView(recreatecontrollabels);

  // op 1 parameter label
  size (pos_op1controllabelX, pos_op1controllabelY, pos_op1controllabelX + g_op1controllabels->getWidth(), pos_op1controllabelY + (g_op1controllabels->getHeight()/NUM_OPS));
  op1controllabel = new CMovieBitmap (size, this, P_POINTOP1, NUM_OPS, g_op1controllabels->getHeight()/NUM_OPS, g_op1controllabels, point);
  op1controllabel->setValue(effect->getParameter(P_POINTOP1));
  frame->addView(op1controllabel);

  // op 2 parameter label
  size.offset (0, pos_sliderincY);
  op2controllabel = new CMovieBitmap (size, this, P_POINTOP2, NUM_OPS, g_op2controllabels->getHeight()/NUM_OPS, g_op2controllabels, point);
  op2controllabel->setValue(effect->getParameter(P_POINTOP2));
  frame->addView(op2controllabel);

  // op 3 parameter label
  size.offset (0, pos_sliderincY);
  op3controllabel = new CMovieBitmap (size, this, P_POINTOP3, NUM_OPS, g_op3controllabels->getHeight()/NUM_OPS, g_op3controllabels, point);
  op3controllabel->setValue(effect->getParameter(P_POINTOP3));
  frame->addView(op3controllabel);


  //--initialize the help display-----------------------------------------
  size (pos_helpboxX, pos_helpboxY, pos_helpboxX + pos_helpboxwidth, pos_helpboxY + pos_helpboxheight);
  //    displayoffset (pos_helpboxX, pos_helpboxY);
  //    helpbox = new CMovieBitmap (size, this, P_, NUM_HELPFRAMES, pos_helpboxheight, g_background, displayoffset);
  helpbox = new CMovieBitmap (size, this, 0, NUM_HELPFRAMES, pos_helpboxheight, g_generalhelp, point);
  helpbox->setValue(1.0f);
  frame->addView(helpbox);



  // make an array of pointers to all of the sliders, for glow management
  sliders = (CHorizontalSlider**)malloc(sizeof(CHorizontalSlider*)*NUM_SLIDERS);
  sliders[0] = landmarkscontrolslider;
  sliders[1] = recreatecontrolslider;
  sliders[2] = op1controlslider;
  sliders[3] = op2controlslider;
  sliders[4] = op3controlslider;


  return true;
}

//-----------------------------------------------------------------------------
void GeometerEditor::close() {
  if (frame)
    delete frame;
  frame = 0;

  free(sliders);

  chunk->resetLearning();       // resets the state of MIDI learning & the learner

  /* ---free some bitmaps--- */
  // slider & fine tune controls
  if (g_sliderhandle)
    g_sliderhandle->forget();
  g_sliderhandle = 0;
  if (g_glowingsliderhandle)
    g_glowingsliderhandle->forget();
  g_glowingsliderhandle = 0;
  if (g_finedownbutton)
    g_finedownbutton->forget();
  g_finedownbutton = 0;
  if (g_fineupbutton)
    g_fineupbutton->forget();
  g_fineupbutton = 0;
  if (g_windowshapemenu)
    g_windowshapemenu->forget();
  g_windowshapemenu = 0;
  // option menus
  if (g_windowsizemenu)
    g_windowsizemenu->forget();
  g_windowsizemenu = 0;
  if (g_landmarksmenu)
    g_landmarksmenu->forget();
  g_landmarksmenu = 0;
  if (g_opsmenu)
    g_opsmenu->forget();
  g_opsmenu = 0;
  if (g_recreatemenu)
    g_recreatemenu->forget();
  g_recreatemenu = 0;
  // parameter labels
  if (g_landmarkscontrollabels)
    g_landmarkscontrollabels->forget();
  g_landmarkscontrollabels = 0;
  if (g_recreatecontrollabels)
    g_recreatecontrollabels->forget();
  g_recreatecontrollabels = 0;
  if (g_op1controllabels)
    g_op1controllabels->forget();
  g_op1controllabels = 0;
  if (g_op2controllabels)
    g_op2controllabels->forget();
  g_op2controllabels = 0;
  if (g_op3controllabels)
    g_op3controllabels->forget();
  g_op3controllabels = 0;
  // help displays
  if (g_generalhelp)
    g_generalhelp->forget();
  g_generalhelp = 0;
  if (g_windowshapehelp)
    g_windowshapehelp->forget();
  g_windowshapehelp = 0;
  if (g_landmarkshelp)
    g_landmarkshelp->forget();
  g_landmarkshelp = 0;
  if (g_opshelp)
    g_opshelp->forget();
  g_opshelp = 0;
  if (g_recreatehelp)
    g_recreatehelp->forget();
  g_recreatehelp = 0;
  // MIDI learn button
  if (g_midilearnbutton)
    g_midilearnbutton->forget();
  g_midilearnbutton = 0;
  // web links
  if (g_destroyfxlink)
    g_destroyfxlink->forget();
  g_destroyfxlink = 0;
  if (g_smartelectronixlink)
    g_smartelectronixlink->forget();
  g_smartelectronixlink = 0;
}

//-----------------------------------------------------------------------------
// called from GeometerEdit
void GeometerEditor::setParameter(long index, float value) {
  if (!frame)
    return;

  long paramtag;
  switch (index) {
  case P_SHAPE:
    if (windowshapemenu)
      windowshapemenu->setValue(effect->getParameter(index));
    break;

  case P_BUFSIZE:
    if (windowsizemenu)
      windowsizemenu->setValue(effect->getParameter(index));
    break;

  case P_POINTSTYLE:
    // update the controls for the parameter that just changed
    if (landmarksmenu)
      landmarksmenu->setValue(effect->getParameter(index));
    if (landmarkscontrollabels)
      landmarkscontrollabels->setValue(effect->getParameter(index));
    // & reassociate "linked" controls with a new parameter
    paramtag = CHOOSE_LANDMARK_PARAM;
    if (landmarkscontrolslider) {
      landmarkscontrolslider->setTag(paramtag);
      landmarkscontrolslider->setValue(effect->getParameter(paramtag));
      landmarkscontrolslider->setDirty();
    }
    if (landmarkscontroldisplay) {
      landmarkscontroldisplay->setTag(paramtag);
      landmarkscontroldisplay->setValue(effect->getParameter(paramtag));
      landmarkscontroldisplay->setDirty();
    }
    if (landmarkscontrolfinedownbutton) {
      landmarkscontrolfinedownbutton->setTag(paramtag);
      landmarkscontrolfinedownbutton->setValue(effect->getParameter(paramtag));
    }
    if (landmarkscontrolfineupbutton) {
      landmarkscontrolfineupbutton->setTag(paramtag);
      landmarkscontrolfineupbutton->setValue(effect->getParameter(paramtag));
    }
    break;

  case P_INTERPSTYLE:
    // update the controls for the parameter that just changed
    if (recreatemenu)
      recreatemenu->setValue(effect->getParameter(index));
    if (recreatecontrollabels)
      recreatecontrollabels->setValue(effect->getParameter(index));
    // & reassociate "linked" controls with a new parameter
    paramtag = CHOOSE_RECREATE_PARAM;
    if (recreatecontrolslider) {
      recreatecontrolslider->setTag(paramtag);
      recreatecontrolslider->setValue(effect->getParameter(paramtag));
      recreatecontrolslider->setDirty();
    }
    if (recreatecontroldisplay) {
      recreatecontroldisplay->setTag(paramtag);
      recreatecontroldisplay->setValue(effect->getParameter(paramtag));
      recreatecontroldisplay->setDirty();
    }
    if (recreatecontrolfinedownbutton) {
      recreatecontrolfinedownbutton->setTag(paramtag);
      recreatecontrolfinedownbutton->setValue(effect->getParameter(paramtag));
    }
    if (recreatecontrolfineupbutton) {
      recreatecontrolfineupbutton->setTag(paramtag);
      recreatecontrolfineupbutton->setValue(effect->getParameter(paramtag));
    }
    break;

  case P_POINTOP1:
    // update the controls for the parameter that just changed
    if (op1menu)
      op1menu->setValue(effect->getParameter(index));
    if (op1controllabel)
      op1controllabel->setValue(effect->getParameter(index));
    // & reassociate "linked" controls with a new parameter
    paramtag = CHOOSE_OP1_PARAM;
    if (op1controlslider) {
      op1controlslider->setTag(paramtag);
      op1controlslider->setValue(effect->getParameter(paramtag));
      op1controlslider->setDirty();
    }
    if (op1controldisplay) {
      op1controldisplay->setTag(paramtag);
      op1controldisplay->setValue(effect->getParameter(paramtag));
      op1controldisplay->setDirty();
    }
    if (op1controlfinedownbutton) {
      op1controlfinedownbutton->setTag(paramtag);
      op1controlfinedownbutton->setValue(effect->getParameter(paramtag));
    }
    if (op1controlfineupbutton) {
      op1controlfineupbutton->setTag(paramtag);
      op1controlfineupbutton->setValue(effect->getParameter(paramtag));
    }
    break;

  case P_POINTOP2:
    // update the controls for the parameter that just changed
    if (op2menu)
      op2menu->setValue(effect->getParameter(index));
    if (op2controllabel)
      op2controllabel->setValue(effect->getParameter(index));
    // & reassociate "linked" controls with a new parameter
    paramtag = CHOOSE_OP2_PARAM;
    if (op2controlslider) {
      op2controlslider->setTag(paramtag);
      op2controlslider->setValue(effect->getParameter(paramtag));
      op2controlslider->setDirty();
    }
    if (op2controldisplay) {
      op2controldisplay->setTag(paramtag);
      op2controldisplay->setValue(effect->getParameter(paramtag));
      op2controldisplay->setDirty();
    }
    if (op2controlfinedownbutton) {
      op2controlfinedownbutton->setTag(paramtag);
      op2controlfinedownbutton->setValue(effect->getParameter(paramtag));
    }
    if (op2controlfineupbutton) {
      op2controlfineupbutton->setTag(paramtag);
      op2controlfineupbutton->setValue(effect->getParameter(paramtag));
    }
    break;

  case P_POINTOP3:
    // update the controls for the parameter that just changed
    if (op3menu)
      op3menu->setValue(effect->getParameter(index));
    if (op3controllabel)
      op3controllabel->setValue(effect->getParameter(index));
    // & reassociate "linked" controls with a new parameter
    paramtag = CHOOSE_OP3_PARAM;
    if (op3controlslider) {
      op3controlslider->setTag(paramtag);
      op3controlslider->setValue(effect->getParameter(paramtag));
      op3controlslider->setDirty();
    }
    if (op3controldisplay) {
      op3controldisplay->setTag(paramtag);
      op3controldisplay->setValue(effect->getParameter(paramtag));
      op3controldisplay->setDirty();
    }
    if (op3controlfinedownbutton) {
      op3controlfinedownbutton->setTag(paramtag);
      op3controlfinedownbutton->setValue(effect->getParameter(paramtag));
    }
    if (op3controlfineupbutton) {
      op3controlfineupbutton->setTag(paramtag);
      op3controlfineupbutton->setValue(effect->getParameter(paramtag));
    }
    break;

  case P_POINTPARAM0:
  case P_POINTPARAM1:
  case P_POINTPARAM2:
  case P_POINTPARAM3:
  case P_POINTPARAM4:
  case P_POINTPARAM5:
  case P_POINTPARAM6:
  case P_POINTPARAM7:
    if (CHOOSE_LANDMARK_PARAM == index) {
      if (landmarkscontrolslider)
        landmarkscontrolslider->setValue(effect->getParameter(index));
      if (landmarkscontroldisplay)
        landmarkscontroldisplay->setValue(effect->getParameter(index));
      if (landmarkscontrolfinedownbutton)
        landmarkscontrolfinedownbutton->setValue(effect->getParameter(index));
      if (landmarkscontrolfineupbutton)
        landmarkscontrolfineupbutton->setValue(effect->getParameter(index));
    }
    break;

  case P_INTERPARAM0:
  case P_INTERPARAM1:
  case P_INTERPARAM2:
  case P_INTERPARAM3:
  case P_INTERPARAM4:
  case P_INTERPARAM5:
  case P_INTERPARAM6:
  case P_INTERPARAM7:
    if (CHOOSE_RECREATE_PARAM == index) {
      if (recreatecontrolslider)
        recreatecontrolslider->setValue(effect->getParameter(index));
      if (recreatecontroldisplay)
        recreatecontroldisplay->setValue(effect->getParameter(index));
      if (recreatecontrolfinedownbutton)
        recreatecontrolfinedownbutton->setValue(effect->getParameter(index));
      if (recreatecontrolfineupbutton)
        recreatecontrolfineupbutton->setValue(effect->getParameter(index));
    }
    break;

  case P_OPPAR1_0:
  case P_OPPAR1_1:
  case P_OPPAR1_2:
  case P_OPPAR1_3:
  case P_OPPAR1_4:
  case P_OPPAR1_5:
  case P_OPPAR1_6:
  case P_OPPAR1_7:
    if (CHOOSE_OP1_PARAM == index) {
      if (op1controlslider)
        op1controlslider->setValue(effect->getParameter(index));
      if (op1controldisplay)
        op1controldisplay->setValue(effect->getParameter(index));
      if (op1controlfinedownbutton)
        op1controlfinedownbutton->setValue(effect->getParameter(index));
      if (op1controlfineupbutton)
        op1controlfineupbutton->setValue(effect->getParameter(index));
    }
    break;

  case P_OPPAR2_0:
  case P_OPPAR2_1:
  case P_OPPAR2_2:
  case P_OPPAR2_3:
  case P_OPPAR2_4:
  case P_OPPAR2_5:
  case P_OPPAR2_6:
  case P_OPPAR2_7:
    if (CHOOSE_OP2_PARAM == index) {
      if (op2controlslider)
        op2controlslider->setValue(effect->getParameter(index));
      if (op2controldisplay)
        op2controldisplay->setValue(effect->getParameter(index));
      if (op2controlfinedownbutton)
        op2controlfinedownbutton->setValue(effect->getParameter(index));
      if (op2controlfineupbutton)
        op2controlfineupbutton->setValue(effect->getParameter(index));
    }
    break;

  case P_OPPAR3_0:
  case P_OPPAR3_1:
  case P_OPPAR3_2:
  case P_OPPAR3_3:
  case P_OPPAR3_4:
  case P_OPPAR3_5:
  case P_OPPAR3_6:
  case P_OPPAR3_7:
    if (CHOOSE_OP3_PARAM == index) {
      if (op3controlslider)
        op3controlslider->setValue(effect->getParameter(index));
      if (op3controldisplay)
        op3controldisplay->setValue(effect->getParameter(index));
      if (op3controlfinedownbutton)
        op3controlfinedownbutton->setValue(effect->getParameter(index));
      if (op3controlfineupbutton)
        op3controlfineupbutton->setValue(effect->getParameter(index));
    }
    break;

  default:
    return;
  }

  postUpdate();
}

//-----------------------------------------------------------------------------
void GeometerEditor::valueChanged(CDrawContext* context, CControl* control) {
  long tag = control->getTag();


  switch (tag) {
  case id_midilearnbutton:
    chunk->setParameter(control->getValue());
    control->update(context);
    break;

  case P_BUFSIZE:
  case P_SHAPE:
  case P_POINTSTYLE:
  case P_POINTPARAM0:
  case P_POINTPARAM1:
  case P_POINTPARAM2:
  case P_POINTPARAM3:
  case P_POINTPARAM4:
  case P_POINTPARAM5:
  case P_POINTPARAM6:
  case P_POINTPARAM7:
  case P_INTERPSTYLE:
  case P_INTERPARAM0:
  case P_INTERPARAM1:
  case P_INTERPARAM2:
  case P_INTERPARAM3:
  case P_INTERPARAM4:
  case P_INTERPARAM5:
  case P_INTERPARAM6:
  case P_INTERPARAM7:
  case P_POINTOP1:
  case P_OPPAR1_0:
  case P_OPPAR1_1:
  case P_OPPAR1_2:
  case P_OPPAR1_3:
  case P_OPPAR1_4:
  case P_OPPAR1_5:
  case P_OPPAR1_6:
  case P_OPPAR1_7:
  case P_POINTOP2:
  case P_OPPAR2_0:
  case P_OPPAR2_1:
  case P_OPPAR2_2:
  case P_OPPAR2_3:
  case P_OPPAR2_4:
  case P_OPPAR2_5:
  case P_OPPAR2_6:
  case P_OPPAR2_7:
  case P_POINTOP3:
  case P_OPPAR3_0:
  case P_OPPAR3_1:
  case P_OPPAR3_2:
  case P_OPPAR3_3:
  case P_OPPAR3_4:
  case P_OPPAR3_5:
  case P_OPPAR3_6:
  case P_OPPAR3_7:
    effect->setParameterAutomated(tag, control->getValue());

    if (chunk->midiLearn) {
      chunk->learner = tag;
      for (int i=0; i < NUM_SLIDERS; i++) {
        if (sliders[i]->getTag() == tag)
          setGlowing(i);
      }
    }

    control->update(context);
    break;

  default:
    break;
  }
}

//-----------------------------------------------------------------------------
void GeometerEditor::idle() {
  bool helpchanged = false, glowingchanged = false;


  if ( (helpbox != NULL) && (frame != NULL) ) {
    // first look to see if any of the main control menus are moused over
    CControl *control = (CControl*)(frame->getCurrentView());
    bool controlfound = false;
    CBitmap *helpbackground = helpbox->getBackground();
    float helpvalue = helpbox->getValue();
    if (control != NULL) {
      if (control == windowshapemenu) {
        controlfound = true;
        if (helpbackground != g_windowshapehelp) {
          helpbox->setBackground(g_windowshapehelp);
          helpchanged = true;
        }
        if (helpvalue != windowshapemenu->getValue()) {
          helpbox->setValue(windowshapemenu->getValue());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (control == landmarksmenu) {
        controlfound = true;
        if (helpbackground != g_landmarkshelp) {
          helpbox->setBackground(g_landmarkshelp);
          helpchanged = true;
        }
        if (helpvalue != landmarksmenu->getValue()) {
          helpbox->setValue(landmarksmenu->getValue());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (control == recreatemenu) {
        controlfound = true;
        if (helpbackground != g_recreatehelp) {
          helpbox->setBackground(g_recreatehelp);
          helpchanged = true;
        }
        if (helpvalue != recreatemenu->getValue()) {
          helpbox->setValue(recreatemenu->getValue());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (control == op1menu) {
        controlfound = true;
        if (helpbackground != g_opshelp) {
          helpbox->setBackground(g_opshelp);
          helpchanged = true;
        }
        if (helpvalue != op1menu->getValue()) {
          helpbox->setValue(op1menu->getValue());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (control == op2menu) {
        controlfound = true;
        if (helpbackground != g_opshelp) {
          helpbox->setBackground(g_opshelp);
          helpchanged = true;
        }
        if (helpvalue != op2menu->getValue()) {
          helpbox->setValue(op2menu->getValue());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (control == op3menu) {
        controlfound = true;
        if (helpbackground != g_opshelp) {
          helpbox->setBackground(g_opshelp);
          helpchanged = true;
        }
        if (helpvalue != op3menu->getValue()) {
          helpbox->setValue(op3menu->getValue());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (control == windowsizemenu) {
        controlfound = true;
        if (helpbackground != g_generalhelp) {
          helpbox->setBackground(g_generalhelp);
          helpchanged = true;
        }
        if (helpvalue != UNMKHELP(HELP_WINDOWSIZE)) {
          helpbox->setValue(UNMKHELP(HELP_WINDOWSIZE));
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (control == midilearnbutton) {
        controlfound = true;
        if (helpbackground != g_generalhelp) {
          helpbox->setBackground(g_generalhelp);
          helpchanged = true;
        }
        if (helpvalue != UNMKHELP(HELP_MIDILEARN)) {
          helpbox->setValue(UNMKHELP(HELP_MIDILEARN));
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
    }
    // look to see if we're over text labels, for general help
    if (!controlfound) {
      CRect windowshapearea (51, 164, 170, 180);
      CRect windowsizearea (290, 164, 392, 178);
      CRect landmarksarea (51, 208, 216, 240);
      CRect opsarea (378, 208, 496, 240);
      CRect recreatearea (51, 323, 182, 354);
      CPoint where;
      frame->getCurrentLocation(where);
      if (where.isInside(windowshapearea)) {
        controlfound = true;
        if (helpbackground != g_generalhelp) {
          helpbox->setBackground(g_generalhelp);
          helpchanged = true;
        }
        if (helpvalue != UNMKHELP(HELP_WINDOWSHAPE)) {
          helpbox->setValue(UNMKHELP(HELP_WINDOWSHAPE));
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (where.isInside(windowsizearea)) {
        controlfound = true;
        if (helpbackground != g_generalhelp) {
          helpbox->setBackground(g_generalhelp);
          helpchanged = true;
        }
        if (helpvalue != UNMKHELP(HELP_WINDOWSIZE)) {
          helpbox->setValue(UNMKHELP(HELP_WINDOWSIZE));
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (where.isInside(landmarksarea)) {
        controlfound = true;
        if (helpbackground != g_generalhelp) {
          helpbox->setBackground(g_generalhelp);
          helpchanged = true;
        }
        if (helpvalue != UNMKHELP(HELP_LANDMARKS)) {
          helpbox->setValue(UNMKHELP(HELP_LANDMARKS));
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (where.isInside(opsarea)) {
        controlfound = true;
        if (helpbackground != g_generalhelp) {
          helpbox->setBackground(g_generalhelp);
          helpchanged = true;
        }
        if (helpvalue != UNMKHELP(HELP_OPS)) {
          helpbox->setValue(UNMKHELP(HELP_OPS));
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if (where.isInside(recreatearea)) {
        controlfound = true;
        if (helpbackground != g_generalhelp) {
          helpbox->setBackground(g_generalhelp);
          helpchanged = true;
        }
        if (helpvalue != UNMKHELP(HELP_RECREATE)) {
          helpbox->setValue(UNMKHELP(HELP_RECREATE));
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
    }
    // failing all of that, check to see if the empty help box should be loaded
    if (!controlfound) {
      if (helpbackground != g_generalhelp) {
        helpbox->setBackground(g_generalhelp);
        helpbox->setDirty();
        helpchanged = true;
      }
      if (helpvalue != 1.0f) {
        helpbox->setValue(1.0f);          
        helpbox->setDirty();
        helpchanged = true;
      }
    }
  }


  // turn off any glowing controls that are no longer learning
  for (int i=0; i < NUM_SLIDERS; i++) {
    if (sliders[i]->getTag() != chunk->learner)
      glowingchanged = setGlowing(i, false);
  }
  
#if 1
  static int when = 0;
  /* maybe I don't need to do this every frame... */
  if (1 || !when--) {
    gview->reflect();
    when = 100;
  }
#endif

  // some hosts need this call otherwise stuff doesn't redraw
  if (helpchanged || glowingchanged || 1)
    postUpdate();  

  // this is called so that idle() actually happens
  AEffGUIEditor::idle();
}


//-----------------------------------------------------------------------------
bool GeometerEditor::setGlowing(long index, bool glow) {
  if ( glow && (sliders[index]->getHandle() == g_sliderhandle) ) {
    sliders[index]->setHandle(g_glowingsliderhandle);
    sliders[index]->setDirty();
    return true;
  }

  else if ( !glow && (sliders[index]->getHandle() == g_glowingsliderhandle) ) {
    sliders[index]->setHandle(g_sliderhandle);
    sliders[index]->setDirty();
    return true;
  }

  return false;
}
