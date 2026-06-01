// all-pass filter implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain


#include "allpass.hpp"

#include <algorithm>


namespace freeverb
{


AllPassFilter::AllPassFilter(double timeInSeconds, double sampleRate)
:   mBuffer(detail::secondsToSamples(timeInSeconds, sampleRate), 0.f)
{
}


void AllPassFilter::clear() noexcept [[clang::nonblocking]]
{
	std::ranges::fill(mBuffer, 0.f);
}


}
