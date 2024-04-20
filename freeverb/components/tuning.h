// Reverb model tuning values
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain


#pragma once


#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>


namespace freeverb
{


static constexpr size_t kNumCombFilters     = 8;
static constexpr size_t kNumAllPassFilters  = 4;
static constexpr float  kAllPassFeedback    = 0.5f;

static constexpr float  kInputGainScale     = 0.015f;
static constexpr float  kDampingScale       = 0.4f;
static constexpr float  kRoomSizeMin        = 0.7f;
static constexpr float  kRoomSizeMax        = kRoomSizeMin + 0.28f;
static constexpr float  kWetLevelMax        = 3.0f;
static constexpr float  kDryLevelMax        = 2.0f;

static constexpr float  kRoomSizeDefault    = ((kRoomSizeMax - kRoomSizeMin) * 0.5f) + kRoomSizeMin;
static constexpr float  kDampingDefault     = 0.5f;
static constexpr float  kDryLevelDefault    = 1.0f;
static constexpr float  kWetLevelDefault    = 0.5f;
static constexpr float  kWidthDefault       = 1.0f;
static constexpr bool   kFreezeModeDefault  = false;


// The original author's values assume 44.1 kHz sample rate, 
// so translate them to seconds to adapt to any sample rate.
// The values were obtained by listening tests.
namespace detail
{
    static consteval double referenceSamplesToSeconds(size_t timeInSamples)
    {
        constexpr double referenceSampleRate = 44100.0;
        return static_cast<double>(timeInSamples) / referenceSampleRate;
    }
    static inline size_t secondsToSamples(double timeInSeconds, double sampleRate)
    {
        return static_cast<size_t>( std::max(std::lrint(timeInSeconds * sampleRate), 0L) );
    }
}

static constexpr auto kStereoSpread = detail::referenceSamplesToSeconds(23);

static constexpr auto kCombTuningL = std::to_array(
{
    detail::referenceSamplesToSeconds(1116),
    detail::referenceSamplesToSeconds(1188),
    detail::referenceSamplesToSeconds(1277),
    detail::referenceSamplesToSeconds(1356),
    detail::referenceSamplesToSeconds(1422),
    detail::referenceSamplesToSeconds(1491),
    detail::referenceSamplesToSeconds(1557),
    detail::referenceSamplesToSeconds(1617)
});
static_assert(kCombTuningL.size() == kNumCombFilters);

static constexpr auto kAllPassTuningL = std::to_array(
{
    detail::referenceSamplesToSeconds(556),
    detail::referenceSamplesToSeconds(441),
    detail::referenceSamplesToSeconds(341),
    detail::referenceSamplesToSeconds(225)
});
static_assert(kAllPassTuningL.size() == kNumAllPassFilters);


}
