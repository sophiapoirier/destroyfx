#ifndef __dfx_temporatetable
#define __dfx_temporatetable


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
	TempoRateTable(long typeOfTable = kNormalTempoRateTable);
	~TempoRateTable();

	float getScalar(float paramValue)
		{	return scalars[float2index(paramValue)];	}
	char * getDisplay(float paramValue)
		{	return displays[float2index(paramValue)];	}

protected:
	int float2index(float f)
	{
		if (f < 0.0f)
			f = 0.0f;
		else if (f > 1.0f)
			f = 1.0f;
		return (int) (f * ((float)numTempoRates-0.9f));
	}
		
	float *scalars;
	char **displays;

	long numTempoRates;
	long typeOfTable;
};


#endif