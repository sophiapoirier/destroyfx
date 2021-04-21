/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2010-2021  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
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

#include "dfxplugin-rtas-parameters.h"
#include "FicTDMControl.h"
#include "SliderConversions.h"
#include "PlugInAssert.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>



//-----------------------------------------------------------------------------
CPluginControl_DfxCurved::CPluginControl_DfxCurved(OSType id, char const* name, double min, double max, 
		int numSteps, double defaultValue, bool isAutomatable, 
		DfxParam::Curve inCurve, double inCurveSpec, 
		PrefixDictionaryEntry const* begin, PrefixDictionaryEntry const* end)
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
	continuous = DfxParam::contract(continuous, GetMin(), GetMax(), mCurve, mCurveSpec);
	return DoubleToLongControl(continuous, 0.0, 1.0);
}

//-----------------------------------------------------------------------------
double CPluginControl_DfxCurved::ConvertControlToContinuous(long control) const
{
	double const continous = LongControlToDouble(control, 0.0, 1.0);
	return DfxParam::expand(continous, GetMin(), GetMax(), mCurve, mCurveSpec);
}
