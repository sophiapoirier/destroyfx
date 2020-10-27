/*------------------------------------------------------------------------
Copyright (C) 2002-2020  Tom Murphy 7 and Sophia Poirier

This file is part of BrokenFFT.

BrokenFFT is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

BrokenFFT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BrokenFFT.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "brokenfft.h"

#if TARGET_OS_MAC
  #include <Accelerate/Accelerate.h>
#endif
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <mutex>
#include <numeric>
#include <string>

#include "dfxmath.h"

// DC FFT
#include "fourier.h"

/* this macro does boring entry point stuff for us */
DFX_EFFECT_ENTRY(BrokenFFT)
DFX_CORE_ENTRY(BrokenFFTDSP)

// XXX This is in std now right?
#define PI 3.141592653589
static constexpr int MAXECHO = 8192;

using AmplEntry = BrokenFFTDSP::AmplEntry;

BrokenFFT::BrokenFFT(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, NUM_PARAMS, NUM_PRESETS) {

  initparameter_list(P_BUFSIZE, {"window size", "wsize", "WSiz"}, 9, 9, BUFFERSIZESSIZE, DfxParam::Unit::Samples);
  initparameter_list(P_SHAPE, {"window shape", "wshape", "WShp"}, WINDOW_TRIANGLE, WINDOW_TRIANGLE, MAX_WINDOWSHAPES);

  initparameter_list(P_METHOD, {"fft method", "how"}, METHOD_WEST, METHOD_WEST, MAX_METHODS);

  using Unit = DfxParam::Unit;
  using Curve = DfxParam::Curve;

  // Placeholder to port old parameter definitions... make these nicer.
  // Pass unit names (last param).
#define FPARAM(p, name, def) \
  initparameter_f(p, {"" name ""}, def, def, 0.0, 1.0f, Unit::Generic, Curve::Linear, "")

  FPARAM(P_DESTRUCT, "very special", 1.0f);
  FPARAM(P_PERTURB, "perturb", 0.0f);
  FPARAM(P_QUANT, "operation q", 1.0f);
  FPARAM(P_ROTATE, "rotate", 0.0f);
  FPARAM(P_BINQUANT, "operation bq", 0.0f);
  FPARAM(P_COMPRESS, "comp???", 1.f);
  FPARAM(P_SPIKE, "spike", 1.0f);
  FPARAM(P_MUG, "M.U.G.", 0.0f);
  FPARAM(P_SPIKEHOLD, "spikehold", 0.0f);
  FPARAM(P_ECHOMIX, "eo mix", 0.0f);
  FPARAM(P_ECHOTIME, "eo time", 0.5f);
  FPARAM(P_ECHOMODF, "eo mod f", float(2.0 * PI));
  FPARAM(P_ECHOMODW, "eo mod w", 0.0f);
  FPARAM(P_ECHOFB, "eo fbx", 0.50f);
  FPARAM(P_ECHOLOW, "eo >", 0.0f);
  FPARAM(P_ECHOHI, "eo <", 1.0f);
  FPARAM(P_POSTROT, "postrot", 0.5f);
  FPARAM(P_MOMENTS, "moments", 0.0f);
  FPARAM(P_BRIDE, "bride", 1.0f);
  FPARAM(P_BLOW, "blow", 0.0f);
  FPARAM(P_LOWP, "lowp", 1.0f);
  FPARAM(P_CONV, "convolve", 0.0f);
  FPARAM(P_HARM, "harm", 0.0f);
  FPARAM(P_ALOW, "afterlow", 1.0f);
  FPARAM(P_NORM, "anorm", 0.0f);


  /* windowing */
  for (size_t i = 0; i < buffersizes.size(); i++) {
    std::array<char, dfx::kParameterValueStringMaxLength> bufstr;
    constexpr int thousand = 1000;
    if (buffersizes[i] >= thousand) {
      snprintf(bufstr.data(), bufstr.size(), "%d,%03d",
	       buffersizes[i] / thousand, buffersizes[i] % thousand);
    } else {
      snprintf(bufstr.data(), bufstr.size(), "%d", buffersizes[i]);
    }
    setparametervaluestring(P_BUFSIZE, static_cast<long>(i), bufstr.data());
  }

  setparametervaluestring(P_SHAPE, WINDOW_TRIANGLE, "linear");
  setparametervaluestring(P_SHAPE, WINDOW_ARROW, "arrow");
  setparametervaluestring(P_SHAPE, WINDOW_WEDGE, "wedge");
  setparametervaluestring(P_SHAPE, WINDOW_BEST, "best");
  setparametervaluestring(P_SHAPE, WINDOW_COS2, "cos^2");
  for (long i = NUM_WINDOWSHAPES; i < MAX_WINDOWSHAPES; i++)
    setparametervaluestring(P_SHAPE, i, "???");

  addparametergroup("windowing", {P_BUFSIZE, P_SHAPE});

  // Default preset name
  setpresetname(0, "Mr. Fourier");
  makepresets();
}

