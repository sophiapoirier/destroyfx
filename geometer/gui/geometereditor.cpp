
#ifndef __DFX_GEOMETEREDITOR_H
#include "geometereditor.hpp"
#endif

#ifndef __DFX_GEOMETER_H
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
  id_midiresetbutton,
  id_destroyfxlink,
  id_smartelectronixlink,

  // button sizes
  stdsize = 32,
  controllabels_height = 13,

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
  pos_displayY = pos_sliderY - 13,
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

  pos_midiresetbuttonX = 228,
  pos_midiresetbuttonY = 343,

  pos_destroyfxlinkX = 269,
  pos_destroyfxlinkY = 500,
  pos_smartelectronixlinkX = 407,
  pos_smartelectronixlinkY = 500,

  pos_geometerviewx = 20,
  pos_geometerviewy = 14,
  pos_geometervieww = 476,
  pos_geometerviewh = 133,

  /* parameter tags for the buttons that are not real parameters */
  tag_midilearn = id_midilearnbutton + 30000,
  tag_midireset = id_midiresetbutton + 30000,
  tag_destroyfxlink = id_destroyfxlink + 30000,
  tag_smartelectronixlink = id_smartelectronixlink + 30000

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
  g_midiresetbutton = 0;
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

  chunk = ((DfxPlugin*)effect)->getsettings_ptr();     // this just simplifies pointing
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
  // MIDI learn/reset buttons
  if (!g_midilearnbutton)
    g_midilearnbutton = new CBitmap (id_midilearnbutton);
  if (!g_midiresetbutton)
    g_midiresetbutton = new CBitmap (id_midiresetbutton);

  // web links
  if (!g_destroyfxlink)
    g_destroyfxlink = new CBitmap (id_destroyfxlink);
  if (!g_smartelectronixlink)
    g_smartelectronixlink = new CBitmap (id_smartelectronixlink);


  chunk->resetLearning();       // resets the state of MIDI learning & the learner
  prevms = 0;

  //--initialize the background frame--------------------------------------
  CRect size (0, 0, g_background->getWidth(), g_background->getHeight());
  frame = new CFrame (size, ptr, this);
  frame->setBackground(g_background);



  //--initialize the options menus----------------------------------------
  CPoint zero (0, 0);


  /* geometer view */
  size(pos_geometerviewx, pos_geometerviewy, 
       pos_geometerviewx + pos_geometervieww, pos_geometerviewy + pos_geometerviewh);
  gview = new GeometerView(size, (Geometer*)effect);
  gview->setTransparency(false);
  frame->addView(gview);
  gview->init();
  

  // window shape menu
  size (pos_windowshapemenuX, pos_windowshapemenuY, 
        pos_windowshapemenuX + stdsize, 
        pos_windowshapemenuY + stdsize);
  windowshapemenu = new MultiKick (size, this, P_SHAPE, NUM_WINDOWSHAPES, 
                                   stdsize, g_windowshapemenu, zero, 
                                   kKickPairs, MAX_WINDOWSHAPES);
  windowshapemenu->setValue(effect->getParameter(P_SHAPE));
  frame->addView(windowshapemenu);

  // window size menu
  size (pos_windowsizemenuX, pos_windowsizemenuY, 
        pos_windowsizemenuX + stdsize,
        pos_windowsizemenuY + stdsize);
  windowsizemenu = new MultiKick (size, this, P_BUFSIZE, BUFFERSIZESSIZE, 
                                  stdsize, g_windowsizemenu, zero, 
                                  kKickPairs, BUFFERSIZESSIZE);
  windowsizemenu->setValue(effect->getParameter(P_BUFSIZE));
  frame->addView(windowsizemenu);

  // how to generate landmarks menu
  size (pos_landmarksmenuX, pos_landmarksmenuY, 
        pos_landmarksmenuX + stdsize,
        pos_landmarksmenuY + stdsize);
  landmarksmenu = new MultiKick (size, this, P_POINTSTYLE, NUM_POINTSTYLES, 
                                 stdsize, g_landmarksmenu, zero, kKickPairs, MAX_POINTSTYLES);
  landmarksmenu->setValue(effect->getParameter(P_POINTSTYLE));
  frame->addView(landmarksmenu);

  // how to recreate them menu
  size (pos_recreatemenuX, pos_recreatemenuY, 
        pos_recreatemenuX + stdsize,
        pos_recreatemenuY + stdsize);
  recreatemenu = new MultiKick (size, this, P_INTERPSTYLE, NUM_INTERPSTYLES, 
                                stdsize, g_recreatemenu, zero, kKickPairs, MAX_INTERPSTYLES);
  recreatemenu->setValue(effect->getParameter(P_INTERPSTYLE));
  frame->addView(recreatemenu);

  // op 1 menu
  size (pos_op1menuX, pos_op1menuY, pos_op1menuX + stdsize,
        pos_op1menuY + stdsize);
  op1menu = new MultiKick (size, this, P_POINTOP1, NUM_OPS, 
                           stdsize,
                           g_opsmenu, zero, kKickPairs,
                           MAX_OPS);
  op1menu->setValue(effect->getParameter(P_POINTOP1));
  frame->addView(op1menu);

  // op 2 menu
  size.offset (pos_opmenuinc, 0);
  op2menu = new MultiKick (size, this, P_POINTOP2, NUM_OPS, 
                           stdsize,
                           g_opsmenu, zero, kKickPairs,
                           MAX_OPS);
  op2menu->setValue(effect->getParameter(P_POINTOP2));
  frame->addView(op2menu);

  // op 3 menu
  size.offset (pos_opmenuinc, 0);
  op3menu = new MultiKick (size, this, P_POINTOP3, NUM_OPS, 
                           stdsize,
                           g_opsmenu, zero, kKickPairs,
                           MAX_OPS);
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
  landmarkscontrolslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, 
                                                  g_sliderhandle, g_background, displayoffset, kLeft);
  landmarkscontrolslider->setValue(effect->getParameter(tag));
  //    landmarkscontrolslider->setDefaultValue(.f);
  frame->addView(landmarkscontrolslider);

  // "how to recreate the waveform" control slider
  tag = CHOOSE_RECREATE_PARAM;
  size.offset (0, pos_sliderincY);
  displayoffset.offset (0, pos_sliderincY);
  recreatecontrolslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, 
                                                 g_sliderhandle, g_background, displayoffset, kLeft);
  recreatecontrolslider->setValue(effect->getParameter(tag));
  //    recreatecontrolslider->setDefaultValue(.f);
  frame->addView(recreatecontrolslider);

  minpos += pos_sliderincX;
  maxpos += pos_sliderincX;
  // op 1 slider
  tag = CHOOSE_OP1_PARAM;
  size.offset (pos_sliderincX, -pos_sliderincY);
  displayoffset.offset (pos_sliderincX, -pos_sliderincY);
  op1controlslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, 
                                            g_sliderhandle, g_background, displayoffset, kLeft);
  op1controlslider->setValue(effect->getParameter(tag));
  //    op1controlslider->setDefaultValue(.f);
  frame->addView(op1controlslider);

  // op 2 slider
  tag = CHOOSE_OP2_PARAM;
  size.offset (0, pos_sliderincY);
  displayoffset.offset (0, pos_sliderincY);
  op2controlslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, 
                                            g_sliderhandle, g_background, displayoffset, kLeft);
  op2controlslider->setValue(effect->getParameter(tag));
  //    op2controlslider->setDefaultValue(.f);
  frame->addView(op2controlslider);

  // op 3 slider
  tag = CHOOSE_OP3_PARAM;
  size.offset (0, pos_sliderincY);
  displayoffset.offset (0, pos_sliderincY);
  op3controlslider = new CHorizontalSlider (size, this, tag, minpos, maxpos, 
                                            g_sliderhandle, g_background, displayoffset, kLeft);
  op3controlslider->setValue(effect->getParameter(tag));
  //    op3controlslider->setDefaultValue(.f);
  frame->addView(op3controlslider);


  //--initialize the buttons----------------------------------------------

  // fine tune down buttons
  tag = CHOOSE_LANDMARK_PARAM;
  size (pos_finedownX, pos_finedownY, pos_finedownX + g_finedownbutton->getWidth(), 
        pos_finedownY + (g_finedownbutton->getHeight())/2);
  landmarkscontrolfinedownbutton = 
    new CFineTuneButton(size, this, tag, 
                        (g_finedownbutton->getHeight())/2, g_finedownbutton, zero, kFineDown);
  landmarkscontrolfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(landmarkscontrolfinedownbutton);
  //
  tag = CHOOSE_RECREATE_PARAM;
  size.offset (0, pos_finebuttonincY);
  recreatecontrolfinedownbutton = 
    new CFineTuneButton(size, this, tag, 
                        (g_finedownbutton->getHeight())/2, g_finedownbutton, zero, kFineDown);
  recreatecontrolfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(recreatecontrolfinedownbutton);
  //
  tag = CHOOSE_OP1_PARAM;
  size.offset (pos_finebuttonincX, -pos_finebuttonincY);
  op1controlfinedownbutton = 
    new CFineTuneButton(size, this, tag, 
                        (g_finedownbutton->getHeight())/2, g_finedownbutton, zero, kFineDown);
  op1controlfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(op1controlfinedownbutton);
  //
  tag = CHOOSE_OP2_PARAM;
  size.offset (0, pos_finebuttonincY);
  op2controlfinedownbutton = 
    new CFineTuneButton(size, this, tag, 
                        (g_finedownbutton->getHeight())/2, g_finedownbutton, zero, kFineDown);
  op2controlfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(op2controlfinedownbutton);
  //
  tag = CHOOSE_OP3_PARAM;
  size.offset (0, pos_finebuttonincY);
  op3controlfinedownbutton = 
    new CFineTuneButton(size, this, tag, 
                        (g_finedownbutton->getHeight())/2, g_finedownbutton, zero, kFineDown);
  op3controlfinedownbutton->setValue(effect->getParameter(tag));
  frame->addView(op3controlfinedownbutton);

  // fine tune up buttons
  tag = CHOOSE_LANDMARK_PARAM;
  size (pos_fineupX, pos_fineupY, pos_fineupX + g_fineupbutton->getWidth(), 
        pos_fineupY + (g_fineupbutton->getHeight())/2);
  landmarkscontrolfineupbutton = 
    new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, zero, kFineUp);
  landmarkscontrolfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(landmarkscontrolfineupbutton);
  //
  tag = CHOOSE_RECREATE_PARAM;
  size.offset (0, pos_finebuttonincY);
  recreatecontrolfineupbutton = 
    new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, zero, kFineUp);
  recreatecontrolfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(recreatecontrolfineupbutton);
  //
  tag = CHOOSE_OP1_PARAM;
  size.offset (pos_finebuttonincX, -pos_finebuttonincY);
  op1controlfineupbutton = 
    new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, zero, kFineUp);
  op1controlfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(op1controlfineupbutton);
  //
  tag = CHOOSE_OP2_PARAM;
  size.offset (0, pos_finebuttonincY);
  op2controlfineupbutton = 
    new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, zero, kFineUp);
  op2controlfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(op2controlfineupbutton);
  //
  tag = CHOOSE_OP3_PARAM;
  size.offset (0, pos_finebuttonincY);
  op3controlfineupbutton = 
    new CFineTuneButton(size, this, tag, (g_fineupbutton->getHeight())/2, g_fineupbutton, zero, kFineUp);
  op3controlfineupbutton->setValue(effect->getParameter(tag));
  frame->addView(op3controlfineupbutton);


  // MIDI learn button
  size (pos_midilearnbuttonX, pos_midilearnbuttonY, 
        pos_midilearnbuttonX + g_midilearnbutton->getWidth(), 
        pos_midilearnbuttonY + (g_midilearnbutton->getHeight())/4);
  midilearnbutton = new MultiKick (size, this, tag_midilearn, 2, 
                                   (g_midilearnbutton->getHeight())/4, g_midilearnbutton, zero);
  midilearnbutton->setValue(0.0f);
  frame->addView(midilearnbutton);

  // MIDI reset button
  size (pos_midiresetbuttonX, pos_midiresetbuttonY, 
        pos_midiresetbuttonX + g_midiresetbutton->getWidth(), 
        pos_midiresetbuttonY + (g_midiresetbutton->getHeight())/2);
  midiresetbutton = new CKickButton (size, this, tag_midireset, 
                                   (g_midiresetbutton->getHeight())/2, g_midiresetbutton, zero);
  midiresetbutton->setValue(0.0f);
  frame->addView(midiresetbutton);

  // Destroy FX web page link
  size (pos_destroyfxlinkX, pos_destroyfxlinkY, 
        pos_destroyfxlinkX + g_destroyfxlink->getWidth(), 
        pos_destroyfxlinkY + (g_destroyfxlink->getHeight())/2);
  destroyfxlink = new CWebLink (size, this, tag_destroyfxlink, DESTROYFX_URL, g_destroyfxlink);
  frame->addView(destroyfxlink);

  // Smart Electronix web page link
  size (pos_smartelectronixlinkX, pos_smartelectronixlinkY, 
        pos_smartelectronixlinkX + g_smartelectronixlink->getWidth(), 
        pos_smartelectronixlinkY + (g_smartelectronixlink->getHeight())/2);
  smartelectronixlink = new CWebLink (size, this, tag_smartelectronixlink, SMARTELECTRONIX_URL, g_smartelectronixlink);
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
  size (pos_landmarkscontrollabelX, pos_landmarkscontrollabelY, 
        pos_landmarkscontrollabelX + g_landmarkscontrollabels->getWidth(), 
        pos_landmarkscontrollabelY + controllabels_height);

  landmarkscontrollabels =
    new CMovieBitmap (size, this, P_POINTSTYLE, MAX_POINTSTYLES, 
                      controllabels_height,
                      g_landmarkscontrollabels, zero);
  landmarkscontrollabels->setValue(effect->getParameter(P_POINTSTYLE));
  frame->addView(landmarkscontrollabels);

  // recreatecontrol parameter label
  size (pos_recreatecontrollabelX, pos_recreatecontrollabelY, 
        pos_recreatecontrollabelX + g_recreatecontrollabels->getWidth(), 
        pos_recreatecontrollabelY + controllabels_height);
  recreatecontrollabels = 
    new CMovieBitmap (size, this, P_INTERPSTYLE, MAX_INTERPSTYLES, 
                      controllabels_height,
                      g_recreatecontrollabels, zero);
  recreatecontrollabels->setValue(effect->getParameter(P_INTERPSTYLE));
  frame->addView(recreatecontrollabels);

  // op 1 parameter label
  size (pos_op1controllabelX, pos_op1controllabelY, 
        pos_op1controllabelX + g_op1controllabels->getWidth(), 
        pos_op1controllabelY + controllabels_height);
  op1controllabel = new CMovieBitmap (size, this, P_POINTOP1, MAX_OPS, 
                                      controllabels_height,
                                      g_op1controllabels, zero);
  op1controllabel->setValue(effect->getParameter(P_POINTOP1));
  frame->addView(op1controllabel);

  // op 2 parameter label
  size.offset (0, pos_sliderincY);
  op2controllabel = new CMovieBitmap (size, this, P_POINTOP2, MAX_OPS, 
                                      controllabels_height,
                                      g_op2controllabels, zero);
  op2controllabel->setValue(effect->getParameter(P_POINTOP2));
  frame->addView(op2controllabel);

  // op 3 parameter label
  size.offset (0, pos_sliderincY);
  op3controllabel = new CMovieBitmap (size, this, P_POINTOP3, MAX_OPS, 
                                      controllabels_height,
                                      g_op3controllabels, zero);
  op3controllabel->setValue(effect->getParameter(P_POINTOP3));
  frame->addView(op3controllabel);


  //--initialize the help display-----------------------------------------
  size (pos_helpboxX, pos_helpboxY, pos_helpboxX + pos_helpboxwidth, 
        pos_helpboxY + pos_helpboxheight);

  helpbox = new IndexBitmap (size, pos_helpboxheight, g_generalhelp, zero);
  helpbox->setindex(HELP_EMPTY);
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

  // MIDI learn/reset buttons
  if (g_midilearnbutton)
    g_midilearnbutton->forget();
  g_midilearnbutton = 0;
  if (g_midiresetbutton)
    g_midiresetbutton->forget();
  g_midiresetbutton = 0;

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

  if (index >= P_POINTPARAMS &&
      index < P_POINTPARAMS + MAX_POINTSTYLES) {

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

  } else if (index >= P_INTERPARAMS &&
             index < P_INTERPARAMS + MAX_INTERPSTYLES) {

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

  } else if (index >= P_OPPAR1S &&
             index < P_OPPAR1S + MAX_OPS) {

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

  } else if (index >= P_OPPAR2S &&
             index < P_OPPAR2S + MAX_OPS) {

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

  } else if (index >= P_OPPAR3S &&
             index < P_OPPAR3S + MAX_OPS) {
    
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

  } else switch (index) {
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
      landmarkscontrollabels->setValue(effect->getParameter(P_POINTSTYLE));
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

  default:
    return;
  }

  postUpdate();
}

