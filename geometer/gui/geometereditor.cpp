
#include "geometereditor.hpp"
#include "geometer.hpp"
#include "geometerhelp.h"


const long NUM_SLIDERS = 5;

const char * fontface_snoot = "snoot.org pixel10";
const float fontsize_snoot = 14.0f;
const DGColor fontcolor_values(75.0f/255.0f, 151.0f/255.0f, 71.0f/255.0f);
const DGColor fontcolor_labels = kWhiteDGColor;

const float finetuneinc = 0.0001f;

const char * landmarks_labelstrings[NUM_POINTSTYLES] = 
  { "zero", "freq", "num", "span", "size", "level" };
const char * ops_labelstrings[NUM_OPS] = 
  { "jump", "????", "????", "size", "size", "times", "times", " " };
const char * recreate_labelstrings[NUM_INTERPSTYLES] = 
  { "dim", "dim", "exp", "????", "size", "o'lap", "mod", "dist" };



//-----------------------------------------------------------------------------
enum {
  // button sizes
  stdsize = 32,

  // positions
  pos_sliderX = 59,
  pos_sliderY = 255,
  pos_sliderwidth = 196,
  pos_sliderheight = 16,
  pos_sliderincX = 245,
  pos_sliderincY = 35,

  pos_sliderlabelX = 19,
  pos_sliderlabelY = 250 + 3,
  pos_sliderlabelwidth = 32,
  pos_sliderlabelheight = 13 - 3,

  pos_finedownX = 27,
  pos_finedownY = 263,
  pos_fineupX = pos_finedownX + 9,
  pos_fineupY = pos_finedownY,
  pos_finebuttonincX = 240,
  pos_finebuttonincY = pos_sliderincY,

  pos_displayX = 180,
  pos_displayY = pos_sliderY - 10,
  pos_displaywidth = pos_sliderX + pos_sliderwidth - pos_displayX - 2,
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

  pos_helpiconX = 19,
  pos_helpiconY = 365,
  pos_helpboxX = pos_helpiconX + 99,
  pos_helpboxY = pos_helpiconY + 1,
        
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
  pos_geometerviewh = 133

};


//-----------------------------------------------------------------------------
// callbacks for button-triggered action

void midilearnGeometer(long value, void * editor) {
  if (editor != NULL) {
    if (value == 0)
      ((DfxGuiEditor*)editor)->setmidilearning(false);
    else
      ((DfxGuiEditor*)editor)->setmidilearning(true);
  }
}

void midiresetGeometer(long value, void * editor) {
  if ( (editor != NULL) && (value != 0) )
    ((DfxGuiEditor*)editor)->resetmidilearn();
}

void buttonswitched(long value, void * button) {
  // XXX note that this still won't catch a change under your mouse caused by parameter automation
  DGButton * gbutton = (DGButton*) button;
  if (button != NULL)
    ((GeometerEditor*)(gbutton->getDfxGuiEditor()))->changehelp(gbutton);
}

//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void geometerDisplayProc(float value, char * outText, void *) {
  sprintf(outText, "%.7f", value);
}


//-----------------------------------------------------------------------------
// parameter listener procedure
static void baseParamsListenerProc(void * inUserData, void * inObject, const AudioUnitParameter * inParameter, Float32 inValue) {
  if ( (inObject == NULL) || (inParameter == NULL) )
    return;

  DGControl * control = (DGControl*) inObject;
  GeometerEditor * geditor = (GeometerEditor*) inUserData;

  long newParameterID = geditor->choose_multiparam(inParameter->mParameterID);
  control->setParameterID(newParameterID);
}



#pragma mark -

//--------------------------------------------------------------------------
GeometerHelpBox::GeometerHelpBox(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground)
 : DGTextDisplay(inOwnerEditor, DFX_PARAM_INVALID_ID, inRegion, NULL, NULL, inBackground, 
                 kDGTextAlign_left, fontsize_snoot, kBlackDGColor, fontface_snoot), 
   helpCategory(HELP_CATEGORY_GENERAL), itemNum(HELP_EMPTY)
{
  setRespondToMouse(false);
}