void BrokenFFT::dfx_PostConstructor() {
  /* since we don't use notes for any specialized control of BrokenFFT,
     allow them to be assigned to control parameters via MIDI learn */
  getsettings().setAllowPitchbendEvents(true);
  getsettings().setAllowNoteEvents(true);
}

long BrokenFFT::dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex,
				    size_t& outDataSize, dfx::PropertyFlags& outFlags) {
  switch (inPropertyID) {
    case PROP_LAST_WINDOW_TIMESTAMP:
      outDataSize = sizeof(uint64_t);
      outFlags = dfx::kPropertyFlag_Readable;
      return dfx::kStatus_NoError;
    case PROP_FFT_DATA:
      outDataSize = sizeof(BrokenFFTViewData);
      outFlags = dfx::kPropertyFlag_Readable;
      return dfx::kStatus_NoError;
    default:
      return DfxPlugin::dfx_GetPropertyInfo(inPropertyID, inScope, inItemIndex, outDataSize, outFlags);
  }
}

long BrokenFFT::dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex,
				void* outData) {
  switch (inPropertyID) {
    case PROP_LAST_WINDOW_TIMESTAMP:
      *static_cast<uint64_t*>(outData) = lastwindowtimestamp.load(std::memory_order_relaxed);
      return dfx::kStatus_NoError;
    case PROP_FFT_DATA: {
      std::lock_guard const guard(windowcachelock);
      *static_cast<BrokenFFTViewData*>(outData) = windowcache;
      return dfx::kStatus_NoError;
    }
    default:
      return DfxPlugin::dfx_GetProperty(inPropertyID, inScope, inItemIndex, outData);
  }
}

void BrokenFFT::randomizeparameter(long inParameterIndex) {
  // we need to constrain the range of values of the parameters that have extra (currently unused) room for future expansion
  int64_t maxValue = 0;
  switch (inParameterIndex) {
    case P_SHAPE:
      maxValue = NUM_WINDOWSHAPES;
      break;
    default:
      DfxPlugin::randomizeparameter(inParameterIndex);
      return;
  }

  int64_t const newValue = rand() % maxValue;
  setparameter_i(inParameterIndex, newValue);

  // inform any parameter listeners of the changes
  postupdate_parameter(inParameterIndex);
}

void BrokenFFT::clearwindowcache() {
  {
    std::unique_lock const guard(windowcachelock, std::try_to_lock);
    if (guard.owns_lock()) {
      windowcache.clear();
    }
  }
  lastwindowtimestamp.store(0, std::memory_order_relaxed);
}

void BrokenFFT::updatewindowcache(BrokenFFTDSP const *brokenfftcore) {
  bool updated = false;
  {
    // willing to drop window cache updates to ensure realtime-safety by not blocking here
    std::unique_lock const guard(windowcachelock, std::try_to_lock);
    if ((updated = guard.owns_lock())) {
#if 1
      std::copy_n(brokenfftcore->getinput(), BrokenFFTViewData::samples, windowcache.inputs.data());
#else
      for (int i=0; i < BrokenFFTViewData::samples; i++) {
        windowcache.inputs[i] = std::sin((i * 10 * dfx::math::kPi<float>) / BrokenFFTViewData::samples);
      }
#endif

      windowcache.apts = std::min(brokenfftcore->getframesize(), BrokenFFTViewData::samples);

      // XXX I guess we actually run the process function just to produce the cache in geometer.
      // for brokenfft I guess we could still run a baby version, but maybe it makes more sense
      // to have the processing code occasionally save its data and only copy here?

      // This used to return the number of points
      windowcache.numpts = 0;
      /*
      brokenfftcore->ProcessW(windowcache.inputs.data(), windowcache.outputs.data(), windowcache.apts,
			      windowcache.pointsx.data(), windowcache.pointsy.data(),
			      BrokenFFTViewData::samples - 1, tmpx.data(), tmpy.data());
      */
    }
  }

  if (updated) {
    lastwindowtimestamp.store(std::chrono::steady_clock::now().time_since_epoch().count(), std::memory_order_relaxed);
  }
}

