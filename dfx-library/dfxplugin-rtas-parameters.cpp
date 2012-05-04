#include "dfxplugin-rtas-parameters.h"
#include "FicTDMControl.h"
#include "SliderConversions.h"
#include "PlugInAssert.h"

#include <cstdlib>
#include <string.h>
#include <cstdio>



//-----------------------------------------------------------------------------
CPluginControl_DfxCurved::CPluginControl_DfxCurved(OSType id, const char * name, double min, double max, 
		int numSteps, double defaultValue, bool isAutomatable, 
		DfxParamCurve inCurve, double inCurveSpec, 
		const PrefixDictionaryEntry * begin, const PrefixDictionaryEntry * end)
:	CPluginControl_Continuous(id, name, min, max, defaultValue, isAutomatable, begin, end),
	mCurve(inCurve), mCurveSpec(inCurveSpec), mNumSteps(numSteps)
{
	PIASSERT(numSteps != 0);	// mNumSteps==0 can cause problems with devide-by-zero in PT
	SetValue( GetDefaultValue() );
}

//-----------------------------------------------------------------------------
long CPluginControl_DfxCurved::GetNumSteps() const
{
	return mNumSteps;
}

//-----------------------------------------------------------------------------
long CPluginControl_DfxCurved::ConvertContinuousToControl(double continuous) const
{
	continuous = DFX_ContractParameterValue(continuous, GetMin(), GetMax(), mCurve, mCurveSpec);
	return DoubleToLongControl(continuous, 0.0, 1.0);
}

//-----------------------------------------------------------------------------
double CPluginControl_DfxCurved::ConvertControlToContinuous(long control) const
{
	double continous = LongControlToDouble(control, 0.0, 1.0);
	return DFX_ExpandParameterValue(continous, GetMin(), GetMax(), mCurve, mCurveSpec);
}
