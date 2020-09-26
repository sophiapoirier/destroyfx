// comb filter class declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain


#pragma once


#include <stddef.h>
#include <vector>


namespace freeverb
{


class CombFilter
{
public:
                    CombFilter(double timeInSeconds, double sampleRate);

    void            clear();
    inline float    process(float inputAudioSample, float feedback, float damping);

private:
    float   mFilterHistory = 0.0f;
    std::vector<float> mBuffer;
    size_t  mBufferIndex = 0;
};


inline float CombFilter::process(float inputAudioSample, float feedback, float damping)
{
    const auto output = mBuffer[mBufferIndex];

    mFilterHistory = (output * (1.0f - damping)) + (mFilterHistory * damping);

    mBuffer[mBufferIndex] = inputAudioSample + (mFilterHistory * feedback);

    mBufferIndex++;
    if (mBufferIndex >= mBuffer.size())
    {
        mBufferIndex = 0;
    }

    return output;
}


}
