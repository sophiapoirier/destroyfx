//=============================================================================
//               Scrubby multiple-parameter keyboard GUI object
//                         by Marc Poirier, May 2002
//=============================================================================

#ifndef __scrubbykeyboard
#define __scrubbykeyboard

#ifndef __dfxguimulticontrols
#include "dfxguimulticontrols.h"
#endif



//-----------------------------------------------------------------------------
class ScrubbyKeyboard : public CMultiControl
{
public:
	ScrubbyKeyboard(const CRect &size, CControlListener *listener, 
					long    startTag,
					CBitmap *background,
					CBitmap *onImage,
					long    blackKeyWidth, 
					long    blackKeyHeight, 
					long    blackKey1Xpos, 
					long    blackKey2Xpos, 
					long    blackKey3Xpos, 
					long    blackKey4Xpos, 
					long    blackKey5Xpos);
	virtual ~ScrubbyKeyboard();

	virtual void draw(CDrawContext*);
	virtual void mouse(CDrawContext *pContext, CPoint &where);

	virtual long getTag() { if (currentTag) return *currentTag;	else return tags[0]; }

protected:
	CBitmap *onImage;

	long startTag;
	long *currentTag;

	long keyboardWidth;
	long keyboardHeight;
	long whiteKeyWidth;
	long blackKeyWidth;
	long blackKeyHeight;
	CRect *whiteKeyLocations;
	CRect *blackKeyLocations;

	long *whiteTags, *blackTags;
};


#endif