#include "transverbeditor.h"
#include "transverb.hpp"

#include "dfxguislider.h"
#include "dfxguidisplay.h"


//-----------------------------------------------------------------------------
enum {
	// positions
	kWideFaderX = 20,
	kWideFaderY = 35,
	kWideFaderInc = 24,
	kWideFaderMoreInc = 42,
	kWideFaderEvenMoreInc = 50,
	kTallFaderX = kWideFaderX,
	kTallFaderY = 265,
	kTallFaderInc = 28,

	kQualityButtonX = kWideFaderX,
	kRandomButtonX = 185,
	kTomsoundButtonX = 425,
	kButtonY = 236,

	kFineDownButtonX = 503,
	kFineUpButtonX = 512,
	kFineButtonY = kWideFaderY,
	kSpeedModeButtonX = 503,
	kSpeedModeButtonY = 22,

	kMidiLearnButtonX = 237,
	kMidiLearnButtonY = 254,
	kMidiResetButtonX = 288,
	kMidiResetButtonY = 254,

	kDFXlinkX = 107,
	kDFXlinkY = 281,
	kSuperDFXlinkX = 159,
	kSuperDFXlinkY = 339,
	kSmartElectronixLinkX = 306,
	kSmartElectronixLinkY = kSuperDFXlinkY,

	kDisplayX = 318,
	kDisplayY = 24,
	kDisplayWidth = 180,
	kDisplayHeight = 10
};


const char * SNOOT_FONT = "snoot.org pixel10";
const DGColor kDisplayTextColor(103.0f/255.0f, 161.0f/255.0f, 215.0f/255.0f);
const float kDisplayTextSize = 14.0f;
const float kFineTuneInc = 0.0001f;



//-----------------------------------------------------------------------------
// callbacks for button-triggered action

void randomizeTransverb(long value, void * editor)
{
	if (editor != NULL)
		((DfxGuiEditor*)editor)->randomizeparameters(true);
}

void midilearnTransverb(long value, void * editor)
{
	if (editor != NULL)
	{
		if (value == 0)
			((DfxGuiEditor*)editor)->setmidilearning(false);
		else
			((DfxGuiEditor*)editor)->setmidilearning(true);
	}
}

void midiresetTransverb(long value, void * editor)
{
	if ( (editor != NULL) && (value != 0) )
		((DfxGuiEditor*)editor)->resetmidilearn();
}



//-----------------------------------------------------------------------------
// value text display procedures

void bsizeDisplayProcedure(float value, char * outText, void *)
{
	long thousands = (long)value / 1000;
	float remainder = fmodf(value, 1000.0f);

	if (thousands > 0)
		sprintf(outText, "%ld,%05.1f", thousands, remainder);
	else
		sprintf(outText, "%.1f", value);
	strcat(outText, " ms");
}

void speedDisplayProcedure(float value, char * outText, void *)
{
	char * semitonesString = (char*) malloc(16);
	float speed = value;
	float remainder = fmodf(fabsf(speed), 1.0f);
	float semitones = remainder * 12.0f;
	// make sure that these float crap doesn't result in wacky stuff 
	// like displays that say "-1 octave & 12.00 semitones"
	sprintf(semitonesString, "%.3f", semitones);
	if ( (strcmp(semitonesString, "12.000") == 0) || (strcmp(semitonesString, "-12.000") == 0) )
	{
		semitones = 0.0f;
		if (speed < 0.0f)
			speed -= 0.003f;
		else
			speed += 0.003f;
	}
	int octaves = (int) speed;

	if (speed > 0.0f)
	{
		if (octaves == 0)
			sprintf(outText, "+%.2f semitones", semitones);
		else if (octaves == 1)
			sprintf(outText, "+%d octave & %.2f semitones", octaves, semitones);
		else
			sprintf(outText, "+%d octaves & %.2f semitones", octaves, semitones);
	}
	else if (octaves == 0)
		sprintf(outText, "-%.2f semitones", semitones);
	else
	{
		if (octaves == -1)
			sprintf(outText, "%d octave & %.2f semitones", octaves, semitones);
		else
			sprintf(outText, "%d octaves & %.2f semitones", octaves, semitones);
	}

	if (semitonesString)
		free(semitonesString);
}

void feedbackDisplayProcedure(float value, char * outText, void *)
{
	sprintf(outText, "%ld%%", (long)value);
}

void distDisplayProcedure(float value, char * outText, void * editor)
{
	float distance = value;
	if (editor != NULL)
		distance = value * ((DfxGuiEditor*)editor)->getparameter_f(kBsize);
	long thousands = (long)distance / 1000;
	float remainder = fmodf(distance, 1000.0f);

	if (thousands > 0)
		sprintf(outText, "%ld,%06.2f", thousands, remainder);
	else
		sprintf(outText, "%.2f", distance);
	strcat(outText, " ms");
}