void BrokenFFTDSP::clearwindowcache() {
  if (IsFFTDataSource()) {
    brokenfft->clearwindowcache();
  }
}

void BrokenFFTDSP::updatewindowcache(BrokenFFTDSP const *brokenfftcore) {
  if (IsFFTDataSource()) {
    brokenfft->updatewindowcache(brokenfftcore);
  }
}

std::optional<dfx::ParameterAssignment>
BrokenFFT::settings_getLearningAssignData(long inParameterIndex) const {
  auto const getConstrainedToggleAssignment = [](long inNumStates, long inNumUsableStates) {
      dfx::ParameterAssignment result;
      result.mEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_Toggle;
      result.mDataInt1 = inNumStates;
      result.mDataInt2 = inNumUsableStates;
      return result;
    };

  switch (inParameterIndex) {
    case P_SHAPE:
      return getConstrainedToggleAssignment(MAX_WINDOWSHAPES, NUM_WINDOWSHAPES);
    default:
      return {};
  }
}

BrokenFFTDSP::BrokenFFTDSP(DfxPlugin *inDfxPlugin)
  : DfxPluginCore(inDfxPlugin),
    brokenfft(dynamic_cast<BrokenFFT*>(inDfxPlugin)) {
  /* determine the size of the largest window size */
  constexpr auto maxframe = *std::max_element(BrokenFFT::buffersizes.cbegin(), BrokenFFT::buffersizes.cend());

  /* add some leeway? */
  in0.assign(maxframe, 0.0f);
  out0.assign(maxframe * 2, 0.0f);

  /* prevmix is only a single third long */
  prevmix.assign(maxframe / 2, 0.0f);

  windowenvelope.assign(maxframe, 0.0f);

  const int framesize = BrokenFFT::buffersizes.at(getparameter_i(P_BUFSIZE));
  // does not matter which DSP core, but this just should happen only once
  if (IsFFTDataSource()) {
    getplugin()->setlatency_samples(framesize);
    getplugin()->settailsize_samples(framesize);
  }

  echor = (float*)malloc(MAXECHO * sizeof(float));
  echoc = (float*)malloc(MAXECHO * sizeof(float));

  ampl = (AmplEntry*)malloc(sizeof (AmplEntry) * maxframe);

  fftr = (float*)malloc(maxframe * sizeof(float));
  ffti = (float*)malloc(maxframe * sizeof(float));
  tmp = (float*)malloc(maxframe * sizeof(float));
  oot = (float*)malloc(maxframe * sizeof(float));

  // XXX can just call updatewindowsize (or even reset?)
  plan = rfftw_create_plan(framesize, FFTW_FORWARD, FFTW_ESTIMATE);
  rplan = rfftw_create_plan(framesize, FFTW_BACKWARD, FFTW_ESTIMATE);
}

void BrokenFFTDSP::reset() {
  updatewindowsize();
  updatewindowshape();

  clearwindowcache();
}


void BrokenFFTDSP::processparameters() {
  fft_method = getparameter_i(P_METHOD);

  destruct = getparameter_f(P_DESTRUCT);
  perturb = getparameter_f(P_PERTURB);
  quant = getparameter_f(P_QUANT);
  rotate = getparameter_f(P_ROTATE);
  binquant = getparameter_f(P_BINQUANT);
  spike = getparameter_f(P_SPIKE);
  spikehold = getparameter_f(P_SPIKEHOLD);
  compress = getparameter_f(P_COMPRESS);
  makeupgain = getparameter_f(P_MUG);
  echomix = getparameter_f(P_ECHOMIX);
  echotime = getparameter_f(P_ECHOTIME);
  echomodf = getparameter_f(P_ECHOMODF);
  echomodw = getparameter_f(P_ECHOMODW);
  echofb = getparameter_f(P_ECHOFB);
  echolow = getparameter_f(P_ECHOLOW);
  echohi = getparameter_f(P_ECHOHI);
  postrot = getparameter_f(P_POSTROT);
  lowpass = getparameter_f(P_LOWP);
  moments = getparameter_f(P_MOMENTS);
  bride = getparameter_f(P_BRIDE);
  blow = getparameter_f(P_BLOW);
  convolve = getparameter_f(P_CONV);
  harm = getparameter_f(P_HARM);
  afterlow = getparameter_f(P_ALOW);
  norm = getparameter_f(P_NORM);


  if (getparameterchanged(P_BUFSIZE)) {
    updatewindowsize();
    updatewindowshape();
  } else if (getparameterchanged(P_SHAPE)) {
    updatewindowshape();
  }
}