//-----------------------------------------------------------------------------
void GeometerEditor::valueChanged(CDrawContext* context, CControl* control) {
  long tag = control->getTag();


  if (tag == tag_midilearn)
    chunk->setParameterMidiLearn(control->getValue());
  else if (tag == tag_midireset)
    chunk->setParameterMidiReset(control->getValue());

  else if (tag < NUM_PARAMS) {
    /* XXX for anything? */

  switch (tag) {
    // for the switch parameters, set them up for toggle-style MIDI automation
    case P_BUFSIZE:
      chunk->setLearner(tag, kEventBehaviourToggle, BUFFERSIZESSIZE);
      break;
    case P_SHAPE:
      chunk->setLearner(tag, kEventBehaviourToggle, MAX_WINDOWSHAPES, NUM_WINDOWSHAPES);
      break;
    case P_POINTSTYLE:
      chunk->setLearner(tag, kEventBehaviourToggle, MAX_POINTSTYLES, NUM_POINTSTYLES);
      break;
    case P_INTERPSTYLE:
      chunk->setLearner(tag, kEventBehaviourToggle, MAX_INTERPSTYLES, NUM_INTERPSTYLES);
      break;
    case P_POINTOP1:
    case P_POINTOP2:
    case P_POINTOP3:
      chunk->setLearner(tag, kEventBehaviourToggle, MAX_OPS, NUM_OPS);
      break;
    // otherwise, set the learner regularly
    default:
      chunk->setLearner(tag);
  }

    effect->setParameterAutomated(tag, control->getValue());

    if (chunk->isLearning()) {
      for (int i=0; i < NUM_SLIDERS; i++) {
        if (sliders[i]->getTag() == tag)
          setGlowing(i);
      }
    }
  }

  control->update(context);
}

