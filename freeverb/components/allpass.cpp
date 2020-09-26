// all-pass filter implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain


#include "allpass.hpp"


namespace freeverb
{


AllPassFilter::AllPassFilter(double timeInSeconds, double sampleRate)
:   mBuffer(detail::secondsToSamples(timeInSeconds, sampleRate), 0.0f)
{
}


void AllPassFilter::clear()
{
    for (auto& value : mBuffer)
    {
        value = 0.0f;
    }
}


}