static float Quantize(float q, float old) {
  /*	if (q >= 1.0) return old;*/
  long scale = (long)(512 * q);
  if (scale == 0) scale = 2;
  long X = (long)(scale * old);

  return (float)(X / (float)scale);
}

// Modifies fftr/ffti in place.
void BrokenFFTDSP::Normalize(long samples, float much) {
  static constexpr float MAXVALUE = 1000.0f;
  float mar = 0.0;
  float mai = 0.0;
  for (int i = 0; i < samples; i ++) {
    if (mar < fabs(fftr[i])) mar = fabs(fftr[i]);
    if (mai < fabs(ffti[i])) mai = fabs(ffti[i]);
  }

  float facr = 1.0, faci = 1.0;

  if (mar > 0.0001) facr = MAXVALUE / mar;

  if (mai > 0.0001) faci = MAXVALUE / mai;

  for (int ji = 0; ji < samples; ji ++) {
    fftr[ji] = (much * facr * fftr[ji]) + ((1.0 - much) * fftr[ji]);
    ffti[ji] = (much * faci * ffti[ji]) + ((1.0 - much) * ffti[ji]);
  }
}

// Rearranges the array (of n elements) so that the first num
// elements are each greater than or equal to the remaining.
//
//    PERF: I think there are faster ways of doing this...
//    XXX: Code can get simpler if using std::swap.
static void PutGreatestFirst(AmplEntry *low, int size, int num) {
  /* temporaries for swaps */
  float t;
  int i;
  if (size <= 1) return;
  if (num <= 0) return;
  if (num >= size) return;

  if (size == 2) {
    if (low->a >= low[1].a) return;
    t = low->a;
    i = low->i;
    low->a = low[1].a;
    low->i = low[1].i;
    low[1].a = t;
    low[1].i = i;
#if 0
  } else if (size == 3) {
    /* add this ... */
#endif
  } else {
    // Pick a random pivot and put it at the beginning.
    int pivot = rand() % size;
    t = low[pivot].a;
    i = low[pivot].i;
    low[pivot].a = low->a;
    low[pivot].i = low->i;
    low->a = t;
    low->i = i;

    int l = 1, r = size - 1;

    // PERF: Could just use std::partition?
    while (l < r) {
      /* move fingers.
	 it's possible to run off the array if
	 the pivot is equal to one of the last
	 or first elements (so check that). */

      while ((l < size) && low[l].a >= low->a) l++;
      while (r && low[r].a <= low->a) r--;
      if (l >= r) break;
      /* swap */
      t = low[l].a;
      i = low[l].i;
      low[l].a = low[r].a;
      low[l].i = low[r].i;
      low[r].a = t;
      low[r].i = i;
      // PERF why not l++, r--?
    }

    /* put the pivot back. */

    t = low[l - 1].a;
    i = low[l - 1].i;
    low[l - 1].a = low->a;
    low[l - 1].i = low->i;
    low->a = t;
    low->i = i;

    /* recurse. */

    if (num <= l) {
      PutGreatestFirst(low, l - 1, num);
    } else {
      PutGreatestFirst(low + l, size - l, num - l);
    }
    /* done. */
  }
}

/* this function modifies 'samples' number of floats in
   fftr and ffti */
