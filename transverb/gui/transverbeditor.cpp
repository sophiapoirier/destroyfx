#include "transverbeditor.h"
#include "transverb.hpp"

#include "dfxguislider.h"
#include "dfxguibutton.h"
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
	kDisplayY = 23,
	kDisplayWidth = 180,
	kDisplayHeight = 10
};


const char * SNOOT_FONT = "snoot.org pixel10";
const DGColor kDisplayTextColor(103.0f/255.0f, 161.0f/255.0f, 215.0f/255.0f);
const float kDisplayTextSize = 14.0f;



//-----------------------------------------------------------------------------

// callback for button-triggered action
void randomizeTransverb(SInt32 value, void * editor);
void randomizeTransverb(SInt32 value, void * editor)
{
	if (editor != NULL)
		((DfxGuiEditor*)editor)->randomizeparameters(true);
}

void midilearnTransverb(SInt32 value, void * editor);
void midilearnTransverb(SInt32 value, void * editor)
{
	if (editor != NULL)
	{
		if (value == 0)
			((DfxGuiEditor*)editor)->setmidilearning(false);
		else
			((DfxGuiEditor*)editor)->setmidilearning(true);
	}
}

void midiresetTransverb(SInt32 value, void * editor);
void midiresetTransverb(SInt32 value, void * editor)
{
	if ( (editor != NULL) && (value != 0) )
		((DfxGuiEditor*)editor)->resetmidilearn();
}

void bsizeDisplayProcedure(Float32 value, char * outText, void *);
void bsizeDisplayProcedure(Float32 value, char * outText, void *)
{
	float buffersize = value;
	long thousands = (long)buffersize / 1000;
	float remainder = fmodf(buffersize, 1000.0f);

	if (thousands > 0)
	{
		if (remainder < 10)
			sprintf(outText, "%ld,00%.1f ms", thousands, remainder);
		else if (remainder < 100)
			sprintf(outText, "%ld,0%.1f ms", thousands, remainder);
		else
			sprintf(outText, "%ld,%.1f ms", thousands, remainder);
	}
	else
		sprintf(outText, "%.1f ms", buffersize);
}

void speedDisplayProcedure(Float32 value, char * outText, void *);
void speedDisplayProcedure(Float32 value, char * outText, void *)
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

void feedbackDisplayProcedure(Float32 value, char * outText, void *);
void feedbackDisplayProcedure(Float32 value, char * outText, void *)
{
	sprintf(outText, "%ld%%", (long)value);
}

void distDisplayProcedure(Float32 value, char * outText, void * editor);
void distDisplayProcedure(Float32 value, char * outText, void * editor)
{
	float distance = value;
	if (editor != NULL)
		distance = value * ((DfxGuiEditor*)editor)->getparameter_f(kBsize);
	long thousands = (long)distance / 1000;
	float remainder = fmodf(distance, 1000.0f);

	if (thousands > 0)
	{
		if (remainder < 10.0f)
			sprintf(outText, "%ld,00%.2f ms", thousands, remainder);
		else if (remainder < 100.0f)
			sprintf(outText, "%ld,0%.2f ms", thousands, remainder);
		else
			sprintf(outText, "%ld,%.2f ms", thousands, remainder);
	}
	else
		sprintf(outText, "%.2f ms", distance);
}

void valueDisplayProcedure(Float32 value, char * outText, void * userData);
void valueDisplayProcedure(Float32 value, char * outText, void * userData)
{
	if (outText != NULL)
		sprintf(outText, "%.2f", value);
}



// ____________________________________________________________________________
COMPONENT_ENTRY(TransverbEditor);

// ____________________________________________________________________________
TransverbEditor::TransverbEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
}

// ____________________________________________________________________________
TransverbEditor::~TransverbEditor()
{
}

// ____________________________________________________________________________
long TransverbEditor::open(float inXOffset, float inYOffset)
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
	DGImage * gMidiLearnButton = new DGImage("midi-learn-button.png", this);
	DGImage * gMidiResetButton = new DGImage("midi-reset-button.png", this);
	DGImage * gDfxLinkButton = new DGImage("dfx-link.png", this);
	DGImage * gSuperDestroyFXlinkButton = new DGImage("super-destroy-fx-link.png", this);
	DGImage * gSmartElectronixLinkButton = new DGImage("smart-electronix-link.png", this);



	DGRect pos, pos2;

	// Make horizontal sliders and add them to the pane
	pos.set(kWideFaderX, kWideFaderY, gHorizontalSliderBackground->getWidth(), gHorizontalSliderBackground->getHeight());
	pos2.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
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

		DGTextDisplay * display = new DGTextDisplay(this, tag, &pos2, displayProc, userData, NULL, SNOOT_FONT);
		display->setTextAlignment(kDGTextAlign_right);
		display->setFontSize(kDisplayTextSize);
		display->setFontColor(kDisplayTextColor);

		long yoff =  kWideFaderInc;
		if (tag == kDist1)
			yoff = kWideFaderMoreInc;
		else if (tag == kDist2)
			yoff =  kWideFaderEvenMoreInc;
		pos.offset(0, yoff);
		pos2.offset(0, yoff);
	}

	DGSlider * bsizeSlider = new DGSlider(this, kBsize, &pos, kDGSliderAxis_horizontal, gGreyHorizontalSliderHandle, gGreyHorizontalSliderBackground);
	bsizeSlider->shrinkForeBounds(1, 0, 2, 0);

	DGTextDisplay * display = new DGTextDisplay(this, kBsize, &pos2, bsizeDisplayProcedure, NULL, NULL, SNOOT_FONT);
	display->setTextAlignment(kDGTextAlign_right);
	display->setFontSize(kDisplayTextSize);
	display->setFontColor(kDisplayTextColor);

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


	return noErr;
}
