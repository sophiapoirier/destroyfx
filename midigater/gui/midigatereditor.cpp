#include "midigatereditor.hpp"
#include "midigater.hpp"

#include "dfxguislider.h"
#include "dfxguidisplay.h"
#include "dfxguibutton.h"


//-----------------------------------------------------------------------------
enum {
	// positions
	kSliderX = 13,
	kSlopeSliderY = 37,
	kVelInfluenceSliderY = 77,
	kFloorSliderY = 116,
	kSliderWidth = 289 - kSliderX,

	kDisplayX = 333+1,
	kSlopeDisplayY = kSlopeSliderY + 1,
	kVelInfluenceDisplayY = kVelInfluenceSliderY + 1,
	kFloorDisplayY = kFloorSliderY + 2,
	kDisplayWidth = 114,
	kDisplayHeight = 12,
	kVelInfluenceLabelWidth = kDisplayWidth - 33,

	kDestroyFXlinkX = 5,
	kDestroyFXlinkY = 7
};


const DGColor kValueTextColor(152, 221, 251);
//const char * kValueTextFont = "Arial";
const char * kValueTextFont = "Trebuchet MS";
const float kValueTextSize = 10.5f;


//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void slopeDisplayProc(float value, char *outText, void*);
void slopeDisplayProc(float value, char *outText, void*)
{
	long thousands = (long)value / 1000;
	float remainder = fmodf(value, 1000.0f);
	if (thousands > 0)
		sprintf(outText, "%ld,%05.1f", thousands, remainder);
	else
		sprintf(outText, "%.1f", value);
	strcat(outText, " ms");
}

void velInfluenceDisplayProc(float value, char *outText, void*);
void velInfluenceDisplayProc(float value, char *outText, void*)
{
	sprintf(outText, "%.1f%%", value * 100.0f);
}

void floorDisplayProc(float value, char *outText, void*);
void floorDisplayProc(float value, char *outText, void*)
{
	if (value <= 0.0f)
//		sprintf(outText, "-\xB0 dB");
		sprintf(outText, "-oo dB");
	else
		sprintf(outText, "%.1f dB", linear2dB(value));
}



// ____________________________________________________________________________
COMPONENT_ENTRY(MidiGaterEditor);