void BrokenFFTDSP::FFTOps(long samples) {

  for (int li = lowpass * lowpass * samples; li < samples; li ++) {
    fftr[li] = 0;
    ffti[li] = 0;
  }

  // Convolve.
  // Mysteriously, we do a linear combination of the ith sample with the ith
  // real component (which makes no sense) and additionally multiply the
  // imaginary component with the sample magintude for each imaginary bucket
  // (which makes even less sense).
  if (convolve > 0.0001f) {
    for (int ci = 0; ci < samples; ci ++) {
      fftr[ci] = convolve * tmp[ci] + (1.0 - convolve) * fftr[ci];
      ffti[ci] = convolve * tmp[ci] * ffti[ci] + (1.0 - convolve) * ffti[ci];
    }
    Normalize(samples, 1.0f);
  }

  for (int i = 0; i < samples; i ++) {

    // Operation bq is bin quantization.
    // We copy bin values into a series of consecutive following bins.
    if (binquant > 0.0f) {
      if (samplesleft -- > 0) {
	fftr[i] = sampler;
	ffti[i] = samplei;
      } else {
	sampler = fftr[i];
	samplei = ffti[i];
	samplesleft = 1 + (int)(binquant * binquant * binquant) * (samples >> 3);
      }
    }

    if (destruct < 1.0f) {
      /* very special */
      // PERF rather than do division, we could store 'destruct' as an
      // integer threshold
      if ((rand ()/(float)RAND_MAX) > destruct) {
	/* swap with random sample */
	int j = (int)((rand()/(float)RAND_MAX) * (samples - 1));
	float u = fftr[i];
	fftr[i] = fftr[j];
	fftr[j] = u;

	u = ffti[i];
	ffti[i] = ffti[j];
	ffti[j] = u;
      }
    }

    /* operation Q */
    if (quant <= 0.9999999) {
      fftr[i] = Quantize(fftr[i], quant);
      ffti[i] = Quantize(ffti[i], quant);
    }

    // Perturb just mixes random noise with each sample.
    if (perturb > 0.000001) {
      fftr[i] = (double)((rand()/(float)RAND_MAX) * perturb +
			 fftr[i]*(1.0f-perturb));
      ffti[i] = (double)((rand()/(float)RAND_MAX) * perturb +
			 ffti[i]*(1.0f-perturb));
    }

    /* rotate */
    // XXX should rotate imaginary parts, right?
    // (Note there is also post-rotate. So this is a buggy thing
    // that we should only keep if it's somehow interesting on
    // its own?)
    // (Yes it is kind of great)
    int j = (i + (int)(rotate * samples)) % samples;

    fftr[i] = fftr[j];
    /* XXX and ffti? */

    /* compress */

    if (compress < 0.9999999) {
      fftr[i] = std::copysignf(fftr[i], powf(fabsf(fftr[i]), compress));
      ffti[i] = std::copysignf(ffti[i], powf(fabsf(ffti[i]), compress));
    }

    /* M.U.G. */
    if (makeupgain > 0.00001) {
      fftr[i] *= 1.0f + (3.0f * makeupgain);
      ffti[i] *= 1.0f + (3.0f * makeupgain);
    }

  }

  /* pretty expensive spike operation */
  if (spike < 0.999999) {

    double loudness = 0.0;

    for ( long i=0; i<samples; i++ ) {
      loudness += fftr[i]*fftr[i] + ffti[i]*ffti[i];
    }

    if (! -- amplhold) {
      for ( long i=0; i<samples; i++ ) {
	ampl[i].a = fftr[i]*fftr[i] + ffti[i]*ffti[i];
	ampl[i].i = i;
      }

      /* sort the amplitude bins */

      stopat = 1+(int)((spike*spike*spike)*samples);

      /* consider a special case for when abs(stopat-samples) < lg samples? */
      PutGreatestFirst(ampl, samples, stopat);

      /* chop off everything after the first i */

      amplhold = 1 + (int)(spikehold * (20.0));

    }

    double newloudness = loudness;
    for (long iw = stopat - 1; iw < samples; iw++) {
      newloudness -= ampl[iw].a;
      fftr[ampl[iw].i] = 0.0f;
      ffti[ampl[iw].i] = 0.0f;
    }

    /* boost what remains. */
    double boostby = loudness / newloudness;
    for (long ie = 0; ie < samples; ie ++) {
      fftr[ie] *= boostby;
      /* XXX ffti? */
    }

  }

  /* EO FX */


  for (long iy = 0; iy < samples; iy ++) {
    echor[echoctr] = fftr[iy];
    echoc[echoctr++] = ffti[iy];
    echoctr %= MAXECHO;

    /* you want somma this magic?? */

    if (echomix > 0.000001) {

      double warble = (echomodw * cos(echomodf * (echoctr / (float)MAXECHO)));
      double echott = echotime + warble;
      if (echott > 1.0) echott = 1.0;
      int echot = (MAXECHO - (MAXECHO * echott));
      int xx = (echoctr + echot) % MAXECHO;
      int last = (echoctr + MAXECHO - 1) % MAXECHO;

      if (echofb > 0.00001) {
	echoc[last] += echoc[xx] * echofb;

	echor[last] += echor[xx] * echofb;
      }

      if (iy >= (int)((echolow * echolow * echolow) * samples) &&
	  iy <= (int)((echohi * echohi * echohi) * samples)) {
	fftr[iy] = (1.0f - echomix) * fftr[iy] + echomix * echor[xx];

	ffti[iy] = (1.0f - echomix) * ffti[iy] + echomix * echoc[xx];
      }
    }
  }

  /* bufferride */
  if (bride < 0.999999f) {
    int smd = 0;
    int md = bride * samples;
    for (int ss = 0; ss < samples; ss ++) {
      fftr[ss] = fftr[smd];
      ffti[ss] = ffti[smd];
      smd++;
      if (smd > md) smd = 0;
    }
  }

  /* first moment */
  /* XXX support later moments (and do what with them??) */
  if (moments > 0.00001f) {
    float mtr = 0.0f;
    float mti = 0.0f;

    float magr = 0.0f;
    float magi = 0.0f;

    for (int ih=0; ih < samples; ih ++) {
      float pwr = fftr[ih] * ih;
      float pwi = ffti[ih] * ih;

      mtr += pwr;
      mti += pwi;

      magr += fftr[ih];
      magi += ffti[ih];
    }

    mtr /= magr;
    mti /= magi;

    for (int zc = 0; zc < samples; zc++)
      fftr[zc] = ffti[zc] = 0.0f;

    /* stick it all in one spot. */

    int mtrx = abs(mtr);
    int mtix = abs(mti);

    fftr[mtrx % samples] = magr;
    ffti[mtix % samples] = magi;

  }

  /* XXX sounds crappy */
  /* freq(i) = 44100.0 * i / samples */
  if (harm > 0.000001f) {
    for (int i = 0; i < samples; i ++) {
      /* j = bin(freq(i)/2) */
      int fi = 44100.0 * i /(float) samples;
      int fj = fi / 2.0;
      int j = (int)((fi * (float)samples) / 44100.0);
      if (j >= 0 && j < samples) {
	fftr[i] += fftr[j] * harm;
	ffti[i] += ffti[j] * harm;
      }
    }
  }

  /* XXX I changed 2.0f to 1.0f -- old range was boring... */

  // TODO: Option to have this treat the buffer cyclically?

  // TODO: Since we drop buckets here, should preserve the
  // total amplitude by distributing it among remaining
  // buckets?
  /* post-processing rotate-up */
  if (postrot > 0.5f) {
    int rotn = (postrot - 0.5f) * 1.0f * samples;
    if (rotn != 0) {
      for (int v = samples - 1; v >= 0; v --) {
	if (v < rotn) fftr[v] = ffti[v] = 0.0f;
	else {
	  fftr[v] = fftr[v - rotn];
	  ffti[v] = ffti[v - rotn];
	}
      }
    }
  } else if (postrot < 0.5f) {
    int rotn = (int)((0.5f - postrot) * 1.0f * bufsize);
    if (rotn != 0) {
      for (int v = 0; v < samples; v++) {
	if (v > (samples - rotn)) fftr[v] = ffti[v] = 0.0f;
	else {
	  fftr[v] = fftr[(samples - rotn) - v];
	  ffti[v] = ffti[(samples - rotn) - v];
	}
      }
    }
  }


  if (bride < 0.9999f) {
    int low = blow * blow * samples;

    int modsize = 1 + (bride * bride) * samples;

    for (int ii=0; ii < samples; ii++) {
      fftr[ii] = fftr[((ii + low) % modsize) % samples];
      ffti[ii] = ffti[((ii + low) % modsize) % samples];
    }

  }

  for (int ali = afterlow * afterlow * samples; ali < samples; ali ++) {
    fftr[ali] = 0;
    ffti[ali] = 0;
  }

  if (norm > 0.0001f) {
    Normalize(samples, norm);
  }
}


