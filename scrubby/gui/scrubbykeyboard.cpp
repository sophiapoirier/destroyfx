#include <stdlib.h>
#include<stdio.h>

#include "scrubbykeyboard.h"


//------------------------------------------------------------------------
ScrubbyKeyboard::ScrubbyKeyboard(const CRect &size, CControlListener *listener, 
								long    startTag,
								CBitmap *background,
								CBitmap *onImage, 
								long    blackKeyWidth, 
								long    blackKeyHeight, 
								long    blackKey1Xpos, 
								long    blackKey2Xpos, 
								long    blackKey3Xpos, 
								long    blackKey4Xpos, 
								long    blackKey5Xpos)
:	CMultiControl (size, listener, 12, 0, background), 
    startTag(startTag), onImage(onImage) //, pOScreen(0)	// 2.2b2
{
	whiteKeyLocations = 0;
	blackKeyLocations = 0;
	whiteTags = 0;
	blackTags = 0;

	for (int i=0; i < 12; i++)
		setTagIndexed(i, startTag+i);

	if (onImage)
		onImage->remember();

	keyboardWidth  = size.right - size.left;
	keyboardHeight = size.bottom - size.top;
	whiteKeyWidth = keyboardWidth / 7;

	whiteKeyLocations = (CRect*)malloc(sizeof(CRect) * 7);
	blackKeyLocations = (CRect*)malloc(sizeof(CRect) * 5);

	for (int j=0; j < 7; j++)
	{
		whiteKeyLocations[j].left = (whiteKeyWidth * j) + 1;
		whiteKeyLocations[j].right = whiteKeyLocations[j].left + whiteKeyWidth;
		whiteKeyLocations[j].top = 0;
		whiteKeyLocations[j].bottom = keyboardHeight;
	}
	whiteKeyLocations[0].left = 0;	// because we actually started at 1
	// in case keyboardWidth is not a multiple of 7
	whiteKeyLocations[6].right = keyboardWidth;

	blackKeyLocations[0].left = blackKey1Xpos;
	blackKeyLocations[1].left = blackKey2Xpos;
	blackKeyLocations[2].left = blackKey3Xpos;
	blackKeyLocations[3].left = blackKey4Xpos;
	blackKeyLocations[4].left = blackKey5Xpos;
	for (int k=0; k < 5; k++)
	{
		blackKeyLocations[k].right = blackKeyLocations[k].left + blackKeyWidth;
		blackKeyLocations[k].top = 0;
		blackKeyLocations[k].bottom = blackKeyHeight;
	}

	whiteTags = new long[7];
	blackTags = new long[5];
	whiteTags[0] = tags[0];
	whiteTags[1] = tags[2];
	whiteTags[2] = tags[4];
	whiteTags[3] = tags[5];
	whiteTags[4] = tags[7];
	whiteTags[5] = tags[9];
	whiteTags[6] = tags[11];
	blackTags[0] = tags[1];
	blackTags[1] = tags[3];
	blackTags[2] = tags[6];
	blackTags[3] = tags[8];
	blackTags[4] = tags[10];

	currentTag = tags;
}

//------------------------------------------------------------------------
ScrubbyKeyboard::~ScrubbyKeyboard()
{
	if (onImage)
		onImage->forget();

	if (whiteKeyLocations)
		free(whiteKeyLocations);
	whiteKeyLocations = 0;
	if (blackKeyLocations)
		free(blackKeyLocations);
	blackKeyLocations = 0;

	if (whiteTags)
		delete[] whiteTags;
	whiteTags = 0;
	if (blackTags)
		delete[] blackTags;
	blackTags = 0;
}

//------------------------------------------------------------------------
void ScrubbyKeyboard::draw(CDrawContext *pContext)
{
	COffscreenContext *pOffScreen = new COffscreenContext(getParent(), keyboardWidth, keyboardHeight, kWhiteCColor);

	// (re)draw background
	CRect rect(0, 0, keyboardWidth, keyboardHeight);
	if (pBackground)
		pBackground->draw(pOffScreen, rect);

	// draw the "on" version of any of the keys that are selected
	if (onImage)
	{
		CPoint where;
		for (int w=0; w < 7; w++)
		{
			if (getValueTagged(whiteTags[w]) > 0.5f)
			{
				where (whiteKeyLocations[w].left, whiteKeyLocations[w].top);
				onImage->draw(pOffScreen, whiteKeyLocations[w], where);
			}
		}
		for (int b=0; b < 5; b++)
		{
			where (blackKeyLocations[b].left, blackKeyLocations[b].top);
			if (getValueTagged(blackTags[b]) > 0.5f)
				onImage->draw(pOffScreen, blackKeyLocations[b], where);
			else if (pBackground)
				pBackground->draw(pOffScreen, blackKeyLocations[b], where);
		}
	}

	pOffScreen->copyFrom(pContext, size);
	delete pOffScreen;

	setDirty(false);
}

//------------------------------------------------------------------------
void ScrubbyKeyboard::mouse(CDrawContext *pContext, CPoint &where)
{
	if (!bMouseEnabled)
		return;

 	long button = pContext->getMouseButtons();
	if ( !(button & kLButton) )
		return;

	where.h -= size.left;
	where.v -= size.top;
	long clickedTag = -1;
	for (int w=0; w < 7; w++)
	{
		if (where.isInside(whiteKeyLocations[w]))
		{
			clickedTag = whiteTags[w];
			break;
		}
	}
	for (int b=0; b < 5; b++)
	{
		if (where.isInside(blackKeyLocations[b]))
		{
			clickedTag = blackTags[b];
			break;
		}
	}

	if (clickedTag >= 0)
	{
		clickedTag -= startTag;	// make it the tags index rather than actual tag value
		values[clickedTag] = (values[clickedTag] > 0.5f) ? 0.0f : 1.0f;
		currentTag = &(tags[clickedTag]);
		draw(pContext);
		listener->valueChanged(pContext, this);
	}
}