void valueDisplayProcedure(float value, char * outText, void * userData)
{
	if (outText != NULL)
		sprintf(outText, "%.2f", value);
}



//-----------------------------------------------------------------------------

static void SpeedModeListenerProc(void * inRefCon, void * inObject, const AudioUnitParameter * inParameter, Float32 inValue)
{
	if ( (inObject == NULL) || (inParameter == NULL) )
		return;

	TransverbSpeedTuneButton * button = (TransverbSpeedTuneButton*) inObject;
	button->setTuneMode( button->getDfxGuiEditor()->getparameter_i(inParameter->mParameterID) );
}

double nearestIntegerBelow(double number)
{
	bool sign = (number >= 0.0);
	double fraction = fmod(fabs(number), 1.0);

	if (fraction <= 0.0001)
		return number;

	if (sign)
		return (double) ((long)fabs(number));
	else
		return 0.0 - (double) ((long)fabs(number) + 1);
}

double nearestIntegerAbove(double number)
{
	bool sign = (number >= 0.0);
	double fraction = fmod(fabs(number), 1.0);

	if (fraction <= 0.0001)
		return number;

	if (sign)
		return (double) ((long)fabs(number) + 1);
	else
		return 0.0 - (double) ((long)fabs(number));
}

//-----------------------------------------------------------------------------
void TransverbSpeedTuneButton::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	if (tuneMode == kFineMode)
	{
		DGFineTuneButton::mouseDown(inXpos, inYpos, inMouseButtons, inKeyModifiers);
		return;
	}

	entryValue = GetControl32BitValue(carbonControl);

	double oldSpeedValue = getDfxGuiEditor()->getparameter_d( getParameterID() );
	double newSpeedValue;
	bool isInc = (valueChangeAmount >= 0.0f);
	double snapAmount = (isInc) ? 1.001 : -1.001;
	double snapScalar = (tuneMode == kSemitoneMode) ? 12.0 : 1.0;

	newSpeedValue = (oldSpeedValue * snapScalar) + snapAmount;
	newSpeedValue = isInc ? nearestIntegerBelow(newSpeedValue) : nearestIntegerAbove(newSpeedValue);
	newSpeedValue /= snapScalar;
	getAUVP().SetValue(getDfxGuiEditor()->getParameterListener(), getAUVcontrol(), newSpeedValue);

	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	getAUVcontrol()->ParameterToControl(newSpeedValue);
	newValue = GetControl32BitValue(carbonControl);
	if (newValue > max)
		newValue = max;
	if (newValue < min)
		newValue = min;

	mouseIsDown = true;
	if (newValue != entryValue)
		redraw();	// redraw with mouse down
}



//-----------------------------------------------------------------------------
COMPONENT_ENTRY(TransverbEditor);

//-----------------------------------------------------------------------------
TransverbEditor::TransverbEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
	speed1UpButton = NULL;
	speed1DownButton = NULL;
	speed2UpButton = NULL;
	speed2DownButton = NULL;

	parameterListener = NULL;
	OSStatus status = AUListenerCreate(SpeedModeListenerProc, this,
		CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.030f, // 30 ms
		&parameterListener);
	if (status != noErr)
		parameterListener = NULL;
}

//-----------------------------------------------------------------------------
TransverbEditor::~TransverbEditor()
{
	if (parameterListener != NULL)
	{
		if (speed1DownButton != NULL)
			AUListenerRemoveParameter(parameterListener, speed1DownButton, &speed1modeAUP);
		if (speed1UpButton != NULL)
			AUListenerRemoveParameter(parameterListener, speed1UpButton, &speed1modeAUP);
		if (speed2DownButton != NULL)
			AUListenerRemoveParameter(parameterListener, speed2DownButton, &speed2modeAUP);
		if (speed2UpButton != NULL)
			AUListenerRemoveParameter(parameterListener, speed2UpButton, &speed2modeAUP);

		AUListenerDispose(parameterListener);
	}
}

