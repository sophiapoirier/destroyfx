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

    void    clear() noexcept [[clang::nonblocking]];

    void    process(const float* inAudioL, const float* inAudioR, float* outAudioL, float* outAudioR, size_t frameCount) noexcept [[clang::nonblocking]];
    void    process(const float* inAudio, float* outAudio, size_t frameCount) noexcept [[clang::nonblocking]];

    void    setRoomSize(float amount) noexcept [[clang::nonblocking]];
    float   getRoomSize() const noexcept [[clang::nonblocking]];
    void    setDamping(float amount) noexcept [[clang::nonblocking]];
    float   getDamping() const noexcept [[clang::nonblocking]];
    void    setDryLevel(float amount) noexcept [[clang::nonblocking]];
    float   getDryLevel() const noexcept [[clang::nonblocking]];
    void    setWetLevel(float amount) noexcept [[clang::nonblocking]];
    float   getWetLevel() const noexcept [[clang::nonblocking]];
    void    setWidth(float amount) noexcept [[clang::nonblocking]];
    float   getWidth() const noexcept [[clang::nonblocking]];
    void    setFreezeMode(bool enabled) noexcept [[clang::nonblocking]];
    bool    getFreezeMode() const noexcept [[clang::nonblocking]];

private:
    float   mRoomSize = kRoomSizeDefault;
    float   mDampingNormalized = kDampingDefault;
    float   mDryLevel = kDryLevelDefault;
    float   mWetLevel = kWetLevelDefault;
    float   mWidth = kWidthDefault;
    bool    mFreezeMode = kFreezeModeDefault;

	dfx::SmoothedValue<float> mRoomSizeSmoothed;
    dfx::SmoothedValue<float> mDampingSmoothed;
    dfx::SmoothedValue<float> mDryLevelSmoothed;
    dfx::SmoothedValue<float> mWetLevelSmoothed;
    dfx::SmoothedValue<float> mWidthSmoothed;
    dfx::SmoothedValue<float> mInputGainSmoothed;

    std::vector<CombFilter> mCombFiltersL;
    std::vector<CombFilter> mCombFiltersR;

    std::vector<AllPassFilter> mAllPassFiltersL;
    std::vector<AllPassFilter> mAllPassFiltersR;
};


}
