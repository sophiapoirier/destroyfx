#include "bufferoverrideeditor.h"
#include "bufferoverride.hpp"

#include "dfxguipane.h"


#define VALUE_DISPLAY_FONT	"Helvetica"
const float VALUE_DISPLAY_REGULAR_FONT_SIZE = 10.8f;
const float VALUE_DISPLAY_TINY_FONT_SIZE = 10.2f;
#define HELP_DISPLAY_FONT	"Helvetica"
const float HELP_DISPLAY_FONT_SIZE = 9.6f;
const DGColor HELP_DISPLAY_TEXT_COLOR = { 201, 201, 201 };



//-----------------------------------------------------------------------------
enum {
	// sliders
	kSliderWidth = 122,
	kSliderHeight = 14,
	kLFOsliderHeight = 18,
	//
	kDivisorLFOrateSliderX = 7,
	kDivisorLFOrateSliderY = 175,
	kDivisorLFOdepthSliderX = kDivisorLFOrateSliderX,
	kDivisorLFOdepthSliderY = 199,
	kBufferLFOrateSliderX = 351,
	kBufferLFOrateSliderY = 175,
	kBufferLFOdepthSliderX = kBufferLFOrateSliderX,
	kBufferLFOdepthSliderY = 199,
	//
	kPitchbendSliderX = 6,
	kPitchbendSliderY = 28,
	kSmoothSliderX = 354,
	kSmoothSliderY = 28,
	kDryWetMixSliderX = kSmoothSliderX,
	kDryWetMixSliderY = 65,
	//
	kTempoSliderWidth = 172,
	kTempoSliderX = 121,
	kTempoSliderY = 3,

	// displays
	kDisplayWidth = 36,
	kDisplayHeight = 12,
	//
	kDivisorDisplayX = 127 - kDisplayWidth,
	kDivisorDisplayY = 78,
	kBufferDisplayX = 241 - (kDisplayWidth/2),
	kBufferDisplayY = 195,
	//
	kLFOrateDisplayWidth = 21,
	kDivisorLFOrateDisplayX = 174 - kLFOrateDisplayWidth,//155,
	kDivisorLFOrateDisplayY = 188,
	kDivisorLFOdepthDisplayX = 171 - kDisplayWidth + 2,
	kDivisorLFOdepthDisplayY = 212,
	kBufferLFOrateDisplayX = 328 - kLFOrateDisplayWidth,
	kBufferLFOrateDisplayY = 188,
	kBufferLFOdepthDisplayX = 331 - kDisplayWidth,
	kBufferLFOdepthDisplayY = 212,
	//
	kPitchbendDisplayX = 101 - kDisplayWidth,
	kPitchbendDisplayY = 42,
	kSmoothDisplayX = 413 - kDisplayWidth,
	kSmoothDisplayY = 42,
	kDryWetMixDisplayX = 413 - kDisplayWidth,
	kDryWetMixDisplayY = 79,
	kTempoDisplayX = 312 - 1,
	kTempoDisplayY = 4,
	//
	kHelpDisplayX = 0,
	kHelpDisplayY = 231,

	// XY box
	kDivisorBufferBoxX = 156,
	kDivisorBufferBoxY = 40,
	kDivisorBufferBoxWidth = 169,
	kDivisorBufferBoxHeight = 138,

	// buttons
	kBufferInterruptButtonX = 227,
	kBufferInterruptButtonY = 20,
	kBufferInterruptButtonCornerX = 325,
	kBufferInterruptButtonCornerY = 20,

	kBufferTempoSyncButtonX = 156,
	kBufferTempoSyncButtonY = 20,
	kBufferTempoSyncButtonCornerX = 133,
	kBufferTempoSyncButtonCornerY = 20,
	kBufferSizeLabelX = 222,
	kBufferSizeLabelY = 212,

	kMidiModeButtonX = 5,
	kMidiModeButtonY = 6,

	kDivisorLFOshapeSwitchX = 8,
	kDivisorLFOshapeSwitchY = 145,
	kDivisorLFOtempoSyncButtonX = 52,
	kDivisorLFOtempoSyncButtonY = 119,
	kDivisorLFOrateLabelX = 178,
	kDivisorLFOrateLabelY = 188,

	kBufferLFOshapeSwitchX = 352,
	kBufferLFOshapeSwitchY = 145,
	kBufferLFOtempoSyncButtonX = 351,
	kBufferLFOtempoSyncButtonY = 120,
	kBufferLFOrateLabelX = 277,
	kBufferLFOrateLabelY = 188,

	kMidiLearnButtonX = 5,
	kMidiLearnButtonY = 63,
	kMidiResetButtonX = 4,
	kMidiResetButtonY = 86,

	kGoButtonX = 386,
	kGoButtonY = 6,
};



//-----------------------------------------------------------------------------

void midilearnBufferOverride(UInt32 value, void *editor);
void midilearnBufferOverride(UInt32 value, void *editor)
{
	if (editor != NULL)
	{
		if (value == 0)
			((DfxGuiEditor*)editor)->setmidilearning(false);
		else
			((DfxGuiEditor*)editor)->setmidilearning(true);
	}
}

