#include "rezsyntheditor.h"
#include "rezsynth.hpp"

#include "dfxguibutton.h"


//-----------------------------------------------------------------------------
enum {
	// positions
	kSliderX = 22,
	kSliderY = 59,
	kSliderInc = 35,

	kDisplayWidth = 123,
	kDisplayHeight = 11,
	kDisplayX = kSliderX,
	kDisplayY = kSliderY - kDisplayHeight - 2,

	kTallSliderX = 383,
	kTallSliderY = 60,
	kTallSliderInc = 51,
	//
kTallDisplayWidener = 8,
	kTallDisplayX = kTallSliderX - 10 - (kTallDisplayWidener/2),
	kTallDisplayY = kTallSliderY + 189,
	kTallDisplayWidth = 36 + kTallDisplayWidener,
	kTallDisplayHeight = 12,
	//
	kTallLabelWidth = kTallSliderInc,
	kTallLabelHeight = kDisplayHeight,
	kTallLabelX = kTallDisplayX - ((kTallLabelWidth - kTallDisplayWidth) / 2),
	kTallLabelY = kTallDisplayY + 17,
	kTallLabelInc = 12,

	kSepModeButtonX = 274,
	kSepModeButtonY = 121,
	kScaleButtonX = 400,
	kScaleButtonY = 33,
	kLegatoButtonX = 435,
	kLegatoButtonY = 296,
	kFadesButtonX = 274,
	kFadesButtonY = 176,
	kFoldoverButtonX = 274,
	kFoldoverButtonY = 87,
	kWiseAmpButtonX = 368,
	kWiseAmpButtonY = 296,

	kMidiLearnButtonX = 189,
	kMidiLearnButtonY = 331,
	kMidiResetButtonX = kMidiLearnButtonX + 72,
	kMidiResetButtonY = 331,

	kGoButtonX = 22,
	kGoButtonY = 331,

	kDestroyFXlinkX = 453,
	kDestroyFXlinkY = 327,
	kSmartElectronixLinkX = 358,
	kSmartElectronixLinkY = 345,
};


const char * kValueTextFont = "Arial";
const float kValueTextFontSize = 10.0f;
const DGColor kBackgroundColor(43.0f/255.0f, 93.0f/255.0f, 63.0f/255.0f);
const DGColor kMyGreyColor(111.0f/255.0f, 111.0f/255.0f, 111.0f/255.0f);
const DGColor kMyLightGreyColor(230.0f/255.0f, 230.0f/255.0f, 228.0f/255.0f);
const DGColor kMyVeryLightGreyColor(233.0f/255.0f, 242.0f/255.0f, 237.0f/255.0f);
const DGColor kMyDarkColor(54.0f/255.0f, 51.0f/255.0f, 39.0f/255.0f);



//-----------------------------------------------------------------------------

void midilearnRezSynth(SInt32 value, void * editor)
{
	if (editor != NULL)
	{
		if (value == 0)
			((DfxGuiEditor*)editor)->setmidilearning(false);
		else
			((DfxGuiEditor*)editor)->setmidilearning(true);
	}
}

void midiresetRezSynth(SInt32 value, void * editor)
{
	if ( (editor != NULL) && (value != 0) )
		((DfxGuiEditor*)editor)->resetmidilearn();
}


//-----------------------------------------------------------------------------
// parameter listener procedure
static void SepModeListenerProc(void * inRefCon, void * inObject, const AudioUnitParameter * inParameter, Float32 inValue)
{
	if ( (inObject == NULL) || (inParameter == NULL) )
		return;

	DGControl * control = (DGControl*) inObject;
//	long newParamID = (control->getDfxGuiEditor()->getparameter_i(kSepMode) == kSepMode_octaval) ? kSepAmount_octaval : kSepAmount_linear;
	long newParamID = ( (long) (inValue + 0.001f) == kSepMode_octaval ) ? kSepAmount_octaval : kSepAmount_linear;
	control->setParameterID(newParamID);
}


//-----------------------------------------------------------------------------
// value text display procedures

void bandwidthDisplayProc(float value, char *outText, void *)
{
	sprintf(outText, "%.3f Hz", value);
}

void numBandsDisplayProc(float value, char *outText, void *)
{
	sprintf(outText, "%.0f", value );
}

void sepAmountDisplayProc(float value, char *outText, void * editor)
{
	if ( ((DfxGuiEditor*)editor)->getparameter_i(kSepMode) == kSepMode_octaval )
		sprintf(outText, "%.3f semitones", value);
	else	// linear values
		sprintf(outText, "%.3f x", value);
}

