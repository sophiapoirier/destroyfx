/*------------------- by Marc Poirier  ][  January 2002 ------------------*/

#include "temporatetable.h"

#include <string.h>
#include <math.h>	// for fabs


//-----------------------------------------------------------------------------
TempoRateTable::TempoRateTable(long inTypeOfTable)
:	typeOfTable(inTypeOfTable)
{
	switch (typeOfTable)
	{
		case kSlowTempoRates:
			numTempoRates = 25;
			break;
		case kNoExtremeTempoRates:
			numTempoRates = 21;
			break;
		default:
			numTempoRates = 24;
			break;
	}

	scalars = 0;
	scalars = (float*) malloc(sizeof(float) * numTempoRates);
	displays = 0;
	displays = (char**) malloc(sizeof(char*) * numTempoRates);
	long i;
	for (i = 0; i < numTempoRates; i++)
		displays[i] = (char*) malloc(sizeof(char) * 16);

	i = 0;
	if (typeOfTable == kSlowTempoRates)
	{
		scalars[i] = 1.f/12.f;	strcpy(displays[i++], "1/12");
		scalars[i] = 1.f/8.f;	strcpy(displays[i++], "1/8");
		scalars[i] = 1.f/7.f;	strcpy(displays[i++], "1/7");
	}
	if (typeOfTable != kNoExtremeTempoRates)
	{
		scalars[i] = 1.f/6.f;	strcpy(displays[i++], "1/6");
		scalars[i] = 1.f/5.f;	strcpy(displays[i++], "1/5");
	}
	scalars[i] = 1.f/4.f;	strcpy(displays[i++], "1/4");
	scalars[i] = 1.f/3.f;	strcpy(displays[i++], "1/3");
	scalars[i] = 1.f/2.f;	strcpy(displays[i++], "1/2");
	scalars[i] = 2.f/3.f;	strcpy(displays[i++], "2/3");
	scalars[i] = 3.f/4.f;	strcpy(displays[i++], "3/4");
	scalars[i] = 1.0f;		strcpy(displays[i++], "1");
	scalars[i] = 2.0f;		strcpy(displays[i++], "2");
	scalars[i] = 3.0f;		strcpy(displays[i++], "3");
	scalars[i] = 4.0f;		strcpy(displays[i++], "4");
	scalars[i] = 5.0f;		strcpy(displays[i++], "5");
	scalars[i] = 6.0f;		strcpy(displays[i++], "6");
	scalars[i] = 7.0f;		strcpy(displays[i++], "7");
	scalars[i] = 8.0f;		strcpy(displays[i++], "8");
	scalars[i] = 12.0f;		strcpy(displays[i++], "12");
	scalars[i] = 16.0f;		strcpy(displays[i++], "16");
	scalars[i] = 24.0f;		strcpy(displays[i++], "24");
	scalars[i] = 32.0f;		strcpy(displays[i++], "32");
	scalars[i] = 48.0f;		strcpy(displays[i++], "48");
	scalars[i] = 64.0f;		strcpy(displays[i++], "64");
	scalars[i] = 96.0f;		strcpy(displays[i++], "96");
	if (typeOfTable != kSlowTempoRates)
	{
		scalars[i] = 333.0f;		strcpy(displays[i++], "333");
	}
	if ( (typeOfTable != kSlowTempoRates) && (typeOfTable != kNoExtremeTempoRates) )
	{
		scalars[i] = 3000.0f;	strcpy(displays[i++], "infinity");
	}
}

//-----------------------------------------------------------------------------
/*
// moved to header so that the implementation is seen by DfxPlugin::DfxPlugin
TempoRateTable::~TempoRateTable()
{
	if (scalars)
		free(scalars);
	scalars = 0;
	for (int i=0; i < numTempoRates; i++)
	{
		if (displays[i])
			free(displays[i]);
	}
	if (displays)
		free(displays);
	displays = 0;
}
*/


//-----------------------------------------------------------------------------
// given a tempo rate value, return the index of the tempo rate 
// that is closest to that requested value
long TempoRateTable::getNearestTempoRateIndex(float tempoRateValue)
{
	float bestdiff = tempoRateValue;
	long bestindex = 0;
	for (long i=0; i < numTempoRates; i++)
	{
		float diff = (float) fabs(tempoRateValue - scalars[i]);
		if (diff < bestdiff)
		{
			bestdiff = diff;
			bestindex = i;
		}
	}
	return bestindex;
}
