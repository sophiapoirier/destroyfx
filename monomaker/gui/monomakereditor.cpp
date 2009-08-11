#include "monomakereditor.h"
#include "monomaker.h"


//-----------------------------------------------------------------------------
enum {
	kSliderX = 15,
	kSliderY = 81,
	kSliderInc = 61,
	kSliderWidth = 227,

	kDisplayX = 252,
	kDisplayY = 77,
	kDisplayWidth = 83,
	kDisplayHeight = 12,

	kMonomergeAnimationX = 14,
	kMonomergeAnimationY = 28,
	kPanAnimationX = 15,
	kPanAnimationY = 116,

	kButtonX = 21,
	kButtonY = 184,
	kButtonInc = 110,

	kDestroyFXlinkX = 270,
	kDestroyFXlinkY = 3
};

//const DGColor kBackgroundColor(64.0f/255.0f, 54.0f/255.0f, 40.0f/255.0f);
const DGColor kBackgroundColor(42.0f/255.0f, 34.0f/255.0f, 22.0f/255.0f);
const char * kValueTextFont = "Arial";
const float kValueTextSize = 11.0f;



//-----------------------------------------------------------------------------
// parameter value display text conversion functions

void monomergeDisplayProc(float inValue, char * outText, void *);
void monomergeDisplayProc(float inValue, char * outText, void *)
{
	sprintf(outText, " %.1f %%", inValue);
}

void panDisplayProc(float inValue, char * outText, void *);
void panDisplayProc(float inValue, char * outText, void *)
{
	if (inValue >= 0.5004f)
		sprintf(outText, " +%.3f", inValue);
	else
		sprintf(outText, " %.3f", inValue);
}



//____________________________________________________________________________
DFX_EDITOR_ENTRY(MonomakerEditor)

//-----------------------------------------------------------------------------
MonomakerEditor::MonomakerEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long MonomakerEditor::OpenEditor()
{
	//--load the images-------------------------------------

	// background image
	DGImage * backgroundImage = new DGImage("background.png", 0, this);
	SetBackgroundImage(backgroundImage);

	DGImage * sliderHandleImage = new DGImage("slider-handle.png", 0, this);
	DGImage * monomergeAnimationImage = new DGImage("monomerge-blobs.png", 0, this);
	DGImage * panAnimationImage = new DGImage("pan-blobs.png", 0, this);

	DGImage * inputSelectionButtonImage = new DGImage("input-selection-button.png", 0, this);
	DGImage * monomergeModeButtonImage = new DGImage("monomerge-mode-button.png", 0, this);
	DGImage * panModeButtonImage = new DGImage("pan-mode-button.png", 0, this);
	DGImage * destroyFXLinkImage = new DGImage("destroy-fx-link.png", 0, this);



	//--create the controls-------------------------------------
	DGRect pos;


	// --- sliders ---
	DGSlider * slider;
	DGAnimation * blobs;
	const long numAnimationFrames = 19;

	// monomerge slider
	pos.set(kSliderX, kSliderY, kSliderWidth, sliderHandleImage->getHeight());
	slider = new DGSlider(this, kMonomerge, &pos, kDGAxis_horizontal, sliderHandleImage, NULL);

	// pan slider
	pos.offset(0, kSliderInc);
	slider = new DGSlider(this, kPan, &pos, kDGAxis_horizontal, sliderHandleImage, NULL);

	// monomerge animation
	pos.set(kMonomergeAnimationX, kMonomergeAnimationY, monomergeAnimationImage->getWidth(), (monomergeAnimationImage->getHeight())/numAnimationFrames);
	blobs = new DGAnimation(this, kMonomerge, &pos, monomergeAnimationImage, numAnimationFrames);
	blobs->setMouseAxis(kDGAxis_horizontal);

	// pan animation
	pos.set(kPanAnimationX, kPanAnimationY, panAnimationImage->getWidth(), (panAnimationImage->getHeight())/numAnimationFrames);
	blobs = new DGAnimation(this, kPan, &pos, panAnimationImage, numAnimationFrames);
	blobs->setMouseAxis(kDGAxis_horizontal);


	// --- text displays ---
	DGTextDisplay * display;

	// mono merge
	pos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kMonomerge, &pos, monomergeDisplayProc, NULL, NULL, kDGTextAlign_center, 
								kValueTextSize, kDGColor_black, kValueTextFont);

	// pan
	pos.offset(0, kSliderInc - 1);
	display = new DGTextDisplay(this, kPan, &pos, panDisplayProc, NULL, NULL, kDGTextAlign_center, 
								kValueTextSize, kDGColor_black, kValueTextFont);


	// --- buttons ---
	DGButton * button;

	// input selection button
	pos.set(kButtonX, kButtonY, inputSelectionButtonImage->getWidth(), (inputSelectionButtonImage->getHeight())/kNumInputSelections);
	button = new DGButton(this, kInputSelection, &pos, inputSelectionButtonImage, kNumInputSelections, kDGButtonType_incbutton);

	// monomerge mode button
	pos.offset(kButtonInc, 0);
	pos.resize(monomergeModeButtonImage->getWidth(), (monomergeModeButtonImage->getHeight()) / kNumMonomergeModes);
	button = new DGButton(this, kMonomergeMode, &pos, monomergeModeButtonImage, kNumMonomergeModes, kDGButtonType_incbutton);

	// pan mode button
	pos.offset(kButtonInc, 0);
	pos.resize(panModeButtonImage->getWidth(), (panModeButtonImage->getHeight()) / kNumPanModes);
	button = new DGButton(this, kPanMode, &pos, panModeButtonImage, kNumPanModes, kDGButtonType_incbutton);

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, destroyFXLinkImage->getWidth(), destroyFXLinkImage->getHeight()/2);
	DGWebLink * dfxLinkButton = new DGWebLink(this, &pos, destroyFXLinkImage, DESTROYFX_URL);



	return noErr;
}
