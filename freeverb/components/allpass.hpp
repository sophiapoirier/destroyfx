// all-pass filter declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain


#pragma once


#include <stddef.h>
#include <vector>

#include "tuning.h"


namespace freeverb
{


class AllPassFilter
{
public:
                    AllPassFilter(double timeInSeconds, double sampleRate);

    void            clear() noexcept [[clang::nonblocking]];
    inline float    process(float inputAudioSample) noexcept [[clang::nonblocking]];

private:
    std::vector<float> mBuffer;
    size_t mBufferIndex = 0;
};


inline float AllPassFilter::process(float inputAudioSample) noexcept [[clang::nonblocking]]
{
    const auto bufferedValue = mBuffer[mBufferIndex];

    mBuffer[mBufferIndex] = inputAudioSample + (bufferedValue * kAllPassFeedback);

    mBufferIndex++;
    if (mBufferIndex >= mBuffer.size())
    {
        mBufferIndex = 0;
    }

    return bufferedValue - inputAudioSample;
}


}