//--------------------------------------------------------------------------
void GeometerHelpBox::draw(DGGraphicsContext * inContext) {

  if ( (helpCategory == HELP_CATEGORY_GENERAL) && (itemNum == HELP_EMPTY) )
    return;

  if (backgroundImage != NULL)
    backgroundImage->draw(getBounds(), inContext);

  DGRect textpos(getBounds());
  textpos.h = 10;
  textpos.w -= 5;
  textpos.offset(4, 3);

  for (int i=0; i < NUM_HELP_TEXT_LINES; i++) {

    const char ** helpstrings;
    switch (helpCategory) {
      case HELP_CATEGORY_GENERAL:
        helpstrings = general_helpstrings[itemNum];
        break;
      case HELP_CATEGORY_WINDOWSHAPE:
        helpstrings = windowshape_helpstrings[itemNum];
        break;
      case HELP_CATEGORY_LANDMARKS:
        helpstrings = landmarks_helpstrings[itemNum];
        break;
      case HELP_CATEGORY_OPS:
        helpstrings = ops_helpstrings[itemNum];
        break;
      case HELP_CATEGORY_RECREATE:
        helpstrings = recreate_helpstrings[itemNum];
        break;
      default:
        helpstrings = NULL;
        break;
    }
    if (helpstrings == NULL)
      break;

    if (i == 0) {
      fontColor = kBlackDGColor;
      drawText(&textpos, helpstrings[0], inContext);
      textpos.offset(-1, 0);
      drawText(&textpos, helpstrings[0], inContext);
      textpos.offset(1, 16);
      fontColor = kWhiteDGColor;
    } else {
      drawText(&textpos, helpstrings[i], inContext);
      textpos.offset(0, 12);
    }
  }

}

//--------------------------------------------------------------------------
void GeometerHelpBox::setDisplayItem(long inHelpCategory, long inItemNum) {

  bool changed = false;
  if ( (helpCategory != inHelpCategory) || (itemNum != inItemNum) )
    changed = true;

  helpCategory = inHelpCategory;
  itemNum = inItemNum;

  if (changed)
    redraw();
}






#pragma mark -

//-----------------------------------------------------------------------------
COMPONENT_ENTRY(GeometerEditor)

//-----------------------------------------------------------------------------
GeometerEditor::GeometerEditor(AudioUnitCarbonView inInstance)
 : DfxGuiEditor(inInstance) {

  helpicon = NULL;
  helpbox = NULL;

  sliders = (DGSlider**) malloc(NUM_SLIDERS * sizeof(DGSlider*));
  displays = (DGTextDisplay**) malloc(NUM_SLIDERS * sizeof(DGTextDisplay*));
  finedownbuttons = (DGFineTuneButton**) malloc(NUM_SLIDERS * sizeof(DGFineTuneButton*));
  fineupbuttons = (DGFineTuneButton**) malloc(NUM_SLIDERS * sizeof(DGFineTuneButton*));
  sliderAUPs = (AudioUnitParameter*) malloc(NUM_SLIDERS * sizeof(AudioUnitParameter));
  for (int i=0; i < NUM_SLIDERS; i++) {
    sliders[i] = NULL;
    displays[i] = NULL;
    finedownbuttons[i] = NULL;
    fineupbuttons[i] = NULL;
  }

  genhelpitemcontrols = (DGControl**) malloc(NUM_GEN_HELP_ITEMS * sizeof(DGControl*));
  for (int i=0; i < NUM_GEN_HELP_ITEMS; i++)
    genhelpitemcontrols[i] = NULL;

  g_helpicons = (DGImage**) malloc(NUM_HELP_CATEGORIES * sizeof(DGImage*));
  for (int i=0; i < NUM_HELP_CATEGORIES; i++)
    g_helpicons[i] = NULL;

  parameterListener = NULL;
  OSStatus status = AUListenerCreate(baseParamsListenerProc, this,
      CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.030f, // 30 ms
      &parameterListener);
  if (status != noErr)
    parameterListener = NULL;
}

