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
	kDisplayHeight = 10,

	kFineDownOffset = 3333,
	kFineUpOffset = 9999,
	kSpeed1FineDown = kSpeed1 + kFineDownOffset,
	kSpeed1FineUp = kSpeed1 + kFineUpOffset,
	kSpeed2FineDown = kSpeed2 + kFineDownOffset,
	kSpeed2FineUp = kSpeed2 + kFineUpOffset,

	kRandomButton = 333,

	kSpeedTextEditOffset = 300,
	kSpeed1TextEdit = kSpeed1 + kSpeedTextEditOffset,
	kSpeed2TextEdit = kSpeed2 + kSpeedTextEditOffset
};


#define SNOOT_FONT	"snoot.org pixel10"



//-----------------------------------------------------------------------------

// callback for button-triggered action
void randomizeTransverb(SInt32 value, void *editor);
void randomizeTransverb(SInt32 value, void *editor)
{
	if (editor != NULL)
		((DfxGuiEditor*)editor)->randomizeparameters(true);
}

void midilearnTransverb(SInt32 value, void *editor);
void midilearnTransverb(SInt32 value, void *editor)
{
	if (editor != NULL)
	{
		if (value == 0)
			((DfxGuiEditor*)editor)->setmidilearning(false);
		else
			((DfxGuiEditor*)editor)->setmidilearning(true);
	}
}

void midiresetTransverb(SInt32 value, void *editor);
void midiresetTransverb(SInt32 value, void *editor)
{
	if ( (editor != NULL) && (value != 0) )
		((DfxGuiEditor*)editor)->resetmidilearn();
}

void bsizeDisplayProcedure(Float32 value, char *outText, void*);
void bsizeDisplayProcedure(Float32 value, char *outText, void*)
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

void speedDisplayProcedure(Float32 value, char *outText, void*);
void speedDisplayProcedure(Float32 value, char *outText, void*)
{
	char *semitonesString = (char*) malloc(16);
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

void feedbackDisplayProcedure(Float32 value, char *outText, void*);
void feedbackDisplayProcedure(Float32 value, char *outText, void*)
{
	sprintf(outText, "%ld%%", (long)value);
}

void distDisplayProcedure(Float32 value, char *outText, void *editor);
void distDisplayProcedure(Float32 value, char *outText, void *editor)
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

void valueDisplayProcedure(Float32 value, char *outText, void *userData);
void valueDisplayProcedure(Float32 value, char *outText, void *userData)
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
//printf("creating TransverbEditor\n");
}

// ____________________________________________________________________________
TransverbEditor::~TransverbEditor()
{
//printf("destroying TransverbEditor\n");
}

