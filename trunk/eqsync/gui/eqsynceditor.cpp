#ifndef __EQSYNCEDITOR_H
#include "eqsynceditor.hpp"
#endif

#ifndef __EQSYNC_H
#include "eqsync.hpp"
#endif

#include <stdio.h>

// this is all for the internet stuff
#if MAC
	#ifndef __InternetConfig
	#include <InternetConfig.h>
	#endif
	#ifndef __Gestalt
	#include <Gestalt.h>
	#endif
	#ifndef __CodeFragments
	#include <CodeFragments.h>
	#endif
#endif
#if WIN32
	#ifndef __shlobj
	#include <shlobj.h>
	#endif
	#ifndef __shellapi
	#include <shellapi.h>
	#endif
#endif


//-----------------------------------------------------------------------------
enum {
	// resource IDs
	kBackgroundID = 128,
	kFaderSlideID,
	kFaderHandleID,
	kTallFaderSlideID,
	kTallFaderHandleID,
	kDestroyFXlinkID,

	// positions
	kWideFaderX = 138,
	kWideFaderY = 60,
	kWideFaderInc = 8 * 5,

	kTallFaderX = 134,
	kTallFaderY = 193,
	kTallFaderInc = 8 * 6,

	kDisplayX = 348,
	kDisplayY = kWideFaderY + 2,
	kDisplayWidth = 81,
	kDisplayHeight = 12,
	
	kDestroyFXlinkX = 158,
	kDestroyFXlinkY = 11,

	kTempoTextEdit = 333
};



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void tempoRateDisplayConvert(float value, char *string, void *temporate);
void tempoRateDisplayConvert(float value, char *string, void *temporate)
{
	if (temporate != NULL)
		strcpy(string, (char*)temporate);
}

void smoothDisplayConvert(float value, char *string);
void smoothDisplayConvert(float value, char *string)
{
	sprintf(string, "%.1f %%", (value*100.0f));
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
EQsyncEditor::EQsyncEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) 
{
	frame = 0;

	// initialize the graphics pointers
	gBackground = 0;
	gFaderSlide = 0;
	gFaderHandle = 0;
	gTallFaderSlide = 0;
	gTallFaderHandle = 0;
	gDestroyFXlink = 0;

	// initialize the controls pointers
	tempoRateFader = 0;
	tempoFader = 0;
	tempoTextEdit = 0;
	smoothFader = 0;
	a0Fader = 0;
	a1Fader = 0;
	a2Fader = 0;
	b1Fader = 0;
	b2Fader = 0;
	DestroyFXlink = 0;

	// initialize the value display box pointers
	tempoRateDisplay = 0;
	smoothDisplay = 0;

	// load the background bitmap
	// we don't need to load all bitmaps, this could be done when open is called
	gBackground = new CBitmap (kBackgroundID);

	// init the size of the plugin
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = (short)gBackground->getWidth();
	rect.bottom = (short)gBackground->getHeight();

	tempoString = new char[256];
	tempoRateString = new char[16];
}

//-----------------------------------------------------------------------------
EQsyncEditor::~EQsyncEditor()
{
	// free background bitmap
	if (gBackground)
		gBackground->forget();
	gBackground = 0;

	if (tempoString)
		delete[] tempoString;
	if (tempoRateString)
		delete[] tempoRateString;
}

//-----------------------------------------------------------------------------
long EQsyncEditor::getRect(ERect **erect)
{
	*erect = &rect;
	return true;
}

