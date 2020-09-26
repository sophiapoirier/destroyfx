// Reverb model declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain


#pragma once


#include <vector>

#include "allpass.hpp"
#include "comb.hpp"
#include "dfxsmoothedvalue.h"
#include "tuning.h"


namespace freeverb
{


class ReverbModel
{
public:
    explicit ReverbModel(double sampleRate);

    void    clear();

    void    process(const float* inAudioL, const float* inAudioR, float* outAudioL, float* outAudioR, size_t frameCount);
    void    process(const float* inAudio, float* outAudio, size_t frameCount);

    void    setRoomSize(float amount);
    float   getRoomSize() const;
    void    setDamping(float amount);
    float   getDamping() const;
    void    setDryLevel(float amount);
    float   getDryLevel() const;
    void    setWetLevel(float amount);
    float   getWetLevel() const;
    void    setWidth(float amount);
    float   getWidth() const;
    void    setFreezeMode(bool enabled);
    bool    getFreezeMode() const;

private:
    float   mRoomSize = kRoomSizeDefault;
    float   mDampingNormalized = kDampingDefault;
    float   mDryLevel = kDryLevelDefault;
    float   mWetLevel = kWetLevelDefault;
    float   mWidth = kWidthDefault;
    bool    mFreezeMode = kFreezeModeDefault;

    DfxSmoothedValue<float> mRoomSizeSmoothed;
    DfxSmoothedValue<float> mDampingSmoothed;
    DfxSmoothedValue<float> mDryLevelSmoothed;
    DfxSmoothedValue<float> mWetLevelSmoothed;
    DfxSmoothedValue<float> mWidthSmoothed;
    DfxSmoothedValue<float> mInputGainSmoothed;

    std::vector<CombFilter> mCombFiltersL;
    std::vector<CombFilter> mCombFiltersR;

    std::vector<AllPassFilter> mAllPassFiltersL;
    std::vector<AllPassFilter> mAllPassFiltersR;
};


}
