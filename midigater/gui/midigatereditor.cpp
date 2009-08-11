#include "midigatereditor.h"
#include "midigater.h"


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
// parameter value display text conversion functions

void slopeDisplayProc(float inValue, char * outText, void *);
void slopeDisplayProc(float inValue, char * outText, void *)
{
	long thousands = (long)inValue / 1000;
	float remainder = fmodf(inValue, 1000.0f);
	if (thousands > 0)
		sprintf(outText, "%ld,%05.1f", thousands, remainder);
	else
		sprintf(outText, "%.1f", inValue);
	strcat(outText, " ms");
}

void velInfluenceDisplayProc(float inValue, char * outText, void *);
void velInfluenceDisplayProc(float inValue, char * outText, void *)
{
	sprintf(outText, "%.1f%%", inValue * 100.0f);
}

void floorDisplayProc(float inValue, char * outText, void *);
void floorDisplayProc(float inValue, char * outText, void *)
{
	if (inValue <= 0.0f)
//		sprintf(outText, "-\xB0 dB");
		sprintf(outText, "-oo dB");
	else
		sprintf(outText, "%.1f dB", DFX_Linear2dB(inValue));
}



//____________________________________________________________________________
DFX_EDITOR_ENTRY(MidiGaterEditor)

//-----------------------------------------------------------------------------
MidiGaterEditor::MidiGaterEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long MidiGaterEditor::OpenEditor()
{
	//--load the images-------------------------------------

	// background image
	DGImage * backgroundImage = new DGImage("midi-gater-background.png", 0, this);
	SetBackgroundImage(backgroundImage);

	DGImage * slopeSliderHandleImage = new DGImage("slider-handle-slope.png", 0, this);
	DGImage * floorSliderHandleImage = new DGImage("slider-handle-floor.png", 0, this);
	DGImage * velInfluenceSliderHandleImage = new DGImage("slider-handle-velocity-influence.png", 0, this);

	DGImage * destroyFXLinkButtonImage = new DGImage("destroy-fx-link-button.png", 0, this);


	//--create the controls-------------------------------------
	DGRect pos;

	// --- sliders ---
	DGSlider * slider;

	// attack slope
	pos.set(kSliderX, kAttackSlopeSliderY, kSliderWidth, slopeSliderHandleImage->getHeight());
	slider = new DGSlider(this, kAttackSlope, &pos, kDGAxis_horizontal, slopeSliderHandleImage, NULL);

	// release slope
	pos.set(kSliderX, kReleaseSlopeSliderY, kSliderWidth, slopeSliderHandleImage->getHeight());
	slider = new DGSlider(this, kReleaseSlope, &pos, kDGAxis_horizontal, slopeSliderHandleImage, NULL);

	// velocity influence
	pos.set(kSliderX, kVelInfluenceSliderY, kSliderWidth, velInfluenceSliderHandleImage->getHeight());
	slider = new DGSlider(this, kVelInfluence, &pos, kDGAxis_horizontal, velInfluenceSliderHandleImage, NULL);

	// floor
	pos.set(kSliderX, kFloorSliderY, kSliderWidth, floorSliderHandleImage->getHeight());
	slider = new DGSlider(this, kFloor, &pos, kDGAxis_horizontal, floorSliderHandleImage, NULL);


	// --- text displays ---
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


	// --- buttons ---

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, destroyFXLinkButtonImage->getWidth(), destroyFXLinkButtonImage->getHeight()/2);
	DGWebLink * dfxLinkButton = new DGWebLink(this, &pos, destroyFXLinkButtonImage, DESTROYFX_URL);



	return noErr;
}
