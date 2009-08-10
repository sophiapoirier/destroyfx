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
Welcome to our LFO.
by Sophia Poirier  ][  January 2002
------------------------------------------------------------------------*/

#include "lfo.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dfxmath.h"	// for kDFX_PI_f


//------------------------------------------------------------------------
LFO::LFO()
{
	sineTable = new float[NUM_LFO_POINTS];
	triangleTable = new float[NUM_LFO_POINTS];
	squareTable = new float[NUM_LFO_POINTS];
	sawTable = new float[NUM_LFO_POINTS];
	reverseSawTable = new float[NUM_LFO_POINTS];
	thornTable = new float[NUM_LFO_POINTS];

	fillLFOtables();
	table = sineTable;	// just to have it pointing to something at least

	srand( (unsigned int)time(NULL) );	// sets a seed value for rand() from the system clock

	reset();
}

//------------------------------------------------------------------------
LFO::~LFO()
{
	if (sineTable)
		delete[] sineTable;
	if (triangleTable)
		delete[] triangleTable;
	if (squareTable)
		delete[] squareTable;
	if (sawTable)
		delete[] sawTable;
	if (reverseSawTable)
		delete[] reverseSawTable;
	if (thornTable)
		delete[] thornTable;
}

//------------------------------------------------------------------------
void LFO::reset()
{
	position = 0.0f;
	stepSize = 1.0f;	// just to avoid anything really screwy
	oldRandomNumber = (float)rand() / (float)RAND_MAX;
	randomNumber = (float)rand() / (float)RAND_MAX;
	smoothSamples = 0;
	granularityCounter = 0;
}

//-----------------------------------------------------------------------------------------
// this function creates tables for mapping out the sine, triangle, and saw LFO shapes
void LFO::fillLFOtables()
{
  long i, n;


	// fill the sine waveform table (oscillates from 0 to 1 and back to 0)
	for (i = 0; (i < NUM_LFO_POINTS); i++)
		sineTable[i] = (sinf( ( ((float)i/(float)NUM_LFO_POINTS)-0.25f ) * 2.0f * kDFX_PI_f ) + 1.0f) * 0.5f;

	// fill the triangle waveform table
	// ramp from 0 to 1 for the first half
	for (i = 0; (i < NUM_LFO_POINTS/2); i++)
		triangleTable[i] = (float)i / (float)(NUM_LFO_POINTS/2);
	// and ramp from 1 to 0 for the second half
	for (n = 0; (i < NUM_LFO_POINTS); n++)
	{
		triangleTable[i] = 1.0f - ((float)n / (float)(NUM_LFO_POINTS/2));
		i++;
	}

	// fill the square waveform table
	// stay at 1 for the first half
	for (i = 0; (i < NUM_LFO_POINTS/2); i++)
		squareTable[i] = 1.0f;
	// and 0 for the second half
	for (n = 0; (i < NUM_LFO_POINTS); n++)
	{
		squareTable[i] = 0.0f;
		i++;
	}

	// fill the sawtooth waveform table (ramps from 0 to 1)
	for (i = 0; (i < NUM_LFO_POINTS); i++)
		sawTable[i] = (float)i / (float)(NUM_LFO_POINTS-1);

	// fill the reverse sawtooth waveform table (ramps from 1 to 0)
	for (i = 0; (i < NUM_LFO_POINTS); i++)
		reverseSawTable[i] = (float)(NUM_LFO_POINTS-i-1) / (float)(NUM_LFO_POINTS-1);

	// fill the thorn waveform table
	// exponentially slope up from 0 to 1 for the first half
	for (i = 0; (i < NUM_LFO_POINTS/2); i++)
		thornTable[i] = powf( ((float)i / (float)(NUM_LFO_POINTS/2)), 2.0f );
	// and exponentially slope down from 1 to 0 for the second half
	for (n = 0; (i < NUM_LFO_POINTS); n++)
	{
		thornTable[i] = powf( (1.0f - ((float)n / (float)(NUM_LFO_POINTS/2))), 2.0f );
		i++;
	}
}


//--------------------------------------------------------------------------------------
void LFO::getShapeName(char * outNameString)
{
	getShapeNameIndexed(iShape, outNameString);
}

//--------------------------------------------------------------------------------------
const char * LFO::getShapeNameIndexed_ptr(long inIndex)
{
	switch (inIndex)
	{
		case kLFOshape_sine                : return "sine";
		case kLFOshape_triangle            : return "triangle";
		case kLFOshape_square              : return "square";
		case kLFOshape_saw                 : return "sawtooth";
		case kLFOshape_reversesaw          : return "reverse sawtooth";
		case kLFOshape_thorn               : return "thorn";
		case kLFOshape_random              : return "random";
		case kLFOshape_randominterpolating : return "random interpolating";
		default:	return NULL;
	}
}

//--------------------------------------------------------------------------------------
void LFO::getShapeNameIndexed(long inIndex, char * outNameString)
{
	if (outNameString == NULL)
		return;

	const char * shapename = getShapeNameIndexed_ptr(inIndex);
	if (shapename == NULL)
		return;

	strcpy(outNameString, shapename);
}


//--------------------------------------------------------------------------------------
// this function points the LFO table pointers to the correct waveform tables
void LFO::pickTheLFOwaveform()
{
	switch (iShape)
	{
		case kLFOshape_sine:
			table = sineTable;
			break;
		case kLFOshape_triangle:
			table = triangleTable;
			break;
		case kLFOshape_square:
			table = squareTable;
			break;
		case kLFOshape_saw:
			table = sawTable;
			break;
		case kLFOshape_reversesaw:
			table = reverseSawTable;
			break;
		case kLFOshape_thorn:
			table = thornTable;
			break;
		default:
			table = sineTable;
			break;
	}
}

//--------------------------------------------------------------------------------------
// calculates the position within an LFO's cycle needed to sync to the song's beat
void LFO::syncToTheBeat(long inSamplesToBar)
{
	// calculate how many samples long the LFO cycle is
	float cyclesize = NUM_LFO_POINTS_FLOAT / stepSize;
	// calculate many more samples it will take for this cycle to coincide with the beat
	float countdown = fmodf( (float)inSamplesToBar,  cyclesize);
	// and convert that into the correct LFO position according to its table step size
	position = (cyclesize - countdown) * stepSize;
	// wrap around the new position if it is beyond the end of the LFO table
	if (position >= NUM_LFO_POINTS_FLOAT)
		position = fmodf(position, NUM_LFO_POINTS_FLOAT);
	else if (position < 0.0f)
		position = 0.0f;
}
