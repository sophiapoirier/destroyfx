/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2010-2015  Sophia Poirier

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
---------------------------------------------------------------*/


#include "dfxenvelope.h"
#include <math.h>


//-----------------------------------------------------------------------------
DfxEnvelope::DfxEnvelope()
{
	mAttackDur = mDecayDur = mReleaseDur = 0.0;
	mSustainLevel = 1.0;
	mCurveType = kDfxEnvCurveType_Cubed;
	mResumedAttackMode = false;
	mSampleRate = 1.0;

	mState = kDfxEnvState_Attack;
	mLastValue = 0.0;
	mStartValue = 0.0;
	mTargetValue = 1.0;
	mSectionPos = 0;
	mSectionLength = 0;
	mSectionLength_inv = 1.0;
}

//-----------------------------------------------------------------------------
void DfxEnvelope::setParameters(double inAttackDur, double inDecayDur, double inSustainLevel, double inReleaseDur)
{
	mAttackDur = inAttackDur;
	mDecayDur = inDecayDur;
	mSustainLevel = inSustainLevel;
	mReleaseDur = inReleaseDur;

	if (mAttackDur < 0.0)
		mAttackDur = 0.0;
	if (mDecayDur < 0.0)
		mDecayDur = 0.0;
	if (mReleaseDur < 0.0)
		mReleaseDur = 0.0;
	if (mSustainLevel >= 1.0)
	{
		mSustainLevel = 1.0;
		mDecayDur = 0.0;
	}
	else if (mSustainLevel < 0.0)
	{
		mSustainLevel = 0.0;
		mReleaseDur = 0.0;
	}
}

//-----------------------------------------------------------------------------
void DfxEnvelope::setSampleRate(double inSampleRate)
{
	double prevSampleRate = mSampleRate;
	mSampleRate = inSampleRate;
	if ( (prevSampleRate != inSampleRate) && (prevSampleRate != 0.0) )
	{
		double srScalar = inSampleRate / prevSampleRate;
		mSectionPos = lround((double)mSectionPos * srScalar);
		mSectionLength = lround((double)mSectionLength * srScalar);
		if (mSectionLength > 0)
			mSectionLength_inv = 1.0 / (double)mSectionLength;
	}
}

//-----------------------------------------------------------------------------
void DfxEnvelope::setInactive()
{
	mState = kDfxEnvState_Dormant;
}

//-----------------------------------------------------------------------------
bool DfxEnvelope::isInactive()
{
	return (getState() == kDfxEnvState_Dormant);
}

//-----------------------------------------------------------------------------
void DfxEnvelope::beginAttack()
{
	DfxEnvState prevState = mState;
	mState = kDfxEnvState_Attack;
	mSectionPos = 0;
	mSectionLength = lround(mAttackDur * mSampleRate);
	if (mSectionLength > 0)
	{
		mSectionLength_inv = 1.0 / (double)mSectionLength;
		mStartValue = 0.0;
		mTargetValue = 1.0;
		if ( mResumedAttackMode && (prevState == kDfxEnvState_Release) )
		{
			mSectionPos = lround(deriveAttackPosFromEnvValue(mLastValue) * (double)mSectionLength);
		}
	}
	else
	{
		mSectionLength_inv = 1.0;
		process();	// do this to move ahead to the next envelope segment, since there is no attack
	}
}

//-----------------------------------------------------------------------------
void DfxEnvelope::beginRelease()
{
	mState = kDfxEnvState_Release;
	mSectionPos = 0;
	mSectionLength = lround(mReleaseDur * mSampleRate);
	if (mSectionLength > 0)
	{
		mSectionLength_inv = 1.0 / (double)mSectionLength;
		mStartValue = mLastValue;
		mTargetValue = 0.0;
	}
	else
	{
		mState = kDfxEnvState_Dormant;
	}
}

//-----------------------------------------------------------------------------
double DfxEnvelope::process()
{
	double outputValue = 0.0;

	switch (mState)
	{
		case kDfxEnvState_Attack:
			outputValue = calculateRise( (double)(mSectionPos+1) * mSectionLength_inv );
			mSectionPos++;
			if (mSectionPos >= mSectionLength)
			{
				mSectionPos = 0;
				mState = kDfxEnvState_Decay;
				mSectionLength = lround(mDecayDur * mSampleRate);
				if (mSectionLength > 0)
				{
					mSectionLength_inv = 1.0 / (double)mSectionLength;
					mStartValue = 1.0;
					mTargetValue = mSustainLevel;
				}
				else
				{
					mState = kDfxEnvState_Sustain;
				}
			}
			break;

		case kDfxEnvState_Decay:
			outputValue = calculateFall( (double)(mSectionPos+1) * mSectionLength_inv );
			mSectionPos++;
			if (mSectionPos >= mSectionLength)
			{
				mSectionPos = 0;
				mState = kDfxEnvState_Sustain;
			}
			break;

		case kDfxEnvState_Sustain:
			outputValue = mSustainLevel;
			break;

		case kDfxEnvState_Release:
			outputValue = calculateFall( (double)(mSectionPos+1) * mSectionLength_inv );
			mSectionPos++;
			if (mSectionPos >= mSectionLength)
			{
				mSectionPos = 0;
				mState = kDfxEnvState_Dormant;
			}
			break;

		case kDfxEnvState_Dormant:
		default:
			outputValue = 0.0;
			break;
	}

	mLastValue = outputValue;
	return outputValue;
}

//-----------------------------------------------------------------------------
double DfxEnvelope::calculateRise(unsigned long inPos, unsigned long inLength)
{
	if (inLength <= 0)
		return 0.0;
	return calculateRise( (double)(inPos+1) / (double)inLength );
}

//-----------------------------------------------------------------------------
double DfxEnvelope::calculateRise(double inPosNormalized)
{
	double outputValue = inPosNormalized;

	if (mCurveType == kDfxEnvCurveType_Cubed)
	{
		outputValue = 1.0 - outputValue;
		outputValue = outputValue * outputValue * outputValue;
		outputValue = 1.0 - outputValue;
	}

	// sine fade (stupendously inefficient)
//	outputValue = ( sin((outputValue*kDFX_PI_d)-(kDFX_PI_d*0.5)) + 1.0 ) * 0.5;

//	outputValue = (outputValue * (mTargetValue - mStartValue)) + mStartValue;

	return outputValue;
}

//-----------------------------------------------------------------------------
double DfxEnvelope::calculateFall(unsigned long inPos, unsigned long inLength)
{
	if (inLength <= 0)
		return 0.0;
	return calculateFall( (double)(inPos+1) / (double)inLength );
}

//-----------------------------------------------------------------------------
double DfxEnvelope::calculateFall(double inPosNormalized)
{
	double outputValue = 1.0 - inPosNormalized;

	if (mCurveType == kDfxEnvCurveType_Cubed)
	{
		outputValue = outputValue * outputValue * outputValue;
	}

	outputValue = (outputValue * (mStartValue - mTargetValue)) + mTargetValue;

	return outputValue;
}

//-----------------------------------------------------------------------------
double DfxEnvelope::deriveAttackPosFromEnvValue(double inValue)
{
	if (mCurveType == kDfxEnvCurveType_Cubed)
	{
		double outputValue = 1.0 - inValue;
		outputValue = cbrt(outputValue);
		outputValue = 1.0 - outputValue;
		return outputValue;
	}
	else
		return inValue;
}
