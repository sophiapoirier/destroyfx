#ifndef __DFXGUI_BUTTON_H
#define __DFXGUI_BUTTON_H


#include "dfxgui.h"


typedef enum {
	kDGButtonType_pushbutton = 0,
	kDGButtonType_incbutton,
	kDGButtonType_decbutton,
	kDGButtonType_radiobutton, 
	kDGButtonType_picturereel
} DfxGuiBottonMode;


typedef void (*DGButtonUserProcedure) (long inValue, void * inUserData);


//-----------------------------------------------------------------------------
class DGButton : public DGControl
{
public:
	DGButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage, 
				long inNumStates, DfxGuiBottonMode inMode, bool inDrawMomentaryState = false);
	DGButton(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, 
				long inNumStates, DfxGuiBottonMode inMode, bool inDrawMomentaryState = false);
	virtual ~DGButton();
	virtual void post_embed();

	virtual void draw(DGGraphicsContext * inContext);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);
	virtual bool mouseWheel(long inDelta, DGMouseWheelAxis inAxis, DGKeyModifiers inKeyModifiers);

	void setMouseIsDown(bool newMouseState);
	bool getMouseIsDown()
		{	return mouseIsDown;	}

	void setValue(long inValue);
	long getValue();

	void setNumStates(long inNumStates)
		{	if (inNumStates > 0) numStates = inNumStates;	}
	long getNumStates()
		{	return numStates;	}

	void setButtonImage(DGImage * inImage);

	void setUserProcedure(DGButtonUserProcedure inProc, void * inUserData);
	void setUserReleaseProcedure(DGButtonUserProcedure inProc, void * inUserData);
	void setUseReleaseProcedureOnlyAtEndWithNoCancel(bool inNewPolicy)
		{   useReleaseProcedureOnlyAtEndWithNoCancel = inNewPolicy; }

protected:
	DGImage * buttonImage;

	DGButtonUserProcedure userProcedure;
	DGButtonUserProcedure userReleaseProcedure;
	void * userProcData;
	void * userReleaseProcData;
	bool useReleaseProcedureOnlyAtEndWithNoCancel;

	long numStates;
	DfxGuiBottonMode mode;
	bool drawMomentaryState;
	bool mouseIsDown;
	long entryValue, newValue;

private:
	void init();
};


//-----------------------------------------------------------------------------
class DGFineTuneButton : public DGControl
{
public:
	DGFineTuneButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
						DGImage * inImage, float inValueChangeAmount = 0.0001f);
	virtual ~DGFineTuneButton();

	virtual void draw(DGGraphicsContext * inContext);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);

	void setValueChangeAmount(float inChangeAmount)
		{	valueChangeAmount = inChangeAmount;	}

protected:
	DGImage * buttonImage;
	float valueChangeAmount;
	bool mouseIsDown;
	long entryValue, newValue;
};


//-----------------------------------------------------------------------------
class DGWebLink : public DGButton
{
public:
	DGWebLink(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, const char * inURL);
	virtual ~DGWebLink();

	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);

private:
	char * urlString;
};


//-----------------------------------------------------------------------------
class DGSplashScreen : public DGControl
{
public:
	DGSplashScreen(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inSplashImage);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);

	void removeSplashDisplay();

private:
	DGImage * splashImage;
	class DGSplashScreenDisplay : public DGControl
	{
		public:
			DGSplashScreenDisplay(DGSplashScreen * inParentControl, DGRect * inRegion, DGImage * inSplashImage);
			virtual void draw(DGGraphicsContext * inContext);
		private:
			DGSplashScreen * parentSplashControl;
			DGImage * splashImage;
	};
	DGSplashScreenDisplay * splashDisplay;
};


#endif