void midiresetBufferOverride(UInt32 value, void *editor);
void midiresetBufferOverride(UInt32 value, void *editor)
{
	if ( (editor != NULL) && (value != 0) )
		((DfxGuiEditor*)editor)->resetmidilearn();
}

void linkKickButtonsDownProc(UInt32, void *otherbutton);
void linkKickButtonsDownProc(UInt32, void *otherbutton)
{
	if (otherbutton != NULL)
	{
		((DGButton*)otherbutton)->setMouseIsDown(true);
		((DGButton*)otherbutton)->redraw();
	}
}

void linkKickButtonsUpProc(UInt32, void *otherbutton);
void linkKickButtonsUpProc(UInt32, void *otherbutton)
{
	if (otherbutton != NULL)
	{
		((DGButton*)otherbutton)->setMouseIsDown(false);
		((DGButton*)otherbutton)->redraw();
	}
}


static void TempoSyncListenerProc(void *inRefCon, void *inObject, const AudioUnitParameter *inParameter, Float32 inValue);
static void TempoSyncListenerProc(void *inRefCon, void *inObject, const AudioUnitParameter *inParameter, Float32 inValue)
{
	if ( (inObject == NULL) || (inParameter == NULL) )
		return;

	DGControl *control = (DGControl*) inObject;
	bool useSyncParam = FBOOL(inValue);
	AudioUnitParameterID newParameterID;
	switch (inParameter->mParameterID)
	{
		case kBufferTempoSync:
			newParameterID = useSyncParam ? kBufferSize_sync : kBufferSize_abs;
			break;
		case kDivisorLFOtempoSync:
			newParameterID = useSyncParam ? kDivisorLFOrate_sync : kDivisorLFOrate_abs;
			break;
		case kBufferLFOtempoSync:
			newParameterID = useSyncParam ? kBufferLFOrate_sync : kBufferLFOrate_abs;
			break;
		default:
			return;
	}
	control->setParameterID(newParameterID);
}


void divisorDisplayProc(Float32 value, char *outText, void*);
void divisorDisplayProc(Float32 value, char *outText, void*)
{
	if (value < 2.0f)
		sprintf(outText, "%.2f", 1.0f);
	else
		sprintf(outText, "%.2f", value);
}

void bufferSizeDisplayProc(Float32 value, char *outText, void *editor);
void bufferSizeDisplayProc(Float32 value, char *outText, void *editor)
{
	if ( ((DfxGuiEditor*)editor)->getparameter_b(kBufferTempoSync) )
		((DfxGuiEditor*)editor)->getparametervaluestring(kBufferSize_sync, outText);
	else
		sprintf(outText, "%.1f", value);
}

void divisorLFOrateDisplayProc(Float32 value, char *outText, void *editor);
void divisorLFOrateDisplayProc(Float32 value, char *outText, void *editor)
{
	if ( ((DfxGuiEditor*)editor)->getparameter_b(kDivisorLFOtempoSync) )
		((DfxGuiEditor*)editor)->getparametervaluestring(kDivisorLFOrate_sync, outText);
	else
	{
		if (value < 10.0f)
			sprintf(outText, "%.2f", value);
		else
			sprintf(outText, "%.1f", value);
	}
}

void bufferLFOrateDisplayProc(Float32 value, char *outText, void *editor);
void bufferLFOrateDisplayProc(Float32 value, char *outText, void *editor)
{
	if ( ((DfxGuiEditor*)editor)->getparameter_b(kBufferLFOtempoSync) )
		((DfxGuiEditor*)editor)->getparametervaluestring(kBufferLFOrate_sync, outText);
	else
	{
		if (value < 10.0f)
			sprintf(outText, "%.2f", value);
		else
			sprintf(outText, "%.1f", value);
	}
}

void LFOdepthDisplayProc(Float32 value, char *outText, void*);
void LFOdepthDisplayProc(Float32 value, char *outText, void*)
{
	sprintf(outText, "%ld%%", (long)value);
}

void smoothDisplayProc(Float32 value, char *outText, void*);
void smoothDisplayProc(Float32 value, char *outText, void*)
{
	sprintf(outText, "%.1f%%", value);
}

void dryWetMixDisplayProc(Float32 value, char *outText, void*);
void dryWetMixDisplayProc(Float32 value, char *outText, void*)
{
	sprintf(outText, "%ld%%", (long)value);
}

void pitchbendDisplayProc(Float32 value, char *outText, void*);
void pitchbendDisplayProc(Float32 value, char *outText, void*)
{
	sprintf(outText, "\xB1 %.2f", value);
}

void tempoDisplayProc(Float32 value, char *outText, void*);
void tempoDisplayProc(Float32 value, char *outText, void*)
{
	sprintf(outText, "%.2f", value);
}



// ____________________________________________________________________________
COMPONENT_ENTRY(BufferOverrideEditor);