//-----------------------------------------------------------------------------
MidiGaterEditor::MidiGaterEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
OSStatus MidiGaterEditor::open(Float32 inXOffset, Float32 inYOffset)
{
	// load some graphics
	//
	// background image
	DGGraphic *gBackground = new DGGraphic("midi-gater-background.png");
	addImage(gBackground);
	SetBackgroundImage(gBackground);
	//
	DGGraphic *gSlopeSliderHandle = new DGGraphic("slider-handle-slope.png");
	addImage(gSlopeSliderHandle);
	DGGraphic *gVelInfluenceSliderHandle = new DGGraphic("slider-handle-velocity-influence.png");
	addImage(gVelInfluenceSliderHandle);
	DGGraphic *gFloorSliderHandle = new DGGraphic("slider-handle-floor.png");
	addImage(gFloorSliderHandle);
	DGGraphic *gDestroyFXlinkButton = new DGGraphic("destroy-fx-link-button.png");
	addImage(gDestroyFXlinkButton);


	DGRect pos;

	//--initialize the horizontal faders-------------------------------------
	DGSlider *slider;

	// slope
	pos.set(kSliderX, kSlopeSliderY, kSliderWidth, gSlopeSliderHandle->getHeight());
	slider = new DGSlider(this, kSlope, &pos, kDGSliderStyle_horizontal, gSlopeSliderHandle, NULL);
	addControl(slider);

	// velocity influence
	pos.set(kSliderX, kVelInfluenceSliderY, kSliderWidth, gVelInfluenceSliderHandle->getHeight());
	slider = new DGSlider(this, kVelInfluence, &pos, kDGSliderStyle_horizontal, gVelInfluenceSliderHandle, NULL);
	addControl(slider);

	// floor
	pos.set(kSliderX, kFloorSliderY, kSliderWidth, gFloorSliderHandle->getHeight());
	slider = new DGSlider(this, kFloor, &pos, kDGSliderStyle_horizontal, gFloorSliderHandle, NULL);
	addControl(slider);


	//--initialize the displays---------------------------------------------
	DGTextDisplay *display;
	DGStaticTextDisplay *label;
	AUVParameter auvp;

	// slope
	pos.set(kDisplayX, kSlopeDisplayY, kDisplayWidth/2, kDisplayHeight);
	label = new DGStaticTextDisplay(this, &pos, NULL, kValueTextFont);
	label->setFontSize(kValueTextSize);
	label->setTextAlignmentStyle(kDGTextAlign_left);
	label->setFontColor(kValueTextColor);
	auvp = AUVParameter(GetEditAudioUnit(), kSlope, kAudioUnitScope_Global, (AudioUnitElement)0);
	label->setText(auvp.ParamInfo().name);
	addControl(label);
	//
	pos.offset(kDisplayWidth/2, 0);
	display = new DGTextDisplay(this, kSlope, &pos, slopeDisplayProc, NULL, NULL, kValueTextFont);
	display->setFontSize(kValueTextSize);
	display->setTextAlignmentStyle(kDGTextAlign_right);
	display->setFontColor(kValueTextColor);
	addControl(display);

	// velocity influence
	pos.set(kDisplayX - 1, kVelInfluenceDisplayY, kVelInfluenceLabelWidth + 1, kDisplayHeight);
	label = new DGStaticTextDisplay(this, &pos, NULL, kValueTextFont);
	label->setFontSize(kValueTextSize - 0.48f);
	label->setTextAlignmentStyle(kDGTextAlign_left);
	label->setFontColor(kValueTextColor);
	auvp = AUVParameter(GetEditAudioUnit(), kVelInfluence, kAudioUnitScope_Global, (AudioUnitElement)0);
	label->setText(auvp.ParamInfo().name);
	addControl(label);
	//
	pos.set(kDisplayX + kVelInfluenceLabelWidth, kVelInfluenceDisplayY, kDisplayWidth - kVelInfluenceLabelWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kVelInfluence, &pos, velInfluenceDisplayProc, NULL, NULL, kValueTextFont);
	display->setFontSize(kValueTextSize);
	display->setTextAlignmentStyle(kDGTextAlign_right);
	display->setFontColor(kValueTextColor);
	addControl(display);

	// floor
	pos.set(kDisplayX, kFloorDisplayY, kDisplayWidth/2, kDisplayHeight);
	label = new DGStaticTextDisplay(this, &pos, NULL, kValueTextFont);
	label->setFontSize(kValueTextSize);
	label->setTextAlignmentStyle(kDGTextAlign_left);
	label->setFontColor(kValueTextColor);
	auvp = AUVParameter(GetEditAudioUnit(), kFloor, kAudioUnitScope_Global, (AudioUnitElement)0);
	label->setText(auvp.ParamInfo().name);
	addControl(label);
	//
	pos.offset(kDisplayWidth/2, 0);
	display = new DGTextDisplay(this, kFloor, &pos, floorDisplayProc, NULL, NULL, kValueTextFont);
	display->setFontSize(kValueTextSize);
	display->setTextAlignmentStyle(kDGTextAlign_right);
	display->setFontColor(kValueTextColor);
	addControl(display);


	//--initialize the buttons----------------------------------------------

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, gDestroyFXlinkButton->getWidth(), gDestroyFXlinkButton->getHeight()/2);
	DGWebLink *dfxLinkButton = new DGWebLink(this, &pos, gDestroyFXlinkButton, DESTROYFX_URL);
	addControl(dfxLinkButton);



	return noErr;
}
