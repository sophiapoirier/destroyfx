/*------------------- by Marc Poirier  ][  January 2002 ------------------*/

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

	float getScalar(long index)
		{	return scalars[TempoRateTable::safeIndex(index)];	}
	const char * getDisplay(long index)
		{	return displays[TempoRateTable::safeIndex(index)];	}
	float getScalar_gen(float genValue)
		{	return scalars[TempoRateTable::float2index(genValue)];	}
	char * getDisplay_gen(float genValue)
		{	return displays[TempoRateTable::float2index(genValue)];	}
	long getNumTempoRates() { return numTempoRates; }
	long getNearestTempoRateIndex(float tempoRateValue);

private:
	long float2index(float f)
	{
		if (f < 0.0f)
			return 0;
		else if (f > 1.0f)
			return numTempoRates-1;
		return (long) (f * ((float)numTempoRates-0.9f));
	}
	long safeIndex(long index)
	{
		if (index < 0)
			return 0;
		else if (index >= numTempoRates)
			return numTempoRates-1;
		return index;
	}

	float * scalars;
	char ** displays;

	long numTempoRates;
	long typeOfTable;
};


#endif