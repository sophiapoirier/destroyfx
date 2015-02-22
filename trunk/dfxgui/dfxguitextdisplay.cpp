/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2015  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "dfxguitextdisplay.h"

#include <stdio.h>


//-----------------------------------------------------------------------------
static bool DFXGUI_GenericValueToTextProc(float inValue, char outTextUTF8[], void* inUserData);
static bool DFXGUI_GenericValueToTextProc(float inValue, char outTextUTF8[], void* inUserData)
{
	sprintf(outTextUTF8, "%.2f", inValue);
	return true;
}

static bool DFXGUI_GenericTextToValueProc(UTF8StringPtr inText, float& outValue, void* inUserData);
static bool DFXGUI_GenericTextToValueProc(UTF8StringPtr inText, float& outValue, void* inUserData)
{
	const int readCount = sscanf(inText, "%f", &outValue);
	return (readCount >= 1);
}



#pragma mark -
#pragma mark DGTextDisplay

//-----------------------------------------------------------------------------
// Text Display
//-----------------------------------------------------------------------------
DGTextDisplay::DGTextDisplay(DfxGuiEditor *					inOwnerEditor,
							long							inParamID, 
							DGRect *						inRegion,
							CParamDisplayValueToStringProc	inTextProc, 
							void *							inUserData,
							DGImage *						inBackgroundImage, 
							DGTextAlignment					inTextAlignment, 
							float							inFontSize, 
							DGColor							inFontColor, 
							const char *					inFontName)
:	CTextEdit(*inRegion, inOwnerEditor, inParamID, NULL, inBackgroundImage), 
	DGControl(this, inOwnerEditor)
{
	valueToTextProcBridgeData.mThis = this;
	valueToTextProcBridgeData.mProc = NULL;
	valueToTextProcBridgeData.mUserData = NULL;

	textToValueProcBridgeData.mThis = this;
	textToValueProcBridgeData.mProc = NULL;
	textToValueProcBridgeData.mUserData = NULL;

	setTransparency(true);
	if (inBackgroundImage == NULL)
	{
		const CPoint backgroundOffset(inRegion->left, inRegion->top);
		setBackOffset(backgroundOffset);
	}

	setTextAlignment(inTextAlignment);
	setFontColor(inFontColor);

	CFontRef fontDesc = DFXGUI_CreateVstGuiFont(inFontSize, inFontName);
	if (fontDesc != NULL)
	{
		setFont(fontDesc);
		fontDesc->forget();
	}

	if (inTextProc == NULL)
		inTextProc = DFXGUI_GenericValueToTextProc;
	if (inUserData == NULL)
		inUserData = this;
	setValueToStringProc(inTextProc, inUserData);

	setStringToValueProc(DFXGUI_GenericTextToValueProc, this);

	mouseAxis = kDGAxis_vertical;
	setAntialias(true);
}

//-----------------------------------------------------------------------------
DGTextDisplay::~DGTextDisplay()
{
}

//-----------------------------------------------------------------------------
CMouseEventResult DGTextDisplay::onMouseDown(CPoint & inPos, const CButtonState & inButtons)
{
#if 0
	lastX = inPos.x;
	lastY = inPos.y;
#endif
	return CTextEdit::onMouseDown(inPos, inButtons);
}