//-----------------------------------------------------------------------------
GeometerEditor::~GeometerEditor() {

  if (parameterListener != NULL) {

    for (int i=0; i < NUM_SLIDERS; i++) {
      if (sliders[i] != NULL)
        AUListenerRemoveParameter(parameterListener, sliders[i], &(sliderAUPs[i]));
      if (displays[i] != NULL)
        AUListenerRemoveParameter(parameterListener, displays[i], &(sliderAUPs[i]));
      if (finedownbuttons[i] != NULL)
        AUListenerRemoveParameter(parameterListener, finedownbuttons[i], &(sliderAUPs[i]));
      if (fineupbuttons[i] != NULL)
        AUListenerRemoveParameter(parameterListener, fineupbuttons[i], &(sliderAUPs[i]));
    }

    AUListenerDispose(parameterListener);
  }

  free(sliders);
  free(displays);
  free(sliderAUPs);
  free(genhelpitemcontrols);
  free(g_helpicons);
  free(finedownbuttons);
  free(fineupbuttons);
}

//-----------------------------------------------------------------------------
long GeometerEditor::open() {

  /* ---load some images--- */
  // background image
//  DGImage * g_background = new DGImage("geometer-background.png", this);
  DGImage * g_background = new DGImage("geometer-background-short.png", this);
  SetBackgroundImage(g_background);
  // slider and fine tune controls
  DGImage * g_sliderbackground = new DGImage("slider-background.png", this);
  DGImage * g_sliderhandle = new DGImage("slider-handle.png", this);
//  DGImage * g_sliderhandle_glowing = new DGImage("slider-handle-glowing.png", this);
  DGImage * g_finedownbutton = new DGImage("fine-tune-down-button.png", this);
  DGImage * g_fineupbutton = new DGImage("fine-tune-up-button.png", this);
  // option menus
  DGImage * g_windowshapemenu = new DGImage("window-shape-button.png", this);
  DGImage * g_windowsizemenu = new DGImage("window-size-button.png", this);
  DGImage * g_landmarksmenu = new DGImage("landmarks-button.png", this);
  DGImage * g_opsmenu = new DGImage("ops-button.png", this);
  DGImage * g_recreatemenu = new DGImage("recreate-button.png", this);
  // help displays
  DGImage * g_helpbackground = new DGImage("help-background.png", this);
  g_helpicons[HELP_CATEGORY_GENERAL] = new DGImage("help-general.png", this);
  g_helpicons[HELP_CATEGORY_WINDOWSHAPE] = new DGImage("help-window-shape.png", this);
  g_helpicons[HELP_CATEGORY_LANDMARKS] = new DGImage("help-landmarks.png", this);
  g_helpicons[HELP_CATEGORY_OPS] = new DGImage("help-ops.png", this);
  g_helpicons[HELP_CATEGORY_RECREATE] = new DGImage("help-recreate.png", this);
  // MIDI learn/reset buttons
  DGImage * g_midilearnbutton = new DGImage("midi-learn-button.png", this);
  DGImage * g_midiresetbutton = new DGImage("midi-reset-button.png", this);

  // web links
  DGImage * g_destroyfxlink = new DGImage("destroy-fx-link.png", this);
  DGImage * g_smartelectronixlink = new DGImage("smart-electronix-link.png", this);



  DGRect pos;

  //--initialize the options menus----------------------------------------
  DGButton * button;

  // window shape menu
  pos.set(pos_windowshapemenuX, pos_windowshapemenuY, stdsize, stdsize);
  button = new DGButton(this, P_SHAPE, &pos, g_windowshapemenu, 
                        NUM_WINDOWSHAPES, kDGButtonType_incbutton, true);
  button->setUserProcedure(buttonswitched, button);
  pos.set(51, 164, 119, 16);
  genhelpitemcontrols[HELP_WINDOWSHAPE] = new DGControl(this, &pos, 0.0f);

  // window size menu
  pos.set(pos_windowsizemenuX, pos_windowsizemenuY, stdsize, stdsize);
  button = new DGButton(this, P_BUFSIZE, &pos, g_windowsizemenu, 
                        BUFFERSIZESSIZE, kDGButtonType_incbutton, true);
  pos.set(290, 164, 102, 14);
  genhelpitemcontrols[HELP_WINDOWSIZE] = new DGControl(this, &pos, 0.0f);

  // how to generate landmarks menu
  pos.set(pos_landmarksmenuX, pos_landmarksmenuY,  stdsize, stdsize);
  button = new DGButton(this, P_POINTSTYLE, &pos, g_landmarksmenu, 
                        NUM_POINTSTYLES, kDGButtonType_incbutton, true);
  button->setUserProcedure(buttonswitched, button);
  pos.set(51, 208, 165, 32);
  genhelpitemcontrols[HELP_LANDMARKS] = new DGControl(this, &pos, 0.0f);

  // how to recreate them menu
  pos.set(pos_recreatemenuX, pos_recreatemenuY, stdsize, stdsize);
  button = new DGButton(this, P_INTERPSTYLE, &pos, g_recreatemenu, 
                        NUM_INTERPSTYLES, kDGButtonType_incbutton, true);
  button->setUserProcedure(buttonswitched, button);
  pos.set(51, 323, 131, 31);
  genhelpitemcontrols[HELP_RECREATE] = new DGControl(this, &pos, 0.0f);

  // op 1 menu
  pos.set(pos_op1menuX, pos_op1menuY, stdsize, stdsize);
  button = new DGButton(this, P_POINTOP1, &pos, g_opsmenu, 
                        NUM_OPS, kDGButtonType_incbutton, true);
  button->setUserProcedure(buttonswitched, button);

  // op 2 menu
  pos.offset(pos_opmenuinc, 0);
  button = new DGButton(this, P_POINTOP2, &pos, g_opsmenu, 
                        NUM_OPS, kDGButtonType_incbutton, true);
  button->setUserProcedure(buttonswitched, button);

  // op 3 menu
  pos.offset(pos_opmenuinc, 0);
  button = new DGButton(this, P_POINTOP3, &pos, g_opsmenu, 
                        NUM_OPS, kDGButtonType_incbutton, true);
  button->setUserProcedure(buttonswitched, button);

  pos.set(378, 208, 118, 32);
  genhelpitemcontrols[HELP_OPS] = new DGControl(this, &pos, 0.0f);


  pos.set(pos_sliderX, pos_sliderY, g_sliderbackground->getWidth(), g_sliderbackground->getHeight());
  DGRect fdpos(pos_finedownX, pos_finedownY, g_finedownbutton->getWidth(), g_finedownbutton->getHeight()/2);
  DGRect fupos(pos_fineupX, pos_fineupY, g_fineupbutton->getWidth(), g_fineupbutton->getHeight()/2);
  DGRect dpos(pos_displayX, pos_displayY, pos_displaywidth, pos_displayheight);
  DGRect lpos(pos_sliderlabelX, pos_sliderlabelY, pos_sliderlabelwidth, pos_sliderlabelheight);
  for (int i=0; i < NUM_SLIDERS; i++)
  {
    long baseparam;
    const char ** labelstrings = ops_labelstrings;
    long numlabelstrings = NUM_OPS;
    long xoff = 0, yoff = 0;
    // how to generate landmarks
    if (i == 0) {
      baseparam = P_POINTSTYLE;
      labelstrings = landmarks_labelstrings;
      numlabelstrings = NUM_POINTSTYLES;
      yoff = pos_sliderincY;
    }
    // how to recreate the waveform
    else if (i == 1) {
      baseparam = P_INTERPSTYLE;
      labelstrings = recreate_labelstrings;
      numlabelstrings = NUM_INTERPSTYLES;
      xoff = pos_sliderincX;
      yoff = -pos_sliderincY;
    }
    // op 1
    else if (i == 2) {
      baseparam = P_POINTOP1;
      yoff = pos_sliderincY;
    }
    // op 2
    else if (i == 3) {
      baseparam = P_POINTOP2;
      yoff = pos_sliderincY;
    }
    // op 3
    else {
      baseparam = P_POINTOP3;
    }
    long param = choose_multiparam(baseparam);

    sliders[i] = new DGSlider(this, param, &pos, kDGSliderAxis_horizontal, 
                                     g_sliderhandle, g_sliderbackground);

    // fine tune down button
    finedownbuttons[i] = new DGFineTuneButton(this, param, &fdpos, g_finedownbutton, -finetuneinc);
    // fine tune up button
    fineupbuttons[i] = new DGFineTuneButton(this, param, &fupos, g_fineupbutton, finetuneinc);

    // value display
    displays[i] = new DGTextDisplay(this, param, &dpos, geometerDisplayProc, 
                                    NULL, NULL, kDGTextAlign_right, fontsize_snoot, 
                                    fontcolor_values, fontface_snoot);
    // units label
    DGTextArrayDisplay * label = new DGTextArrayDisplay(this, baseparam, &lpos, numlabelstrings, 
             kDGTextAlign_center, NULL, fontsize_snoot, fontcolor_labels, fontface_snoot);
    for (long j=0; j < numlabelstrings; j++)
      label->setText(j, labelstrings[j]);

    if (parameterListener != NULL) {
      sliderAUPs[i].mAudioUnit = GetEditAudioUnit();
      sliderAUPs[i].mScope = kAudioUnitScope_Global;
      sliderAUPs[i].mElement = (AudioUnitElement)0;
      sliderAUPs[i].mParameterID = baseparam;

      AUListenerAddParameter(parameterListener, sliders[i], &(sliderAUPs[i]));
      AUListenerAddParameter(parameterListener, displays[i], &(sliderAUPs[i]));
      AUListenerAddParameter(parameterListener, finedownbuttons[i], &(sliderAUPs[i]));
      AUListenerAddParameter(parameterListener, fineupbuttons[i], &(sliderAUPs[i]));
    }

    pos.offset(xoff, yoff);
    fdpos.offset(xoff, yoff);
    fupos.offset(xoff, yoff);
    dpos.offset(xoff, yoff);
    lpos.offset(xoff, yoff);
  }


  //--initialize the buttons----------------------------------------------

  // MIDI learn button
  pos.set(pos_midilearnbuttonX, pos_midilearnbuttonY, g_midilearnbutton->getWidth()/2, g_midilearnbutton->getHeight()/2);
  DGButton * midilearnbutton = new DGButton(this, &pos, g_midilearnbutton, 2, kDGButtonType_incbutton, true);
  midilearnbutton->setUserProcedure(midilearnGeometer, this);
  genhelpitemcontrols[HELP_MIDILEARN] = midilearnbutton;

  // MIDI reset button
  pos.set(pos_midiresetbuttonX, pos_midiresetbuttonY, g_midiresetbutton->getWidth(), g_midiresetbutton->getHeight()/2);
  DGButton * midiresetbutton = new DGButton(this, &pos, g_midiresetbutton, 2, kDGButtonType_pushbutton);
  midiresetbutton->setUserProcedure(midiresetGeometer, this);
  genhelpitemcontrols[HELP_MIDIRESET] = midiresetbutton;

  DGWebLink * weblink;
  // Destroy FX web page link
  pos.set(pos_destroyfxlinkX, pos_destroyfxlinkY, 
        g_destroyfxlink->getWidth(), g_destroyfxlink->getHeight()/2);
  weblink = new DGWebLink(this, &pos, g_destroyfxlink, DESTROYFX_URL);

  // Smart Electronix web page link
  pos.set(pos_smartelectronixlinkX, pos_smartelectronixlinkY, 
        g_smartelectronixlink->getWidth(), g_smartelectronixlink->getHeight()/2);
  weblink = new DGWebLink(this, &pos, g_smartelectronixlink, SMARTELECTRONIX_URL);


  //--initialize the help displays-----------------------------------------

  long initcat = HELP_CATEGORY_GENERAL;
  pos.set(pos_helpiconX, pos_helpiconY, g_helpicons[initcat]->getWidth(), g_helpicons[initcat]->getHeight()/NUM_GEN_HELP_ITEMS);
  helpicon = new DGButton(this, &pos, g_helpicons[initcat], NUM_GEN_HELP_ITEMS, kDGButtonType_picturereel);
  helpicon->setValue(HELP_EMPTY);

  pos.set(pos_helpboxX, pos_helpboxY, g_helpbackground->getWidth(), g_helpbackground->getHeight());
  helpbox = new GeometerHelpBox(this, &pos, g_helpbackground);



// XXX hack for Geometer missing waveform display (shift all controls up 136 pixels)
DGControlsList * tempcl = controlsList;
while (tempcl != NULL) {
  tempcl->control->setOffset(0, -136);
  tempcl = tempcl->next;
}



  return noErr;
}


