/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Transverb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Transverb.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "transverb.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>

#include "dfxmath.h"
#include "firfilter.h"


using namespace dfx::TV;



void TransverbDSP::process(std::span<float const> inAudio, std::span<float> outAudio) {

  auto const bsize_float = static_cast<double>(bsize);  // cut down on casting
  auto const quality = getparameter_i(kQuality);
  auto const tomsound = getparameter_b(kTomsound);
  auto const freeze = getparameter_b(kFreeze);
  int const writerIncrement = freeze ? 0 : 1;
  auto const attenuateFeedbackByMixLevel = getparameter_b(kAttenuateFeedbackByMixLevel);
  std::array<float, kNumDelays> delayvals {};  // delay buffer output values


  /////////////   S O P H I A S O U N D   //////////////
  // do it proper
  if (!tomsound) {

    auto const samplerate = getsamplerate();
    auto const speedSmoothingStride = dfx::math::GetFrequencyBasedSmoothingStride(samplerate);
    // int versions of these float values, for reducing casting operations
    std::array<int, kNumDelays> speed_ints {};
    // position trackers for the lowpass filters
    std::array<int, kNumDelays> lowpasspos {};
    // the type of filtering to use in ultra hi-fi mode
    std::array<FilterMode, kNumDelays> filtermodes {};
    filtermodes.fill(FilterMode::None);
    // make-up gain for lowpass filtering
    std::array<float, kNumDelays> mugs {};
    mugs.fill(1.f);

    for (size_t i = 0; i < outAudio.size(); i++)  // samples loop
    {
      bool const firstSample = (i == 0);
      bool const speedSmoothingStrideHit = ((i % speedSmoothingStride) == 0);
      float delaysum = 0.f;

      for (size_t h = 0; h < kNumDelays; h++)  // delay heads loop
      {
        auto const read_int = static_cast<int>(heads[h].read);
        auto const reverseread = (distchangemode == kDistChangeMode_Reverse) && heads[h].targetdist.has_value();
        auto const distcatchup = (distchangemode == kDistChangeMode_AdHocVarispeed) && heads[h].targetdist.has_value();
        auto const speed = heads[h].speed.getValue() * (distcatchup ? heads[h].distspeedfactor : 1.);

        // filter setup
        if (quality == kQualityMode_UltraHiFi)
        {
          if (firstSample || (heads[h].speed.isSmoothing() && speedSmoothingStrideHit))
          {
            lowpasspos[h] = read_int;
            // check to see if we need to lowpass the first delay head and init coefficients if so
            if (speed > kUnitySpeed)
            {
              filtermodes[h] = FilterMode::LowpassIIR;
              speed_ints[h] = static_cast<int>(speed);
              // it becomes too costly to try to IIR at higher speeds, so switch to FIR filtering
              if (speed >= kFIRSpeedThreshold)
              {
                filtermodes[h] = FilterMode::LowpassFIR;
                // compensate for gain lost from filtering
                mugs[h] = static_cast<float>(std::pow(speed / kFIRSpeedThreshold, 0.78));
                // update the coefficients only if necessary
                if (std::exchange(heads[h].speedHasChanged, false))
                {
                  dfx::FIRFilter::calculateIdealLowpassCoefficients((samplerate / speed) * dfx::FIRFilter::kShelfStartLowpass,
                                                                    samplerate, heads[h].firCoefficients, firCoefficientsWindow);
                  heads[h].filter.reset();
                }
              }
              else if (std::exchange(heads[h].speedHasChanged, false))
              {
                heads[h].filter.setLowpassCoefficients((samplerate / speed) * dfx::IIRFilter::kShelfStartLowpass);
              }
            }
            // we need to highpass the delay head to remove mega sub bass
            else
            {
              filtermodes[h] = FilterMode::Highpass;
              if (std::exchange(heads[h].speedHasChanged, false))
              {
                heads[h].filter.setHighpassCoefficients(kHighpassFilterCutoff / speed);
              }
            }
          }
        }

        // read from read heads
        switch (quality)
        {
          // no interpolation or filtering
          case kQualityMode_DirtFi:
          default:
            delayvals[h] = heads[h].buf[read_int];
            break;
          // spline interpolation, but no filtering
          case kQualityMode_HiFi:
            //delayvals[h] = interpolateLinear(std::span(heads[h].buf).subspan(0, bsize), heads[h].read, writer);
            delayvals[h] = interpolateHermite(std::span(heads[h].buf).subspan(0, bsize), heads[h].read, writer);
            break;
          // spline interpolation plus anti-aliasing lowpass filtering for high speeds
          // or sub-bass-removing highpass filtering for low speeds
          case kQualityMode_UltraHiFi:
            switch (filtermodes[h])
            {
              case FilterMode::Highpass:
              case FilterMode::LowpassIIR:
                // interpolate the values in the IIR output history
                delayvals[h] = heads[h].filter.interpolateHermitePostFilter(heads[h].read);
                break;
              case FilterMode::LowpassFIR:
              {
                // get two consecutive FIR output values for linear interpolation
                auto const lp1 = dfx::FIRFilter::process(std::span(heads[h].buf).subspan(0, bsize), heads[h].firCoefficients,
                                                         mod_bipolar(read_int - static_cast<int>(kNumFIRTaps), bsize));
                auto const lp2 = dfx::FIRFilter::process(std::span(heads[h].buf).subspan(0, bsize), heads[h].firCoefficients,
                                                         mod_bipolar(read_int - static_cast<int>(kNumFIRTaps) + 1, bsize));
                // interpolate output linearly (avoid shit sound) and compensate gain
                delayvals[h] = interpolateLinear(lp1, lp2, heads[h].read) * mugs[h];
                break;
              }
              default:
                delayvals[h] = interpolateHermite(std::span(heads[h].buf).subspan(0, bsize), heads[h].read, writer);
                break;
            }
            break;
        }  // end of quality switch

        // crossfade the last stored smoothing sample with
        // the current sample if smoothing is in progress
        if (heads[h].smoothcount > 0) {
          auto const smoothpos = heads[h].smoothstep * static_cast<float>(heads[h].smoothcount);
          delayvals[h] = std::lerp(delayvals[h], heads[h].lastdelayval, smoothpos);
          heads[h].smoothcount--;
        }

        // then write into buffer (w/ feedback)
        if (!freeze) {
          float const mixlevel = attenuateFeedbackByMixLevel ? heads[h].mix.getValue() : 1.f;
          heads[h].buf[writer] = inAudio[i] + (delayvals[h] * heads[h].feed.getValue() * mixlevel);
        }

        // make output
        delaysum += delayvals[h] * heads[h].mix.getValue();

        // start smoothing if the writer has passed a reader or vice versa,
        // though not if reader and writer move at the same speed
        // (check the positions before wrapping around the heads)
        auto const nextRead = static_cast<int>(heads[h].read + speed);
        auto const nextWrite = writer + 1;
        bool const readCrossingAhead = (read_int < writer) && (nextRead >= nextWrite);
        bool const readCrossingBehind = (read_int >= writer) && (nextRead <= nextWrite);
        bool const speedIsUnity = (speed == kUnitySpeed);
        if ((readCrossingAhead || readCrossingBehind) && !speedIsUnity) {
          // check because, at slow speeds, it's possible to go into this twice or more in a row
          if (heads[h].smoothcount <= 0) {
            // store the most recent output as the head's smoothing sample
            heads[h].lastdelayval = delayvals[h];
            // truncate the smoothing duration if we're using too small of a buffer size
            auto const bufferReadSteps = static_cast<int>(bsize_float / speed);
            auto const smoothdur = std::min(bufferReadSteps, kAudioSmoothingDur_samples);
            heads[h].smoothstep = 1.f / static_cast<float>(smoothdur);  // the scalar step value
            heads[h].smoothcount = smoothdur;  // set the counter to the total duration
          }
        }

        // update read heads, wrapping around if they have gone past the end of the buffer
        if (reverseread) {
          heads[h].read -= speed;
          while (heads[h].read < 0.) {
            heads[h].read += bsize_float;
          }
        } else {
          heads[h].read += speed;
          if (heads[h].read >= bsize_float) {
            heads[h].read = std::fmod(heads[h].read, bsize_float);
          }
        }
        if (distcatchup || reverseread) {
          if (std::fabs(getdist(heads[h].read) - *heads[h].targetdist) < speed) {
            heads[h].targetdist.reset();
          }
        }

        // if we're doing IIR lowpass filtering,
        // then we probably need to process a few consecutive samples in order
        // to get the continuous impulse (or whatever you call that),
        // probably whatever the speed multiplier is, that's how many samples
        if (filtermodes[h] == FilterMode::LowpassIIR)
        {
          int lowpasscount = 0;
          int const direction = reverseread ? -1 : 1;
          while (lowpasscount < speed_ints[h])
          {
            switch (speed_ints[h] - lowpasscount)
            {
              case 1:
                heads[h].filter.processToCacheH1(heads[h].buf[lowpasspos[h]]);
                lowpasspos[h] = mod_bipolar(lowpasspos[h] + (1 * direction), bsize);
                lowpasscount++;
                break;
              case 2:
                heads[h].filter.processToCacheH2(std::span(heads[h].buf).subspan(0, bsize), dfx::math::ToUnsigned(lowpasspos[h]));
                lowpasspos[h] = mod_bipolar(lowpasspos[h] + (2 * direction), bsize);
                lowpasscount += 2;
                break;
              case 3:
                heads[h].filter.processToCacheH3(std::span(heads[h].buf).subspan(0, bsize), dfx::math::ToUnsigned(lowpasspos[h]));
                lowpasspos[h] = mod_bipolar(lowpasspos[h] + (3 * direction), bsize);
                lowpasscount += 3;
                break;
              default:
                heads[h].filter.processToCacheH4(std::span(heads[h].buf).subspan(0, bsize), dfx::math::ToUnsigned(lowpasspos[h]));
                lowpasspos[h] = mod_bipolar(lowpasspos[h] + (4 * direction), bsize);
                lowpasscount += 4;
                break;
            }
          }
          auto const nextread_int = static_cast<int>(heads[h].read);
          // check whether we need to consume one more sample
          bool const extrasample = [=, this] {
            if (reverseread) {
              return ((lowpasspos[h] > nextread_int) && ((lowpasspos[h] - 1) == nextread_int)) ||
                     ((lowpasspos[h] == 0) && (nextread_int == (bsize - 1)));
            }
            return ((lowpasspos[h] < nextread_int) && ((lowpasspos[h] + 1) == nextread_int)) ||
                   ((lowpasspos[h] == (bsize - 1)) && (nextread_int == 0));
          }();
          if (extrasample)
          {
            heads[h].filter.processToCacheH1(heads[h].buf[lowpasspos[h]]);
            lowpasspos[h] = mod_bipolar(lowpasspos[h] + (1 * direction), bsize);
          }
        }
        // it's simpler for highpassing;
        // we may not even need to process anything for this sample
        else if (filtermodes[h] == FilterMode::Highpass)
        {
          // only if we've traversed to a new integer sample position
          if (static_cast<int>(heads[h].read) != read_int)
          {
            heads[h].filter.processToCache(heads[h].buf[read_int]);
          }
        }

        heads[h].speedHasChanged |= heads[h].speed.isSmoothing();
      }  // end of delay heads loop

      // mix output
      outAudio[i] = (inAudio[i] * drymix.getValue()) + delaysum;

      // update write head
      writer += writerIncrement;
      // wrap around the write head if it has gone past the end of the buffer
      writer %= bsize;

      incrementSmoothedAudioValues();
    }  // end of samples loop

  }  // end of !TOMSOUND



  /////////////   T O M S O U N D   //////////////
  else {
    // the essense of TOMSOUND comes from the error that Tom made
    // of putting the channels loop within the samples loop,
    // rather than the other way around; the result is that the
    // writer and readers get incremented for each channel on each
    // sample frame, i.e. doubly incremented, hence the double r/w
    // incrementing in this single-channel emulation of TOMSOUND
    constexpr int tomsoundMultiple = 2;
    constexpr auto tomsoundMultiple_float = static_cast<double>(tomsoundMultiple);

    // If a speed value is very near but not quite a whole number,
    // and if the buffer size is an even number of samples,
    // it is possible for that small variance to accumulate in the
    // read position until the read head is consistently reading
    // odd-number buffer samples while the write head is (due to
    // the even-size buffer size) only writing into even-number
    // buffer sample position, resulting in reading only silence.
    // This workaround forces the writer to always wrap to an
    // odd value of buffer size (rounding down, for bounds safety)
    // regardless of the actual buffer size.
    auto const bsizeWriteWrap = bsize - ((bsize % tomsoundMultiple) ? 0 : 1);

    for(size_t i = 0; i < outAudio.size(); i++) {
      //for(size_t ch = 0; ch < getnumoutputs(); ch++) {

      /* read from read heads */

      for(size_t h = 0; h < kNumDelays; h++) {
        /* another characteristic of TOMSOUND is sharing a single buffer across heads */
        /* (however it is only viable with the legacy behavior of applying mix to feedback) */
        auto& buf = attenuateFeedbackByMixLevel ? heads.front().buf : heads[h].buf;

        switch(quality) {
          case kQualityMode_DirtFi:
          default:
            delayvals[h] = buf[static_cast<size_t>(heads[h].read)];
            break;
          case kQualityMode_HiFi:
            delayvals[h] = interpolateLinear(std::span(buf).subspan(0, bsize), heads[h].read);
            break;
          case kQualityMode_UltraHiFi:
            delayvals[h] = dfx::math::InterpolateHermite(std::span(buf).subspan(0, bsize), heads[h].read);
            break;
        }
      }

      /* then write into buffer (w/ feedback) */
      if (!freeze) {
        if(attenuateFeedbackByMixLevel) {
          auto& buf = heads.front().buf;
          buf[writer] = inAudio[i];
          for(size_t h = 0; h < kNumDelays; h++) {
            buf[writer] +=
              heads[h].feed.getValue() * heads[h].mix.getValue() * delayvals[h];
          }
        } else {
          for(size_t h = 0; h < kNumDelays; h++) {
            heads[h].buf[writer] =
              inAudio[i] +
              (heads[h].feed.getValue() * delayvals[h]);
          }
        }
      }

      /* update rw heads */
      writer += writerIncrement * tomsoundMultiple;
      if (writer >= bsize)
        writer %= bsizeWriteWrap;

      for (auto& head : heads) {
        head.read += head.speed.getValue() * tomsoundMultiple_float;
        if (head.read >= bsize_float)
          head.read = std::fmod(head.read, bsize_float);
      }

      /* make output */
      outAudio[i] = inAudio[i] * drymix.getValue();
      for(size_t h = 0; h < kNumDelays; h++) {
        outAudio[i] += heads[h].mix.getValue() * delayvals[h];
      }
      //}  /* end of channels loop */

      incrementSmoothedAudioValues();
    }  /* end of samples loop */
  }  /* end of TOMSOUND */
}
