/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2002-2009  Sophia Poirier

This program is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, please visit http://destroyfx.org/ 
and use the contact form.

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
Welcome to our FIR filter.
by Sophia Poirier  ][  January 2002
------------------------------------------------------------------------*/

#ifndef __DFX_FIR_FILTER_H
#define __DFX_FIR_FILTER_H


const double SHELF_START_FIR_LOWPASS = 0.333;


//-----------------------------------------------------------------------------
void calculateFIRidealLowpassCoefficients(double inCutoff, double inSampleRate, 
											long inNumTaps, float * inCoefficients);
void applyKaiserWindow(long inNumTaps, float * inCoefficients, float inAttenuation);
float besselIzero(float input);
float besselIzero2(float input);

//-----------------------------------------------------------------------------
inline float processFIRfilter(float * inAudio, long inNumTaps, float * inCoefficients, 
								long inPos, long inBufferSize)
{
	float outval = 0.0f;
	if ( (inPos+inNumTaps) > inBufferSize )
	{
		for (long i=0; i < inNumTaps; i++)
			outval += inAudio[ (inPos + i) % inBufferSize] * inCoefficients[i];
	}
	else
	{
		for (long i=0; i < inNumTaps; i++)
			outval += inAudio[inPos+i] * inCoefficients[i];
	}
	return outval;
} 


#endif
