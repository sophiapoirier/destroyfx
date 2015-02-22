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

#ifndef __DFXGUI_TEXT_DISPLAY_H
#define __DFXGUI_TEXT_DISPLAY_H


#include "dfxguieditor.h"


static const size_t kDGTextDisplay_stringSize = 256;


//-----------------------------------------------------------------------------
class DGTextDisplay : public CTextEdit, public DGControl
{
public:
	DGTextDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
					CParamDisplayValueToStringProc inTextProc, void * inUserData, DGImage * inBackground, 
					DGTextAlignment inTextAlignment = kDGTextAlign_left, float inFontSize = 12.0f, 
					DGColor inFontColor = kBlackCColor, const char * inFontName = NULL);
	virtual ~DGTextDisplay();

	virtual CMouseEventResult onMouseDown(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseMoved(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;

	virtual void setValueToStringProc(CParamDisplayValueToStringProc inProc, void* inUserData = 0) VSTGUI_OVERRIDE_VMETHOD;
	virtual void setStringToValueProc(CTextEditStringToValueProc inProc, void* inUserData = 0)VSTGUI_OVERRIDE_VMETHOD;

	void setTextAlignment(DGTextAlignment inTextAlignment);
	DGTextAlignment getTextAlignment();
	void setMouseAxis(DGAxis inMouseAxis)
		{	mouseAxis = inMouseAxis;	}

protected:
	static bool valueToTextProcBridge(float inValue, char outTextUTF8[256], void * inUserData);
	typedef struct
	{
		DGTextDisplay* mThis;
		CParamDisplayValueToStringProc mProc;
		void* mUserData;
	} ValueToTextProcBridgeData;
	ValueToTextProcBridgeData valueToTextProcBridgeData;

	static bool textToValueProcBridge(UTF8StringPtr inText, float& outValue, void* inUserData);
	typedef struct
	{
		DGTextDisplay* mThis;
		CTextEditStringToValueProc mProc;
		void* mUserData;
	} TextToValueProcBridgeData;
	TextToValueProcBridgeData textToValueProcBridgeData;

	DGAxis					mouseAxis;	// flags indicating which directions you can mouse to adjust the control value
	float					lastX, lastY;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGTextDisplay
{
public:
	DGStaticTextDisplay(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground, 
						DGTextAlignment inTextAlignment = kDGTextAlign_left, float inFontSize = 12.0f, 
						DGColor inFontColor = kBlackCColor, const char * inFontName = NULL);
	virtual ~DGStaticTextDisplay();

	virtual void draw(CDrawContext * inContext) VSTGUI_OVERRIDE_VMETHOD;

	void setText(const char * inNewText);
#if TARGET_OS_MAC
	void setCFText(CFStringRef inNewText);
#endif

protected:
	char * displayString;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGTextArrayDisplay : public DGTextDisplay
{
public:
	DGTextArrayDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, long inNumStrings, 
						DGTextAlignment inTextAlignment = kDGTextAlign_left, DGImage * inBackground = NULL, 
						float inFontSize = 12.0f, DGColor inFontColor = kBlackCColor, const char * inFontName = NULL);
	virtual ~DGTextArrayDisplay();

	virtual void draw(CDrawContext * inContext) VSTGUI_OVERRIDE_VMETHOD;

	void setText(long inStringNum, const char * inNewText);

protected:
	long numStrings;
	char ** displayStrings;
};




#endif
