/*------------------------------------------------------------------------
Copyright (C) 2002-2019  Tom Murphy 7 and Sophia Poirier

This file is part of Geometer.

Geometer is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Geometer is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Geometer.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "geometereditor.h"

#include <algorithm>
#include <cassert>
#include <sstream>

#include "geometer-base.h"
#include "geometerhelp.h"
#include "geometerview.h"


constexpr size_t NUM_SLIDERS = 5;

constexpr DGColor fontcolor_values(75, 151, 71);
constexpr DGColor fontcolor_labels = DGColor::kWhite;

constexpr float finetuneinc = 0.0001f;

static constexpr std::initializer_list<char const * const> landmarks_labelstrings =
  { "zero", "freq", "num", "span", "size", "level" };
static constexpr std::initializer_list<char const * const> ops_labelstrings =
  { "jump", "????", "????", "size", "size", "times", "times", " " };
static constexpr std::initializer_list<char const * const> recreate_labelstrings =
  { "dim", "dim", "exp", "????", "size", "o'lap", "mod", "dist" };



//-----------------------------------------------------------------------------
enum {
  // button sizes
  stdsize = 32,

  // positions
  pos_sliderX = 59,
  pos_sliderY = 254,
  pos_sliderwidth = 196,
  pos_sliderheight = 16,
  pos_sliderincX = 245,
  pos_sliderincY = 35,

  pos_sliderlabelX = 19,
  pos_sliderlabelY = pos_sliderY - 1,
  pos_sliderlabelwidth = 32,
  pos_sliderlabelheight = 10,

  pos_finedownX = 27,
  pos_finedownY = 263,
  pos_fineupX = pos_finedownX + 9,
  pos_fineupY = pos_finedownY,
  pos_finebuttonincX = 240,
  pos_finebuttonincY = pos_sliderincY,

  pos_displayheight = 10,
  pos_displayX = 180,
  pos_displayY = pos_sliderY - pos_displayheight - 1,
  pos_displaywidth = pos_sliderX + pos_sliderwidth - pos_displayX - 2,

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
  pos_geometervieww = GeometerViewData::samples,
  pos_geometerviewh = 133
};



#pragma mark -

//--------------------------------------------------------------------------
GeometerHelpBox::GeometerHelpBox(DfxguiEditor * inOwnerEditor, DGRect const & inRegion, DGImage * inBackground)
  : DGStaticTextDisplay(inOwnerEditor, inRegion, inBackground, dfx::TextAlignment::Left, 
			dfx::kFontSize_SnootPixel10, DGColor::kBlack, dfx::kFontName_SnootPixel10), 
   helpCategory(HELP_CATEGORY_GENERAL), itemNum(HELP_EMPTY)
{
}

//--------------------------------------------------------------------------
void GeometerHelpBox::draw(VSTGUI::CDrawContext * inContext) {

  if (itemNum < 0)
    return;
  if ((helpCategory == HELP_CATEGORY_GENERAL) && (itemNum == HELP_EMPTY))
    return;

  if (auto const image = getDrawBackground())
    image->draw(inContext, getViewSize());

  DGRect textpos(getViewSize());
  textpos.setSize(textpos.getWidth() - 5, 10);
  textpos.offset(4, 3);

  auto const helpstrings = [this]() -> char const * {
    switch (helpCategory) {
      case HELP_CATEGORY_GENERAL:
        return general_helpstrings.at(itemNum);
      case HELP_CATEGORY_WINDOWSHAPE:
        return windowshape_helpstrings.at(itemNum);
      case HELP_CATEGORY_LANDMARKS:
        return landmarks_helpstrings.at(itemNum);
      case HELP_CATEGORY_OPS:
        return ops_helpstrings.at(itemNum);
      case HELP_CATEGORY_RECREATE:
        return recreate_helpstrings.at(itemNum);
      default:
        return nullptr;
    }
  }();

  if (helpstrings) {
    std::istringstream stream(helpstrings);
    std::string line;
    bool headerDrawn = false;
    while (std::getline(stream, line)) {
      if (!std::exchange(headerDrawn, true)) {
        setFontColor(DGColor::kBlack);
        drawPlatformText(inContext, VSTGUI::UTF8String(line).getPlatformString(), textpos);
        textpos.offset(-1, 0);
        drawPlatformText(inContext, VSTGUI::UTF8String(line).getPlatformString(), textpos);
        textpos.offset(1, 16);
        setFontColor(DGColor::kWhite);
      } else {
        drawPlatformText(inContext, VSTGUI::UTF8String(line).getPlatformString(), textpos);
        textpos.offset(0, 12);
      }
    }
  }

  setDirty(false);
}

//--------------------------------------------------------------------------
void GeometerHelpBox::setDisplayItem(int inHelpCategory, int inItemNum) {

  bool const changed = ((helpCategory != inHelpCategory) || (itemNum != inItemNum));

  helpCategory = inHelpCategory;
  itemNum = inItemNum;

  if (changed)
    redraw();
}






#pragma mark -

//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(GeometerEditor)

//-----------------------------------------------------------------------------
GeometerEditor::GeometerEditor(DGEditorListenerInstance inInstance)
 : DfxGuiEditor(inInstance),
   sliders(NUM_SLIDERS, nullptr),
   displays(NUM_SLIDERS, nullptr),
   finedownbuttons(NUM_SLIDERS, nullptr),
   fineupbuttons(NUM_SLIDERS, nullptr),
   genhelpitemcontrols(NUM_GEN_HELP_ITEMS, nullptr),
   g_helpicons(NUM_HELP_CATEGORIES) {

  assert(landmarks_labelstrings.size() == NUM_POINTSTYLES);
  assert(ops_labelstrings.size() == NUM_OPS);
  assert(recreate_labelstrings.size() == NUM_INTERPSTYLES);
}

//-----------------------------------------------------------------------------
long GeometerEditor::OpenEditor() {

  /* ---load some images--- */
  // slider and fine tune controls
  auto const g_sliderbackground = VSTGUI::makeOwned<DGImage>("slider-background.png");
  auto const g_sliderhandle = VSTGUI::makeOwned<DGImage>("slider-handle.png");
  auto const g_sliderhandle_glowing = VSTGUI::makeOwned<DGImage>("slider-handle-glowing.png");
  auto const g_finedownbutton = VSTGUI::makeOwned<DGImage>("fine-tune-down-button.png");
  auto const g_fineupbutton = VSTGUI::makeOwned<DGImage>("fine-tune-up-button.png");
  // option menus
  auto const g_windowshapemenu = VSTGUI::makeOwned<DGImage>("window-shape-button.png");
  auto const g_windowsizemenu = VSTGUI::makeOwned<DGImage>("window-size-button.png");
  auto const g_landmarksmenu = VSTGUI::makeOwned<DGImage>("landmarks-button.png");
  auto const g_opsmenu = VSTGUI::makeOwned<DGImage>("ops-button.png");
  auto const g_recreatemenu = VSTGUI::makeOwned<DGImage>("recreate-button.png");
  // help displays
  auto const g_helpbackground = VSTGUI::makeOwned<DGImage>("help-background.png");
  g_helpicons[HELP_CATEGORY_GENERAL] = VSTGUI::makeOwned<DGImage>("help-general.png");
  g_helpicons[HELP_CATEGORY_WINDOWSHAPE] = VSTGUI::makeOwned<DGImage>("help-window-shape.png");
  g_helpicons[HELP_CATEGORY_LANDMARKS] = VSTGUI::makeOwned<DGImage>("help-landmarks.png");
  g_helpicons[HELP_CATEGORY_OPS] = VSTGUI::makeOwned<DGImage>("help-ops.png");
  g_helpicons[HELP_CATEGORY_RECREATE] = VSTGUI::makeOwned<DGImage>("help-recreate.png");
  // MIDI learn/reset buttons
  auto const g_midilearnbutton = VSTGUI::makeOwned<DGImage>("midi-learn-button.png");
  auto const g_midiresetbutton = VSTGUI::makeOwned<DGImage>("midi-reset-button.png");

  // web links
  auto const g_destroyfxlink = VSTGUI::makeOwned<DGImage>("destroy-fx-link.png");
  auto const g_smartelectronixlink = VSTGUI::makeOwned<DGImage>("smart-electronix-link.png");



  DGRect pos;

  //--initialize the options menus----------------------------------------

  /* geometer view */
  pos.set(pos_geometerviewx, pos_geometerviewy, pos_geometervieww, pos_geometerviewh);
  getFrame()->addView(new GeometerView(pos));

  // window shape menu
  pos.set(pos_windowshapemenuX, pos_windowshapemenuY, stdsize, stdsize);
  emplaceControl<DGButton>(this, P_SHAPE, pos, g_windowshapemenu, 
                           DGButton::Mode::Increment, true)->setNumStates(NUM_WINDOWSHAPES);
  pos.set(51, 164, 119, 16);
  genhelpitemcontrols[HELP_WINDOWSHAPE] = emplaceControl<DGNullControl>(this, pos);

  // window size menu
  pos.set(pos_windowsizemenuX, pos_windowsizemenuY, stdsize, stdsize);
  emplaceControl<DGButton>(this, P_BUFSIZE, pos, g_windowsizemenu, 
                           DGButton::Mode::Increment, true);
  pos.set(290, 164, 102, 14);
  genhelpitemcontrols[HELP_WINDOWSIZE] = emplaceControl<DGNullControl>(this, pos);

  // how to generate landmarks menu
  pos.set(pos_landmarksmenuX, pos_landmarksmenuY,  stdsize, stdsize);
  emplaceControl<DGButton>(this, P_POINTSTYLE, pos, g_landmarksmenu, 
                           DGButton::Mode::Increment, true)->setNumStates(NUM_POINTSTYLES);
  pos.set(51, 208, 165, 32);
  genhelpitemcontrols[HELP_LANDMARKS] = emplaceControl<DGNullControl>(this, pos);

  // how to recreate them menu
  pos.set(pos_recreatemenuX, pos_recreatemenuY, stdsize, stdsize);
  emplaceControl<DGButton>(this, P_INTERPSTYLE, pos, g_recreatemenu, 
                           DGButton::Mode::Increment, true)->setNumStates(NUM_INTERPSTYLES);
  pos.set(51, 323, 131, 31);
  genhelpitemcontrols[HELP_RECREATE] = emplaceControl<DGNullControl>(this, pos);

  // op 1 menu
  pos.set(pos_op1menuX, pos_op1menuY, stdsize, stdsize);
  emplaceControl<DGButton>(this, P_POINTOP1, pos, g_opsmenu, 
                           DGButton::Mode::Increment, true)->setNumStates(NUM_OPS);

  // op 2 menu
  pos.offset(pos_opmenuinc, 0);
  emplaceControl<DGButton>(this, P_POINTOP2, pos, g_opsmenu, 
                           DGButton::Mode::Increment, true)->setNumStates(NUM_OPS);

  // op 3 menu
  pos.offset(pos_opmenuinc, 0);
  emplaceControl<DGButton>(this, P_POINTOP3, pos, g_opsmenu, 
                           DGButton::Mode::Increment, true)->setNumStates(NUM_OPS);

  pos.set(378, 208, 118, 32);
  genhelpitemcontrols[HELP_OPS] = emplaceControl<DGNullControl>(this, pos);


  pos.set(pos_sliderX, pos_sliderY, g_sliderbackground->getWidth(), g_sliderbackground->getHeight());
  DGRect fdpos(pos_finedownX, pos_finedownY, g_finedownbutton->getWidth(), g_finedownbutton->getHeight() / 2);
  DGRect fupos(pos_fineupX, pos_fineupY, g_fineupbutton->getWidth(), g_fineupbutton->getHeight() / 2);
  DGRect dpos(pos_displayX, pos_displayY, pos_displaywidth, pos_displayheight);
  DGRect lpos(pos_sliderlabelX, pos_sliderlabelY, pos_sliderlabelwidth, pos_sliderlabelheight);
  for (size_t i=0; i < NUM_SLIDERS; i++) {
    auto const baseparam = get_base_param_for_slider(i);
    auto labelstrings = &ops_labelstrings;
    long xoff = 0, yoff = 0;
    // how to generate landmarks
    if (baseparam == P_POINTSTYLE) {
      labelstrings = &landmarks_labelstrings;
      yoff = pos_sliderincY;
    }
    // how to recreate the waveform
    else if (baseparam == P_INTERPSTYLE) {
      labelstrings = &recreate_labelstrings;
      xoff = pos_sliderincX;
      yoff = -pos_sliderincY;
    }
    // op 1
    else if (baseparam == P_POINTOP1) {
      yoff = pos_sliderincY;
    }
    // op 2
    else if (baseparam == P_POINTOP2) {
      yoff = pos_sliderincY;
    }
    // op 3
    else {
    }
    auto const param = choose_multiparam(baseparam);

    constexpr long sliderRangeMargin = 1;
    sliders[i] = emplaceControl<DGSlider>(this, param, pos, dfx::kAxis_Horizontal, 
                                          g_sliderhandle, g_sliderbackground, sliderRangeMargin);
    sliders[i]->setAlternateHandle(g_sliderhandle_glowing);

    // fine tune down button
    finedownbuttons[i] = emplaceControl<DGFineTuneButton>(this, param, fdpos, g_finedownbutton, -finetuneinc);
    // fine tune up button
    fineupbuttons[i] = emplaceControl<DGFineTuneButton>(this, param, fupos, g_fineupbutton, finetuneinc);

    // value display
    auto const geometerDisplayProc = [](float value, char * outText, void *) -> bool {
      return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.7f", value) > 0;
    };
    displays[i] = emplaceControl<DGTextDisplay>(this, param, dpos, geometerDisplayProc, 
                                                nullptr, nullptr, dfx::TextAlignment::Right, dfx::kFontSize_SnootPixel10, 
                                                fontcolor_values, dfx::kFontName_SnootPixel10);
    // units label
    auto const label = emplaceControl<DGTextArrayDisplay>(this, baseparam, lpos, labelstrings->size(), 
                                                          dfx::TextAlignment::Center, nullptr, 
                                                          dfx::kFontSize_SnootPixel10, fontcolor_labels, 
                                                          dfx::kFontName_SnootPixel10);
    long j = 0;
    for (auto const& labelstring : *labelstrings) {
      label->setText(j, labelstring);
      j++;
    }

    pos.offset(xoff, yoff);
    fdpos.offset(xoff, yoff);
    fupos.offset(xoff, yoff);
    dpos.offset(xoff, yoff);
    lpos.offset(xoff, yoff);
  }


  //--initialize the buttons----------------------------------------------

  // MIDI learn button
  genhelpitemcontrols[HELP_MIDILEARN] = CreateMidiLearnButton(pos_midilearnbuttonX, pos_midilearnbuttonY, g_midilearnbutton, true);

  // MIDI reset button
  genhelpitemcontrols[HELP_MIDIRESET] = CreateMidiResetButton(pos_midiresetbuttonX, pos_midiresetbuttonY, g_midiresetbutton);

  // Destroy FX web page link
  pos.set(pos_destroyfxlinkX, pos_destroyfxlinkY, 
          g_destroyfxlink->getWidth(), g_destroyfxlink->getHeight() / 2);
  emplaceControl<DGWebLink>(this, pos, g_destroyfxlink, DESTROYFX_URL);

  // Smart Electronix web page link
  pos.set(pos_smartelectronixlinkX, pos_smartelectronixlinkY, 
          g_smartelectronixlink->getWidth(), g_smartelectronixlink->getHeight() / 2);
  emplaceControl<DGWebLink>(this, pos, g_smartelectronixlink, SMARTELECTRONIX_URL);


  //--initialize the help displays-----------------------------------------

  constexpr auto initcat = HELP_CATEGORY_GENERAL;
  pos.set(pos_helpiconX, pos_helpiconY, g_helpicons[initcat]->getWidth(), g_helpicons[initcat]->getHeight() / NUM_GEN_HELP_ITEMS);
  helpicon = emplaceControl<DGButton>(this, pos, g_helpicons[initcat], NUM_GEN_HELP_ITEMS, DGButton::Mode::PictureReel);
  helpicon->setMouseEnabled(false);
  helpicon->setValue_i(HELP_EMPTY);

  pos.set(pos_helpboxX, pos_helpboxY, g_helpbackground->getWidth(), g_helpbackground->getHeight());
  helpbox = emplaceControl<GeometerHelpBox>(this, pos, g_helpbackground);



  return dfx::kStatus_NoError;
}