// ____________________________________________________________________________
BufferOverrideEditor::BufferOverrideEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
	AUListenerCreate(TempoSyncListenerProc, this,
		CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.010f, // 10 ms
		&parameterListener);

	midilearnButton = NULL;
	midiresetButton = NULL;
	helpDisplay = NULL;
//printf("creating BufferOverrideEditor\n");
}

// ____________________________________________________________________________
BufferOverrideEditor::~BufferOverrideEditor()
{
	AUListenerRemoveParameter(parameterListener, bufferSizeSlider, &bufferSizeTempoSyncAUP);
	AUListenerRemoveParameter(parameterListener, bufferSizeDisplay, &bufferSizeTempoSyncAUP);
	AUListenerRemoveParameter(parameterListener, divisorLFOrateSlider, &divisorLFOtempoSyncAUP);
	AUListenerRemoveParameter(parameterListener, divisorLFOrateDisplay, &divisorLFOtempoSyncAUP);
	AUListenerRemoveParameter(parameterListener, bufferLFOrateSlider, &bufferLFOtempoSyncAUP);
	AUListenerRemoveParameter(parameterListener, bufferLFOrateDisplay, &bufferLFOtempoSyncAUP);

	AUListenerDispose(parameterListener);
//printf("destroying BufferOverrideEditor\n");
}