//-----------------------------------------------------------------------------
long TransverbEditor::open()
{
	// Background image
	DGImage * gBackground = new DGImage("transverb-background.png", this);
	SetBackgroundImage(gBackground);

	// slider handles
	DGImage * gHorizontalSliderHandle = new DGImage("purple-wide-fader-handle.png", this);
	DGImage * gGreyHorizontalSliderHandle = new DGImage("grey-wide-fader-handle.png", this);
	DGImage * gVerticalSliderHandle = new DGImage("tall-fader-handle.png", this);
	// slider backgrounds
	DGImage * gHorizontalSliderBackground = new DGImage("purple-wide-fader-slide.png", this);
	DGImage * gGreyHorizontalSliderBackground = new DGImage("grey-wide-fader-slide.png", this);
	DGImage * gVerticalSliderBackground = new DGImage("tall-fader-slide.png", this);
	// buttons
	DGImage * gQualityButton = new DGImage("quality-button.png", this);
	DGImage * gTomsoundButton = new DGImage("tomsound-button.png", this);
	DGImage * gRandomizeButton = new DGImage("randomize-button.png", this);
	DGImage * gFineDownButton = new DGImage("fine-down-button.png", this);
	DGImage * gFineUpButton = new DGImage("fine-up-button.png", this);
	DGImage * gSpeedModeButton = new DGImage("speed-mode-button.png", this);
	DGImage * gMidiLearnButton = new DGImage("midi-learn-button.png", this);
	DGImage * gMidiResetButton = new DGImage("midi-reset-button.png", this);
	DGImage * gDfxLinkButton = new DGImage("dfx-link.png", this);
	DGImage * gSuperDestroyFXlinkButton = new DGImage("super-destroy-fx-link.png", this);
	DGImage * gSmartElectronixLinkButton = new DGImage("smart-electronix-link.png", this);



	DGRect pos, pos2, pos3, pos4;

	// Make horizontal sliders and add them to the pane
	pos.set(kWideFaderX, kWideFaderY, gHorizontalSliderBackground->getWidth(), gHorizontalSliderBackground->getHeight());
	pos2.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	pos3.set(kFineDownButtonX, kFineButtonY, gFineDownButton->getWidth(), gFineDownButton->getHeight() / 2);
	pos4.set(kFineUpButtonX, kFineButtonY, gFineUpButton->getWidth(), gFineUpButton->getHeight() / 2);
	for (long tag=kSpeed1; tag <= kDist2; tag++)
	{
		displayTextProcedure displayProc;
		void * userData = NULL;
		if ( (tag == kSpeed1) || (tag == kSpeed2) )
			displayProc = speedDisplayProcedure;
		else if ( (tag == kFeed1) || (tag == kFeed2) )
			displayProc = feedbackDisplayProcedure;
		else
		{
			displayProc = distDisplayProcedure;
			userData = this;
		}
		DGSlider * slider = new DGSlider(this, tag, &pos, kDGSliderAxis_horizontal, gHorizontalSliderHandle, gHorizontalSliderBackground);
		slider->shrinkForeBounds(1, 0, 2, 0);

		DGTextDisplay * display = new DGTextDisplay(this, tag, &pos2, displayProc, userData, NULL, 
										kDGTextAlign_right, kDisplayTextSize, kDisplayTextColor, SNOOT_FONT);

		if (tag == kSpeed1)
		{
			long tuneMode = getparameter_i(kSpeed1mode);
			speed1DownButton = new TransverbSpeedTuneButton(this, tag, &pos3, gFineDownButton, -kFineTuneInc, tuneMode);
			speed1UpButton = new TransverbSpeedTuneButton(this, tag, &pos4, gFineUpButton, kFineTuneInc, tuneMode);
		}
		else if (tag == kSpeed2)
		{
			long tuneMode = getparameter_i(kSpeed2mode);
			speed2DownButton = new TransverbSpeedTuneButton(this, tag, &pos3, gFineDownButton, -kFineTuneInc, tuneMode);
			speed2UpButton = new TransverbSpeedTuneButton(this, tag, &pos4, gFineUpButton, kFineTuneInc, tuneMode);
		}
		else
		{
			DGFineTuneButton * button = new DGFineTuneButton(this, tag, &pos3, gFineDownButton, -kFineTuneInc);
			button = new DGFineTuneButton(this, tag, &pos4, gFineUpButton, kFineTuneInc);
		}

		long yoff =  kWideFaderInc;
		if (tag == kDist1)
			yoff = kWideFaderMoreInc;
		else if (tag == kDist2)
			yoff =  kWideFaderEvenMoreInc;
		pos.offset(0, yoff);
		pos2.offset(0, yoff);
		pos3.offset(0, yoff);
		pos4.offset(0, yoff);
	}

	DGSlider * bsizeSlider = new DGSlider(this, kBsize, &pos, kDGSliderAxis_horizontal, gGreyHorizontalSliderHandle, gGreyHorizontalSliderBackground);
	bsizeSlider->shrinkForeBounds(1, 0, 2, 0);

	DGTextDisplay * display = new DGTextDisplay(this, kBsize, &pos2, bsizeDisplayProcedure, NULL, NULL, 
										kDGTextAlign_right, kDisplayTextSize, kDisplayTextColor, SNOOT_FONT);

	DGFineTuneButton * fineTuneButton = new DGFineTuneButton(this, kBsize, &pos3, gFineDownButton, -kFineTuneInc);
	fineTuneButton = new DGFineTuneButton(this, kBsize, &pos4, gFineUpButton, kFineTuneInc);


	// Make horizontal sliders and add them to the pane
	pos.set(kTallFaderX, kTallFaderY, gVerticalSliderBackground->getWidth(), gVerticalSliderBackground->getHeight());
	for (long tag=kDrymix; tag <= kMix2; tag++)
	{
		DGSlider * slider = new DGSlider(this, tag, &pos, kDGSliderAxis_vertical, gVerticalSliderHandle, gVerticalSliderBackground);
		slider->shrinkForeBounds(0, 1, 0, 2);
		pos.offset(kTallFaderInc, 0);
	}


	DGButton * button;

	// quality mode button
	pos.set(kQualityButtonX, kButtonY, gQualityButton->getWidth()/2, gQualityButton->getHeight()/numQualities);
	button = new DGButton(this, kQuality, &pos, gQualityButton, numQualities, kDGButtonType_incbutton, true);

	// TOMSOUND button
	pos.set(kTomsoundButtonX, kButtonY, gTomsoundButton->getWidth()/2, gTomsoundButton->getHeight()/2);
	button = new DGButton(this, kTomsound, &pos, gTomsoundButton, 2, kDGButtonType_incbutton, true);

	// randomize button
	pos.set(kRandomButtonX, kButtonY, gRandomizeButton->getWidth(), gRandomizeButton->getHeight()/2);
	button = new DGButton(this, &pos, gRandomizeButton, 2, kDGButtonType_pushbutton);
	button->setUserProcedure(randomizeTransverb, this);

	// speed 1 mode button
	pos.set(kSpeedModeButtonX, kSpeedModeButtonY, gSpeedModeButton->getWidth()/2, gSpeedModeButton->getHeight()/numSpeedModes);
	button = new DGButton(this, kSpeed1mode, &pos, gSpeedModeButton, numSpeedModes, kDGButtonType_incbutton, true);
	//
	// speed 2 mode button
	pos.offset(0, (kWideFaderInc * 2) + kWideFaderMoreInc);
	button = new DGButton(this, kSpeed2mode, &pos, gSpeedModeButton, numSpeedModes, kDGButtonType_incbutton, true);

	// MIDI learn button
	pos.set(kMidiLearnButtonX, kMidiLearnButtonY, gMidiLearnButton->getWidth()/2, gMidiLearnButton->getHeight()/2);
	button = new DGButton(this, &pos, gMidiLearnButton, 2, kDGButtonType_incbutton);
	button->setUserProcedure(midilearnTransverb, this);

	// MIDI reset button
	pos.set(kMidiResetButtonX, kMidiResetButtonY, gMidiResetButton->getWidth(), gMidiResetButton->getHeight()/2);
	button = new DGButton(this, &pos, gMidiResetButton, 2, kDGButtonType_pushbutton);
	button->setUserProcedure(midiresetTransverb, this);

	// DFX web link
	pos.set(kDFXlinkX, kDFXlinkY, gDfxLinkButton->getWidth(), gDfxLinkButton->getHeight()/2);
	button = new DGWebLink(this, &pos, gDfxLinkButton, DESTROYFX_URL);

	// Super Destroy FX web link
	pos.set(kSuperDFXlinkX, kSuperDFXlinkY, gSuperDestroyFXlinkButton->getWidth(), gSuperDestroyFXlinkButton->getHeight()/2);
	button = new DGWebLink(this, &pos, gSuperDestroyFXlinkButton, DESTROYFX_URL);

	// Smart Electronix web link
	pos.set(kSmartElectronixLinkX, kSmartElectronixLinkY, gSmartElectronixLinkButton->getWidth(), gSmartElectronixLinkButton->getHeight()/2);
	button = new DGWebLink(this, &pos, gSmartElectronixLinkButton, SMARTELECTRONIX_URL);


	if (parameterListener != NULL)
	{
		speed1modeAUP.mAudioUnit = speed2modeAUP.mAudioUnit = GetEditAudioUnit();
		speed1modeAUP.mScope = speed2modeAUP.mScope = kAudioUnitScope_Global;
		speed1modeAUP.mElement = speed2modeAUP.mElement = (AudioUnitElement)0;
		speed1modeAUP.mParameterID = kSpeed1mode;
		speed2modeAUP.mParameterID = kSpeed2mode;

		AUListenerAddParameter(parameterListener, speed1DownButton, &speed1modeAUP);
		AUListenerAddParameter(parameterListener, speed1UpButton, &speed1modeAUP);
		AUListenerAddParameter(parameterListener, speed2DownButton, &speed2modeAUP);
		AUListenerAddParameter(parameterListener, speed2UpButton, &speed2modeAUP);
	}


	return noErr;
}