void attackReleaseDisplayProc(float value, char *outText, void *)
{
	long thousands = (long)value / 1000;
	float remainder = fmodf(value, 1000.0f);

	if (thousands > 0)
		sprintf(outText, "%ld,%05.1f", thousands, remainder);
	else
		sprintf(outText, "%.1f", value);
	strcat(outText, " ms");
}

void velInfluenceDisplayProc(float value, char *outText, void *)
{
	sprintf(outText, "%.0f%%", value);
}

void velCurveDisplayProc(float value, char *outText, void *)
{
	sprintf(outText, "%.3f", value);
}

void pitchbendDisplayProc(float value, char *outText, void *)
{
	sprintf(outText, "\xB1 %.2f semitones", value);
}

void gainDisplayProc(float value, char *outText, void *)
{
	if (value <= 0.0f)
//		strcpy(outText, "-\xB0");
		strcpy(outText, "-oo");
	else if (value > 1.0001f)
		sprintf(outText, "+%.1f", linear2dB(value));
	else
		sprintf(outText, "%.1f", linear2dB(value));
	strcat(outText, " dB");
}

void dryWetMixDisplayProc(float value, char *outText, void *)
{
	sprintf(outText, "%.0f%%", value);
}



//-----------------------------------------------------------------------------
COMPONENT_ENTRY(RezSynthEditor);

//-----------------------------------------------------------------------------
RezSynthEditor::RezSynthEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
	sepAmountSlider = NULL;
	sepAmountDisplay = NULL;

	AUListenerCreate(SepModeListenerProc, this,
		CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.030f, // 30 ms
		&parameterListener);
}

//-----------------------------------------------------------------------------
RezSynthEditor::~RezSynthEditor()
{
	if (sepAmountSlider != NULL)
		AUListenerRemoveParameter(parameterListener, sepAmountSlider, &sepModeAUP);
	if (sepAmountDisplay != NULL)
		AUListenerRemoveParameter(parameterListener, sepAmountDisplay, &sepModeAUP);

	AUListenerDispose(parameterListener);
}

