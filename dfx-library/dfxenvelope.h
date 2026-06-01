/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2010-2026  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org
---------------------------------------------------------------*/

#pragma once


#include <cstddef>
#include <utility>

#include "dfx-base.h"
#include "iirfilter.h"


//-----------------------------------------------------------------------------
class DfxEnvelope
{
public:
	enum class Phase
	{
		Attack,
		Decay,
		Sustain,
		Release,
		Dormant
	};

	enum CurveType
	{
		kCurveType_Linear,
		kCurveType_Cubed,
		kCurveType_NumTypes
	};

	void setParameters(double inAttackDur, double inDecayDur, double inSustainLevel, double inReleaseDur) noexcept DFX_RT_ATTR;
	void setSampleRate(double inSampleRate) noexcept DFX_RT_ATTR;
	void setCurveType(CurveType inCurveType) noexcept DFX_RT_ATTR
	{
		mCurveType = inCurveType;
	}
	CurveType getCurveType() const noexcept DFX_RT_ATTR
	{
		return mCurveType;
	}
	void setResumedAttackMode(bool inMode) noexcept DFX_RT_ATTR
	{
		mResumedAttackMode = inMode;
	}
	bool isResumedAttackMode() const noexcept DFX_RT_ATTR
	{
		return mResumedAttackMode;
	}
	Phase getPhase() const noexcept DFX_RT_ATTR
	{
		return mPhase;
	}
	bool isAttackPhase() const noexcept DFX_RT_ATTR
	{
		return mPhase == Phase::Attack;
	}
	bool isDecayPhase() const noexcept DFX_RT_ATTR
	{
		return mPhase == Phase::Decay;
	}
	bool isSustainPhase() const noexcept DFX_RT_ATTR
	{
		return mPhase == Phase::Sustain;
	}
	bool isReleasePhase() const noexcept DFX_RT_ATTR
	{
		return mPhase == Phase::Release;
	}
	bool isDormantPhase() const noexcept DFX_RT_ATTR
	{
		return mPhase == Phase::Dormant;
	}
	void setInactive() noexcept DFX_RT_ATTR;
	bool isActive() const noexcept DFX_RT_ATTR;

	void beginAttack() noexcept DFX_RT_ATTR;
	void beginRelease() noexcept DFX_RT_ATTR;
	[[nodiscard]] double process() noexcept DFX_RT_ATTR;
	// returns the filter coefficients needed for lowpass gating as well as
	// a post-filter gain to prevent closed-filter audio leakage
	[[nodiscard]] std::pair<dfx::IIRFilter::Coefficients, float> processLowpassGate() noexcept DFX_RT_ATTR;

private:
	double calculateRise(size_t inPos, size_t inLength) const noexcept DFX_RT_ATTR;
	double calculateRise(double inPosNormalized) const noexcept DFX_RT_ATTR;
	double calculateFall(size_t inPos, size_t inLength) const noexcept DFX_RT_ATTR;
	double calculateFall(double inPosNormalized) const noexcept DFX_RT_ATTR;
	double deriveAttackPosFromEnvValue(double inValue) const noexcept DFX_RT_ATTR;
	// maps the envelope gain level returned by process to lowpass coefficients
	[[nodiscard]] dfx::IIRFilter::Coefficients getLowpassGateCoefficients(double inLevel) const noexcept DFX_RT_ATTR;

	double mAttackDur = 0.0, mDecayDur = 0.0, mSustainLevel = 1.0, mReleaseDur = 0.0;
	CurveType mCurveType = kCurveType_Cubed;
	bool mResumedAttackMode = false;
	double mSampleRate = 1.0;

	Phase mPhase = Phase::Attack;
	double mLastValue = 0., mStartValue = 0., mTargetValue = 1.;
	size_t mSectionPos = 0, mSectionLength = 0;
	double mSectionLength_inv = 1.;
};
