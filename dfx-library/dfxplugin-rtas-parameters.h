#ifndef __DFXPLUGIN_RTAS_PARAMETERS_H
#define __DFXPLUGIN_RTAS_PARAMETERS_H


#include "CPluginControl_Continuous.h"
#include "CPluginControl_Frequency.h"

#include "dfxparameter.h"

#if WINDOWS_VERSION
	#pragma warning( disable : 4250 ) // function being inherited through dominance
#endif



//-----------------------------------------------------------------------------
class CPluginControl_DfxCurved : virtual public CPluginControl_Continuous
{
public:
	CPluginControl_DfxCurved(OSType id, const char * name, double min, double max, 
		int numSteps, double defaultValue, bool isAutomatable, 
		DfxParamCurve inCurve, double inCurveSpec = 1.0, 
		const PrefixDictionaryEntry * begin = cStandardPrefixDictionary+ePrefixOffset_no_prefix, 
		const PrefixDictionaryEntry * end = cStandardPrefixDictionary+ePrefixOffset_End);

	// CPluginControl overrides
	virtual long GetNumSteps() const;

	// CPluginControl_Continuous overrides
	virtual long ConvertContinuousToControl(double continuous) const;
	virtual double ConvertControlToContinuous(long control) const;
	
private:
	DfxParamCurve mCurve;
	double mCurveSpec;
	int mNumSteps;
};



//-----------------------------------------------------------------------------
class CPluginControl_DfxCurvedFrequency : public CPluginControl_Frequency, public CPluginControl_DfxCurved
{
public:
	CPluginControl_DfxCurvedFrequency(OSType id, const char * name, double min, double max, 
		int numSteps, double defaultValue, bool isAutomatable, 
		DfxParamCurve inCurve, double inCurveSpec = 1.0, 
		const PrefixDictionaryEntry * begin = cStandardPrefixDictionary+ePrefixOffset_no_prefix, 
		const PrefixDictionaryEntry * end = cStandardPrefixDictionary+ePrefixOffset_End)
	:
		CPluginControl_Continuous(id, name, min, max, defaultValue, isAutomatable, begin, end),
		CPluginControl_Frequency(id, name, min, max, defaultValue, isAutomatable, begin, end),
		CPluginControl_DfxCurved(id, name, min, max, numSteps, defaultValue, isAutomatable, 
			inCurve, inCurveSpec, begin, end)
	{}

private:
	CPluginControl_DfxCurvedFrequency(const CPluginControl_DfxCurvedFrequency&);
	CPluginControl_DfxCurvedFrequency& operator=(const CPluginControl_DfxCurvedFrequency&);
};



#endif