// ____________________________________________________________________________
OSStatus BufferOverrideEditor::open(Float32 inXOffset, Float32 inYOffset)
{
printf("\n\n--------------------------------------------------\n");
printf("       creating Buffer Override GUI\n");
printf("--------------------------------------------------\n\n");
	bufferSizeTempoSyncAUP.mAudioUnit = divisorLFOtempoSyncAUP.mAudioUnit = bufferLFOtempoSyncAUP.mAudioUnit = GetEditAudioUnit();
	bufferSizeTempoSyncAUP.mScope = divisorLFOtempoSyncAUP.mScope = bufferLFOtempoSyncAUP.mScope = kAudioUnitScope_Global;
	bufferSizeTempoSyncAUP.mElement = divisorLFOtempoSyncAUP.mElement = bufferLFOtempoSyncAUP.mElement = (AudioUnitElement)0;
	bufferSizeTempoSyncAUP.mParameterID = kBufferTempoSync;
	divisorLFOtempoSyncAUP.mParameterID = kDivisorLFOtempoSync;
	bufferLFOtempoSyncAUP.mParameterID = kBufferLFOtempoSync;

/***************************************
	create graphics objects from PNG resource files
***************************************/

	// Background image
	DGGraphic *gBackground = new DGGraphic("buffer-override-background.png");
	addImage(gBackground);

	// these move across the drawing rectangle
	DGGraphic *gSliderHandle = new DGGraphic("slider-handle.png");
	addImage(gSliderHandle);
	DGGraphic *gXYboxHandle = new DGGraphic("xy-box-handle.png");
	addImage(gXYboxHandle);

	// buttons
	DGGraphic *gBufferTempoSyncButton = new DGGraphic("buffer-tempo-sync-button.png");
	addImage(gBufferTempoSyncButton);
	DGGraphic *gBufferTempoSyncButtonCorner = new DGGraphic("buffer-tempo-sync-button-corner.png");
	addImage(gBufferTempoSyncButtonCorner);
	//
	DGGraphic *gBufferSizeLabel = new DGGraphic("buffer-size-label.png");
	addImage(gBufferSizeLabel);

	DGGraphic *gBufferInterruptButton = new DGGraphic("buffer-interrupt-button.png");
	addImage(gBufferInterruptButton);
	DGGraphic *gBufferInterruptButtonCorner = new DGGraphic("buffer-interrupt-button-corner.png");
	addImage(gBufferInterruptButtonCorner);

	DGGraphic *gDivisorLFOtempoSyncButton = new DGGraphic("divisor-lfo-tempo-sync-button.png");
	addImage(gDivisorLFOtempoSyncButton);
	//
	DGGraphic *gDivisorLFOrateLabel = new DGGraphic("divisor-lfo-rate-label.png");
	addImage(gDivisorLFOrateLabel);

	DGGraphic *gBufferLFOtempoSyncButton = new DGGraphic("buffer-lfo-tempo-sync-button.png");
	addImage(gBufferLFOtempoSyncButton);
	//
	DGGraphic *gBufferLFOrateLabel = new DGGraphic("buffer-lfo-rate-label.png");
	addImage(gBufferLFOrateLabel);

	DGGraphic *gDivisorLFOshapeSwitch = new DGGraphic("divisor-lfo-shape-switch.png");
	addImage(gDivisorLFOshapeSwitch);

	DGGraphic *gBufferLFOshapeSwitch = new DGGraphic("buffer-lfo-shape-switch.png");
	addImage(gBufferLFOshapeSwitch);

	DGGraphic *gMidiModeButton = new DGGraphic("midi-mode-button.png");
	addImage(gMidiModeButton);

	DGGraphic *gMidiLearnButton = new DGGraphic("midi-learn-button.png");
	addImage(gMidiLearnButton);

	DGGraphic *gMidiResetButton = new DGGraphic("midi-reset-button.png");
	addImage(gMidiResetButton);

	DGGraphic *gGoButton = new DGGraphic("go-button.png");
	addImage(gGoButton);


/***************************************
	create controls
***************************************/
	
	DGRect pos;

	// get size of GUI from background image and create background pane
	SInt16 width = gBackground->getWidth();
	SInt16 height = gBackground->getHeight();
	pos.set (0, 0, width, height);
	DGPane *mainPane = new DGPane(this, &pos, gBackground);
	addControl(mainPane);


	DGSlider *slider;

	long divisorLFOrateTag = getparameter_b(kDivisorLFOtempoSync) ? kDivisorLFOrate_sync : kDivisorLFOrate_abs;
	pos.set (kDivisorLFOrateSliderX, kDivisorLFOrateSliderY, kSliderWidth, kLFOsliderHeight);
	divisorLFOrateSlider = new DGSlider(this, divisorLFOrateTag, &pos, kDGSliderStyle_horizontal, gSliderHandle, gBackground);
	mainPane->addControl(divisorLFOrateSlider);

	pos.set (kDivisorLFOdepthSliderX, kDivisorLFOdepthSliderY, kSliderWidth, kLFOsliderHeight);
	slider = new DGSlider(this, kDivisorLFOdepth, &pos, kDGSliderStyle_horizontal, gSliderHandle, gBackground);
	mainPane->addControl(slider);

	long bufferLFOrateTag = getparameter_b(kBufferLFOtempoSync) ? kBufferLFOrate_sync : kBufferLFOrate_abs;
	pos.set (kBufferLFOrateSliderX, kBufferLFOrateSliderY, kSliderWidth, kLFOsliderHeight);
	bufferLFOrateSlider = new DGSlider(this, bufferLFOrateTag, &pos, kDGSliderStyle_horizontal, gSliderHandle, gBackground);
	mainPane->addControl(bufferLFOrateSlider);

	pos.set (kBufferLFOdepthSliderX, kBufferLFOdepthSliderY, kSliderWidth, kLFOsliderHeight);
	slider = new DGSlider(this, kBufferLFOdepth, &pos, kDGSliderStyle_horizontal, gSliderHandle, gBackground);
	mainPane->addControl(slider);

	pos.set (kSmoothSliderX, kSmoothSliderY, kSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kSmooth, &pos, kDGSliderStyle_horizontal, gSliderHandle, gBackground);
	mainPane->addControl(slider);

	pos.set (kDryWetMixSliderX, kDryWetMixSliderY, kSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kDryWetMix, &pos, kDGSliderStyle_horizontal, gSliderHandle, gBackground);
	mainPane->addControl(slider);

	pos.set (kPitchbendSliderX, kPitchbendSliderY, kSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kPitchbend, &pos, kDGSliderStyle_horizontal, gSliderHandle, gBackground);
	mainPane->addControl(slider);

	pos.set (kTempoSliderX, kTempoSliderY, kTempoSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kTempo, &pos, kDGSliderStyle_horizontal, gSliderHandle, gBackground);
	mainPane->addControl(slider);

	pos.set (kDivisorBufferBoxX + 3, kDivisorBufferBoxY + 27, kDivisorBufferBoxWidth - 6, kSliderHeight);
	slider = new DGSlider(this, kDivisor, &pos, kDGSliderStyle_horizontal, gXYboxHandle, gBackground);
	mainPane->addControl(slider);

	long bufferSizeTag = getparameter_b(kBufferTempoSync) ? kBufferSize_sync : kBufferSize_abs;
	pos.offset(0, 57);
	bufferSizeSlider = new DGSlider(this, bufferSizeTag, &pos, kDGSliderStyle_horizontal, gXYboxHandle, gBackground);
	mainPane->addControl(bufferSizeSlider);



	pos.set (kDivisorDisplayX, kDivisorDisplayY, kDisplayWidth, kDisplayHeight);
	DGTextDisplay *divisorDisplay = new DGTextDisplay(this, kDivisor, &pos, divisorDisplayProc, NULL, gBackground, VALUE_DISPLAY_FONT);
	divisorDisplay->setFontSize(VALUE_DISPLAY_REGULAR_FONT_SIZE);
	divisorDisplay->setTextAlignmentStyle(kDGTextAlign_right);
	divisorDisplay->setFontColor(kWhiteDGColor);
	mainPane->addControl(divisorDisplay);

	pos.set (kBufferDisplayX, kBufferDisplayY, kDisplayWidth, kDisplayHeight);
	bufferSizeDisplay = new DGTextDisplay(this, bufferSizeTag, &pos, bufferSizeDisplayProc, this, gBackground, VALUE_DISPLAY_FONT);
	bufferSizeDisplay->setFontSize(VALUE_DISPLAY_REGULAR_FONT_SIZE);
	bufferSizeDisplay->setFontColor(kWhiteDGColor);
	bufferSizeDisplay->setTextAlignmentStyle(kDGTextAlign_center);
	mainPane->addControl(bufferSizeDisplay);

	pos.set (kDivisorLFOrateDisplayX, kDivisorLFOrateDisplayY, kLFOrateDisplayWidth, kDisplayHeight);
	divisorLFOrateDisplay = new DGTextDisplay(this, divisorLFOrateTag, &pos, divisorLFOrateDisplayProc, this, gBackground, VALUE_DISPLAY_FONT);
	divisorLFOrateDisplay->setFontSize(VALUE_DISPLAY_TINY_FONT_SIZE);
	divisorLFOrateDisplay->setTextAlignmentStyle(kDGTextAlign_right);
	divisorLFOrateDisplay->setFontColor(kWhiteDGColor);
	mainPane->addControl(divisorLFOrateDisplay);

	pos.set (kDivisorLFOdepthDisplayX, kDivisorLFOdepthDisplayY, kDisplayWidth, kDisplayHeight);
	DGTextDisplay *divisorLFOdepthDisplay = new DGTextDisplay(this, kDivisorLFOdepth, &pos, LFOdepthDisplayProc, NULL, gBackground, VALUE_DISPLAY_FONT);
	divisorLFOdepthDisplay->setFontSize(VALUE_DISPLAY_TINY_FONT_SIZE);
	divisorLFOdepthDisplay->setTextAlignmentStyle(kDGTextAlign_right);
	divisorLFOdepthDisplay->setFontColor(kWhiteDGColor);
	mainPane->addControl(divisorLFOdepthDisplay);

	pos.set (kBufferLFOrateDisplayX, kBufferLFOrateDisplayY, kLFOrateDisplayWidth, kDisplayHeight);
	bufferLFOrateDisplay = new DGTextDisplay(this, bufferLFOrateTag, &pos, bufferLFOrateDisplayProc, this, gBackground, VALUE_DISPLAY_FONT);
	bufferLFOrateDisplay->setFontSize(VALUE_DISPLAY_TINY_FONT_SIZE);
	bufferLFOrateDisplay->setTextAlignmentStyle(kDGTextAlign_right);
	bufferLFOrateDisplay->setFontColor(kWhiteDGColor);
	mainPane->addControl(bufferLFOrateDisplay);

	pos.set (kBufferLFOdepthDisplayX, kBufferLFOdepthDisplayY, kDisplayWidth, kDisplayHeight);
	DGTextDisplay *bufferLFOdepthDisplay = new DGTextDisplay(this, kBufferLFOdepth, &pos, LFOdepthDisplayProc, NULL, gBackground, VALUE_DISPLAY_FONT);
	bufferLFOdepthDisplay->setFontSize(VALUE_DISPLAY_TINY_FONT_SIZE);
	bufferLFOdepthDisplay->setTextAlignmentStyle(kDGTextAlign_right);
	bufferLFOdepthDisplay->setFontColor(kWhiteDGColor);
	mainPane->addControl(bufferLFOdepthDisplay);

	pos.set (kSmoothDisplayX, kSmoothDisplayY, kDisplayWidth, kDisplayHeight);
	DGTextDisplay *smoothDisplay = new DGTextDisplay(this, kSmooth, &pos, smoothDisplayProc, NULL, gBackground, VALUE_DISPLAY_FONT);
	smoothDisplay->setFontSize(VALUE_DISPLAY_REGULAR_FONT_SIZE);
	smoothDisplay->setTextAlignmentStyle(kDGTextAlign_right);
	smoothDisplay->setFontColor(kWhiteDGColor);
	mainPane->addControl(smoothDisplay);

	pos.set (kDryWetMixDisplayX, kDryWetMixDisplayY, kDisplayWidth, kDisplayHeight);
	DGTextDisplay *dryWetMixDisplay = new DGTextDisplay(this, kDryWetMix, &pos, dryWetMixDisplayProc, NULL, gBackground, VALUE_DISPLAY_FONT);
	dryWetMixDisplay->setFontSize(VALUE_DISPLAY_REGULAR_FONT_SIZE);
	dryWetMixDisplay->setTextAlignmentStyle(kDGTextAlign_right);
	dryWetMixDisplay->setFontColor(kWhiteDGColor);
	mainPane->addControl(dryWetMixDisplay);

	pos.set (kPitchbendDisplayX, kPitchbendDisplayY, kDisplayWidth, kDisplayHeight);
	DGTextDisplay *pitchbendDisplay = new DGTextDisplay(this, kPitchbend, &pos, pitchbendDisplayProc, NULL, gBackground, VALUE_DISPLAY_FONT);
	pitchbendDisplay->setFontSize(VALUE_DISPLAY_REGULAR_FONT_SIZE);
	pitchbendDisplay->setTextAlignmentStyle(kDGTextAlign_right);
	pitchbendDisplay->setFontColor(kWhiteDGColor);
	mainPane->addControl(pitchbendDisplay);

	pos.set (kTempoDisplayX, kTempoDisplayY, kDisplayWidth, kDisplayHeight);
	DGTextDisplay *tempoDisplay = new DGTextDisplay(this, kTempo, &pos, tempoDisplayProc, NULL, gBackground, VALUE_DISPLAY_FONT);
	tempoDisplay->setFontSize(VALUE_DISPLAY_TINY_FONT_SIZE);
	tempoDisplay->setTextAlignmentStyle(kDGTextAlign_left);
	tempoDisplay->setFontColor(kWhiteDGColor);
	mainPane->addControl(tempoDisplay);


	// forced buffer size tempo sync button
	pos.set (kBufferTempoSyncButtonX, kBufferTempoSyncButtonY, gBufferTempoSyncButton->getWidth()/2, gBufferTempoSyncButton->getHeight()/2);
	DGButton *bufferTempoSyncButton = new DGButton(this, kBufferTempoSync, &pos, gBufferTempoSyncButton, 2, kDGButtonType_incbutton, true);
	mainPane->addControl(bufferTempoSyncButton);
	//
	pos.set (kBufferTempoSyncButtonCornerX, kBufferTempoSyncButtonCornerY, gBufferTempoSyncButtonCorner->getWidth()/2, gBufferTempoSyncButtonCorner->getHeight()/2);
	DGButton *bufferTempoSyncButtonCorner = new DGButton(this, kBufferTempoSync, &pos, gBufferTempoSyncButtonCorner, 2, kDGButtonType_incbutton, true);
	mainPane->addControl(bufferTempoSyncButtonCorner);
	//
	bufferTempoSyncButton->setUserProcedure(linkKickButtonsDownProc, bufferTempoSyncButtonCorner);
	bufferTempoSyncButtonCorner->setUserProcedure(linkKickButtonsDownProc, bufferTempoSyncButton);
	bufferTempoSyncButton->setUserReleaseProcedure(linkKickButtonsUpProc, bufferTempoSyncButtonCorner);
	bufferTempoSyncButtonCorner->setUserReleaseProcedure(linkKickButtonsUpProc, bufferTempoSyncButton);

	// buffer interrupt button
	pos.set (kBufferInterruptButtonX, kBufferInterruptButtonY, gBufferInterruptButton->getWidth()/2, gBufferInterruptButton->getHeight()/2);
	DGButton *bufferInterruptButton = new DGButton(this, kBufferInterrupt, &pos, gBufferInterruptButton, 2, kDGButtonType_incbutton, true);
	mainPane->addControl(bufferInterruptButton);
	//
	pos.set (kBufferInterruptButtonCornerX, kBufferInterruptButtonCornerY, gBufferInterruptButtonCorner->getWidth()/2, gBufferInterruptButtonCorner->getHeight()/2);
	DGButton *bufferInterruptButtonCorner = new DGButton(this, kBufferInterrupt, &pos, gBufferInterruptButtonCorner, 2, kDGButtonType_incbutton, true);
	mainPane->addControl(bufferInterruptButtonCorner);
	//
	bufferInterruptButtonCorner->setUserProcedure(linkKickButtonsDownProc, bufferInterruptButton);
	bufferInterruptButton->setUserProcedure(linkKickButtonsDownProc, bufferInterruptButtonCorner);
	bufferInterruptButtonCorner->setUserReleaseProcedure(linkKickButtonsUpProc, bufferInterruptButton);
	bufferInterruptButton->setUserReleaseProcedure(linkKickButtonsUpProc, bufferInterruptButtonCorner);

	// forced buffer size LFO tempo sync button
	pos.set (kBufferLFOtempoSyncButtonX, kBufferLFOtempoSyncButtonY, gBufferLFOtempoSyncButton->getWidth()/2, gBufferLFOtempoSyncButton->getHeight()/2);
	DGButton *bufferLFOtempoSyncButton = new DGButton(this, kBufferLFOtempoSync, &pos, gBufferLFOtempoSyncButton, 2, kDGButtonType_incbutton, true);
	mainPane->addControl(bufferLFOtempoSyncButton);

	// divisor LFO tempo sync button
	pos.set (kDivisorLFOtempoSyncButtonX, kDivisorLFOtempoSyncButtonY, gDivisorLFOtempoSyncButton->getWidth()/2, gDivisorLFOtempoSyncButton->getHeight()/2);
	DGButton *divisorLFOtempoSyncButton = new DGButton(this, kDivisorLFOtempoSync, &pos, gDivisorLFOtempoSyncButton, 2, kDGButtonType_incbutton, true);
	mainPane->addControl(divisorLFOtempoSyncButton);

	// MIDI mode button
	pos.set (kMidiModeButtonX, kMidiModeButtonY, gMidiModeButton->getWidth()/2, gMidiModeButton->getHeight()/kNumMidiModes);
	DGButton *midiModeButton = new DGButton(this, kMidiMode, &pos, gMidiModeButton, kNumMidiModes, kDGButtonType_incbutton, true);
	mainPane->addControl(midiModeButton);

	// MIDI learn button
	pos.set (kMidiLearnButtonX, kMidiLearnButtonY, gMidiLearnButton->getWidth(), gMidiLearnButton->getHeight()/2);
	midilearnButton = new DGButton(this, &pos, gMidiLearnButton, 2, kDGButtonType_incbutton);
	midilearnButton->setUserProcedure(midilearnBufferOverride, this);
	mainPane->addControl(midilearnButton);

	// MIDI reset button
	pos.set (kMidiResetButtonX, kMidiResetButtonY, gMidiResetButton->getWidth(), gMidiResetButton->getHeight()/2);
	midiresetButton = new DGButton(this, &pos, gMidiResetButton, 2, kDGButtonType_pushbutton);
	midiresetButton->setUserProcedure(midiresetBufferOverride, this);
	mainPane->addControl(midiresetButton);

	// go button
	pos.set (kGoButtonX, kGoButtonY, gGoButton->getWidth(), gGoButton->getHeight()/2);
	DGWebLink *goButton = new DGWebLink(this, &pos, gGoButton, "http://www.factoryfarming.com/");
	mainPane->addControl(goButton);


	// forced buffer size LFO shape switch
	pos.set (kBufferLFOshapeSwitchX, kBufferLFOshapeSwitchY, gBufferLFOshapeSwitch->getWidth(), gBufferLFOshapeSwitch->getHeight()/numLFOshapes);
	DGButton *bufferLFOshapeSwitch = new DGButton(this, kBufferLFOshape, &pos, gBufferLFOshapeSwitch, numLFOshapes, kDGButtonType_radiobutton);
	mainPane->addControl(bufferLFOshapeSwitch);

	// divisor LFO shape switch
	pos.set (kDivisorLFOshapeSwitchX, kDivisorLFOshapeSwitchY, gDivisorLFOshapeSwitch->getWidth(), gDivisorLFOshapeSwitch->getHeight()/numLFOshapes);
	DGButton *divisorLFOshapeSwitch = new DGButton(this, kDivisorLFOshape, &pos, gDivisorLFOshapeSwitch, numLFOshapes, kDGButtonType_radiobutton);
	mainPane->addControl(divisorLFOshapeSwitch);


	// forced buffer size label
	pos.set (kBufferSizeLabelX, kBufferSizeLabelY, gBufferSizeLabel->getWidth(), gBufferSizeLabel->getHeight()/2);
	DGButton *bufferSizeLabel = new DGButton(this, kBufferTempoSync, &pos, gBufferSizeLabel, 2, kDGButtonType_picturereel);
	mainPane->addControl(bufferSizeLabel);

	// forced buffer size LFO rate label
	pos.set (kBufferLFOrateLabelX, kBufferLFOrateLabelY, gBufferLFOrateLabel->getWidth(), gBufferLFOrateLabel->getHeight()/2);
	DGButton *bufferLFOrateLabel = new DGButton(this, kBufferLFOtempoSync, &pos, gBufferLFOrateLabel, 2, kDGButtonType_picturereel);
	mainPane->addControl(bufferLFOrateLabel);

	// divisor LFO rate label
	pos.set (kDivisorLFOrateLabelX, kDivisorLFOrateLabelY, gDivisorLFOrateLabel->getHeight(), gDivisorLFOrateLabel->getHeight()/2);
	DGButton *divisorLFOrateLabel = new DGButton(this, kDivisorLFOtempoSync, &pos, gDivisorLFOrateLabel, 2, kDGButtonType_picturereel);
	mainPane->addControl(divisorLFOrateLabel);


/*
HMHelpContentRec helptag;
helptag.version = kMacHelpVersion;
SetRect(&(helptag.absHotRect), 0, 0, 0, 0);
helptag.tagSide = kHMInsideTopCenterAligned;
helptag.content[0].contentType = kHMCFStringContent;
helptag.content[0].u.tagCFString = CFSTR("ooo yeah, MIDI mode shitz");
helptag.content[1].contentType = kHMCFStringContent;
helptag.content[1].u.tagCFString = CFSTR("who deh fuck yoo lookin' at?");
HMSetControlHelpContent(midiModeButton->getCarbonControl(), &helptag);
HMSetHelpTagsDisplayed(true);
*/


	// the help mouseover hint thingy
	pos.set (kHelpDisplayX, kHelpDisplayY, gBackground->getWidth(), kDisplayHeight);
	helpDisplay = new DGStaticTextDisplay(this, &pos, gBackground, HELP_DISPLAY_FONT);
	helpDisplay->setFontSize(HELP_DISPLAY_FONT_SIZE);
	helpDisplay->setTextAlignmentStyle(kDGTextAlign_center);
	helpDisplay->setFontColor(HELP_DISPLAY_TEXT_COLOR);
	mainPane->addControl(helpDisplay);


	AUListenerAddParameter(parameterListener, bufferSizeSlider, &bufferSizeTempoSyncAUP);
	AUListenerAddParameter(parameterListener, bufferSizeDisplay, &bufferSizeTempoSyncAUP);
	AUListenerAddParameter(parameterListener, divisorLFOrateSlider, &divisorLFOtempoSyncAUP);
	AUListenerAddParameter(parameterListener, divisorLFOrateDisplay, &divisorLFOtempoSyncAUP);
	AUListenerAddParameter(parameterListener, bufferLFOrateSlider, &bufferLFOtempoSyncAUP);
	AUListenerAddParameter(parameterListener, bufferLFOrateDisplay, &bufferLFOtempoSyncAUP);


	// set size of overall pane
//	SizeControl(mCarbonPane, width, height);	// not necessary because of EmbedControl done on pane, right?

HMSetTagDelay(9);	// make the hints appear quickly <-- XXX this is sort of a hack

	return noErr;
}