//-----------------------------------------------------------------------------
void GeometerEditor::mouseovercontrolchanged(DGControl * currentControlUnderMouse) {

  changehelp(currentControlUnderMouse);
}

//-----------------------------------------------------------------------------
void GeometerEditor::changehelp(DGControl * currentControlUnderMouse) {

  long helpcategory = HELP_CATEGORY_GENERAL;
  long helpitem = HELP_EMPTY;
  long helpnumitems = NUM_GEN_HELP_ITEMS;
  long paramID = DFX_PARAM_INVALID_ID;

  if (genhelpitemcontrols != NULL) {
    for (int i=0; i < NUM_GEN_HELP_ITEMS; i++) {
      if (currentControlUnderMouse == genhelpitemcontrols[i]) {
        helpcategory = HELP_CATEGORY_GENERAL;
        helpitem = i;
        helpnumitems = NUM_GEN_HELP_ITEMS;
        goto updatehelp;
      }
    }
  }

  if (currentControlUnderMouse != NULL) {
    if ( currentControlUnderMouse->isParameterAttached() )
      paramID = currentControlUnderMouse->getParameterID();
  }

  if (paramID == P_BUFSIZE) {
    helpcategory = HELP_CATEGORY_GENERAL;
    helpitem = HELP_WINDOWSIZE;
    helpnumitems = NUM_GEN_HELP_ITEMS;
  }
  else if (paramID == P_SHAPE) {
    helpcategory = HELP_CATEGORY_WINDOWSHAPE;
    helpitem = getparameter_i(P_SHAPE);
    helpnumitems = NUM_WINDOWSHAPES;
  }
  else if ( (paramID >= P_POINTSTYLE) && (paramID < (P_POINTPARAMS+MAX_POINTSTYLES)) ) {
    helpcategory = HELP_CATEGORY_LANDMARKS;
    helpitem = getparameter_i(P_POINTSTYLE);
    helpnumitems = NUM_POINTSTYLES;
  }
  else if ( (paramID >= P_INTERPSTYLE) && (paramID < (P_INTERPARAMS+MAX_INTERPSTYLES)) ) {
    helpcategory = HELP_CATEGORY_RECREATE;
    helpitem = getparameter_i(P_INTERPSTYLE);
    helpnumitems = NUM_INTERPSTYLES;
  }
  else if ( (paramID >= P_POINTOP1) && (paramID < (P_OPPAR1S+MAX_OPS)) ) {
    helpcategory = HELP_CATEGORY_OPS;
    helpitem = getparameter_i(P_POINTOP1);
    helpnumitems = NUM_OPS;
  }
  else if ( (paramID >= P_POINTOP2) && (paramID < (P_OPPAR2S+MAX_OPS)) ) {
    helpcategory = HELP_CATEGORY_OPS;
    helpitem = getparameter_i(P_POINTOP2);
    helpnumitems = NUM_OPS;
  }
  else if ( (paramID >= P_POINTOP3) && (paramID < (P_OPPAR3S+MAX_OPS)) ) {
    helpcategory = HELP_CATEGORY_OPS;
    helpitem = getparameter_i(P_POINTOP3);
    helpnumitems = NUM_OPS;
  }

updatehelp:
  if (helpicon != NULL) {
    helpicon->setNumStates(helpnumitems);
    helpicon->setButtonImage(g_helpicons[helpcategory]);
    helpicon->setValue(helpitem);
  }
  if (helpbox != NULL)
    helpbox->setDisplayItem(helpcategory, helpitem);
}
