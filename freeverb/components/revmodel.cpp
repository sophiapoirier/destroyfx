// Reverb model implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain


#include "revmodel.hpp"

#include <algorithm>


namespace freeverb
{


ReverbModel::ReverbModel(double sampleRate)
{
    for (const auto combTuningL : kCombTuningL)
    {
        mCombFiltersL.emplace_back(combTuningL, sampleRate);
        mCombFiltersR.emplace_back(combTuningL + kStereoSpread, sampleRate);
    }
    for (const auto allPassTuningL : kAllPassTuningL)
    {
        mAllPassFiltersL.emplace_back(allPassTuningL, sampleRate);
        mAllPassFiltersR.emplace_back(allPassTuningL + kStereoSpread, sampleRate);
    }

    mRoomSizeSmoothed.setSampleRate(sampleRate);
    mDampingSmoothed.setSampleRate(sampleRate);
    mDryLevelSmoothed.setSampleRate(sampleRate);
    mWetLevelSmoothed.setSampleRate(sampleRate);
    mWidthSmoothed.setSampleRate(sampleRate);
    mInputGainSmoothed.setSampleRate(sampleRate);
}


void ReverbModel::clear()
{
    const auto clearItem = [](auto& item)
    {
        item.clear();
    };
    std::for_each(mCombFiltersL.begin(), mCombFiltersL.end(), clearItem);
    std::for_each(mCombFiltersR.begin(), mCombFiltersR.end(), clearItem);
    std::for_each(mAllPassFiltersL.begin(), mAllPassFiltersL.end(), clearItem);
    std::for_each(mAllPassFiltersR.begin(), mAllPassFiltersR.end(), clearItem);
}


void ReverbModel::process(const float* inAudioL, const float* inAudioR, float* outAudioL, float* outAudioR, size_t frameCount)
{
    mRoomSizeSmoothed.setValue(mFreezeMode ? 1.0f : mRoomSize);
    mDampingSmoothed.setValue(mFreezeMode ? 0.0f : (mDampingNormalized * kDampingScale));
    mInputGainSmoothed.setValue(mFreezeMode ? 0.0f : kInputGainScale);
    mDryLevelSmoothed.setValue(mDryLevel);
    mWetLevelSmoothed.setValue(mWetLevel);
    mWidthSmoothed.setValue(mWidth);

    for (size_t samp = 0; samp < frameCount; samp++)
    {
        const float inputValue = (inAudioL[samp] + inAudioR[samp]) * mInputGainSmoothed.getValue();
        float outputValueL = 0.0f, outputValueR = 0.0f;

        // accumulate comb filters in parallel
        for (auto& filter : mCombFiltersL)
        {
            outputValueL += filter.process(inputValue, mRoomSizeSmoothed.getValue(), mDampingSmoothed.getValue());
        }
        for (auto& filter : mCombFiltersR)
        {
            outputValueR += filter.process(inputValue, mRoomSizeSmoothed.getValue(), mDampingSmoothed.getValue());
        }

        // feed through all-passes in series
        for (auto& filter : mAllPassFiltersL)
        {
            outputValueL = filter.process(outputValueL);
        }
        for (auto& filter : mAllPassFiltersR)
        {
            outputValueR = filter.process(outputValueR);
        }

        const float wetFactor1 = mWetLevelSmoothed.getValue() * ((mWidthSmoothed.getValue() * 0.5f) + 0.5f);
        const float wetFactor2 = mWetLevelSmoothed.getValue() * ((1.0f - mWidthSmoothed.getValue()) * 0.5f);
        outAudioL[samp] = (outputValueL * wetFactor1) + (outputValueR * wetFactor2) + (inAudioL[samp] * mDryLevelSmoothed.getValue());
        outAudioR[samp] = (outputValueR * wetFactor1) + (outputValueL * wetFactor2) + (inAudioR[samp] * mDryLevelSmoothed.getValue());

        mRoomSizeSmoothed.inc();
        mDampingSmoothed.inc();
        mDryLevelSmoothed.inc();
        mWetLevelSmoothed.inc();
        mWidthSmoothed.inc();
        mInputGainSmoothed.inc();
    }
}


void ReverbModel::process(const float* inAudio, float* outAudio, size_t frameCount)
{
    mRoomSizeSmoothed.setValue(mFreezeMode ? 1.0f : mRoomSize);
    mDampingSmoothed.setValue(mFreezeMode ? 0.0f : (mDampingNormalized * kDampingScale));
    mInputGainSmoothed.setValue(mFreezeMode ? 0.0f : kInputGainScale);
    mDryLevelSmoothed.setValue(mDryLevel);
    mWetLevelSmoothed.setValue(mWetLevel);

    for (size_t samp = 0; samp < frameCount; samp++)
    {
        const float inputValue = inAudio[samp] * mInputGainSmoothed.getValue();
        float outputValue = 0.0f;

        for (auto& filter : mCombFiltersL)
        {
            outputValue += filter.process(inputValue, mRoomSizeSmoothed.getValue(), mDampingSmoothed.getValue());
        }
        for (auto& filter : mAllPassFiltersL)
        {
            outputValue = filter.process(outputValue);
        }

        outAudio[samp] = (outputValue * mWetLevelSmoothed.getValue()) + (inAudio[samp] * mDryLevelSmoothed.getValue());

        mRoomSizeSmoothed.inc();
        mDampingSmoothed.inc();
        mDryLevelSmoothed.inc();
        mWetLevelSmoothed.inc();
        mInputGainSmoothed.inc();
    }
}


void ReverbModel::setRoomSize(float amount)
{
    mRoomSize = amount;
}


float ReverbModel::getRoomSize() const
{
    return mRoomSize;
}


void ReverbModel::setDamping(float amount)
{
    mDampingNormalized = amount;
}


float ReverbModel::getDamping() const
{
    return mDampingNormalized;
}


void ReverbModel::setDryLevel(float amount)
{
    mDryLevel = amount;
}


float ReverbModel::getDryLevel() const
{
    return mDryLevel;
}


void ReverbModel::setWetLevel(float amount)
{
    mWetLevel = amount;
}


float ReverbModel::getWetLevel() const
{
    return mWetLevel;
}


void ReverbModel::setWidth(float amount)
{
    mWidth = amount;
}


float ReverbModel::getWidth() const
{
    return mWidth;
}


void ReverbModel::setFreezeMode(bool enabled)
{
    mFreezeMode = enabled;
}


bool ReverbModel::getFreezeMode() const
{
    return mFreezeMode;
}


}