// This processes an individual window.
// 1. transform into frequency domain
// 2. do various operations (in FFTOps)
// 3. transform back to time domain
void BrokenFFTDSP::ProcessW(const float *in, float *out, int samples) {
  // Geometer version is const, and instead takes all its temporary buffers
  // as arguments, I think so that it can also be called for visualization
  // purposes. Maybe should do that here too?

  // PERF: can we just output directly into out instead of
  // using 'oot'?

  // what is this??
  static int ss;
  ss = !ss;


  // XXX ded?!
  int i = 0;

  for (int c = 0; c < samples; c++) tmp[c] = in[c];

  /* do the processing */
  // TODO: More recent versions of FFTW are (a) probably much faster
  // and (b) support a variety of real-to-real methods that are probably
  // more suitable here (we mostly treat the complex parts as weird
  // baggage in FFTOps). For example there is "Discrete Hartley Transform"
  // (which is its own inverse) and various DCT/DSTs:
  // http://www.fftw.org/fftw3_doc/Real-even_002fodd-DFTs-_0028cosine_002fsine-transforms_0029.html
  // Maybe we could just get rid of imaginaty parts here, but either
  // way, could add some other methods here.
  switch (fft_method) {
  case METHOD_DC: {
    /* Don Cross style */
    static constexpr int FFT_FORWARD = 0;
    static constexpr int FFT_REVERSE = 1;
    fft_float(samples, FFT_FORWARD, tmp, nullptr, fftr, ffti);
    FFTOps(samples);
    fft_float(samples, FFT_REVERSE, fftr, ffti, oot, tmp);
    break;
  }
  default:
  case METHOD_WEST: {
    /* FFTW .. this still doesn't work. How come? */
    rfftw_one(plan, tmp, fftr);
    // (XXX is this actually "halfcomplex" output perhaps, where
    // the real components are the first half and imaginary the second?)
    memcpy(ffti, fftr, samples * sizeof(float)); /* dup? */

    FFTOps(samples);

    rfftw_one(rplan, fftr, oot);

    // reverse(fwd(amplitudes)) actually computes
    // |amplitudes| * amplitudes in FFTW, so normalize the result.
    float div = 1.0 / samples;
    for (int xa = 0; xa < samples; xa++) {
      oot[xa] = oot[xa] * div;
    }
    break;
  }
  case METHOD_WEST_BUG: {
    // (XXX Is this a good bug to keep?)
    /* bug -- using forward plan both ways, not normalizing */
    rfftw_one(plan, tmp, fftr);
    memcpy(ffti, fftr, samples * sizeof(float)); /* dup? */
    FFTOps(samples);
    rfftw_one(plan, fftr, oot);
    break;
  }
  }

  for (int cc = 0; cc < samples; cc++) out[cc] = oot[cc];
}


