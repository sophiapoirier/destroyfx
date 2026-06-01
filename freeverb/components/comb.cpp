// comb filter implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain


#include "comb.hpp"

#include <algorithm>

#include "tuning.h"


namespace freeverb
{


CombFilter::CombFilter(double timeInSeconds, double sampleRate)
:   mBuffer(detail::secondsToSamples(timeInSeconds, sampleRate), 0.f)
{
}


void CombFilter::clear() noexcept [[clang::nonblocking]]
{
	std::ranges::fill(mBuffer, 0.f);
    mFilterHistory = 0.f;
}


}
