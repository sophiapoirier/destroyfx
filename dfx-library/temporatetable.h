/*------------------- by Sophia Poirier  ][  January 2002 ------------------*/

#ifndef __DFX_TEMPORATETABLE_H
#define __DFX_TEMPORATETABLE_H

#include <stdlib.h>	// for free()


//--------------------------------------------------------------------------
// the number of tempo beat division options
enum TypesOfTempoRateTables
{
	kNormalTempoRates,
	kSlowTempoRates,
	kNoExtremeTempoRates
};



//-------------------------------------------------------------------------- 
// this holds the beat scalar values & textual displays for the tempo rates
class TempoRateTable
{
public:
	TempoRateTable(long inTypeOfTable = kNormalTempoRates);
	~TempoRateTable()
	{
		if (scalars != NULL)
			free(scalars);
		scalars = NULL;
		for (int i=0; i < numTempoRates; i++)
		{
			if (displays[i] != NULL)
				free(displays[i]);
			displays[i] = NULL;
		}
		if (displays != NULL)
			free(displays);
		displays = NULL;
	}

	float getScalar(long inIndex)
		{	return scalars[TempoRateTable::safeIndex(inIndex)];	}
	const char * getDisplay(long inIndex)
		{	return displays[TempoRateTable::safeIndex(inIndex)];	}
	float getScalar_gen(float inGenValue)
		{	return scalars[TempoRateTable::float2index(inGenValue)];	}
	char * getDisplay_gen(float inGenValue)
		{	return displays[TempoRateTable::float2index(inGenValue)];	}
	long getNumTempoRates()
		{	return numTempoRates;	}
	long getNearestTempoRateIndex(float inTempoRateValue);

private:
	long float2index(float inValue)
	{
		if (inValue < 0.0f)
			return 0;
		else if (inValue > 1.0f)
			return numTempoRates-1;
		return (long) (inValue * ((float)numTempoRates-0.9f));
	}
	long safeIndex(long inIndex)
	{
		if (inIndex < 0)
			return 0;
		else if (inIndex >= numTempoRates)
			return numTempoRates-1;
		return inIndex;
	}

	float * scalars;
	char ** displays;

	long numTempoRates;
	long typeOfTable;
};


#endif