//-----------------------------------------------------------------------------
void GeometerEditor::idle() {
  bool helpchanged = false, glowingchanged = false;

  if ( (helpbox != NULL) && (frame != NULL) ) {
    // first look to see if any of the main control menus are moused over
    CControl *control = (CControl*)(frame->getCurrentView());
    bool controlfound = false;
    CBitmap *helpbackground = helpbox->getBackground();
    int helpvalue = helpbox->getindex();
    if (control != NULL) {
      if (control == windowshapemenu) {
        controlfound = true;
        if (helpbackground != g_windowshapehelp) {
          helpbox->setBackground(g_windowshapehelp);
          helpchanged = true;
        }
        if (helpvalue != windowshapemenu->getState()) {
          helpbox->setindex(windowshapemenu->getState());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if ((control == landmarksmenu) || 
               (control == landmarkscontrolslider)) {
        controlfound = true;
        if (helpbackground != g_landmarkshelp) {
          helpbox->setBackground(g_landmarkshelp);
          helpchanged = true;
        }
        if (helpvalue != landmarksmenu->getState()) {
          helpbox->setindex(landmarksmenu->getState());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if ((control == recreatemenu) || 
                (control == recreatecontrolslider)) {
        controlfound = true;
        if (helpbackground != g_recreatehelp) {
          helpbox->setBackground(g_recreatehelp);
          helpchanged = true;
        }
        if (helpvalue != recreatemenu->getState()) {
          helpbox->setindex(recreatemenu->getState());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if ((control == op1menu) || 
               (control == op1controlslider)) {
        controlfound = true;
        if (helpbackground != g_opshelp) {
          helpbox->setBackground(g_opshelp);
          helpchanged = true;
        }
        if (helpvalue != op1menu->getState()) {
          helpbox->setindex(op1menu->getState());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if ((control == op2menu) || 
               (control == op2controlslider)) {
        controlfound = true;
        if (helpbackground != g_opshelp) {
          helpbox->setBackground(g_opshelp);
          helpchanged = true;
        }
        if (helpvalue != op2menu->getState()) {
          helpbox->setindex(op2menu->getState());
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
      else if ((control == op3menu) || 
               (control == op3controlslider)) {
        controlfound = true;
        if (helpbackground != g_opshelp) {
          helpbox->setBackground(g_opshelp);
          helpchanged = true;
        }
        if (helpvalue != op3menu->getState()) {
          helpbox->setindex(op3menu->getState());
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
        if (helpvalue != HELP_WINDOWSIZE) {
          helpbox->setindex(HELP_WINDOWSIZE);
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
        if (helpvalue != HELP_MIDILEARN) {
          helpbox->setindex(HELP_MIDILEARN);
          helpchanged = true;
        }
        if (helpchanged)
          helpbox->setDirty();
      }
#if 0
      else if (control == midiresetbutton) {
        controlfound = true;
        
        /* XXX need help for midiresetbutton */

        helpbox->setDirty();
      }
#endif

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
        if (helpvalue != HELP_WINDOWSHAPE) {
          helpbox->setindex(HELP_WINDOWSHAPE);
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
        if (helpvalue != HELP_WINDOWSIZE) {
          helpbox->setindex(HELP_WINDOWSIZE);
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
        if (helpvalue != HELP_LANDMARKS) {
          helpbox->setindex(HELP_LANDMARKS);
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
        if (helpvalue != HELP_OPS) {
          helpbox->setindex(HELP_OPS);
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
        if (helpvalue != HELP_RECREATE) {
          helpbox->setindex(HELP_RECREATE);
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
      if (helpvalue != HELP_EMPTY) {
        helpbox->setindex(HELP_EMPTY);          
        helpbox->setDirty();
        helpchanged = true;
      }
    }
  }


  // turn off any glowing controls that are no longer learning
  for (int i=0; i < NUM_SLIDERS; i++) {
    if (sliders[i] != NULL) {
      if (sliders[i]->getTag() != chunk->getLearner())
        glowingchanged = setGlowing(i, false);
    }
  }

  /* XXX reevaluate when I should do this. */
#if 1
  bool gviewchanged = false;
  /* maybe I don't need to do this every frame... */
  unsigned long currentms = getTicks();
  unsigned long windowsizems = (unsigned long) 
                               ( (float)((PLUGIN*)effect)->getwindowsize() * 
                                 1000.0f / effect->getSampleRate() );
  unsigned long elapsedms = currentms - prevms;
  if ( (elapsedms > windowsizems) || (currentms < prevms) ) {
    if (gview) {
      gview->reflect();
      gviewchanged = true;
    }
    prevms = currentms;
  }
#endif

  // some hosts need this call otherwise stuff doesn't redraw
  if (helpchanged || glowingchanged || gviewchanged)
    postUpdate();

  /* use this idle routine to watch out for new frame size values, too */
  if (((PLUGIN*)effect)->changed)
    ((AudioEffectX*)effect)->ioChanged();
  ((PLUGIN*)effect)->changed = 0;

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
