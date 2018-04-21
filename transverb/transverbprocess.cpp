/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Transverb is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Transverb.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "transverb.h"

#include <cmath>

#include "firfilter.h"



void TransverbDSP::process(float const* in, float* out, unsigned long numSampleFrames, bool replacing) {

  // int versions of these float values, for reducing casting operations
  int speed1int, speed2int, read1int, read2int;
  int lowpass1pos, lowpass2pos;  // position trackers for the lowpass filters
  float r1val, r2val;  // delay buffer output values
  auto const bsize_float = (double)bsize;  // cut down on casting
  int filterMode1, filterMode2;  // the type of filtering to use in ultra hi-fi mode
  float mug1 = 1.0f, mug2 = 1.0f;  // make-up gain for lowpass filtering
  auto const samplerate = getsamplerate();
  tomsound_sampoffset = GetChannelNum();


  /////////////   S O P H I A S O U N D   //////////////
  // do it proper
  if (!tomsound) {

    /// filter setup stuff ///

    filterMode1 = filterMode2 = kFilterMode_Nothing;  // reset these for now
    if (quality == kQualityMode_UltraHiFi)
    {
      // check to see if we need to lowpass the first delay head and init coefficients if so
      if (speed1 > 1.0)
      {
        filterMode1 = kFilterMode_LowpassIIR;
        speed1int = (int)speed1;
        // it becomes too costly to try to IIR at > 5x speeds, so switch to FIR filtering
        if (speed1int >= 5)
        {
          filterMode1 = kFilterMode_LowpassFIR;
         mug1 = (float) std::pow(speed1 * 0.2, 0.78);  // compensate for gain lost from filtering
          // update the coefficients only if necessary
          if (speed1hasChanged)
          {
            dfx::FIRFilter::calculateIdealLowpassCoefficients((samplerate / speed1) * dfx::FIRFilter::kShelfStartLowpass, 
                                                              samplerate, kNumFIRTaps, firCoefficients1.data());
            dfx::FIRFilter::applyKaiserWindow(kNumFIRTaps, firCoefficients1.data(), 60.0f);
            speed1hasChanged = false;
          }
        }
        else if (speed1hasChanged)
        {
          filter1.setLowpassCoefficients((samplerate / speed1) * dfx::IIRfilter::kShelfStartLowpass);
          speed1hasChanged = false;
        }
      }
      // we need to highpass the delay head to remove mega sub bass
      else
      {
        filterMode1 = kFilterMode_Highpass;
        if (speed1hasChanged)
        {
          filter1.setHighpassCoefficients(33.3 / speed1);
          speed1hasChanged = false;
        }
      }

      // check to see if we need to lowpass the second delay head and init coefficients if so
      if (speed2 > 1.0)
      {
        filterMode2 = kFilterMode_LowpassIIR;
        speed2int = (int)speed2;
        if (speed2int >= 5)
        {
          filterMode2 = kFilterMode_LowpassFIR;
          mug2 = (float) std::pow(speed2 * 0.2, 0.78);  // compensate for gain lost from filtering
          if (speed2hasChanged)
          {
            dfx::FIRFilter::calculateIdealLowpassCoefficients((samplerate / speed2) * dfx::FIRFilter::kShelfStartLowpass, 
                                                              samplerate, kNumFIRTaps, firCoefficients2.data());
            dfx::FIRFilter::applyKaiserWindow(kNumFIRTaps, firCoefficients2.data(), 60.0f);
            speed2hasChanged = false;
          }
        }
        else if (speed2hasChanged)
        {
          filter2.setLowpassCoefficients((samplerate / speed2) * dfx::IIRfilter::kShelfStartLowpass);
          speed2hasChanged = false;
        }
      }
      // we need to highpass the delay head to remove mega sub bass
      else
      {
        filterMode2 = kFilterMode_Highpass;
        if (speed2hasChanged)
        {
          filter2.setHighpassCoefficients(33.3 / speed2);
          speed2hasChanged = false;
        }
      }
    }


    /// audio processing loop ///

    lowpass1pos = (int)read1;
    lowpass2pos = (int)read2;

    for(unsigned long i = 0; i < numSampleFrames; i++) {  // samples loop

      read1int = (int)read1;
      read2int = (int)read2;

      /* read from read heads */
      switch(quality)
      {
        // no interpolation or filtering
        case kQualityMode_DirtFi:
          r1val = buf1[read1int];
          r2val = buf2[read2int];
          break;
        // spline interpolation, but no filtering
        case kQualityMode_HiFi:
//          r1val = Transverb_InterpolateLinear(buf1.data(), read1, bsize, writer - read1int);
          r1val = Transverb_InterpolateHermite(buf1.data(), read1, bsize, writer - read1int);
          r2val = Transverb_InterpolateHermite(buf2.data(), read2, bsize, writer - read2int);
          break;
        // spline interpolation plus anti-aliasing lowpass filtering for high speeds 
        // or sub-bass-removing highpass filtering for low speeds
        case kQualityMode_UltraHiFi:
          float lp1, lp2;
          switch (filterMode1)
          {
            case kFilterMode_Highpass:
            case kFilterMode_LowpassIIR:
              // interpolate the values in the IIR output history
              r1val = filter1.interpolateHermitePostFilter(read1);
              break;
            case kFilterMode_LowpassFIR:
              // get 2 consecutive FIR output values for linear interpolation
              lp1 = dfx::FIRFilter::process(buf1.data(), kNumFIRTaps, firCoefficients1.data(), 
                                            (read1int - kNumFIRTaps + bsize) % bsize, bsize);
              lp2 = dfx::FIRFilter::process(buf1.data(), kNumFIRTaps, firCoefficients1.data(), 
                                            (read1int - kNumFIRTaps + 1 + bsize) % bsize, bsize);
              // interpolate output linearly (avoid shit sound) and compensate gain
              r1val = dfx::math::InterpolateLinear(lp1, lp2, read1) * mug1;
              break;
            default:
              r1val = Transverb_InterpolateHermite(buf1.data(), read1, bsize, writer - read1int);
              break;
          }
          switch (filterMode2)
          {
            case kFilterMode_Highpass:
            case kFilterMode_LowpassIIR:
              // interpolate the values in the IIR output history
              r2val = filter2.interpolateHermitePostFilter(read2);
              break;
            case kFilterMode_LowpassFIR:
              // get 2 consecutive FIR output values for linear interpolation
              lp1 = dfx::FIRFilter::process(buf2.data(), kNumFIRTaps, firCoefficients2.data(), 
                                            (read2int - kNumFIRTaps + bsize) % bsize, bsize);
              lp2 = dfx::FIRFilter::process(buf2.data(), kNumFIRTaps, firCoefficients2.data(), 
                                            (read2int - kNumFIRTaps + 1 + bsize) % bsize, bsize);
              // interpolate output linearly (avoid shit sound) and compensate gain
              r2val = dfx::math::InterpolateLinear(lp1, lp2, read2) * mug2;
              break;
            default:
              r2val = Transverb_InterpolateHermite(buf2.data(), read2, bsize, writer - read2int);
              break;
          }
          break;
        // dirt-fi style again for the safety net
        default:
          r1val = buf1[read1int];
          r2val = buf2[read2int];
          break;
      }  // end of quality switch

      // crossfade the last stored smoothing sample with 
      // the current sample if smoothing is in progress
      if (smoothcount1) {
        r1val = (r1val * (1.0f - (smoothstep1 * (float)smoothcount1))) 
                + (lastr1val * smoothstep1 * (float)smoothcount1);
        smoothcount1--;
      }
      if (smoothcount2) {
        r2val = (r2val * (1.0f - (smoothstep2 * (float)smoothcount2))) 
                + (lastr2val * smoothstep2 * (float)smoothcount2);
        smoothcount2--;
      }
      
      /* then write into buffer (w/ feedback) */

      buf1[writer] = in[i] + (r1val * feed1 * mix1);
      buf2[writer] = in[i] + (r2val * feed2 * mix2);

      /* make output */
    #ifdef TARGET_API_VST
      if (replacing)
    #endif
        out[i] = (in[i]*drymix) + (r1val*mix1) + (r2val*mix2);
    #ifdef TARGET_API_VST
      else
        out[i] += (in[i]*drymix) + (r1val*mix1) + (r2val*mix2);
    #endif

      /* start smoothing stuff if the writer has 
         passed a reader or vice versa.
           (check the positions before wrapping around the heads)
      */

      if (((read1int < writer) && 
           (((int)(read1 + speed1)) >= (writer + 1))) || 
          ((read1int >= writer) && 
           (((int)(read1 + speed1)) <= (writer + 1)))) {
      /* check because, at slow speeds, 
      it's possible to go into this twice or more in a row */
        if (smoothcount1 <= 0) {
          // store the most recent output as the channel 1 smoothing sample
          lastr1val = r1val;
          // truncate the smoothing duration if we're using too small of a buffer size
          smoothdur1 = 
            (kAudioSmoothingDur_samples > (int)(bsize_float / speed1)) ? 
            (int)(bsize_float / speed1) : kAudioSmoothingDur_samples;
          smoothstep1 = 1.0f / (float)smoothdur1;  // the scalar step value
          smoothcount1 = smoothdur1;  // set the counter to the total duration
        }
      }

      // head 2 smoothing stuff
      if (((read2int < writer) && 
           (((int)(read2 + speed2)) >= (writer + 1))) || 
          ((read2int >= writer) && 
           (((int)(read2 + speed2)) <= (writer + 1)))) {
        if (smoothcount2 <= 0) {
          // store the most recent output as the channel 2 smoothing sample
          lastr2val = r2val;
          // truncate the smoothing duration if we're using too small of a buffer size
          smoothdur2 = 
            (kAudioSmoothingDur_samples > (int)(bsize_float / speed2)) ? 
            (int)(bsize_float / speed2) : kAudioSmoothingDur_samples;
          smoothstep2 = 1.0f / (float)smoothdur2;  // the scalar step value
          smoothcount2 = smoothdur2;  // set the counter to the total duration
        }
      }

      /* update rw heads */
      writer++;
      read1 += speed1;
      read2 += speed2;

      // wrap around the rw heads if they've gone past the end of the buffer
      writer %= bsize;
      if (read1 >= bsize_float)
        read1 = std::fmod(std::fabs(read1), bsize_float);
      if (read2 >= bsize_float)
        read2 = std::fmod(std::fabs(read2), bsize_float);

      // if we're doing IIR lowpass filtering, 
      // then we probably need to process a few consecutive samples in order 
      // to get the continuous impulse (or whatever you call that), 
      // probably whatever the speed multiplier is, that's how many samples
      if (filterMode1 == kFilterMode_LowpassIIR)
      {
        int lowpasscount = 0;
        while (lowpasscount < speed1int)
        {
          switch (speed1int - lowpasscount)
          {
            case 1:
              filter1.processH1(buf1[lowpass1pos]);
              lowpass1pos = (lowpass1pos + 1) % bsize;
              lowpasscount++;
              break;
            case 2:
              filter1.processH2(buf1.data(), lowpass1pos, bsize);
              lowpass1pos = (lowpass1pos + 2) % bsize;
              lowpasscount += 2;
              break;
            case 3:
              filter1.processH3(buf1.data(), lowpass1pos, bsize);
              lowpass1pos = (lowpass1pos + 3) % bsize;
              lowpasscount += 3;
              break;
            default:
              filter1.processH4(buf1.data(), lowpass1pos, bsize);
              lowpass1pos = (lowpass1pos + 4) % bsize;
              lowpasscount += 4;
              break;
          }
        }
        read1int = (int)read1;
        // make sure that we don't need to do one more sample
        if (((lowpass1pos < read1int) && ((lowpass1pos + 1) == read1int)) ||
            ((lowpass1pos == (bsize - 1)) && (read1int == 0)))
        {
          filter1.processH1(buf1[lowpass1pos]);
          lowpass1pos = (lowpass1pos + 1) % bsize;
        }
      }
      // it's simpler for highpassing; 
      // we may not even need to process anything for this sample
      else if (filterMode1 == kFilterMode_Highpass)
      {
        // only if we've traversed to a new integer sample position
        if ((int)read1 != read1int)
          filter1.process(buf1[read1int]);
      }

      // head 2 filtering stuff
      if (filterMode2 == kFilterMode_LowpassIIR)
      {
        int lowpasscount = 0;
        while (lowpasscount < speed2int)
        {
          switch (speed2int - lowpasscount)
          {
            case 1:
              filter2.processH1(buf2[lowpass2pos]);
              lowpass2pos = (lowpass2pos + 1) % bsize;
              lowpasscount++;
              break;
            case 2:
              filter2.processH2(buf2.data(), lowpass2pos, bsize);
              lowpass2pos = (lowpass2pos + 2) % bsize;
              lowpasscount += 2;
              break;
            case 3:
              filter2.processH3(buf2.data(), lowpass2pos, bsize);
              lowpass2pos = (lowpass2pos + 3) % bsize;
              lowpasscount += 3;
              break;
            default:
              filter2.processH4(buf2.data(), lowpass2pos, bsize);
              lowpass2pos = (lowpass2pos + 4) % bsize;
              lowpasscount += 4;
              break;
          }
        }
        read2int = (int)read2;
        if (((lowpass2pos < read2int) && ((lowpass2pos + 1) == read2int)) ||
            ((lowpass2pos == (bsize - 1)) && (read2int == 0)))
        {
          filter2.processH1(buf2[lowpass2pos]);
          lowpass2pos = (lowpass2pos + 1) % bsize;
        }
      }
      else if (filterMode2 == kFilterMode_Highpass)
      {
        if ((int)read2 != read2int)
          filter2.process(buf2[read2int]);
      }
    }  /* end of samples loop */

  }  /* end of if(!TOMSOUND) */




  /////////////   T O M S O U N D   //////////////
  else {

    for(unsigned long j = 0; j < numSampleFrames; j++) {
//      for(int i = 0; i < numChannels; i++) {

    /* read from read heads */

    switch(quality) {
      case kQualityMode_DirtFi:
        r1val = mix1 * buf1[(size_t)read1];
        r2val = mix2 * buf1[(size_t)read2];
        break;
      case kQualityMode_HiFi:
      case kQualityMode_UltraHiFi:
        r1val = mix1 * Transverb_InterpolateHermite(buf1.data(), read1, bsize, 333);
        r2val = mix2 * Transverb_InterpolateHermite(buf1.data(), read2, bsize, 333);
        break;
      default:
        r1val = mix1 * buf1[(size_t)read1];
        r2val = mix2 * buf1[(size_t)read2];
        break;
      }

    /* then write into buffer (w/ feedback) */

    buf1[writer] = 
      in[j] + 
      feed1 * r1val + 
      feed2 * r2val;
      
    /* update rw heads */
//    writer++;
    writer += 2;
    writer %= bsize;

//    read1 += speed1;
//    read2 += speed2;
    // the essense of TOMSOUND comes from the error that Tom made 
    // of putting the channels loop within the samples loop, 
    // rather than the other way around; the result is that the 
    // writer and readers get incremented for each channel on each 
    // sample frame, i.e. doubly incremented, hence the double 
    // incrementing below in this single-channel emulation of TOMSOUND
    read1 += speed1 * 2.0;
    read2 += speed2 * 2.0;

    if (read1 >= bsize_float)
      read1 = std::fmod(std::fabs(read1), bsize_float);
    if (read2 >= bsize_float)
      read2 = std::fmod(std::fabs(read2), bsize_float);

    /* make output */
  #ifdef TARGET_API_VST
    if (replacing)
  #endif
      out[j] = in[j] * drymix + r1val + r2val;
  #ifdef TARGET_API_VST
    else
      out[j] += in[j] * drymix + r1val + r2val;
  #endif
//      }
    }
  }
}
