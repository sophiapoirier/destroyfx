/*

CFBundleRef CFBundleGetBundleWithIdentifier(CFStringRef bundleID);
CFURLRef CFBundleCopyResourceURL(CFBundleRef bundle, CFStringRef resourceName, CFStringRef resourceType, CFStringRef subDirName);


// file:///coriander/Developer/Documentation/Carbon/text/ATSTypes/atstypes.html

OSStatus 
ATSFontActivateFromFileSpecification(
const FSSpec *         iFile,
ATSFontContext         iContext,	// kATSFontContextUnspecified, kATSFontContextGlobal, kATSFontContextLocal
ATSFontFormat          iFormat,		// kATSFontFormatUnspecified
void *                 iReserved,
ATSOptionFlags         iOptions,	// undefined (?) UInt32
ATSFontContainerRef *  oContainer)	// UInt32

OSStatus 
ATSFontActivateFromMemory(
LogicalAddress         iData,
ByteCount              iLength,
ATSFontContext         iContext,
ATSFontFormat          iFormat,
void *                 iReserved,
ATSOptionFlags         iOptions,
ATSFontContainerRef *  oContainer)

OSStatus 
ATSFontDeactivate(
ATSFontContainerRef   iContainer,
void *                iRefCon,
ATSOptionFlags        iOptions)



// file:///coriander/Developer/Documentation/Carbon/text/FontManager/fontmanager.html

// file:///coriander/Developer/Documentation/Carbon/text/FontManager/Managing_FontManager/fm_tasks/_Activating_ating_Fonts.html
// file:///coriander/Developer/Documentation/Carbon/text/FontManager/Font_Manager/fmrefchapter/_Activating_ating_Fonts.html

OSStatus FMActivateFonts (
    const FSSpec *iFontContainer, 
    const FMFilter *iFilter, 
    void * iRefCon, 
    OptionBits iOptions
);

OSStatus FMDeactivateFonts (
    const FSSpec *iFontContainer, 
    const FMFilter *iFilter, 
    void * iRefCon, 
    OptionBits iOptions
);

*/



#include "transverbeditor.h"
#include "transverb.hpp"

#include "dfxguipane.h"
#include "dfxguislider.h"
#include "dfxguibutton.h"
#include "dfxguidisplay.h"


#define SNOOT_FONT	"snoot.org pixel10"


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



//-----------------------------------------------------------------------------

// callback for button-triggered action
void randomizeTransverb(UInt32 value, void *editor);
void randomizeTransverb(UInt32 value, void *editor)
{
	if (editor != NULL)
		((DfxGuiEditor*)editor)->randomizeparameters(true);
}

void midilearnTransverb(UInt32 value, void *editor);
void midilearnTransverb(UInt32 value, void *editor)
{
	if (editor != NULL)
	{
		if (value == 0)
			((DfxGuiEditor*)editor)->setmidilearning(false);
		else
			((DfxGuiEditor*)editor)->setmidilearning(true);
	}
}

void midiresetTransverb(UInt32 value, void *editor);
void midiresetTransverb(UInt32 value, void *editor)
{
	if ( (editor != NULL) && (value != 0) )
		((DfxGuiEditor*)editor)->resetmidilearn();
}