//-----------------------------------------------------------------------------
long RezSynthEditor::open()
{
	// create images

	// background image
	DGImage * gBackground = new DGImage("rez-synth-background.png", this);
	SetBackgroundImage(gBackground);

	// sliders
	DGImage * gHorizontalSliderBackground = new DGImage("horizontal-slider-background.png", this);
	DGImage * gHorizontalSliderHandle = new DGImage("horizontal-slider-handle.png", this);
	DGImage * gVerticalSliderBackground = new DGImage("vertical-slider-background.png", this);
	DGImage * gVerticalSliderHandle = new DGImage("vertical-slider-handle.png", this);

	// buttons
	DGImage * gSepModeButton = new DGImage("separation-mode-button.png", this);
	DGImage * gScaleModeButton = new DGImage("scale-mode-button.png", this);
	DGImage * gFadesButton = new DGImage("fades-button.png", this);
	DGImage * gLegatoButton = new DGImage("legato-button.png", this);
	DGImage * gFoldoverButton = new DGImage("foldover-button.png", this);
	DGImage * gWiseAmpButton = new DGImage("wise-amp-button.png", this);
	DGImage * gMidiLearnButton = new DGImage("midi-learn-button.png", this);
	DGImage * gMidiResetButton = new DGImage("midi-reset-button.png", this);
	DGImage * gGoButton = new DGImage("go-button.png", this);
	DGImage * gDestroyFXlink = new DGImage("destroy-fx-link.png", this);
	DGImage * gSmartElectronixLink = new DGImage("smart-electronix-link.png", this);

DGImage * gVerticalValueDisplayBackground = new DGImage("vertical-value-display-background.png", this);



	// create controls

	DGRect pos, pos2, pos3;

	//--create the horizontal sliders and labels and displays---------------------------------
	pos.set(kSliderX, kSliderY, gHorizontalSliderBackground->getWidth(), gHorizontalSliderBackground->getHeight());
	pos2.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	pos3.set(kDisplayX + kDisplayWidth, kDisplayY, gHorizontalSliderBackground->getWidth() - kDisplayWidth, kDisplayHeight);

	for (int i=0; i < 8; i++)
	{
		long paramID = kBandwidth;
		displayTextProcedure displayProc = bandwidthDisplayProc;
		if (i == 1)
		{
			paramID = kNumBands;
			displayProc = numBandsDisplayProc;
		}
		else if (i == 2)
		{
			paramID = (getparameter_i(kSepMode) == kSepMode_octaval) ? kSepAmount_octaval : kSepAmount_linear;
			displayProc = sepAmountDisplayProc;
		}
		else if (i == 3)
		{
			paramID = kAttack;
			displayProc = attackReleaseDisplayProc;
		}
		else if (i == 4)
		{
			paramID = kRelease;
			displayProc = attackReleaseDisplayProc;
		}
		else if (i == 5)
		{
			paramID = kVelInfluence;
			displayProc = velInfluenceDisplayProc;
		}
		else if (i == 6)
		{
			paramID = kVelCurve;
			displayProc = velCurveDisplayProc;
		}
		else if (i == 7)
		{
			paramID = kPitchbendRange;
			displayProc = pitchbendDisplayProc;
		}

		// slider control
		RezSynthSlider * slider = new RezSynthSlider(this, paramID, &pos, kDGSliderAxis_horizontal, gHorizontalSliderHandle, gHorizontalSliderBackground);
		if ( (paramID == kSepAmount_octaval) || (paramID == kSepAmount_linear) )
			sepAmountSlider = slider;

		// parameter name label
		DGStaticTextDisplay * label = new DGStaticTextDisplay(this, &pos2, NULL, kDGTextAlign_left, kValueTextFontSize, kMyVeryLightGreyColor, kValueTextFont);
		AUVParameter auvp(GetEditAudioUnit(), paramID, kAudioUnitScope_Global, (AudioUnitElement)0);
		// get the name of this parameter
		char paramName[DFX_PARAM_MAX_NAME_LENGTH];
		strcpy(paramName, auvp.ParamInfo().name);
		// check if it's a sep amount parameter and, if it is, truncate the "(blah)" qualifying part
		char * breakpoint = strrchr(paramName, '(');
		if (breakpoint != NULL)
			breakpoint[-1] = 0;
		label->setText(paramName);

		// value display
		DGTextDisplay * display = new DGTextDisplay(this, paramID, &pos3, displayProc, this, NULL, kDGTextAlign_right, kValueTextFontSize, kMyVeryLightGreyColor, kValueTextFont);
		if ( (paramID == kSepAmount_octaval) || (paramID == kSepAmount_linear) )
			sepAmountDisplay = display;

		pos.offset(0, kSliderInc);
		pos2.offset(0, kSliderInc);
		pos3.offset(0, kSliderInc);
	}

	//--initialize the vertical gain sliders---------------------------------
	pos.set(kTallSliderX, kTallSliderY, gVerticalSliderBackground->getWidth(), gVerticalSliderBackground->getHeight());
	pos2.set(kTallDisplayX, kTallDisplayY, kTallDisplayWidth, kTallDisplayHeight);
	pos3.set(kTallLabelX, kTallLabelY, kTallLabelWidth, kTallLabelHeight);
	for (int paramID=kGain; paramID <= kDryWetMix; paramID++)
	{
		displayTextProcedure displayProc = gainDisplayProc;
		char * label1 = "filtered";
		char * label2 = "gain";
		if (paramID == kBetweenGain)
			label1 = "between";
		else if (paramID == kDryWetMix)
		{
			label1 = "dry/wet";
			label2 = "mix";
			displayProc = dryWetMixDisplayProc;
		}
		// slider control
		DGSlider * slider = new DGSlider(this, paramID, &pos, kDGSliderAxis_vertical, gVerticalSliderHandle, gVerticalSliderBackground);

		// value display
		DGTextDisplay * display = new DGTextDisplay(this, paramID, &pos2, displayProc, NULL, gVerticalValueDisplayBackground, kDGTextAlign_center, kValueTextFontSize, kMyLightGreyColor, kValueTextFont);
//		display->setBackgroundColor(kMyGreyColor);
//		display->setFrameColor(kBlackColor);

		// parameter name label
		DGStaticTextDisplay * label;
		label = new DGStaticTextDisplay(this, &pos3, NULL, kDGTextAlign_center, kValueTextFontSize, kMyVeryLightGreyColor, kValueTextFont);
		label->setText(label1);
		pos3.offset(0, kTallLabelInc);
		label = new DGStaticTextDisplay(this, &pos3, NULL, kDGTextAlign_center, kValueTextFontSize, kMyVeryLightGreyColor, kValueTextFont);
		label->setText(label2);
		pos3.offset(0, -kTallLabelInc);

		pos.offset(kTallSliderInc, 0);
		pos2.offset(kTallSliderInc, 0);
		pos3.offset(kTallSliderInc, 0);
	}



	//--initialize the buttons----------------------------------------------
	DGButton * button;

	// input gain scaling
	pos.set(kScaleButtonX, kScaleButtonY, gScaleModeButton->getWidth(), (gScaleModeButton->getHeight())/kNumScaleModes);
	button = new DGButton(this, kScaleMode, &pos, gScaleModeButton, kNumScaleModes, kDGButtonType_radiobutton);
	// parameter name label
	pos.set(kScaleButtonX, kScaleButtonY - kDisplayHeight - 6, gScaleModeButton->getWidth(), kDisplayHeight);
	DGStaticTextDisplay * label = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_center, kValueTextFontSize, kMyVeryLightGreyColor, kValueTextFont);
	label->setText("input gain scaling");

	// separation mode (logarithmic or linear)
	pos.set(kSepModeButtonX, kSepModeButtonY, gSepModeButton->getWidth(), (gSepModeButton->getHeight())/kNumSepModes);
	button = new DGButton(this, kSepMode, &pos, gSepModeButton, kNumSepModes, kDGButtonType_incbutton);

	// legato
	pos.set(kLegatoButtonX, kLegatoButtonY, gLegatoButton->getWidth(), (gLegatoButton->getHeight())/2);
	button = new DGButton(this, kLegato, &pos, gLegatoButton, 2, kDGButtonType_incbutton);

	// attack & release fade mode
	pos.set(kFadesButtonX, kFadesButtonY, gFadesButton->getWidth(), (gFadesButton->getHeight())/2);
	button = new DGButton(this, kFades, &pos, gFadesButton, 2, kDGButtonType_incbutton);

	// allow Nyquist foldover or no
	pos.set(kFoldoverButtonX, kFoldoverButtonY, gFoldoverButton->getWidth(), (gFoldoverButton->getHeight())/2);
	button = new DGButton(this, kFoldover, &pos, gFoldoverButton, 2, kDGButtonType_incbutton);

	// wisely lower the output gain to accomodate for resonance or no
	pos.set(kWiseAmpButtonX, kWiseAmpButtonY, gWiseAmpButton->getWidth(), (gWiseAmpButton->getHeight())/2);
	button = new DGButton(this, kWiseAmp, &pos, gWiseAmpButton, 2, kDGButtonType_incbutton);

	// turn on/off MIDI learn mode for CC parameter automation
	pos.set(kMidiLearnButtonX, kMidiLearnButtonY, gMidiLearnButton->getWidth(), (gMidiLearnButton->getHeight())/2);
	button = new DGButton(this, &pos, gMidiLearnButton, 2, kDGButtonType_incbutton);
	button->setUserProcedure(midilearnRezSynth, this);

	// clear all MIDI CC assignments
	pos.set(kMidiResetButtonX, kMidiResetButtonY, gMidiResetButton->getWidth(), (gMidiResetButton->getHeight())/2);
	button = new DGButton(this, &pos, gMidiResetButton, 2, kDGButtonType_pushbutton);
	button->setUserProcedure(midiresetRezSynth, this);

	DGWebLink * weblink;

	// go!
	pos.set(kGoButtonX, kGoButtonY, gGoButton->getWidth(), (gGoButton->getHeight())/2);
	weblink = new DGWebLink(this, &pos, gGoButton, "http://www.streetharassmentproject.org/");

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, gDestroyFXlink->getWidth(), (gDestroyFXlink->getHeight())/2);
	weblink = new DGWebLink(this, &pos, gDestroyFXlink, DESTROYFX_URL);

	// Smart Electronix web page link
	pos.set(kSmartElectronixLinkX, kSmartElectronixLinkY, gSmartElectronixLink->getWidth(), (gSmartElectronixLink->getHeight())/2);
	weblink = new DGWebLink(this, &pos, gSmartElectronixLink, SMARTELECTRONIX_URL);



	// set up the parameter listeners
	sepModeAUP.mAudioUnit = GetEditAudioUnit();
	sepModeAUP.mScope = kAudioUnitScope_Global;
	sepModeAUP.mElement = (AudioUnitElement)0;
	sepModeAUP.mParameterID = kSepMode;

	AUListenerAddParameter(parameterListener, sepAmountSlider, &sepModeAUP);
	AUListenerAddParameter(parameterListener, sepAmountDisplay, &sepModeAUP);



	return noErr;
}