// ____________________________________________________________________________
OSStatus TransverbEditor::open(float inXOffset, float inYOffset)
{
printf("\n\n--------------------------------------------------\n");
printf("       creating Transverb GUI\n");
printf("--------------------------------------------------\n\n");

	// Background image
	DGGraphic *gBackground = new DGGraphic("transverb-background.png");
	addImage(gBackground);
	SetBackgroundImage(gBackground);

	// these move across the drawing rectangle
	DGGraphic *gHorizontalSliderHandle = new DGGraphic("purple-wide-fader-handle.png");
	addImage(gHorizontalSliderHandle);

	DGGraphic *gGreyHorizontalSliderHandle = new DGGraphic("grey-wide-fader-handle.png");
	addImage(gGreyHorizontalSliderHandle);

	DGGraphic *gVerticalSliderHandle = new DGGraphic("tall-fader-handle.png");
	addImage(gVerticalSliderHandle);

	// Backgrounds for Controls
	DGGraphic *gHorizontalSliderBackground = new DGGraphic("purple-wide-fader-slide.png");
	addImage(gHorizontalSliderBackground);

	DGGraphic *gGreyHorizontalSliderBackground = new DGGraphic("grey-wide-fader-slide.png");
	addImage(gGreyHorizontalSliderBackground);
	
	DGGraphic *gVerticalSliderBackground = new DGGraphic("tall-fader-slide.png");
	addImage(gVerticalSliderBackground);
	
	// buttons
	DGGraphic *gQualityButton = new DGGraphic("quality-button.png");
	addImage(gQualityButton);

	DGGraphic *gTomsoundButton = new DGGraphic("tomsound-button.png");
	addImage(gTomsoundButton);

	DGGraphic *gRandomizeButton = new DGGraphic("randomize-button.png");
	addImage(gRandomizeButton);

	DGGraphic *gMidiLearnButton = new DGGraphic("midi-learn-button.png");
	addImage(gMidiLearnButton);

	DGGraphic *gMidiResetButton = new DGGraphic("midi-reset-button.png");
	addImage(gMidiResetButton);

	DGGraphic *gDfxLinkButton = new DGGraphic("dfx-link.png");
	addImage(gDfxLinkButton);

	DGGraphic *gSuperDestroyFXlinkButton = new DGGraphic("super-destroy-fx-link.png");
	addImage(gSuperDestroyFXlinkButton);

	DGGraphic *gSmartElectronixLinkButton = new DGGraphic("smart-electronix-link.png");
	addImage(gSmartElectronixLinkButton);


/***************************************
	create controls
***************************************/

	DGRect where, where2;

	// Make horizontal sliders and add them to the pane
	where.set (kWideFaderX, kWideFaderY, gHorizontalSliderBackground->getWidth(), gHorizontalSliderBackground->getHeight());
	where2.set (kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	for (AudioUnitParameterID tag=kSpeed1; tag <= kDist2; tag++)
	{
		displayTextProcedure displayProc;
		void *userData = NULL;
		if ( (tag == kSpeed1) || (tag == kSpeed2) )
			displayProc = speedDisplayProcedure;
		else if ( (tag == kFeed1) || (tag == kFeed2) )
			displayProc = feedbackDisplayProcedure;
		else
		{
			displayProc = distDisplayProcedure;
			userData = this;
		}
		DGSlider *slider = new DGSlider(this, tag, &where, kDGSliderStyle_horizontal, gHorizontalSliderHandle, gHorizontalSliderBackground);
		slider->shrinkForeBounds(1, 0, 2, 0);
		addControl(slider);

		DGTextDisplay *display = new DGTextDisplay(this, tag, &where2, displayProc, userData, NULL, SNOOT_FONT);
		display->setTextAlignmentStyle(kDGTextAlign_right);
		addControl(display);

		long yoff =  kWideFaderInc;
		if (tag == kDist1)
			yoff = kWideFaderMoreInc;
		else if (tag == kDist2)
			yoff =  kWideFaderEvenMoreInc;
		where.offset (0, yoff);
		where2.offset (0, yoff);
	}

	DGSlider *bsizeSlider = new DGSlider(this, kBsize, &where, kDGSliderStyle_horizontal, gGreyHorizontalSliderHandle, gGreyHorizontalSliderBackground);
	bsizeSlider->shrinkForeBounds(1, 0, 2, 0);
	addControl(bsizeSlider);

	DGTextDisplay *display = new DGTextDisplay(this, kBsize, &where2, bsizeDisplayProcedure, NULL, NULL, SNOOT_FONT);
	display->setTextAlignmentStyle(kDGTextAlign_right);
	addControl(display);

	// Make horizontal sliders and add them to the pane
	where.set (kTallFaderX, kTallFaderY, gVerticalSliderBackground->getWidth(), gVerticalSliderBackground->getHeight());
	for (AudioUnitParameterID tag=kDrymix; tag <= kMix2; tag++)
	{
		DGSlider *slider = new DGSlider(this, tag, &where, kDGSliderStyle_vertical, gVerticalSliderHandle, gVerticalSliderBackground);
		slider->shrinkForeBounds(0, 1, 0, 2);
		addControl(slider);
		where.offset (kTallFaderInc, 0);
	}

	// quality mode button
	where.set (kQualityButtonX, kButtonY, gQualityButton->getWidth()/2, gQualityButton->getHeight()/numQualities);
	DGButton *qualityButton = new DGButton(this, kQuality, &where, gQualityButton, numQualities, kDGButtonType_incbutton, true);
	addControl(qualityButton);

	// TOMSOUND button
	where.set (kTomsoundButtonX, kButtonY, gTomsoundButton->getWidth()/2, gTomsoundButton->getHeight()/2);
	DGButton *tomsoundButton = new DGButton(this, kTomsound, &where, gTomsoundButton, 2, kDGButtonType_incbutton, true);
	addControl(tomsoundButton);

	// randomize button
	where.set (kRandomButtonX, kButtonY, gRandomizeButton->getWidth(), gRandomizeButton->getHeight()/2);
	DGButton *randomizeButton = new DGButton(this, &where, gRandomizeButton, 2, kDGButtonType_pushbutton);
	randomizeButton->setUserProcedure(randomizeTransverb, this);
	addControl(randomizeButton);

	// MIDI learn button
	where.set (kMidiLearnButtonX, kMidiLearnButtonY, gMidiLearnButton->getWidth()/2, gMidiLearnButton->getHeight()/2);
	DGButton *midilearnButton = new DGButton(this, &where, gMidiLearnButton, 2, kDGButtonType_incbutton);
	midilearnButton->setUserProcedure(midilearnTransverb, this);
	addControl(midilearnButton);

	// MIDI reset button
	where.set (kMidiResetButtonX, kMidiResetButtonY, gMidiResetButton->getWidth(), gMidiResetButton->getHeight()/2);
	DGButton *midiresetButton = new DGButton(this, &where, gMidiResetButton, 2, kDGButtonType_pushbutton);
	midiresetButton->setUserProcedure(midiresetTransverb, this);
	addControl(midiresetButton);

	// DFX web link
	where.set (kDFXlinkX, kDFXlinkY, gDfxLinkButton->getWidth(), gDfxLinkButton->getHeight()/2);
	DGWebLink *dfxlinkButton = new DGWebLink(this, &where, gDfxLinkButton, DESTROYFX_URL);
	addControl(dfxlinkButton);

	// Super Destroy FX web link
	where.set (kSuperDFXlinkX, kSuperDFXlinkY, gSuperDestroyFXlinkButton->getWidth(), gSuperDestroyFXlinkButton->getHeight()/2);
	DGWebLink *superdestroyfxlinkButton = new DGWebLink(this, &where, gSuperDestroyFXlinkButton, DESTROYFX_URL);
	addControl(superdestroyfxlinkButton);

	// Smart Electronix web link
	where.set (kSmartElectronixLinkX, kSmartElectronixLinkY, gSmartElectronixLinkButton->getWidth(), gSmartElectronixLinkButton->getHeight()/2);
	DGWebLink *smartelectronixlinkButton = new DGWebLink(this, &where, gSmartElectronixLinkButton, SMARTELECTRONIX_URL);
	addControl(smartelectronixlinkButton);


	return noErr;
}
