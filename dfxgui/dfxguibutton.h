#ifndef __DFXGUI_BUTTON_H
#define __DFXGUI_BUTTON_H


#include "dfxgui.h"


typedef enum {
	kPushButton	= 0,
	kIncButton,
	kDecButton,
	kRadioButton, 
	kPictureReel
} DfxGuiBottonMode;


typedef void (*buttonUserProcedure) (UInt32 value, void *userData);


//-----------------------------------------------------------------------------
class DGButton : public DGControl
{
public:
	DGButton(DfxGuiEditor*, AudioUnitParameterID, DGRect*, DGGraphic*, DGGraphic*, long inNumStates, DfxGuiBottonMode, bool inKick = false);
	DGButton(DfxGuiEditor*, DGRect*, DGGraphic*, DGGraphic*, long inNumStates, DfxGuiBottonMode, bool inKick = false);
	virtual ~DGButton();

	virtual void draw(CGContextRef context, UInt32 portHeight);
	virtual void mouseDown(Point *P, bool, bool);
	virtual void mouseTrack(Point *P, bool, bool);
	virtual void mouseUp(Point *P, bool, bool);
	void setMouseIsDown(bool newMouseState);

	virtual void setUserProcedure(buttonUserProcedure inProc, void *inUserData);
	virtual void setUserReleaseProcedure(buttonUserProcedure inProc, void *inUserData);

private:
	DGGraphic *		ForeGround;
	DGGraphic *		BackGround;
	DfxGuiBottonMode	mode;

	buttonUserProcedure	userProcedure;
	buttonUserProcedure	userReleaseProcedure;
	void *				userProcData;
	void *				userReleaseProcData;

	float				alpha;
	long				numStates;
	bool				kick;
	bool				mouseIsDown;
};


#endif
