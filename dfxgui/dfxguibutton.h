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


typedef void (*buttonUserProcedure) (SInt32 value, void *userData);


//-----------------------------------------------------------------------------
class DGButton : public DGControl
{
public:
	DGButton(DfxGuiEditor*, AudioUnitParameterID, DGRect*, DGImage*, long inNumStates, DfxGuiBottonMode, bool inDrawMomentaryState = false);
	DGButton(DfxGuiEditor*, DGRect*, DGImage*, long inNumStates, DfxGuiBottonMode, bool inDrawMomentaryState = false);
	virtual ~DGButton();

	virtual void draw(CGContextRef context, UInt32 portHeight);
	virtual void mouseDown(Point inPos, bool, bool);
	virtual void mouseTrack(Point inPos, bool, bool);
	virtual void mouseUp(Point inPos, bool, bool);
	void setMouseIsDown(bool newMouseState);
	bool getMouseIsDown()
		{	return mouseIsDown;	}

	virtual void setUserProcedure(buttonUserProcedure inProc, void *inUserData);
	virtual void setUserReleaseProcedure(buttonUserProcedure inProc, void *inUserData);

protected:
	DGImage * buttonImage;

	buttonUserProcedure userProcedure;
	buttonUserProcedure userReleaseProcedure;
	void * userProcData;
	void * userReleaseProcData;

	long numStates;
	DfxGuiBottonMode mode;
	bool drawMomentaryState;
	bool mouseIsDown;
	SInt32 entryValue, newValue;
};


//-----------------------------------------------------------------------------
class DGWebLink : public DGButton
{
public:
	DGWebLink(DfxGuiEditor *inOwnerEditor, DGRect *inRegion, DGImage *inImage, const char *inURL);
	virtual ~DGWebLink();

	virtual void mouseUp(Point inPos, bool, bool);

private:
	char * urlString;
};


#endif