//-----------------------------------------------------------------------------
void GeometerEditor::parameterChanged(long inParameterID) {

  for (size_t i=0; i < NUM_SLIDERS; i++) {
    auto const baseparam = get_base_param_for_slider(i);
    if (inParameterID == baseparam) {
      auto const newParameterID = choose_multiparam(inParameterID);
      sliders[i]->setParameterID(newParameterID);
      displays[i]->setParameterID(newParameterID);
      finedownbuttons[i]->setParameterID(newParameterID);
      fineupbuttons[i]->setParameterID(newParameterID);
    }
  }

  if (GetParameterValueType(inParameterID) == DfxParam::ValueType::Int) {
    changehelp(getCurrentControl_mouseover());
  }
}

//-----------------------------------------------------------------------------
void GeometerEditor::mouseovercontrolchanged(IDGControl * currentControlUnderMouse) {

  changehelp(currentControlUnderMouse);
}

//-----------------------------------------------------------------------------
void GeometerEditor::changehelp(IDGControl * currentControlUnderMouse) {

  auto const updatehelp = [this](int category, int item, long numitems) {
    if (helpicon) {
      helpicon->setNumStates(numitems);
      helpicon->setButtonImage(g_helpicons[category]);
      helpicon->setValue_i(item);
    }
    if (helpbox)
      helpbox->setDisplayItem(category, item);
  };

  auto paramID = dfx::kParameterID_Invalid;
  if (currentControlUnderMouse && currentControlUnderMouse->isParameterAttached())
    paramID = currentControlUnderMouse->getParameterID();

  if (auto const matchedGenHelp = std::find(genhelpitemcontrols.begin(), genhelpitemcontrols.end(), currentControlUnderMouse);
      matchedGenHelp != genhelpitemcontrols.end()) {
    auto const index = std::distance(genhelpitemcontrols.begin(), matchedGenHelp);
    updatehelp(HELP_CATEGORY_GENERAL, index, NUM_GEN_HELP_ITEMS);
  }
  else if (paramID == P_BUFSIZE) {
    updatehelp(HELP_CATEGORY_GENERAL, HELP_WINDOWSIZE, NUM_GEN_HELP_ITEMS);
  }
  else if (paramID == P_SHAPE) {
    updatehelp(HELP_CATEGORY_WINDOWSHAPE, getparameter_i(P_SHAPE), NUM_WINDOWSHAPES);
  }
  else if ((paramID >= P_POINTSTYLE) && (paramID < (P_POINTPARAMS + MAX_POINTSTYLES))) {
    updatehelp(HELP_CATEGORY_LANDMARKS, getparameter_i(P_POINTSTYLE), NUM_POINTSTYLES);
  }
  else if ((paramID >= P_INTERPSTYLE) && (paramID < (P_INTERPARAMS + MAX_INTERPSTYLES))) {
    updatehelp(HELP_CATEGORY_RECREATE, getparameter_i(P_INTERPSTYLE), NUM_INTERPSTYLES);
  }
  else if ((paramID >= P_POINTOP1) && (paramID < (P_OPPAR1S + MAX_OPS))) {
    updatehelp(HELP_CATEGORY_OPS, getparameter_i(P_POINTOP1), NUM_OPS);
  }
  else if ((paramID >= P_POINTOP2) && (paramID < (P_OPPAR2S + MAX_OPS))) {
    updatehelp(HELP_CATEGORY_OPS, getparameter_i(P_POINTOP2), NUM_OPS);
  }
  else if ((paramID >= P_POINTOP3) && (paramID < (P_OPPAR3S + MAX_OPS))) {
    updatehelp(HELP_CATEGORY_OPS, getparameter_i(P_POINTOP3), NUM_OPS);
  }
  else {
    updatehelp(HELP_CATEGORY_GENERAL, HELP_EMPTY, NUM_GEN_HELP_ITEMS);
  }
}

//-----------------------------------------------------------------------------
long GeometerEditor::get_base_param_for_slider(size_t sliderIndex) noexcept {

  switch (sliderIndex) {
    case 0:
      return P_POINTSTYLE;
    case 1:
      return P_INTERPSTYLE;
    case 2:
      return P_POINTOP1;
    case 3:
      return P_POINTOP2;
    default:
      return P_POINTOP3;
  }
}