//-----------------------------------------------------------------------------
long EQsyncEditor::open(void *ptr)
{
  CPoint displayOffset;	// for positioning the background graphic behind display boxes


	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// load some bitmaps
	if (!gFaderSlide)
		gFaderSlide = new CBitmap (kFaderSlideID);
	if (!gFaderHandle)
		gFaderHandle = new CBitmap (kFaderHandleID);

	if (!gTallFaderSlide)
		gTallFaderSlide = new CBitmap (kTallFaderSlideID);
	if (!gTallFaderHandle)
		gTallFaderHandle = new CBitmap (kTallFaderHandleID);

	if (!gDestroyFXlink)
		gDestroyFXlink = new CBitmap (kDestroyFXlinkID);


	//--initialize the background frame--------------------------------------
	CRect size (0, 0, gBackground->getWidth(), gBackground->getHeight());
	frame = new CFrame (size, ptr, this);
	frame->setBackground(gBackground);


	//--initialize the fader------------------------------------------------
	int minPos = kWideFaderX + 1;
	int maxPos = kWideFaderX + gFaderSlide->getWidth() - gFaderHandle->getWidth();
	CPoint point (0, 0);
	CPoint offset (0, 2);

	// tempo rate (cycles per beat)
	// size stores left, top, right, & bottom positions
	size (kWideFaderX, kWideFaderY, kWideFaderX + gFaderSlide->getWidth(), kWideFaderY + gFaderSlide->getHeight());
	tempoRateFader = new CHorizontalSlider (size, this, kRate_sync, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	tempoRateFader->setOffsetHandle(point);
	tempoRateFader->setValue(effect->getParameter(kRate_sync));
	tempoRateFader->setDefaultValue(0.333f);
	frame->addView(tempoRateFader);

	// tempo (in bpm)
	size.offset (0, kWideFaderInc);
	tempoFader = new CHorizontalSlider (size, this, kTempo, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	tempoFader->setOffsetHandle(point);
	tempoFader->setValue(effect->getParameter(kTempo));
	tempoFader->setDefaultValue(0.0f);
	frame->addView(tempoFader);

	// smoothing amount
	size.offset (0, kWideFaderInc);
	smoothFader = new CHorizontalSlider (size, this, kSmooth, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	smoothFader->setOffsetHandle(point);
	smoothFader->setValue(effect->getParameter(kSmooth));
	smoothFader->setDefaultValue(1.0f/3.0f);
	frame->addView(smoothFader);

	int minTallPos = kTallFaderY + 1;
	int maxTallPos = kTallFaderY + gTallFaderSlide->getHeight() - gTallFaderHandle->getHeight();

	// coefficient a0
	size (kTallFaderX, kTallFaderY, kTallFaderX + gTallFaderSlide->getWidth(), kTallFaderY + gTallFaderSlide->getHeight());
	a0Fader = new CVerticalSlider (size, this, ka0, minTallPos, maxTallPos, gTallFaderHandle, gTallFaderSlide, point, kBottom);
	a0Fader->setOffsetHandle(point);
	a0Fader->setValue(effect->getParameter(ka0));
	frame->addView(a0Fader);

	// coefficient a1
	size.offset (kTallFaderInc, 0);
	a1Fader = new CVerticalSlider (size, this, ka1, minTallPos, maxTallPos, gTallFaderHandle, gTallFaderSlide, point, kBottom);
	a1Fader->setOffsetHandle(point);
	a1Fader->setValue(effect->getParameter(ka1));
	frame->addView(a1Fader);

	// coefficient a2
	size.offset (kTallFaderInc, 0);
	a2Fader = new CVerticalSlider (size, this, ka2, minTallPos, maxTallPos, gTallFaderHandle, gTallFaderSlide, point, kBottom);
	a2Fader->setOffsetHandle(point);
	a2Fader->setValue(effect->getParameter(ka2));
	frame->addView(a2Fader);

	// coefficient b1
	size.offset (kTallFaderInc, 0);
	b1Fader = new CVerticalSlider (size, this, kb1, minTallPos, maxTallPos, gTallFaderHandle, gTallFaderSlide, point, kBottom);
	b1Fader->setOffsetHandle(point);
	b1Fader->setValue(effect->getParameter(kb1));
	frame->addView(b1Fader);

	// coefficient b2
	size.offset (kTallFaderInc, 0);
	b2Fader = new CVerticalSlider (size, this, kb2, minTallPos, maxTallPos, gTallFaderHandle, gTallFaderSlide, point, kBottom);
	b2Fader->setOffsetHandle(point);
	b2Fader->setValue(effect->getParameter(kb2));
	frame->addView(b2Fader);

	// Destroy FX web page link
	size (kDestroyFXlinkX, kDestroyFXlinkY, kDestroyFXlinkX + gDestroyFXlink->getWidth(), kDestroyFXlinkY + (gDestroyFXlink->getHeight())/2);
	DestroyFXlink = new CHorizontalSwitch (size, this, kDestroyFXlinkID, 3, (gDestroyFXlink->getHeight())/2, 2, gDestroyFXlink, point);
	DestroyFXlink->setValue(0.0f);
	frame->addView(DestroyFXlink);

	//--initialize the displays---------------------------------------------

	strcpy( tempoRateString, ((EQsync*)effect)->tempoRateTable->getDisplay(((DfxPlugin*)effect)->getparameter_i(kRate_sync)) );

	// tempo rate (cycles per beat)
	size (kDisplayX, kDisplayY, kDisplayX + kDisplayWidth, kDisplayY + kDisplayHeight);
	tempoRateDisplay = new CParamDisplay (size, gBackground);
	displayOffset (kDisplayX, kDisplayY);
	tempoRateDisplay->setBackOffset(displayOffset);
	tempoRateDisplay->setHoriAlign(kLeftText);
	tempoRateDisplay->setFont(kNormalFont);
	tempoRateDisplay->setFontColor(kBlackCColor);
	tempoRateDisplay->setValue(effect->getParameter(kRate_sync));
	tempoRateDisplay->setStringConvert( tempoRateDisplayConvert, tempoRateString );
	frame->addView(tempoRateDisplay);

	// tempo (in bpm)   (editable)
	size.offset (0, kWideFaderInc);
	tempoTextEdit = new CTextEdit (size, this, kTempoTextEdit, 0, gBackground);
	displayOffset.offset (0, kWideFaderInc);
	tempoTextEdit->setBackOffset(displayOffset);
	tempoTextEdit->setFont (kNormalFont);
	tempoTextEdit->setFontColor (kBlackCColor);
	tempoTextEdit->setHoriAlign (kLeftText);
	frame->addView (tempoTextEdit);
	// this makes it display the current value
	setParameter(kTempo, effect->getParameter(kTempo));

	// smoothing amount
	size.offset (0, kWideFaderInc);
	smoothDisplay = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kWideFaderInc - kDisplayHeight + 4);
	smoothDisplay->setBackOffset(displayOffset);
	smoothDisplay->setHoriAlign(kLeftText);
	smoothDisplay->setFont(kNormalFont);
	smoothDisplay->setFontColor(kBlackCColor);
	smoothDisplay->setValue(effect->getParameter(kSmooth));
	smoothDisplay->setStringConvert(smoothDisplayConvert);
	frame->addView(smoothDisplay);

	return 1;
}

//-----------------------------------------------------------------------------
void EQsyncEditor::close()
{
	if (frame)
		delete frame;
	frame = 0;

	// free some bitmaps
	if (gFaderSlide)
		gFaderSlide->forget();
	gFaderSlide = 0;
	if (gFaderHandle)
		gFaderHandle->forget();
	gFaderHandle = 0;

	if (gTallFaderSlide)
		gTallFaderSlide->forget();
	gTallFaderSlide = 0;
	if (gTallFaderHandle)
		gTallFaderHandle->forget();
	gTallFaderHandle = 0;

	if (gDestroyFXlink)
		gDestroyFXlink->forget();
	gDestroyFXlink = 0;
}

//-----------------------------------------------------------------------------
void EQsyncEditor::setParameter(long index, float value)
{
	if (!frame)
		return;

	// called from EQsyncEdit
	switch (index)
	{
		case kRate_sync:
			strcpy(tempoRateString, ((EQsync*)effect)->tempoRateTable->getDisplay(((DfxPlugin*)effect)->getparameter_i(kRate_sync)));
			if (tempoRateFader)
				tempoRateFader->setValue(effect->getParameter(index));
			if (tempoRateDisplay)
				tempoRateDisplay->setValue(effect->getParameter(index));
			break;

		case kTempo:
			if (tempoFader)
				tempoFader->setValue(effect->getParameter(index));
			if (tempoTextEdit)
			{
				if ( (value > 0.0f) || (((EQsync*)effect)->hostCanDoTempo < 1) )
					sprintf(tempoString, "%.3f  bpm", tempoScaled(value));
				else
					strcpy(tempoString, "auto");
				tempoTextEdit->setText(tempoString);
			}
			break;

		case kSmooth:
			if (smoothFader)
				smoothFader->setValue(effect->getParameter(index));
			if (smoothDisplay)
				smoothDisplay->setValue(effect->getParameter(index));
			break;

		case ka0:
			if (a0Fader)
				a0Fader->setValue(effect->getParameter(index));
			break;
		case ka1:
			if (a1Fader)
				a1Fader->setValue(effect->getParameter(index));
			break;
		case ka2:
			if (a2Fader)
				a2Fader->setValue(effect->getParameter(index));
			break;
		case kb1:
			if (b1Fader)
				b1Fader->setValue(effect->getParameter(index));
			break;
		case kb2:
			if (b2Fader)
				b2Fader->setValue(effect->getParameter(index));
			break;

		default:
			return;
	}	

	postUpdate();
}

//-----------------------------------------------------------------------------
void EQsyncEditor::valueChanged(CDrawContext* context, CControl* control)
{
  long tag = control->getTag();
  float tempTempo;
  long sscanfReturn;


	switch (tag)
	{
		// process the tempo text input
		case kTempoTextEdit:
			if (tempoTextEdit)
			{
				tempoTextEdit->getText(tempoString);
				sscanfReturn = sscanf(tempoString, "%f", &tempTempo);
				if (strcmp(tempoString, "auto") == 0)
					effect->setParameterAutomated(kTempo, 0.0f);
				else if ( (sscanfReturn != EOF) && (sscanfReturn > 0) )
				{
					// check if the user typed in something that's not a number
					if (tempTempo == 0.0f)
						tempoTextEdit->setText("very bad");
					// the user typed in a number
					else
					{
						// no negative tempos
						if (tempTempo < 0.0f)
							tempTempo = -tempTempo;
						// if the tempo entered is 0, then leave it at 0.0 as the parameter value
						if (tempTempo > 0.0f)
							// scale the value to a 0.0 to 1.0 parameter value
							tempTempo = tempoUnscaled(tempTempo);
						// this updates the display with "bpm" appended & the fader position
						setParameter(kTempo, tempTempo);
						effect->setParameterAutomated(kTempo, tempTempo);
					}
				}
				// there was a sscanf() error
				else
					tempoTextEdit->setText("bad");
			}
			break;

		// clicking on these parts of the GUI takes you to Destroy FX or SE web pages
		case kDestroyFXlinkID:
			if (control->getValue() >= 0.5f)
			{
			#if MAC
			  ICInstance ICconnection;
			  long urlStart = 1, urlEnd, urlLength, goError;
			#if CALL_NOT_IN_CARBON
			  long gestaltResponse;
				goError = Gestalt('ICAp', &gestaltResponse);
				if (goError == noErr)
			#endif
					goError = ICStart(&ICconnection, '????');
				if ( (goError == noErr) && (ICconnection == (void*)kUnresolvedCFragSymbolAddress) )
					goError = noErr + 3;
			#if CALL_NOT_IN_CARBON
				if (goError == noErr)
					goError = ICFindConfigFile(ICconnection, 0, nil);
			#endif
				if (goError == noErr)
				{
					urlEnd = urlLength = 42;
					goError = ICLaunchURL(ICconnection, "\phttp", "\phttp://www.smartelectronix.com/~destroyfx/", urlLength, &urlStart, &urlEnd);
				}
				if (goError == noErr)
					goError = ICStop(ICconnection);
			#endif
			#if WIN32
				ShellExecute(NULL, "open", "http://www.smartelectronix.com/~destroyfx/", NULL, NULL, SW_SHOWNORMAL);
			#endif
			}
			break;

		case kRate_sync:
		case kTempo:
		case kSmooth:
		case ka0:
		case ka1:
		case ka2:
		case kb1:
		case kb2:
			effect->setParameterAutomated(tag, control->getValue());
		default:
			break;
	}

	control->update(context);
}