// ____________________________________________________________________________
void BufferOverrideEditor::mouseovercontrolchanged()
{
	// there's no point in continuing
	if (helpDisplay == NULL)
		return;

	DGControl * currentcontrol = getCurrentControl_mouseover();
	long currentcontrolparam = DFX_PARAM_INVALID_ID;
	if (currentcontrol != NULL)
	{
		if ( currentcontrol->isAUVPattached() )
			currentcontrolparam = currentcontrol->getParameterID();
	}

	char helpstring[256];
	switch (currentcontrolparam)
	{
		case kDivisor:
			sprintf(helpstring, "buffer divisor is the number of times each forced buffer skips & starts over");
			break;
		case kBufferSize_abs:
		case kBufferSize_sync:
			sprintf(helpstring, "forced buffer size is the length of the sound chunks that Buffer Override works with");
			break;
//		case kBufferDivisorHelpTag:
#if MAC
//			sprintf(helpstring, "left/right is buffer divisor (the number of skips in a forced buffer, hold ctrl).   up/down is forced buffer size (hold option)");
#else
			// shorten display text for Windows (beware larger fonts)
//			sprintf(helpstring, "left/right is buffer divisor (number of skips in a buffer, hold ctrl).  up/down is forced buffer size (hold alt)");
#endif
//			break;
		case kBufferTempoSync:
			strcpy(helpstring, "turn tempo sync on if you want the size of the forced buffers to sync to your tempo");
			break;
		case kBufferInterrupt:
			strcpy(helpstring, "turn this off for the old version 1 style of stubborn \"stuck\" buffers (if you really want that)");
			break;
		case kDivisorLFOrate_abs:
		case kDivisorLFOrate_sync:
			strcpy(helpstring, "this is the speed of the LFO that modulates the buffer divisor");
			break;
		case kDivisorLFOdepth:
			strcpy(helpstring, "the depth (or intensity) of the LFO that modulates the buffer divisor (0% does nothing)");
			break;
		case kDivisorLFOshape:
			strcpy(helpstring, "choose the waveform shape of the LFO that modulates the buffer divisor");
			break;
		case kDivisorLFOtempoSync:
			strcpy(helpstring, "turn tempo sync on if you want the rate of the buffer divisor LFO to sync to your tempo");
			break;
		case kBufferLFOrate_abs:
		case kBufferLFOrate_sync:
			strcpy(helpstring, "this is the speed of the LFO that modulates the forced buffer size");
			break;
		case kBufferLFOdepth:
			strcpy(helpstring, "the depth (or intensity) of the LFO that modulates the forced buffer size (0% does nothing)");
			break;
		case kBufferLFOshape:
			strcpy(helpstring, "choose the waveform shape of the LFO that modulates the forced buffer size");
			break;
		case kBufferLFOtempoSync:
			strcpy(helpstring, "turn tempo sync on if you want the rate of the forced buffer size LFO to sync to your tempo");
			break;
		case kSmooth:
			strcpy(helpstring, "the portion of each minibuffer spent smoothly crossfading the previous one into the new one (prevents glitches)");
			break;
		case kDryWetMix:
			strcpy(helpstring, "the relative mix of the processed sound & the clean/original sound (100% is all processed)");
			break;
		case kPitchbend:
			strcpy(helpstring, "the range, in semitones, of the MIDI pitchbend wheel's effect on the buffer divisor");
			break;
		case kMidiMode:
			strcpy(helpstring, "nudge: MIDI notes adjust the buffer divisor.   trigger: notes also reset the divisor to 1 when they are released");
			break;
		case kTempo:
			strcpy(helpstring, "you can adjust the tempo that Buffer Override uses, or set this to \"auto\" to get the tempo from your sequencer");
			break;
//		case kTempoTextEdit:
//			strcpy(helpstring, "you can type in the tempo that Buffer Override uses, or set this to \"auto\" to get the tempo from your sequencer");
//			break;

		default:
			if (currentcontrol == NULL)
				strcpy(helpstring, " ");
			else if (currentcontrol == midilearnButton)
				strcpy(helpstring, "activate or deactivate learn mode for assigning MIDI CCs to control Buffer Override's parameters");
			else if (currentcontrol == midiresetButton)
				strcpy(helpstring, "press this button to erase all of your CC assignments");
			else
				strcpy(helpstring, " ");
			break;
	}

	helpDisplay->setText(helpstring);
}
