#include "midigatereditor.hpp"
#include "midigater.hpp"

#include "dfxguislider.h"
#include "dfxguidisplay.h"
#include "dfxguibutton.h"


//-----------------------------------------------------------------------------
enum {
	// positions
	kSliderX = 13,
	kAttackSlopeSliderY = 37,
	kReleaseSlopeSliderY = 77,
	kVelInfluenceSliderY = 117,
	kFloorSliderY = 156,
	kSliderWidth = 289 - kSliderX,

	kDisplayX = 333+1,
	kAttackSlopeDisplayY = kAttackSlopeSliderY + 1,
	kReleaseSlopeDisplayY = kReleaseSlopeSliderY + 1,
	kVelInfluenceDisplayY = kVelInfluenceSliderY + 1,
	kFloorDisplayY = kFloorSliderY + 2,
	kDisplayWidth = 114,
	kDisplayWidthHalf = kDisplayWidth / 2,
	kDisplayHeight = 12,
	kVelInfluenceLabelWidth = kDisplayWidth - 33,

	kDestroyFXlinkX = 5,
	kDestroyFXlinkY = 7
};


const DGColor kValueTextColor(152.0f/255.0f, 221.0f/255.0f, 251.0f/255.0f);
//const char * kValueTextFont = "Arial";
const char * kValueTextFont = "Trebuchet MS";
const float kValueTextSize = 10.5f;


//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void slopeDisplayProc(float value, char * outText, void *);
void slopeDisplayProc(float value, char * outText, void *)
{
	long thousands = (long)value / 1000;
	float remainder = fmodf(value, 1000.0f);
	if (thousands > 0)
		sprintf(outText, "%ld,%05.1f", thousands, remainder);
	else
		sprintf(outText, "%.1f", value);
	strcat(outText, " ms");
}

void velInfluenceDisplayProc(float value, char * outText, void *);
void velInfluenceDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%.1f%%", value * 100.0f);
}

void floorDisplayProc(float value, char * outText, void *);
void floorDisplayProc(float value, char * outText, void *)
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
long MidiGaterEditor::open()
{
	// load some graphics

	// background image
	DGImage * gBackground = new DGImage("midi-gater-background.png", this);
	SetBackgroundImage(gBackground);

	DGImage * gSlopeSliderHandle = new DGImage("slider-handle-slope.png", this);
	DGImage * gFloorSliderHandle = new DGImage("slider-handle-floor.png", this);
	DGImage * gVelInfluenceSliderHandle = new DGImage("slider-handle-velocity-influence.png", this);

	DGImage * gDestroyFXlinkButton = new DGImage("destroy-fx-link-button.png", this);


	DGRect pos;

	//--initialize the horizontal faders-------------------------------------
	DGSlider * slider;

	// attack slope
	pos.set(kSliderX, kAttackSlopeSliderY, kSliderWidth, gSlopeSliderHandle->getHeight());
	slider = new DGSlider(this, kAttackSlope, &pos, kDGSliderAxis_horizontal, gSlopeSliderHandle, NULL);

	// release slope
	pos.set(kSliderX, kReleaseSlopeSliderY, kSliderWidth, gSlopeSliderHandle->getHeight());
	slider = new DGSlider(this, kReleaseSlope, &pos, kDGSliderAxis_horizontal, gSlopeSliderHandle, NULL);

	// velocity influence
	pos.set(kSliderX, kVelInfluenceSliderY, kSliderWidth, gVelInfluenceSliderHandle->getHeight());
	slider = new DGSlider(this, kVelInfluence, &pos, kDGSliderAxis_horizontal, gVelInfluenceSliderHandle, NULL);

	// floor
	pos.set(kSliderX, kFloorSliderY, kSliderWidth, gFloorSliderHandle->getHeight());
	slider = new DGSlider(this, kFloor, &pos, kDGSliderAxis_horizontal, gFloorSliderHandle, NULL);


	//--initialize the displays---------------------------------------------
	DGTextDisplay * display;
	DGStaticTextDisplay * label;
	CAAUParameter auvp;

	// attack slope
//	const long kAttackSlopeDisplayWidth = kDisplayWidthHalf + 2;
	pos.set(kDisplayX, kAttackSlopeDisplayY, kDisplayWidthHalf, kDisplayHeight);
	label = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_left, kValueTextSize, kValueTextColor, kValueTextFont);
	auvp = CAAUParameter(GetEditAudioUnit(), kAttackSlope, kAudioUnitScope_Global, (AudioUnitElement)0);
	label->setText(auvp.ParamInfo().name);
	//
	pos.offset(kDisplayWidthHalf, 0);
	display = new DGTextDisplay(this, kAttackSlope, &pos, slopeDisplayProc, NULL, NULL, kDGTextAlign_right, 
								kValueTextSize, kValueTextColor, kValueTextFont);

	// release slope
//	const long kReleaseSlopeDisplayWidth = kDisplayWidthHalf + 5;
	pos.set(kDisplayX, kReleaseSlopeDisplayY, kDisplayWidthHalf, kDisplayHeight);
	label = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_left, kValueTextSize, kValueTextColor, kValueTextFont);
	auvp = CAAUParameter(GetEditAudioUnit(), kReleaseSlope, kAudioUnitScope_Global, (AudioUnitElement)0);
	label->setText(auvp.ParamInfo().name);
	//
	pos.offset(kDisplayWidthHalf, 0);
	display = new DGTextDisplay(this, kReleaseSlope, &pos, slopeDisplayProc, NULL, NULL, kDGTextAlign_right, 
								kValueTextSize, kValueTextColor, kValueTextFont);

	// velocity influence
	pos.set(kDisplayX - 1, kVelInfluenceDisplayY, kVelInfluenceLabelWidth + 1, kDisplayHeight);
	label = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_left, kValueTextSize - 0.48f, kValueTextColor, kValueTextFont);
	auvp = CAAUParameter(GetEditAudioUnit(), kVelInfluence, kAudioUnitScope_Global, (AudioUnitElement)0);
	label->setText(auvp.ParamInfo().name);
	//
	pos.set(kDisplayX + kVelInfluenceLabelWidth, kVelInfluenceDisplayY, kDisplayWidth - kVelInfluenceLabelWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kVelInfluence, &pos, velInfluenceDisplayProc, NULL, NULL, 
								kDGTextAlign_right, kValueTextSize, kValueTextColor, kValueTextFont);

	// floor
	pos.set(kDisplayX, kFloorDisplayY, kDisplayWidthHalf, kDisplayHeight);
	label = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_left, kValueTextSize, kValueTextColor, kValueTextFont);
	auvp = CAAUParameter(GetEditAudioUnit(), kFloor, kAudioUnitScope_Global, (AudioUnitElement)0);
	label->setText(auvp.ParamInfo().name);
	//
	pos.offset(kDisplayWidthHalf, 0);
	display = new DGTextDisplay(this, kFloor, &pos, floorDisplayProc, NULL, NULL, kDGTextAlign_right, 
								kValueTextSize, kValueTextColor, kValueTextFont);


	//--initialize the buttons----------------------------------------------

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, gDestroyFXlinkButton->getWidth(), gDestroyFXlinkButton->getHeight()/2);
	DGWebLink * dfxLinkButton = new DGWebLink(this, &pos, gDestroyFXlinkButton, DESTROYFX_URL);



	return noErr;
}
