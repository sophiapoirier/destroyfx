/*---------------------------------------------------------------
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
---------------------------------------------------------------*/

#pragma once


#include <stddef.h>


//-----------------------------------------------------------------------------
class DfxEnvelope
{
public:
	enum class State
	{
		Attack,
		Decay,
		Sustain,
		Release,
		Dormant
	};

	enum class CurveType
	{
		Linear,
		Cubed,
		NumTypes
	};

	void setParameters(double inAttackDur, double inDecayDur, double inSustainLevel, double inReleaseDur);
	void setSampleRate(double inSampleRate);
	void setCurveType(CurveType inCurveType) noexcept
	{
		mCurveType = inCurveType;
	}
	CurveType getCurveType() const noexcept
	{
		return mCurveType;
	}
	void setResumedAttackMode(bool inMode) noexcept
	{
		mResumedAttackMode = inMode;
	}
	bool isResumedAttackMode() const noexcept
	{
		return mResumedAttackMode;
	}
	State getState() const noexcept
	{
		return mState;
	}
	void setInactive() noexcept;
	bool isInactive() const noexcept;

	void beginAttack();
	void beginRelease();
	double process();

private:
	double calculateRise(size_t inPos, size_t inLength) const;
	double calculateRise(double inPosNormalized) const;
	double calculateFall(size_t inPos, size_t inLength) const;
	double calculateFall(double inPosNormalized) const;
	double deriveAttackPosFromEnvValue(double inValue) const;

	double mAttackDur = 0.0, mDecayDur = 0.0, mSustainLevel = 1.0, mReleaseDur = 0.0;
	CurveType mCurveType = CurveType::Cubed;
	bool mResumedAttackMode = false;
	double mSampleRate = 1.0;

	State mState = State::Attack;
	double mLastValue = 0.0, mStartValue = 0.0, mTargetValue = 1.0;
	size_t mSectionPos = 0, mSectionLength = 0;
	double mSectionLength_inv = 1.0;
};
