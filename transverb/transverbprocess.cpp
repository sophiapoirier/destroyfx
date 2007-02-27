
/* DFX Transverb plugin by Tom 7 and Sophia */

#include "transverb.hpp"

#include "firfilter.h"



void TransverbDSP::process(const float *in, float *out, unsigned long numSampleFrames, bool replacing) {

  // int versions of these float values, for reducing casting operations
  int speed1int, speed2int, read1int, read2int;
  int lowpass1pos, lowpass2pos;	// position trackers for the lowpass filters
  float r1val, r2val;	// delay buffer output values
  double bsize_float = (double)bsize;	// cut down on casting
  int filterMode1, filterMode2;	// the type of filtering to use in ultra hi-fi mode
  float mug1 = 1.0f, mug2 = 1.0f;	// make-up gain for lowpass filtering
  float fsamplerate = getsamplerate_f();
  tomsound_sampoffset = GetChannelNum();


  // there must have not been available memory or something (like WaveLab goofing up), 
  // so try to allocate buffers now
  if ( (buf1 == NULL) || (buf2 == NULL) )
    createbuffers();
  // if the creation failed, then abort audio processing
  if ( (buf1 == NULL) || (buf2 == NULL) )
    return;


  /////////////   M A R C S O U N D   //////////////
  // do it proper
  if (!tomsound) {

	/// filter setup stuff ///

    filterMode1 = filterMode2 = useNothing;	// reset these for now
    if (quality == ultrahifi)
    {
      // check to see if we need to lowpass the first delay head & init coefficients if so
      if (speed1 > 1.0)
      {
        filterMode1 = useLowpassIIR;
        speed1int = (int)speed1;
        // it becomes too costly to try to IIR at > 5x speeds, so switch to FIR filtering
        if (speed1int >= 5)
        {
          filterMode1 = useLowpassFIR;
          mug1 = (float) pow( (speed1*0.2), 0.78 );	// compensate for gain lost from filtering
          // update the coefficients only if necessary
          if (speed1hasChanged)
          {
            calculateFIRidealLowpassCoefficients((fsamplerate/(float)speed1)*SHELF_START_IIR, 
                  fsamplerate, NUM_FIR_TAPS, firCoefficients1);
            applyKaiserWindow(NUM_FIR_TAPS, firCoefficients1, 60.0f);
            speed1hasChanged = false;
          }
        }
        else if (speed1hasChanged)
        {
          filter1[0].calculateLowpassCoefficients((fsamplerate/(float)speed1)*SHELF_START_IIR, fsamplerate);
          speed1hasChanged = false;
        }
      }
      // we need to highpass the delay head to remove mega sub bass
      else
      {
        filterMode1 = useHighpass;
        if (speed1hasChanged)
        {
          filter1[0].calculateHighpassCoefficients(33.3f/(float)speed1, fsamplerate);
          speed1hasChanged = false;
        }
      }

      // check to see if we need to lowpass the second delay head & init coefficients if so
      if (speed2 > 1.0)
      {
        filterMode2 = useLowpassIIR;
        speed2int = (int)speed2;
        if (speed2int >= 5)
        {
          filterMode2 = useLowpassFIR;
          mug2 = (float) pow( (speed2*0.2), 0.78 );	// compensate for gain lost from filtering
          if (speed2hasChanged)
          {
            calculateFIRidealLowpassCoefficients((fsamplerate/(float)speed2)*SHELF_START_IIR, 
                  fsamplerate, NUM_FIR_TAPS, firCoefficients2);
            applyKaiserWindow(NUM_FIR_TAPS, firCoefficients2, 60.0f);
            speed2hasChanged = false;
          }
        }
        else if (speed2hasChanged)
        {
          filter2[0].calculateLowpassCoefficients((fsamplerate/(float)speed2)*SHELF_START_IIR, fsamplerate);
          speed2hasChanged = false;
        }
      }
      // we need to highpass the delay head to remove mega sub bass
      else
      {
        filterMode2 = useHighpass;
        if (speed2hasChanged)
        {
          filter2[0].calculateHighpassCoefficients(33.3f/(float)speed2, fsamplerate);
          speed2hasChanged = false;
        }
      }
    }


	/// audio processing loop ///

    lowpass1pos = (int)read1;
    lowpass2pos = (int)read2;

    for(unsigned long i=0; i < numSampleFrames; i++) {	// samples loop

      read1int = (int)read1;
      read2int = (int)read2;

      /* read from read heads */
      switch(quality)
      {
        // no interpolation or filtering
        case dirtfi:
          r1val = buf1[read1int];
          r2val = buf2[read2int];
          break;
        // spline interpolation, but no filtering
        case hifi:
//          r1val = interpolateLinear(buf1, read1, bsize, writer-read1int);
          r1val = interpolateHermite(buf1, read1, bsize, writer-read1int);
          r2val = interpolateHermite(buf2, read2, bsize, writer-read2int);
          break;
        // spline interpolation plus anti-aliasing lowpass filtering for high speeds 
        // or sub-bass-removing highpass filtering for low speeds
        case ultrahifi:
          float lp1, lp2;
          switch (filterMode1)
          {
            case useHighpass:
            case useLowpassIIR:
              // interpolate the values in the IIR output history
              r1val = interpolateHermitePostFilter(filter1, read1);
              break;
            case useLowpassFIR:
              // get 2 consecutive FIR output values for linear interpolation
              lp1 = processFIRfilter(buf1, NUM_FIR_TAPS, firCoefficients1, 
                                     (read1int-NUM_FIR_TAPS+bsize)%bsize, bsize);
              lp2 = processFIRfilter(buf1, NUM_FIR_TAPS, firCoefficients1, 
                                     (read1int-NUM_FIR_TAPS+1+bsize)%bsize, bsize);
              // interpolate output linearly (avoid shit sound) & compensate gain
              r1val = DFX_InterpolateLinear_Values(lp1, lp2, read1) * mug1;
              break;
            default:
              r1val = interpolateHermite(buf1, read1, bsize, writer-read1int);
              break;
          }
          switch (filterMode2)
          {
            case useHighpass:
            case useLowpassIIR:
              // interpolate the values in the IIR output history
              r2val = interpolateHermitePostFilter(filter2, read2);
              break;
            case useLowpassFIR:
              // get 2 consecutive FIR output values for linear interpolation
              lp1 = processFIRfilter(buf2, NUM_FIR_TAPS, firCoefficients2, 
                                     (read2int-NUM_FIR_TAPS+bsize)%bsize, bsize);
              lp2 = processFIRfilter(buf2, NUM_FIR_TAPS, firCoefficients2, 
                                     (read2int-NUM_FIR_TAPS+1+bsize)%bsize, bsize);
              // interpolate output linearly (avoid shit sound) & compensate gain
              r2val = DFX_InterpolateLinear_Values(lp1, lp2, read2) * mug2;
              break;
            default:
              r2val = interpolateHermite(buf2, read2, bsize, writer-read2int);
              break;
          }
          break;
        // dirt-fi style again for the safety net
        default:
          r1val = buf1[read1int];
          r2val = buf2[read2int];
          break;
      }	// end of quality switch

      // crossfade the last stored smoothing sample with 
      // the current sample if smoothing is in progress
      if (smoothcount1) {
        r1val = ( r1val * (1.0f - (smoothstep1*(float)smoothcount1)) ) 
                + (lastr1val * smoothstep1*(float)smoothcount1);
        smoothcount1--;
      }
      if (smoothcount2) {
        r2val = ( r2val * (1.0f - (smoothstep2*(float)smoothcount2)) ) 
                + (lastr2val * smoothstep2*(float)smoothcount2);
        smoothcount2--;
      }
      
      /* then write into buffer (w/ feedback) */

      // mix very quiet noise (-300 dB) into the input singal 
      // to hopefully avoid any denormal values from IIR filtering
      buf1[writer] = in[i] + (r1val * feed1 * mix1);
      buf2[writer] = in[i] + (r2val * feed2 * mix2);
    #ifndef TARGET_API_AUDIOUNIT
      DFX_UNDENORMALIZE(buf1[writer]);
      DFX_UNDENORMALIZE(buf2[writer]);
    #endif

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

      if ( ( (read1int < writer) && 
             (((int)(read1+speed1)) >= (writer+1)) ) || 
           ( (read1int >= writer) && 
             (((int)(read1+speed1)) <= (writer+1)) ) ) {
      /* check because, at slow speeds, 
      it's possible to go into this twice or more in a row */
        if (smoothcount1 <= 0) {
          // store the most recent output as the channel 1 smoothing sample
          lastr1val = r1val;
          // truncate the smoothing duration if we're using too small of a buffer size
          smoothdur1 = 
            (SMOOTH_DUR > (int)(bsize_float/speed1)) ? 
            (int)(bsize_float/speed1) : SMOOTH_DUR;
          smoothstep1 = 1.0f / (float)smoothdur1;	// the scalar step value
          smoothcount1 = smoothdur1;	// set the counter to the total duration
        }
      }

      // head 2 smoothing stuff
      if ( ( (read2int < writer) && 
             (((int)(read2+speed2)) >= (writer+1)) ) || 
           ( (read2int >= writer) && 
             (((int)(read2+speed2)) <= (writer+1)) ) ) {
        if (smoothcount2 <= 0) {
          // store the most recent output as the channel 2 smoothing sample
          lastr2val = r2val;
          // truncate the smoothing duration if we're using too small of a buffer size
          smoothdur2 = 
            (SMOOTH_DUR > (int)(bsize_float/speed2)) ? 
            (int)(bsize_float/speed2) : SMOOTH_DUR;
          smoothstep2 = 1.0f / (float)smoothdur2;	// the scalar step value
          smoothcount2 = smoothdur2;	// set the counter to the total duration
        }
      }

      /* update rw heads */
      writer++;
      read1 += speed1;
      read2 += speed2;

      // wrap around the rw heads if they've gone past the end of the buffer
      writer %= bsize;
      if (read1 >= bsize_float)
        read1 = fmod(fabs(read1), bsize_float);
      if (read2 >= bsize_float)
        read2 = fmod(fabs(read2), bsize_float);

      // if we're doing IIR lowpass filtering, 
      // then we probably need to process a few consecutive samples in order 
      // to get the continuous impulse (or whatever you call that), 
      // probably whatever the speed multiplier is, that's how many samples
      if (filterMode1 == useLowpassIIR)
      {
        int lowpasscount = 0;
        while (lowpasscount < speed1int)
        {
          switch (speed1int - lowpasscount)
          {
            case 1:
              filter1->processH1(buf1[lowpass1pos]);
              lowpass1pos = (lowpass1pos + 1) % bsize;
              lowpasscount++;
              break;
            case 2:
              filter1->processH2(buf1, lowpass1pos, bsize);
              lowpass1pos = (lowpass1pos + 2) % bsize;
              lowpasscount += 2;
              break;
            case 3:
              filter1->processH3(buf1, lowpass1pos, bsize);
              lowpass1pos = (lowpass1pos + 3) % bsize;
              lowpasscount += 3;
              break;
            default:
              filter1->processH4(buf1, lowpass1pos, bsize);
              lowpass1pos = (lowpass1pos + 4) % bsize;
              lowpasscount += 4;
              break;
          }
        }
        read1int = (int)read1;
        // make sure that we don't need to do one more sample
        if ( ((lowpass1pos < read1int) && ((lowpass1pos+1) == read1int)) ||
             ((lowpass1pos == (bsize-1)) && (read1int == 0)) )
        {
          filter1->processH1(buf1[lowpass1pos]);
          lowpass1pos = (lowpass1pos+1) % bsize;
        }
      }
      // it's simpler for highpassing; 
      // we may not even need to process anything for this sample
      else if (filterMode1 == useHighpass)
      {
        // only if we've traversed to a new integer sample position
        if ((int)read1 != read1int)
          filter1->process(buf1[read1int]);
      }

      // head 2 filtering stuff
      if (filterMode2 == useLowpassIIR)
      {
        int lowpasscount = 0;
        while (lowpasscount < speed2int)
        {
          switch (speed2int - lowpasscount)
          {
            case 1:
              filter2->processH1(buf2[lowpass2pos]);
              lowpass2pos = (lowpass2pos + 1) % bsize;
              lowpasscount++;
              break;
            case 2:
              filter2->processH2(buf2, lowpass2pos, bsize);
              lowpass2pos = (lowpass2pos + 2) % bsize;
              lowpasscount += 2;
              break;
            case 3:
              filter2->processH3(buf2, lowpass2pos, bsize);
              lowpass2pos = (lowpass2pos + 3) % bsize;
              lowpasscount += 3;
              break;
            default:
              filter2->processH4(buf2, lowpass2pos, bsize);
              lowpass2pos = (lowpass2pos + 4) % bsize;
              lowpasscount += 4;
              break;
          }
        }
        read2int = (int)read2;
        if ( ((lowpass2pos < read2int) && ((lowpass2pos+1) == read2int)) ||
             ((lowpass2pos == (bsize-1)) && (read2int == 0)) )
        {
          filter2->processH1(buf2[lowpass2pos]);
          lowpass2pos = (lowpass2pos+1) % bsize;
        }
      }
      else if (filterMode2 == useHighpass)
      {
        if ((int)read2 != read2int)
          filter2->process(buf2[read2int]);
      }
    }	/* end of samples loop */

  }	/* end of if(!TOMSOUND) */




  /////////////   T O M S O U N D   //////////////
  else {

    for(unsigned long j=0; j < numSampleFrames; j++) {
//      for(int i=0; i < numChannels; i++) {

    /* read from read heads */

    switch(quality) {
      case dirtfi:
        r1val = mix1 * buf1[(int)read1];
        r2val = mix2 * buf1[(int)read2];
        break;
      case hifi:
      case ultrahifi:
        r1val = mix1 * interpolateHermite(buf1, read1, bsize, 333);
        r2val = mix2 * interpolateHermite(buf1, read2, bsize, 333);
        break;
      default:
        r1val = mix1 * buf1[(int)read1];
        r2val = mix2 * buf1[(int)read2];
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
      read1 = fmod(fabs(read1), bsize_float);
    if (read2 >= bsize_float)
      read2 = fmod(fabs(read2), bsize_float);

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
