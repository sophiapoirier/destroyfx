#pragma once


#include "ITemplateProcess.h"


//-----------------------------------------------------------------------------
class ITemplateCustomUI
{
public:
	ITemplateCustomUI(void * processPtr);
	virtual ~ITemplateCustomUI();

	virtual bool Open(void * winPtr, short left, short top) = 0;
	virtual bool Close() = 0;
	virtual long UpdateGraphicControl(long index, long value) = 0;
	virtual void Idle() = 0;
	virtual void Draw(long left, long top, long right, long bottom) = 0;
	virtual void GetRect(short * left, short * top, short * right, short * bottom) = 0;
	virtual void SetRect(short left, short top, short right, short bottom) = 0;

	virtual void GetControlIndexFromPoint(long x, long y, long * aControlIndex) = 0;
	virtual void SetControlHighlight(long controlIndex, short isHighlighted, short color) = 0;
	
#if MAC_VERSION
	virtual long MouseCommmand(long x, long y) = 0;
#endif

	// Destroy FX interface
//	virtual void resetClipDetectorLights() = 0;

protected:
	void * m_process_ptr;

};


//-----------------------------------------------------------------------------
ITemplateCustomUI * CreateCTemplateCustomUI(ITemplateProcess * processPtr);