/* this fake process function reads samples one at a time
   from the true input. It simultaneously copies samples from
   the beginning of the output buffer to the true output.
   We maintain that out0 always has at least 'third' samples
   in it; this is enough to pick up for the delay of input
   processing and to make sure we always have enough samples
   to fill the true output buffer.

   If the input frame is full:
    - calls ProcessW on this full input frame
    - applies the windowing envelope to the tail of out0 (output frame)
    - mixes in prevmix with the first half of the output frame
    - increases outsize so that the first half of the output frame is
      now available output
    - copies the second half of the output to be prevmix for next frame.
    - copies the second half of the input buffer to the first,
      resets the size (thus we process each third-size chunk twice)

  If we have read more than 'third' samples out of the out0 buffer:
   - Slide contents to beginning of buffer
   - Reset outstart

*/
void BrokenFFTDSP::process(float const* tin, float* tout, unsigned long samples) {

  for (unsigned long ii = 0; ii < samples; ii++) {

    /* copy sample in */
    in0[insize] = tin[ii];
    insize++;

    if (insize == framesize) {
      /* frame is full! */

      /* in0 -> process -> out0(first free space) */
      ProcessW(in0.data(), out0.data()+outstart+outsize, framesize);
      /*
               pointx.data(), pointy.data(), framesize * 2,
               storex.data(), storey.data());
      */
      updatewindowcache(this);

#if TARGET_OS_MAC
      vDSP_vmul(out0.data()+outstart+outsize, 1, windowenvelope.data(), 1,
		out0.data()+outstart+outsize, 1, static_cast<vDSP_Length>(framesize));
#else
      for (int z=0; z < framesize; z++) {
        out0[z+outstart+outsize] *= windowenvelope[z];
      }
#endif

      /* mix in prevmix */
      for (int u = 0; u < third; u++)
        out0[u+outstart+outsize] += prevmix[u];

      /* prevmix becomes out1 */
      std::copy_n(std::next(out0.cbegin(), outstart + outsize + third), third, prevmix.begin());

      /* copy 2nd third of input over in0 (need to re-use it for next frame),
         now insize = third */
      std::copy_n(std::next(in0.cbegin(), third), third, in0.begin());

      insize = third;

      outsize += third;
    }

    /* send sample out */
    tout[ii] = out0[outstart];

    outstart++;
    outsize--;

    /* make sure there is always enough room for a frame in out buffer */
    if (outstart == third) {
      memmove(out0.data(), out0.data() + outstart, outsize * sizeof (out0.front()));
      outstart = 0;
    }
  }
}


