// comb filter implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain


#include "comb.hpp"

#include "tuning.h"


namespace freeverb
{


CombFilter::CombFilter(double timeInSeconds, double sampleRate)
:   mBuffer(detail::secondsToSamples(timeInSeconds, sampleRate), 0.0f)
{
}


void CombFilter::clear()
{
    for (auto& value : mBuffer)
    {
        value = 0.0f;
    }
    mFilterHistory = 0.0f;
}


}
