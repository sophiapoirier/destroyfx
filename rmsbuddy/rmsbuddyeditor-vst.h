/*----------------------- by Marc Poirier  ][  June 2001 ----------------------*/

#ifndef __RMSBUDDYEDITOR_VST_H
#define __RMSBUDDYEDITOR_VST_H

#include "vstgui.h"


//-----------------------------------------------------------------------------
class RMSbuddyEditor : public AEffGUIEditor, public CControlListener
{
public:
	RMSbuddyEditor(AudioEffect *effect);
	virtual ~RMSbuddyEditor();

protected:
	virtual long getRect(ERect **rect);
	virtual long open(void *ptr);
	virtual void close();

	virtual void valueChanged(CDrawContext* context, CControl* control);

    virtual void idle();

private:
	// controls
	CKickButton *resetRMSbutton;
	CKickButton *resetPeakButton;

	// parameter value display boxes
	CParamDisplay *backgroundDisplay;
	CParamDisplay *leftAverageRMSDisplay;
	CParamDisplay *rightAverageRMSDisplay;
	CParamDisplay *leftContinualRMSDisplay;
	CParamDisplay *rightContinualRMSDisplay;
	CParamDisplay *leftAbsolutePeakDisplay;
	CParamDisplay *rightAbsolutePeakDisplay;
	CParamDisplay *leftContinualPeakDisplay;
	CParamDisplay *rightContinualPeakDisplay;
	CParamDisplay *averageRMSDisplay;
	CParamDisplay *continualRMSDisplay;
	CParamDisplay *absolutePeakDisplay;
	CParamDisplay *continualPeakDisplay;
	CParamDisplay *leftDisplay;
	CParamDisplay *rightDisplay;

	// graphics
	CBitmap *gResetButton;

	float theLeftContinualRMS, theRightContinualRMS;
	float theLeftContinualPeak, theRightContinualPeak;
};

#endif