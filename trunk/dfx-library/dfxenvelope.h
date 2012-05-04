/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2010  Sophia Poirier

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

#ifndef __DFX_ENVELOPE_H
#define __DFX_ENVELOPE_H


//-----------------------------------------------------------------------------
typedef enum {
	kDfxEnvState_Attack,
	kDfxEnvState_Decay,
	kDfxEnvState_Sustain,
	kDfxEnvState_Release,
	kDfxEnvState_Dormant
} DfxEnvState;

typedef enum {
	kDfxEnvCurveType_Linear,
	kDfxEnvCurveType_Cubed,
	kDfxEnvCurveType_NumTypes
} DfxEnvCurveType;


//-----------------------------------------------------------------------------
class DfxEnvelope
{
public:
	DfxEnvelope();

	void setParameters(double inAttackDur, double inDecayDur, double inSustainLevel, double inReleaseDur);
	void setSampleRate(double inSampleRate);
	void setCurveType(DfxEnvCurveType inCurveType)
		{	mCurveType = inCurveType;	}
	DfxEnvCurveType getCurveType()
		{	return mCurveType;	}
	void setResumedAttackMode(bool inMode)
		{	mResumedAttackMode = inMode;	}
	bool getResumedAttackMode()
		{	return mResumedAttackMode;	}
	DfxEnvState getState()
		{	return mState;	}
	void setInactive();
	bool isInactive();

	void beginAttack();
	void beginRelease();
	double process();

	double calculateRise(unsigned long inPos, unsigned long inLength);
	double calculateRise(double inPosNormalized);
	double calculateFall(unsigned long inPos, unsigned long inLength);
	double calculateFall(double inPosNormalized);
	double deriveAttackPosFromEnvValue(double inValue);

private:
	double mAttackDur, mDecayDur, mSustainLevel, mReleaseDur;
	DfxEnvCurveType mCurveType;
	bool mResumedAttackMode;
	double mSampleRate;

	DfxEnvState mState;
	double mLastValue, mStartValue, mTargetValue;
	unsigned long mSectionPos, mSectionLength;
	double mSectionLength_inv;
};


#endif