void openTransverbURL(UInt32 value, void *urlstring);
void openTransverbURL(UInt32 value, void *urlstring)
{
	if ( (value != 0) && (urlstring != NULL) )
		launch_url((char*)urlstring);
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
OSStatus TransverbEditor::open(Float32 inXOffset, Float32 inYOffset)
{

/***************************************
	create graphics objects from PNG resource files
***************************************/

	// Background image
	DGGraphic *backG = new DGGraphic("transverb-background.png");
	addImage(backG);

	// these move across the drawing rectangle
	DGGraphic *horizontal_handle = new DGGraphic("purple-wide-fader-handle.png");
	addImage(horizontal_handle);

	DGGraphic *grey_horizontal_handle = new DGGraphic("grey-wide-fader-handle.png");
	addImage(grey_horizontal_handle);

	DGGraphic *vertical_handle = new DGGraphic("tall-fader-handle.png");
	addImage(vertical_handle);

	// Backgrounds for Controls
	DGGraphic *h_slider_back = new DGGraphic("purple-wide-fader-slide.png");
	addImage(h_slider_back);

	DGGraphic *grey_h_slider_back = new DGGraphic("grey-wide-fader-slide.png");
	addImage(grey_h_slider_back);
	
	DGGraphic *v_slider_back = new DGGraphic("tall-fader-slide.png");
	addImage(v_slider_back);
	
	// buttons
	DGGraphic *quality_button = new DGGraphic("quality-button.png");
	addImage(quality_button);

	DGGraphic *tomsound_button = new DGGraphic("tomsound-button.png");
	addImage(tomsound_button);

	DGGraphic *randomize_button = new DGGraphic("randomize-button.png");
	addImage(randomize_button);

	DGGraphic *midilearn_button = new DGGraphic("midi-learn-button.png");
	addImage(midilearn_button);

	DGGraphic *midireset_button = new DGGraphic("midi-reset-button.png");
	addImage(midireset_button);

	DGGraphic *dfxlink_button = new DGGraphic("dfx-link.png");
	addImage(dfxlink_button);

	DGGraphic *superdestroyfxlink_button = new DGGraphic("super-destroy-fx-link.png");
	addImage(superdestroyfxlink_button);

	DGGraphic *smartelectronixlink_button = new DGGraphic("smart-electronix-link.png");
	addImage(smartelectronixlink_button);


/***************************************
	create controls
***************************************/
	
	// Get size of GUI from Background Image and create background pane
	SInt16 width = backG->getWidth();
	SInt16 height = backG->getHeight();
	DGRect where, where2;
	where.set (0, 0, width, height);
	DGPane *myPane = new DGPane(this, &where, backG);
	addControl(myPane);

/*
	// Build layered Pane with 2 different Control sets
	where.align (k_NEo);
	where.offset (8, -12);
	where.size (pane_back1->getWidth(), pane_back1->getHeight());
	DGLayeredPane *myLayeredPane = new DGLayeredPane(this, &where, pane_back1);
	myPane->addControl(myLayeredPane);
	myLayeredPane->addBackground(pane_back2, 1);
*/

	// Make horizontal sliders and add them to the pane
	where.set (kWideFaderX, kWideFaderY, h_slider_back->getWidth(), h_slider_back->getHeight());	// always relative to embedding pane
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
		DGSlider *slider = new DGSlider(this, tag, &where, kHorizontalSlider, horizontal_handle, h_slider_back);
		slider->shrinkForeBounds(1, 0, 2, 0);
		myPane->addControl(slider);

		DGTextDisplay *display = new DGTextDisplay(this, tag, &where2, displayProc, userData, backG, SNOOT_FONT);
		myPane->addControl(display);

		long yoff;
		if (tag == kDist1)
			yoff = kWideFaderMoreInc;
		else if (tag == kDist2)
			yoff =  kWideFaderEvenMoreInc;
		else
			yoff =  kWideFaderInc;
		where.offset (0, yoff);
		where2.offset (0, yoff);
	}

	DGSlider *bsizeSlider = new DGSlider(this, kBsize, &where, kHorizontalSlider, grey_horizontal_handle, grey_h_slider_back);
	bsizeSlider->shrinkForeBounds(1, 0, 2, 0);
	myPane->addControl(bsizeSlider);

	DGTextDisplay *display = new DGTextDisplay(this, kBsize, &where2, bsizeDisplayProcedure, NULL, backG, SNOOT_FONT);
	myPane->addControl(display);

	// Make horizontal sliders and add them to the pane
	where.set (kTallFaderX, kTallFaderY, v_slider_back->getWidth(), v_slider_back->getHeight());	// always relative to embedding pane
	for (AudioUnitParameterID tag=kDrymix; tag <= kMix2; tag++)
	{
		DGSlider *slider = new DGSlider(this, tag, &where, kVerticalSlider, vertical_handle, v_slider_back);
		slider->shrinkForeBounds(0, 1, 0, 2);
		myPane->addControl(slider);
		where.offset (kTallFaderInc, 0);
	}

	// quality mode button
	where.set (kQualityButtonX, kButtonY, quality_button->getWidth()/2, quality_button->getHeight()/numQualities);
	DGButton *qualityButton = new DGButton(this, kQuality, &where, quality_button, NULL, numQualities, kIncButton, true);
	myPane->addControl(qualityButton);

	// TOMSOUND button
	where.set (kTomsoundButtonX, kButtonY, tomsound_button->getWidth()/2, tomsound_button->getHeight()/2);
	DGButton *tomsoundButton = new DGButton(this, kTomsound, &where, tomsound_button, NULL, 2, kIncButton, true);
	myPane->addControl(tomsoundButton);

	// randomize button
	where.set (kRandomButtonX, kButtonY, randomize_button->getWidth(), randomize_button->getHeight()/2);
	DGButton *randomizeButton = new DGButton(this, &where, randomize_button, NULL, 2, kPushButton);
	randomizeButton->setUserProcedure(randomizeTransverb, this);
	myPane->addControl(randomizeButton);

	// MIDI learn button
	where.set (kMidiLearnButtonX, kMidiLearnButtonY, midilearn_button->getWidth()/2, midilearn_button->getHeight()/2);
	DGButton *midilearnButton = new DGButton(this, &where, midilearn_button, NULL, 2, kIncButton);
	midilearnButton->setUserProcedure(midilearnTransverb, this);
	myPane->addControl(midilearnButton);

	// MIDI reset button
	where.set (kMidiResetButtonX, kMidiResetButtonY, midireset_button->getWidth(), midireset_button->getHeight()/2);
	DGButton *midiresetButton = new DGButton(this, &where, midireset_button, NULL, 2, kPushButton);
	midiresetButton->setUserProcedure(midiresetTransverb, this);
	myPane->addControl(midiresetButton);

	// DFX web link
	where.set (kDFXlinkX, kDFXlinkY, dfxlink_button->getWidth(), dfxlink_button->getHeight()/2);
	DGButton *dfxlinkButton = new DGButton(this, &where, dfxlink_button, NULL, 2, kPushButton);
	dfxlinkButton->setUserProcedure(openTransverbURL, (void*)DESTROYFX_URL);
	myPane->addControl(dfxlinkButton);

	// Super Destroy FX web link
	where.set (kSuperDFXlinkX, kSuperDFXlinkY, superdestroyfxlink_button->getWidth(), superdestroyfxlink_button->getHeight()/2);
	DGButton *superdestroyfxlinkButton = new DGButton(this, &where, superdestroyfxlink_button, NULL, 2, kPushButton);
	superdestroyfxlinkButton->setUserProcedure(openTransverbURL, (void*)DESTROYFX_URL);
	myPane->addControl(superdestroyfxlinkButton);

	// Smart Electronix web link
	where.set (kSmartElectronixLinkX, kSmartElectronixLinkY, smartelectronixlink_button->getWidth(), smartelectronixlink_button->getHeight()/2);
	DGButton *smartelectronixlinkButton = new DGButton(this, &where, smartelectronixlink_button, NULL, 2, kPushButton);
	smartelectronixlinkButton->setUserProcedure(openTransverbURL, (void*)SMARTELECTRONIX_URL);
	myPane->addControl(smartelectronixlinkButton);


	// set size of overall pane
//	SizeControl(mCarbonPane, width, height);	// not necessary because of EmbedControl done on pane, right?

	return noErr;
}