//-----------------------------------------------------------------------------
CMouseEventResult DGTextDisplay::onMouseMoved(CPoint & inPos, const CButtonState & inButtons)
{
#if 0
	const long min = GetControl32BitMinimum(carbonControl);
	const long max = GetControl32BitMaximum(carbonControl);
	long val = GetControl32BitValue(carbonControl);
	const long oldval = val;

	float diff = 0.0f;
	if (mouseAxis & kDGAxis_horizontal)
		diff += inPos.x - lastX;
	if (mouseAxis & kDGAxis_vertical)
		diff += lastY - inPos.y;
	if (inButtons.getModifierState() & kZoomModifier)	// slo-mo
		diff /= getFineTuneFactor();
	val += (long) (diff * (float)(max-min) / getMouseDragRange());

	if (val > max)
		val = max;
	if (val < min)
		val = min;
	if (val != oldval)
		SetControl32BitValue(carbonControl, val);

	lastX = inPos.x;
	lastY = inPos.y;
#endif
	return CTextEdit::onMouseMoved(inPos, inButtons);
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setValueToStringProc(CParamDisplayValueToStringProc inProc, void* inUserData)
{
	valueToTextProcBridgeData.mProc = inProc;
	valueToTextProcBridgeData.mUserData = inUserData;
	CTextEdit::setValueToStringProc(valueToTextProcBridge, &valueToTextProcBridgeData);

	setValue( getValue() );	// trigger an initial value->text conversion
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setStringToValueProc(CTextEditStringToValueProc inProc, void* inUserData)
{
	textToValueProcBridgeData.mProc = inProc;
	textToValueProcBridgeData.mUserData = inUserData;
	CTextEdit::setStringToValueProc(textToValueProcBridge, &textToValueProcBridgeData);
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setTextAlignment(DGTextAlignment inTextAlignment)
{
	switch (inTextAlignment)
	{
		case kDGTextAlign_left:
			setHoriAlign(kLeftText);
			break;
		case kDGTextAlign_center:
			setHoriAlign(kCenterText);
			break;
		case kDGTextAlign_right:
			setHoriAlign(kRightText);
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
DGTextAlignment DGTextDisplay::getTextAlignment()
{
	switch ( getHoriAlign() )
	{
		case kRightText:
			return kDGTextAlign_right;
		case kCenterText:
			return kDGTextAlign_center;
		case kLeftText:
		default:
			return kDGTextAlign_left;
	}
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::valueToTextProcBridge(float inValue, char outTextUTF8[256], void * inUserData)
{
	if (inUserData == NULL)
		return false;

	const ValueToTextProcBridgeData* data = reinterpret_cast<ValueToTextProcBridgeData*>(inUserData);
	if (data->mThis->isParameterAttached())
		inValue = data->mThis->getOwnerEditor()->dfxgui_ExpandParameterValue(data->mThis->getParameterID(), inValue);
	return data->mProc(inValue, outTextUTF8, data->mUserData);
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::textToValueProcBridge(UTF8StringPtr inText, float& outValue, void* inUserData)
{
	if ((inText == NULL) || (inUserData == NULL))
		return false;

	const TextToValueProcBridgeData* data = reinterpret_cast<TextToValueProcBridgeData*>(inUserData);
	const bool success = data->mProc(inText, outValue, data->mUserData);
	if (success)
	{
		if (data->mThis->isParameterAttached())
			outValue = data->mThis->getOwnerEditor()->dfxgui_ContractParameterValue(data->mThis->getParameterID(), outValue);
	}

	return success;
}






#pragma mark -
#pragma mark DGStaticTextDisplay

//-----------------------------------------------------------------------------
DGStaticTextDisplay::DGStaticTextDisplay(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground, 
										DGTextAlignment inTextAlignment, float inFontSize, 
										DGColor inFontColor, const char * inFontName)
:	DGTextDisplay(inOwnerEditor, DFX_PARAM_INVALID_ID, inRegion, NULL, NULL, inBackground, 
					inTextAlignment, inFontSize, inFontColor, inFontName), 
	displayString(NULL)
{
	displayString = (char*) malloc(kDGTextDisplay_stringSize);
	displayString[0] = 0;
	setMouseEnabled(false);
}

//-----------------------------------------------------------------------------
DGStaticTextDisplay::~DGStaticTextDisplay()
{
	if (displayString != NULL)
		free(displayString);
	displayString = NULL;
}

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::setText(const char * inNewText)
{
	if (inNewText == NULL)
		strcpy(displayString, "");
	else
		strcpy(displayString, inNewText);
	redraw();
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
void DGStaticTextDisplay::setCFText(CFStringRef inNewText)
{
	if (inNewText == NULL)
	{
		setText(NULL);
		return;
	}

	const Boolean success = CFStringGetCString(inNewText, displayString, kDGTextDisplay_stringSize, kCFStringEncodingUTF8);
	if (success)
		redraw();
}
#endif

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::draw(CDrawContext * inContext)
{
	if (getBackground() != NULL)
		getBackground()->draw(inContext, size);

	drawText(inContext, displayString);
	setDirty(false);
}






#pragma mark -
#pragma mark DGTextArrayDisplay

//-----------------------------------------------------------------------------
// Static Text Display
//-----------------------------------------------------------------------------
DGTextArrayDisplay::DGTextArrayDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
						long inNumStrings, DGTextAlignment inTextAlignment, DGImage * inBackground, 
						float inFontSize, DGColor inFontColor, const char * inFontName)
:	DGTextDisplay(inOwnerEditor, inParamID, inRegion, NULL, NULL, inBackground, 
					inTextAlignment, inFontSize, inFontColor, inFontName), 
	numStrings(inNumStrings), 
	displayStrings(NULL)
{
	if (numStrings <= 0)
		numStrings = 1;
	displayStrings = (char**) malloc(numStrings * sizeof(char*));
	for (long i=0; i < numStrings; i++)
	{
		displayStrings[i] = (char*) malloc(kDGTextDisplay_stringSize);
		displayStrings[i][0] = 0;
	}

	setMouseEnabled(false);
}

//-----------------------------------------------------------------------------
DGTextArrayDisplay::~DGTextArrayDisplay()
{
	if (displayStrings != NULL)
	{
		for (long i=0; i < numStrings; i++)
		{
			if (displayStrings[i] != NULL)
				free(displayStrings[i]);
			displayStrings[i] = NULL;
		}
		free(displayStrings);
	}
	displayStrings = NULL;
}

#if 0
//-----------------------------------------------------------------------------
void DGTextArrayDisplay::post_embed()
{
	if ( isParameterAttached() && (carbonControl != NULL) )
		SetControl32BitMaximum( carbonControl, numStrings - 1 + GetControl32BitMinimum(carbonControl) );
}
#endif

//-----------------------------------------------------------------------------
void DGTextArrayDisplay::setText(long inStringNum, const char * inNewText)
{
	if ( (inStringNum < 0) || (inStringNum >= numStrings) )
		return;
	if (inNewText == NULL)
		return;

	strcpy(displayStrings[inStringNum], inNewText);
//	redraw();
}

//-----------------------------------------------------------------------------
void DGTextArrayDisplay::draw(CDrawContext * inContext)
{
	if (getBackground() != NULL)
		getBackground()->draw(inContext, size);

#if 0
	const long stringIndex = GetControl32BitValue(carbonControl) - GetControl32BitMinimum(carbonControl);
	if ( (stringIndex >= 0) && (stringIndex < numStrings) )
		drawText(inContext, displayStrings[stringIndex]);
#endif

	setDirty(false);
}
