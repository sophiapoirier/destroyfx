/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2010-2018  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
This is where we connect the RTAS/AudioSuite API to our DfxPlugin system.
------------------------------------------------------------------------*/

#pragma once


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
	CPluginControl_DfxCurved(OSType id, char const* name, double min, double max, 
		int numSteps, double defaultValue, bool isAutomatable, 
		DfxParam::Curve inCurve, double inCurveSpec = 1.0, 
		PrefixDictionaryEntry const* begin = cStandardPrefixDictionary + ePrefixOffset_no_prefix, 
		PrefixDictionaryEntry const* end = cStandardPrefixDictionary + ePrefixOffset_End);

	// CPluginControl overrides
	long GetNumSteps() const override;

	// CPluginControl_Continuous overrides
	long ConvertContinuousToControl(double continuous) const override;
	double ConvertControlToContinuous(long control) const override;
	
private:
	DfxParam::Curve mCurve;
	double mCurveSpec;
	int mNumSteps;
};



//-----------------------------------------------------------------------------
class CPluginControl_DfxCurvedFrequency : public CPluginControl_Frequency, public CPluginControl_DfxCurved
{
public:
	CPluginControl_DfxCurvedFrequency(OSType id, char const* name, double min, double max, 
		int numSteps, double defaultValue, bool isAutomatable, 
		DfxParam::Curve inCurve, double inCurveSpec = 1.0, 
		PrefixDictionaryEntry const* begin = cStandardPrefixDictionary + ePrefixOffset_no_prefix, 
		PrefixDictionaryEntry const* end = cStandardPrefixDictionary + ePrefixOffset_End)
	:
		CPluginControl_Continuous(id, name, min, max, defaultValue, isAutomatable, begin, end),
		CPluginControl_Frequency(id, name, min, max, defaultValue, isAutomatable, begin, end),
		CPluginControl_DfxCurved(id, name, min, max, numSteps, defaultValue, isAutomatable, 
			inCurve, inCurveSpec, begin, end)
	{}

private:
	CPluginControl_DfxCurvedFrequency(CPluginControl_DfxCurvedFrequency const&);
	CPluginControl_DfxCurvedFrequency& operator=(CPluginControl_DfxCurvedFrequency const&);
};