void BrokenFFTDSP::updatewindowsize() {
  // (XXX is this the right place to be clearing stuff like the echo buffer?)
  // This was in "resume" in the oldskool implementation.
  for (int i = 0; i < MAXECHO; i++) {
    echor[i] = echoc[i] = 0.0f;
  }
  echoctr = 0;
  
  amplhold = 1;
  stopat = 1;


  // (XXX arguably these should be local to FFTOps... otherwise
  // we get different alignments when the quantization length
  // doesn't divide the window size?)
  sampler = samplei = 0.0f;
  samplesleft = 0;

  
  // Window size stuff...
  
  framesize = BrokenFFT::buffersizes.at(getparameter_i(P_BUFSIZE));
  third = framesize / 2;
  bufsize = third * 3;

  /* set up buffers. prevmix and first frame of output are always
     filled with zeros. */

  std::fill(prevmix.begin(), prevmix.end(), 0.0f);

  std::fill(out0.begin(), out0.end(), 0.0f);

  /* start input at beginning. Output has a frame of silence. */
  insize = 0;
  outstart = 0;
  outsize = framesize;

  getplugin()->setlatency_samples(framesize);
  /* tail is the same as delay, of course */
  getplugin()->settailsize_samples(framesize);

  // FFT size depends on window size
  plan = rfftw_create_plan(framesize, FFTW_FORWARD, FFTW_ESTIMATE);
  rplan = rfftw_create_plan(framesize, FFTW_BACKWARD, FFTW_ESTIMATE);
}


void BrokenFFTDSP::updatewindowshape() {
  shape = getparameter_i(P_SHAPE);

  float const oneDivThird = 1.0f / static_cast<float>(third);
  switch(shape) {
  case WINDOW_TRIANGLE:
    for (int z = 0; z < third; z++) {
      windowenvelope[z] = (static_cast<float>(z) * oneDivThird);
      windowenvelope[z + third] = (1.0f - (static_cast<float>(z) * oneDivThird));
    }
    break;
  case WINDOW_ARROW:
    for (int z = 0; z < third; z++) {
      float p = static_cast<float>(z) * oneDivThird;
      p *= p;
      windowenvelope[z] = p;
      windowenvelope[z + third] = 1.0f - p;
    }
    break;
  case WINDOW_WEDGE:
    for (int z = 0; z < third; z++) {
      const float p = std::sqrt(static_cast<float>(z) * oneDivThird);
      windowenvelope[z] = p;
      windowenvelope[z + third] = 1.0f - p;
    }
    break;
  case WINDOW_BEST:
    for (int z = 0; z < third; z++) {
      const float p = 0.5f * (-std::cos(dfx::math::kPi<float> * (static_cast<float>(z) * oneDivThird)) + 1.0f);
      windowenvelope[z] = p;
      windowenvelope[z + third] = 1.0f - p;
    }
    break;
  case WINDOW_COS2:
    for (int z = 0; z < third; z++) {
      float p = 0.5f * (-std::cos(dfx::math::kPi<float> * (static_cast<float>(z) * oneDivThird)) + 1.0f);
      p *= p;
      windowenvelope[z] = p;
      windowenvelope[z + third] = 1.0f - p;
    }
    break;
  }
}


void BrokenFFT::makepresets() {
  long i = 1;

  setpresetname(i, "replace this pls");
  setpresetparameter_i(i, P_BUFSIZE, 9);
  setpresetparameter_f(i, P_ROTATE, 0.5f);
  i++;
}
