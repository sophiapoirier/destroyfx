/*------------------------------------------------------------------------
Copyright (C) 2002-2021  Tom Murphy 7 and Sophia Poirier

This file is part of Geometer.

Geometer is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Geometer is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Geometer.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Geometer,
Featuring the Super Destroy FX Windowing System!
------------------------------------------------------------------------*/

#include "geometer.h"

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

/* this macro does boring entry point stuff for us */
DFX_EFFECT_ENTRY(Geometer)
DFX_CORE_ENTRY(PLUGINCORE)

PLUGIN::PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, NUM_PARAMS, NUM_PRESETS) {

  initparameter_list(P_BUFSIZE, {"wsize", "WSiz"}, 9, 9, BUFFERSIZESSIZE, DfxParam::Unit::Samples);
  initparameter_list(P_SHAPE, {"wshape", "WShp"}, WINDOW_TRIANGLE, WINDOW_TRIANGLE, MAX_WINDOWSHAPES);

  initparameter_list(P_POINTSTYLE, {"points where", "PntWher", "PWhere", "PWhr"}, POINT_EXTNCROSS, POINT_EXTNCROSS, MAX_POINTSTYLES);

  initparameter_f(P_POINTPARAMS + POINT_EXTNCROSS, {"point:ext'n'cross", "PExtCrs", "PExtCr"}, 0.0, 0.0, 0.0, 1.0, DfxParam::Unit::Custom, DfxParam::Curve::Linear, "magn");
  initparameter_f(P_POINTPARAMS + POINT_FREQ, {"point:freq", "PntFreq", "PFreq", "PFrq"}, 0.08, 0.08, 0.0, 1.0, DfxParam::Unit::Scalar);
  initparameter_f(P_POINTPARAMS + POINT_RANDOM, {"point:rand", "PntRand", "PRand", "PRnd"}, 0.20, 0.20, 0.0, 1.0, DfxParam::Unit::Scalar);
  initparameter_f(P_POINTPARAMS + POINT_SPAN, {"point:span", "PntSpan", "PSpan", "PSpn"}, 0.20, 0.20, 0.0, 1.0, DfxParam::Unit::Custom, DfxParam::Curve::Linear, "width");
  initparameter_f(P_POINTPARAMS + POINT_DYDX, {"point:dydx", "PntDyDx", "PDyDx", "PDyx"}, 0.50, 0.50, 0.0, 1.0, DfxParam::Unit::Custom, DfxParam::Curve::Linear, "gap");
  initparameter_f(P_POINTPARAMS + POINT_LEVEL, {"point:level", "PntLevl", "PLevel" "PLvl"}, 0.50, 0.50, 0.0, 1.0, DfxParam::Unit::Custom, DfxParam::Curve::Linear, "level");

  for(int pp = NUM_POINTSTYLES; pp < MAX_POINTSTYLES; pp++) {
    initparameter_f(P_POINTPARAMS + pp, {"pointparam:unused", "PUnused", "Pxxx"}, 0.04, 0.04, 0.0, 1.0, DfxParam::Unit::Generic);
    setparameterattributes(P_POINTPARAMS + pp, DfxParam::kAttribute_Unused);	/* don't display as an available parameter */
  }

  initparameter_list(P_INTERPSTYLE, {"interpolate how", "IntHow", "IHow"}, INTERP_POLYGON, INTERP_POLYGON, MAX_INTERPSTYLES);

  initparameter_f(P_INTERPARAMS + INTERP_POLYGON, {"interp:polygon", "IntPoly", "IPoly", "IPly"}, 0.0, 0.0, 0.0, 1.0, DfxParam::Unit::Custom, DfxParam::Curve::Linear, "angle");
  initparameter_f(P_INTERPARAMS + INTERP_WRONGYGON, {"interp:wrongy", "IWrongy", "IWrong", "IWng"}, 0.0, 0.0, 0.0, 1.0, DfxParam::Unit::Custom, DfxParam::Curve::Linear, "angle");
  initparameter_f(P_INTERPARAMS + INTERP_SMOOTHIE, {"interp:smoothie", "ISmooth", "ISmoth", "ISmt"}, 0.5, 0.5, 0.0, 1.0, DfxParam::Unit::Exponent);
  initparameter_f(P_INTERPARAMS + INTERP_REVERSI, {"interp:reversie", "IRevers", "IRevrs", "IRvr"}, 0.0, 0.0, 0.0, 1.0, DfxParam::Unit::Generic);
  initparameter_f(P_INTERPARAMS + INTERP_PULSE, {"interp:pulse", "IPulse", "IPls"}, 0.05, 0.05, 0.0, 1.0, DfxParam::Unit::Custom, DfxParam::Curve::Linear, "pulse");
  initparameter_f(P_INTERPARAMS + INTERP_FRIENDS, {"interp:friends", "IFriend", "IFrend", "IFrn"}, 1.0, 1.0, 0.0, 1.0, DfxParam::Unit::Custom, DfxParam::Curve::Linear, "width");
  initparameter_f(P_INTERPARAMS + INTERP_SING, {"interp:sing", "IntSing", "ISing", "ISng"}, 0.8, 0.8, 0.0, 1.0, DfxParam::Unit::Custom, DfxParam::Curve::Linear, "mod");
  initparameter_f(P_INTERPARAMS + INTERP_SHUFFLE, {"interp:shuffle", "IShuffl", "IShufl", "IShf"}, 0.3, 0.3, 0.0, 1.0, DfxParam::Unit::Generic);

  for(int ip = NUM_INTERPSTYLES; ip < MAX_INTERPSTYLES; ip++) {
    initparameter_f(P_INTERPARAMS + ip, {"inter:unused", "IUnused", "Ixxx"}, 0.0, 0.0, 0.0, 1.0, DfxParam::Unit::Generic);
    setparameterattributes(P_INTERPARAMS + ip, DfxParam::kAttribute_Unused);	/* don't display as an available parameter */
  }

  initparameter_list(P_POINTOP1, {"pointop1", "PntOp1", "POp1"}, OP_NONE, OP_NONE, MAX_OPS);
  initparameter_list(P_POINTOP2, {"pointop2", "PntOp2", "POp2"}, OP_NONE, OP_NONE, MAX_OPS);
  initparameter_list(P_POINTOP3, {"pointop3", "PntOp3", "POp3"}, OP_NONE, OP_NONE, MAX_OPS);

  auto const allop = [this](auto n, auto str, auto shortstr, auto def, auto unit, std::string_view unitstr) {
    constexpr auto curve = DfxParam::Curve::Linear;
    initparameter_f(P_OPPAR1S + n, {std::string("op1:") + str, std::string("1") + shortstr}, def, def, 0.0, 1.0, unit, curve, unitstr);
    initparameter_f(P_OPPAR2S + n, {std::string("op2:") + str, std::string("2") + shortstr}, def, def, 0.0, 1.0, unit, curve, unitstr);
    initparameter_f(P_OPPAR3S + n, {std::string("op3:") + str, std::string("3") + shortstr}, def, def, 0.0, 1.0, unit, curve, unitstr);
  };

  allop(OP_DOUBLE, "double", "duble", 0.5, DfxParam::Unit::LinearGain, {});
  allop(OP_HALF, "half", "half", 0.0, DfxParam::Unit::Generic, {});
  allop(OP_QUARTER, "quarter", "qrtr", 0.0, DfxParam::Unit::Generic, {});
  allop(OP_LONGPASS, "longpass", "lngps", 0.15, DfxParam::Unit::Custom, "length");
  allop(OP_SHORTPASS, "shortpass", "srtps", 0.5, DfxParam::Unit::Custom, "length");
  allop(OP_SLOW, "slow", "slow", 0.25, DfxParam::Unit::Scalar, {});	// "factor"
  allop(OP_FAST, "fast", "fast", 0.5, DfxParam::Unit::Scalar, {});	// "factor"
  allop(OP_NONE, "none", "none", 0.0, DfxParam::Unit::Generic, {});
  
  for(int op = NUM_OPS; op < MAX_OPS; op++) {
    allop(op, "unused", "xxx", 0.5, DfxParam::Unit::Generic, {});
    setparameterattributes(P_OPPAR1S + op, DfxParam::kAttribute_Unused);	/* don't display as an available parameter */
    setparameterattributes(P_OPPAR2S + op, DfxParam::kAttribute_Unused);	/* don't display as an available parameter */
    setparameterattributes(P_OPPAR3S + op, DfxParam::kAttribute_Unused);	/* don't display as an available parameter */
  }

  /* windowing */
  for (size_t i=0; i < buffersizes.size(); i++)
  {
    std::array<char, dfx::kParameterValueStringMaxLength> bufstr {};
    constexpr int thousand = 1000;
    if (buffersizes[i] >= thousand) {
      snprintf(bufstr.data(), bufstr.size(), "%d,%03d", buffersizes[i] / thousand, buffersizes[i] % thousand);
    } else {
      snprintf(bufstr.data(), bufstr.size(), "%d", buffersizes[i]);
    }
    setparametervaluestring(P_BUFSIZE, static_cast<long>(i), bufstr.data());
  }
  setparametervaluestring(P_SHAPE, WINDOW_TRIANGLE, "linear");
  setparametervaluestring(P_SHAPE, WINDOW_ARROW, "arrow");
  setparametervaluestring(P_SHAPE, WINDOW_WEDGE, "wedge");
  setparametervaluestring(P_SHAPE, WINDOW_COS, "best");
  for (long i=NUM_WINDOWSHAPES; i < MAX_WINDOWSHAPES; i++)
    setparametervaluestring(P_SHAPE, i, "???");
  /* geometer */
  setparametervaluestring(P_POINTSTYLE, POINT_EXTNCROSS, "ext 'n cross");
  setparametervaluestring(P_POINTSTYLE, POINT_LEVEL, "at level");
  setparametervaluestring(P_POINTSTYLE, POINT_FREQ, "at freq");
  setparametervaluestring(P_POINTSTYLE, POINT_RANDOM, "randomly");
  setparametervaluestring(P_POINTSTYLE, POINT_SPAN, "span");
  setparametervaluestring(P_POINTSTYLE, POINT_DYDX, "dy/dx");
  for (long i=NUM_POINTSTYLES; i < MAX_POINTSTYLES; i++)
    setparametervaluestring(P_POINTSTYLE, i, "unsup");
  setparametervaluestring(P_INTERPSTYLE, INTERP_POLYGON, "polygon");
  setparametervaluestring(P_INTERPSTYLE, INTERP_WRONGYGON, "wrongygon");
  setparametervaluestring(P_INTERPSTYLE, INTERP_SMOOTHIE, "smoothie");
  setparametervaluestring(P_INTERPSTYLE, INTERP_REVERSI, "reversi");
  setparametervaluestring(P_INTERPSTYLE, INTERP_PULSE, "pulse");
  setparametervaluestring(P_INTERPSTYLE, INTERP_FRIENDS, "friends");
  setparametervaluestring(P_INTERPSTYLE, INTERP_SING, "sing");
  setparametervaluestring(P_INTERPSTYLE, INTERP_SHUFFLE, "shuffle");
  for (long i=NUM_INTERPSTYLES; i < MAX_INTERPSTYLES; i++)
    setparametervaluestring(P_INTERPSTYLE, i, "unsup");
  auto const allopstr = [this](auto n, auto str) {
    setparametervaluestring(P_POINTOP1, n, str);
    setparametervaluestring(P_POINTOP2, n, str);
    setparametervaluestring(P_POINTOP3, n, str);
  };
  allopstr(OP_DOUBLE, "x2");
  allopstr(OP_HALF, "1/2");
  allopstr(OP_QUARTER, "1/4");
  allopstr(OP_LONGPASS, "longpass");
  allopstr(OP_SHORTPASS, "shortpass");
  allopstr(OP_SLOW, "slow");
  allopstr(OP_FAST, "fast");
  allopstr(OP_NONE, "none");
  for (long i=NUM_OPS; i < MAX_OPS; i++)
    allopstr(i, "unsup");

  addparametergroup("windowing", {P_BUFSIZE, P_SHAPE});
  auto const addparameterrangegroup = [this](auto name, long parameterIndexBegin, long parameterIndexEnd) {
    assert(parameterIndexBegin < parameterIndexEnd);
    std::vector<long> parameters(parameterIndexEnd - parameterIndexBegin, dfx::kParameterID_Invalid);
    std::iota(parameters.begin(), parameters.end(), parameterIndexBegin);
    addparametergroup(name, parameters);
  };
  addparameterrangegroup("points", P_POINTSTYLE, P_INTERPSTYLE);
  addparameterrangegroup("interpolation", P_INTERPSTYLE, P_POINTOP1);
  addparameterrangegroup("pointop1", P_POINTOP1, P_POINTOP2);
  addparameterrangegroup("pointop2", P_POINTOP2, P_POINTOP3);
  addparameterrangegroup("pointop3", P_POINTOP3, NUM_PARAMS);

  setpresetname(0, "Geometer LoFi");	/* default preset name */
  makepresets();

  windowcache_reader = &windowcaches.front();
  windowcache_writer = &windowcaches.back();
  tmpx.fill(0);
  tmpy.fill(0.0f);
}

void PLUGIN::dfx_PostConstructor()
{
  /* since we don't use notes for any specialized control of Geometer, 
     allow them to be assigned to control parameters via MIDI learn */
  getsettings().setAllowPitchbendEvents(true);
  getsettings().setAllowNoteEvents(true);
}

long PLUGIN::dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                                 size_t& outDataSize, dfx::PropertyFlags& outFlags)
{
  switch (inPropertyID)
  {
    case PROP_LAST_WINDOW_TIMESTAMP:
      outDataSize = sizeof(uint64_t);
      outFlags = dfx::kPropertyFlag_Readable;
      return dfx::kStatus_NoError;
    case PROP_WAVEFORM_DATA:
      outDataSize = sizeof(GeometerViewData);
      outFlags = dfx::kPropertyFlag_Readable;
      return dfx::kStatus_NoError;
    default:
      return DfxPlugin::dfx_GetPropertyInfo(inPropertyID, inScope, inItemIndex, outDataSize, outFlags);
  }
}

long PLUGIN::dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                             void* outData)
{
  switch (inPropertyID)
  {
    case PROP_LAST_WINDOW_TIMESTAMP:
      *static_cast<uint64_t*>(outData) = lastwindowtimestamp.load(std::memory_order_relaxed);
      return dfx::kStatus_NoError;
    case PROP_WAVEFORM_DATA: {
      std::lock_guard const guard(windowcachelock);
      *static_cast<GeometerViewData*>(outData) = *windowcache_reader;
      return dfx::kStatus_NoError;
    }
    default:
      return DfxPlugin::dfx_GetProperty(inPropertyID, inScope, inItemIndex, outData);
  }
}

void PLUGIN::randomizeparameter(long inParameterIndex)
{
  // we need to constrain the range of values of the parameters that have extra (currently unused) room for future expansion
  int64_t maxValue = 0;
  switch (inParameterIndex)
  {
    case P_SHAPE:
      maxValue = NUM_WINDOWSHAPES;
      break;
    case P_POINTSTYLE:
      maxValue = NUM_POINTSTYLES;
      break;
    case P_INTERPSTYLE:
      maxValue = NUM_INTERPSTYLES;
      break;
    case P_POINTOP1:
    case P_POINTOP2:
    case P_POINTOP3:
      maxValue = NUM_OPS;
      break;
    default:
      DfxPlugin::randomizeparameter(inParameterIndex);
      return;
  }

  int64_t const newValue = rand() % maxValue;
  setparameter_i(inParameterIndex, newValue);

  postupdate_parameter(inParameterIndex);	// inform any parameter listeners of the changes
}

void PLUGIN::clearwindowcache()
{
  windowcache_writer->clear();
  {
    std::unique_lock const guard(windowcachelock, std::try_to_lock);
    if (guard.owns_lock()) {
      std::swap(windowcache_reader, windowcache_writer);
    }
  }
  lastwindowtimestamp.store(0, std::memory_order_relaxed);
}

void PLUGIN::updatewindowcache(PLUGINCORE const * geometercore)
{
#if 1
  std::copy_n(geometercore->getinput(), GeometerViewData::samples, windowcache_writer->inputs.data());
#else
  for (int i=0; i < GeometerViewData::samples; i++) {
    windowcache_writer->inputs[i] = std::sin((i * 10 * dfx::math::kPi<float>) / GeometerViewData::samples);
  }
#endif

  windowcache_writer->apts = std::min(geometercore->getframesize(), GeometerViewData::samples);

  windowcache_writer->numpts = geometercore->processw(windowcache_writer->inputs.data(), windowcache_writer->outputs.data(), 
                                                      windowcache_writer->apts,
                                                      windowcache_writer->pointsx.data(), windowcache_writer->pointsy.data(),
                                                      GeometerViewData::samples - 1, tmpx.data(), tmpy.data());

  bool updated = false;
  {
    // willing to drop window cache updates to ensure realtime-safety by not blocking here
    std::unique_lock const guard(windowcachelock, std::try_to_lock);
    if ((updated = guard.owns_lock())) {
      std::swap(windowcache_reader, windowcache_writer);
    }
  }

  if (updated) {
    lastwindowtimestamp.store(std::chrono::steady_clock::now().time_since_epoch().count(), std::memory_order_relaxed);
  }
}

void PLUGINCORE::clearwindowcache()
{
  if (iswaveformsource()) {
    geometer->clearwindowcache();
  }
}

void PLUGINCORE::updatewindowcache(PLUGINCORE const * geometercore)
{
  if (iswaveformsource()) {
    geometer->updatewindowcache(geometercore);
  }
}

std::optional<dfx::ParameterAssignment> PLUGIN::settings_getLearningAssignData(long inParameterIndex) const
{
  auto const getConstrainedToggleAssignment = [](long inNumStates, long inNumUsableStates)
  {
    dfx::ParameterAssignment result;
    result.mEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_Toggle;
    result.mDataInt1 = inNumStates;
    result.mDataInt2 = inNumUsableStates;
    return result;
  };

  switch (inParameterIndex)
  {
    case P_SHAPE:
      return getConstrainedToggleAssignment(MAX_WINDOWSHAPES, NUM_WINDOWSHAPES);
    case P_POINTSTYLE:
      return getConstrainedToggleAssignment(MAX_POINTSTYLES, NUM_POINTSTYLES);
    case P_INTERPSTYLE:
      return getConstrainedToggleAssignment(MAX_INTERPSTYLES, NUM_INTERPSTYLES);
    case P_POINTOP1:
    case P_POINTOP2:
    case P_POINTOP3:
      return getConstrainedToggleAssignment(MAX_OPS, NUM_OPS);
    default:
      return {};
  }
}

PLUGINCORE::PLUGINCORE(DfxPlugin* inDfxPlugin)
  : DfxPluginCore(inDfxPlugin),
    geometer(dynamic_cast<PLUGIN*>(inDfxPlugin))
{
  /* determine the size of the largest window size */
  constexpr auto maxframe = *std::max_element(PLUGIN::buffersizes.cbegin(), PLUGIN::buffersizes.cend());
  static_assert(maxframe >= GeometerViewData::samples);

  /* add some leeway? */
  in0.assign(maxframe, 0.0f);
  out0.assign(maxframe * 2, 0.0f);

  /* prevmix is only a single third long */
  prevmix.assign(maxframe / 2, 0.0f);

  /* geometer buffers */
  pointx.assign(maxframe * 2 + 3, 0);
  storex.assign(maxframe * 2 + 3, 0);

  pointy.assign(maxframe * 2 + 3, 0.0f);
  storey.assign(maxframe * 2 + 3, 0.0f);

  windowenvelope.assign(maxframe, 0.0f);

  if (iswaveformsource()) {  // does not matter which DSP core, but this just should happen only once
    auto const delay_samples = PLUGIN::buffersizes.at(getparameter_i(P_BUFSIZE));
    getplugin()->setlatency_samples(delay_samples);
    getplugin()->settailsize_samples(delay_samples);
  }
}

void PLUGINCORE::reset() {

  updatewindowsize();
  updatewindowshape();

  clearwindowcache();
}


void PLUGINCORE::processparameters() {

  pointstyle = getparameter_i(P_POINTSTYLE);
  pointparam = getparameter_f(P_POINTPARAMS + pointstyle);
  interpstyle = getparameter_i(P_INTERPSTYLE);
  interparam = getparameter_f(P_INTERPARAMS + interpstyle);
  pointop1 = getparameter_i(P_POINTOP1);
  oppar1 = getparameter_f(P_OPPAR1S + pointop1);
  pointop2 = getparameter_i(P_POINTOP2);
  oppar2 = getparameter_f(P_OPPAR2S + pointop2);
  pointop3 = getparameter_i(P_POINTOP3);
  oppar3 = getparameter_f(P_OPPAR3S + pointop3);

  if (getparameterchanged(P_BUFSIZE)) {
    updatewindowsize();
    updatewindowshape();
  }
  else if (getparameterchanged(P_SHAPE)) {
    updatewindowshape();
  }
}


/* operations on points. this is a separate function
   because it is called once for each operation slot.
   It's static to enforce thread-safety.
*/
int PLUGINCORE::pointops(long pop, int npts, float op_param, int samples,
                         int * px, float * py, int maxpts,
                         int * tempx, float * tempy) {
  /* pointops. */

  switch(pop) {
  case OP_DOUBLE: {
    /* x2 points */
    int t = 0;
    for(int i = 0; i < (npts - 1) && t < (maxpts - 4); i++) {
      /* always include the actual point */
      tempx[t] = px[i];
      tempy[t] = py[i];
      t++;
      /* now, only if there's room... */
      if ((i < npts) && ((px[i+1] - px[i]) > 1)) {
        /* add an extra point, right between the old points.
           (the idea is to double the frequency).
           Pick its y coordinate according to the parameter.
        */

        tempy[t] = (op_param * 2.0f - 1.0f) * py[i];
        tempx[t] = (px[i] + px[i+1]) >> 1;

        t++;
      }
    }
    /* include last if not different from previous */
    if (t > 0 && npts > 0 && tempx[t-1] != px[npts-1]) {
      tempx[t] = px[npts-1];
      tempy[t] = py[npts-1];
      t++;
    }

    for(int c = 0; c < t; c++) {
      px[c] = tempx[c];
      py[c] = tempy[c];
    }
    npts = t;
    break;
  }
  case OP_HALF:
  case OP_QUARTER: {
    int const times = (pop == OP_QUARTER) ? 2 : 1;
    for(int t = 0; t < times; t++) {
      /* cut points in half. never touch first or last. */
      int q = 1;
      int i = 1;
      for(; q < (npts - 1); i++) {
        px[i] = px[q];
        py[i] = py[q];
        q += 2;
      }
      px[i] = px[npts - 1];
      py[i] = py[npts - 1];
      npts = i+1;
    }
    break;
  }
  case OP_LONGPASS: {
    /* longpass. drop any point that's not at least param*samples
       past the previous. */ 
    /* XXX this can cut out the last point? */
    tempx[0] = px[0];
    tempy[0] = py[0];

    int const stretch = (op_param * op_param) * samples;
    int np = 1;

    for(int i=1; i < (npts-1); i++) {
      if (px[i] - tempx[np-1] > stretch) {
        tempx[np] = px[i];
        tempy[np] = py[i];
        np++;
        if (np == maxpts) break;
      }
    }

    for(int c = 1; c < np; c++) {
      px[c] = tempx[c];
      py[c] = tempy[c];
    }
    
    px[np] = px[npts-1];
    py[np] = py[npts-1];
    np++;

    npts = np;

    break;
  }
  case OP_SHORTPASS: {
    /* shortpass. If an interval is longer than the
       specified amount, zero the 2nd endpoint.
    */

    int const stretch = (op_param * op_param) * samples;

    for (int i=1; i < npts; i++) {
      if (px[i] - px[i-1] > stretch) py[i] = 0.0f;
    }

    break;
  }
  case OP_SLOW: {
    /* slow points down. stretches the points out so that
       the tail is lost, but preserves their y values. */
    
    float const factor = 1.0f + op_param;

    /* We don't need to worry about maxpoints, since
       we will just be moving existing samples (and
       truncating)... */
    int i = 0;
    for(; i < (npts-1); i++) {
      px[i] *= factor;
      if (px[i] > samples) {
        /* this sample can't stay. */
        i--;
        break;
      }
    }
    /* but save last point */
    px[i] = px[npts-1];
    py[i] = py[npts-1];
    
    npts = i + 1;

    break;
  }
  case OP_FAST: {

    float const factor = 1.0f + (op_param * 3.0f);
    float const onedivfactor = 1.0f / factor;

    /* number of times we need to loop through samples */
    int const times = (int)(factor + 1.0f);

    int outi = 0;
    for(int rep = 0; rep < times; rep++) {
      /* where this copy of the points begins */
      int const offset = rep * (onedivfactor * samples);
      for (int s = 0; s < npts; s++) {
        /* XXX is destx in range? */
        int const destx = offset + (px[s] * onedivfactor);

        if (destx >= samples) goto op_fast_out_of_points;

        /* check if we already have one here.
           if not, add it and advance, otherwise ignore. 
           XXX: one possibility would be to mix...
        */
        if (!(outi > 0 && tempx[outi-1] == destx)) {
          tempx[outi] = destx;
          tempy[outi] = py[s];
          outi++;
        } 

        if (outi > (maxpts - 2)) goto op_fast_out_of_points;
      }
    }

  op_fast_out_of_points:
    
    /* always save last sample, as usual */
    tempx[outi] = px[npts - 1];
    tempy[outi] = py[npts - 1];

    /* copy.. */

    for(int c = 1; c < outi; c++) {
      px[c] = tempx[c];
      py[c] = tempy[c];
    }

    npts = outi;

    break;
  }
  case OP_NONE:
  default:
    /* nothing ... */
    break;

  } /* end of main switch(op) statement */

  return npts;
}

/* this processes an individual window.
   1. generate points
   2. do operations on points (in slots op1, op2, op3)
   3. generate waveform
*/
int PLUGINCORE::processw(float const * in, float * out, int samples,
                     int * px, float * py, int maxpts,
                     int * tempx, float * tempy) const {

  /* collect points. */

  px[0] = 0;
  py[0] = in[0];
  int numpts = 1;

  switch(pointstyle) {

  case POINT_EXTNCROSS: {
    /* extremities and crossings 
       XXX: Can this generate points out of order? Don't think so...
    */

    float ext = 0.0f;
    int extx = 0;

    enum {SZ, SZC, SA, SB};
    int state = SZ;

    for(int i = 0; i < samples; i++) {
      switch(state) {
      case SZ: {
        /* just output a zero. */
        if (in[i] <= pointparam && in[i] >= -pointparam) state = SZC;
        else if (in[i] < -pointparam) { state = SB; ext = in[i]; extx = i; }
        else { state = SA; ext = in[i]; extx = i; }
        break;
      }
      case SZC: {
        /* continuing zeros */
        if (in[i] <= pointparam && in[i] >= -pointparam) break;

        /* push zero for last spot (we know it was a zero and not pushed). */
        if (numpts < (maxpts-1)) {
          px[numpts] = (i>0)?(i - 1):0;
          py[numpts] = 0.0f;
          numpts++;
        }

        if (in[i] < 0.0f) { 
          state = SB;
        } else {
          state = SA;
        }

        ext = in[i];
        extx = i;
        break;
      }
      case SA: {
        /* above zero */

        if (in[i] <= pointparam) {
          /* no longer above 0. push the highest point I reached. */
          if (numpts < (maxpts-1)) {
            px[numpts] = extx;
            py[numpts] = ext;
            numpts++;
          }
          /* and decide state */
          if (in[i] >= -pointparam) {
            if (numpts < (maxpts-1)) {
              px[numpts] = i;
              py[numpts] = 0.0f;
              numpts++;
            }
            state = SZ;
          } else {
            state = SB;
            ext = in[i];
            extx = i;
          }
        } else {
          if (in[i] > ext) {
            ext = in[i];
            extx = i;
          }
        }
        break;
      }
      case SB: {
        /* below zero */

        if (in[i] >= -pointparam) {
          /* no longer below 0. push the lowest point I reached. */
          if (numpts < (maxpts-1)) {
            px[numpts] = extx;
            py[numpts] = ext;
            numpts++;
          }
          /* and decide state */
          if (in[i] <= pointparam) {
            if (numpts < (maxpts-1)) {
              px[numpts] = i;
              py[numpts] = 0.0f;
              numpts++;
            }
            state = SZ;
          } else {
            state = SA;
            ext = in[i];
            extx = i;
          }
        } else {
          if (in[i] < ext) {
            ext = in[i];
            extx = i;
          }
        }
        break;
      }
      }
    }

    break;
  }

  case POINT_LEVEL: {

    enum { ABOVE, BETWEEN, BELOW };

    int state = BETWEEN;
    float const level = (pointparam * .9999f) + .00005f;
    numpts = 1;

    px[0] = 0;
    py[0] = in[0];

    for(int i = 0; i < samples; i++) {

      if (in[i] > level) {
        if (state != ABOVE) {
          px[numpts] = i;
          py[numpts] = in[i];
          numpts++;
          state = ABOVE;
        }
      } else if (in[i] < -level) {
        if (state != BELOW) {
          px[numpts] = i;
          py[numpts] = in[i];
          numpts++;
          state = BELOW;
        }
      } else {
        if (state != BETWEEN) {
          px[numpts] = i;
          py[numpts] = in[i];
          numpts++;
          state = BETWEEN;
        }
      }

      if (numpts > samples - 2) break;
    }

    px[numpts] = samples - 1;
    py[numpts] = in[samples - 1];

    numpts++;

    break;
  }
  case POINT_FREQ: {
    /* at frequency */

    /* XXX let the user choose hz, do conversion */
    int const nth = (pointparam * pointparam) * samples;
    int ctr = nth;
  
    for(int i = 0; i < samples; i++) {
      ctr--;
      if (ctr <= 0) {
        if (numpts < (maxpts-1)) {
          px[numpts] = i;
          py[numpts] = in[i];
          numpts++;
        } else break; /* no point in continuing... */
        ctr = nth;
      }
    }

    break;
  }

  case POINT_RANDOM: {
    /* randomly */

    int n = (int)(1.0f - pointparam) * samples;

    for(;n--;) {
      if (numpts < (maxpts-1)) {
        px[numpts++] = rand() % samples;
      } else break;
    }

    /* sort them */

    std::sort(px, px + numpts);

    for (int sd = 0; sd < numpts; sd++) {
      py[sd] = in[px[sd]];
    }

    break;
  }
  
  case POINT_SPAN: {
    /* next x determined by sample magnitude
       
    suggested by bram.
    */

    int const span = (pointparam * pointparam) * samples;

    int i = abs((int)(py[0] * span)) + 1;

    while (i < samples) {
      px[numpts] = i;
      py[numpts] = in[i];
      numpts++;
      i = i + abs((int)(in[i] * span)) + 1;
    }

    break;
  }
  
  case POINT_DYDX: {
    /* dy/dx */
    bool lastsign = false;
    float lasts = in[0];

    px[0] = 0;
    py[0] = in[0];
    numpts = 1;

    float pp {};
    bool above {};
    if (pointparam > 0.5f) {
      pp = pointparam - 0.5f;
      above = true;
    } else {
      pp = 0.5f - pointparam;
      above = false;
    }

    pp = std::pow(pp, 2.7f);

    for (int i = 1; i < samples; i++) {
      
      bool sign {};
      if (above)
        sign = (in[i] - lasts) > pp;
      else
        sign = (in[i] - lasts) < pp;

      lasts = in[i];

      if (sign != lastsign) {
        px[numpts] = i;
        py[numpts] = in[i];
        numpts++;
        if (numpts > (maxpts-1)) break;
      }

      lastsign = sign;
    }

    break;

  }
  default:
    /* nothing, unsupported... */
    numpts = 1;
    break;

  } /* end of pointstyle cases */


  /* always push final point for continuity (we saved room) */
  px[numpts] = samples-1;
  py[numpts] = in[samples-1];
  numpts++;

  /* modify the points according to the three slots and
     their parameters */

  numpts = pointops(pointop1, numpts, oppar1, samples, 
                    px, py, maxpts, tempx, tempy);
  numpts = pointops(pointop2, numpts, oppar2, samples, 
                    px, py, maxpts, tempx, tempy);
  numpts = pointops(pointop3, numpts, oppar3, samples, 
                    px, py, maxpts, tempx, tempy);

  switch(interpstyle) {

  case INTERP_SHUFFLE: {
    /* mix around the intervals. The parameter determines
       how mobile an interval is. 

       I build an array of interval indices (integers).
       Then I swap elements with nearby elements (where
       nearness is determined by the parameter). Then
       I reconstruct the wave by reading the intervals
       in their new order.
    */
    
    /* fix last point at last sample -- necessary to
       preserve invariants */
    px[numpts-1] = samples - 1;

    int const intervals = numpts - 1;

    /* generate table */
    for(int a = 0; a < intervals; a++) {
      tempx[a] = a;
    }

    for(int z = 0; z < intervals; z++) {
      if (dfx::math::Rand<float>() < interparam) {
        int dest = z + ((interparam * 
                         interparam * (float)intervals)
                        * dfx::math::Rand<float>()) - (interparam *
                                                       interparam *
                                                       0.5f * (float)intervals);
        dest = std::clamp(dest, 0, intervals - 1);

        std::swap(tempx[z], tempx[dest]);
      }
    }

    /* generate output */
    for(int u = 0, c = 0; u < intervals; u++) {
      int const size = px[tempx[u]+1] - px[tempx[u]];
      std::copy_n(in + px[tempx[u]], size, out + c);
      c += size;
    }

    break;
  }

  case INTERP_FRIENDS: {
    /* bleed each segment into next segment (its "friend"). 
       interparam controls the amount of bleeding, between 
       0 samples and next-segment-size samples. 
       suggestion by jcreed (sorta).
    */

    /* copy last block verbatim. */
    if (numpts > 2)
      for(int s=px[numpts-2]; s < px[numpts-1]; s++)
        out[s] = in[s];

    /* steady state */
    for(int x = numpts - 2; x > 0; x--) {
      /* x points at the beginning of the segment we'll be bleeding
         into. */
      int const sizeright = px[x+1] - px[x];
      int const sizeleft = px[x] - px[x-1];

      int const tgtlen = sizeleft + (sizeright * interparam);

      if (tgtlen > 0) {
        /* to avoid using temporary storage, copy from end of target
           towards beginning, overwriting already used source parts on
           the way. 

           j is an offset from p[x-1], ranging from 0 to tgtlen-1.
           Once we reach p[x], we have to start mixing with the
           data that's already there.
        */
        for (int j = tgtlen - 1;
             j >= 0;
             j--) {

          /* XXX. use interpolated sampling for this */
          float const wet = in[(int)(px[x-1] + sizeleft * 
                                     (j/(float)tgtlen))];

          if ((j + px[x-1]) > px[x]) {
            /* after p[x] -- mix */

            /* linear fade-out */
            float const pct = (j - sizeleft) / (float)(tgtlen - sizeleft);

            out[j + px[x-1]] =
              wet * (1.0f - pct) +
              out[j + px[x-1]] * pct;
          } else {
            /* before p[x] -- no mix */
            out[j + px[x-1]] = wet;
          }

        }
      }
    }

    break;
  }
  case INTERP_POLYGON:
    /* linear interpolation - "polygon" 
       interparam causes dimming effect -- at 1.0 it just does
       straight lines at the median.
    */

    for(int u=1; u < numpts; u++) {
      float const denom = (px[u] - px[u-1]);
      float const minterparam = interparam * (py[u-1] + py[u]) * 0.5f;
      for(int z=px[u-1]; z < px[u]; z++) {
        float const pct = (float)(z-px[u-1]) / denom;
        float const s = py[u-1] * (1.0f - pct) + py[u] * pct;
        out[z] = minterparam + (1.0f - interparam) * s;
      }
    }

    out[samples-1] = in[samples-1];

    break;


  case INTERP_WRONGYGON:
    /* linear interpolation, wrong direction - "wrongygon" 
       same dimming effect from polygon.
    */

    for(int u=1; u < numpts; u++) {
      float const denom = (px[u] - px[u-1]);
      float const minterparam = interparam * (py[u-1] + py[u]) * 0.5f;
      for(int z=px[u-1]; z < px[u]; z++) {
        float const pct = (float)(z-px[u-1]) / denom;
        float const s = py[u-1] * pct + py[u] * (1.0f - pct);
        out[z] = minterparam + (1.0f - interparam) * s;
      }
    }

    out[samples-1] = in[samples-1];

    break;


  case INTERP_SMOOTHIE:
    /* cosine up or down - "smoothie" */

    for(int u=1; u < numpts; u++) {
      float const denom = (px[u] - px[u-1]);
      for(int z=px[u-1]; z < px[u]; z++) {
        float const pct = (float)(z-px[u-1]) / denom;
        
        float p = 0.5f * (-std::cos(dfx::math::kPi<float> * pct) + 1.0f);
        
        if (interparam > 0.5f) {
          p = std::pow(p, (interparam - 0.16666667f) * 3.0f);
        } else {
          p = std::pow(p, interparam * 2.0f);
        }

        float const s = py[u-1] * (1.0f - p) + py[u] * p;

        out[z] = s;
      }
    }

    out[samples-1] = in[samples-1];

    break;


  case INTERP_REVERSI:
    /* x-reverse input samples for each waveform - "reversi" */

    for(int u=1; u < numpts; u++) {
      if (px[u-1] < px[u])
        for(int z = px[u-1]; z < px[u]; z++) {
          int const s = (px[u] - (z + 1)) + px[u - 1];
          out[z] = in[dfx::math::ToIndex(s)];
        }
    }

    break;


  case INTERP_PULSE: {

    int const wid = (int)(100.0f * interparam);
    
    for(int i = 0; i < samples; i++) out[i] = 0.0f;

    for(int z = 0; z < numpts; z++) { 
      out[px[z]] = dfx::math::MagnitudeMax(out[px[z]], py[z]);

      if (wid > 0) {
        /* put w samples on the left, stopping if we hit a sample
           greater than what we're placing */
        int w = wid;
        float const onedivwid = 1.0f / (float)(wid + 1);
        for(int i=px[z]-1; i >= 0 && w > 0; i--, w--) {
          float sam = py[z] * (w * onedivwid);
          if ((out[i] + sam) * (out[i] + sam) > (sam * sam)) out[i] = sam;
          else out[i] += sam;
        }

        w = wid;
        for(int ii=px[z]+1; ii < samples && w > 0; ii++, w--) {
          float const sam = py[z] * (w * onedivwid);
          out[ii] = sam;
        }
      }
    }

    break;

  }
  case INTERP_SING:

    for(int u=1; u < numpts; u++) {
      float const oodenom = 1.0f / (px[u] - px[u-1]);

      for(int z=px[u-1]; z < px[u]; z++) {
        float const pct = (float)(z-px[u-1]) * oodenom;
        
        float const wand = sinf(2.0f * dfx::math::kPi<float> * pct);
        out[z] = wand * 
          interparam + 
          ((1.0f-interparam) * 
           in[z] *
           wand);
      }
    }

    out[samples-1] = in[samples-1];


    break;
  default:

    /* unsupported ... ! */
    for(int i = 0; i < samples; i++) out[i] = 0.0f;

    break;

  } /* end of interpstyle cases */

  return numpts;
}


/* this fake process function reads samples one at a time
   from the true input. It simultaneously copies samples from
   the beginning of the output buffer to the true output.
   We maintain that out0 always has at least 'third' samples
   in it; this is enough to pick up for the delay of input
   processing and to make sure we always have enough samples
   to fill the true output buffer.

   If the input frame is full:
    - calls processw on this full input frame
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

/* to improve: 
   - can we use tail of out0 as prevmix, instead of copying?
   - can we use circular buffers instead of memmoving a lot?
     (probably not)
XXX Sophia's ideas:
   - only calculate a given window the first time that it's needed (how often to we change them anyway?)
   - cache it and use it next time
   - moreover, we only need one cache for the plugin, although that might be tricky with the whole DSP cores abstraction thing
   - possibly we only need to save half since the second half is always the first half reversed
   - this can allow for an easy way to vector-optimize the windowing
   - it would also be nice to make this windowing stuff into a reusable class so that we don't find ourselves maintaining the same code accross so many different plugins
*/

void PLUGINCORE::process(float const* tin, float* tout, unsigned long samples) {
  for (unsigned long ii = 0; ii < samples; ii++) {

    /* copy sample in */
    in0[insize] = tin[ii];
    insize++;
 
    if (insize == framesize) {
      /* frame is full! */

      /* in0 -> process -> out0(first free space) */
      processw(in0.data(), out0.data()+outstart+outsize, framesize,
               pointx.data(), pointy.data(), framesize * 2,
               storex.data(), storey.data());
      updatewindowcache(this);

#if TARGET_OS_MAC
      vDSP_vmul(out0.data()+outstart+outsize, 1, windowenvelope.data(), 1, out0.data()+outstart+outsize, 1, static_cast<vDSP_Length>(framesize));
#else
      for (int z=0; z < framesize; z++) {
        out0[z+outstart+outsize] *= windowenvelope[z];
      }
#endif

      /* mix in prevmix */
      for(int u = 0; u < third; u++)
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


void PLUGINCORE::updatewindowsize()
{
  framesize = PLUGIN::buffersizes.at(getparameter_i(P_BUFSIZE));
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
}


void PLUGINCORE::updatewindowshape()
{
  shape = getparameter_i(P_SHAPE);

  float const oneDivThird = 1.0f / static_cast<float>(third);
  switch(shape) {
    case WINDOW_TRIANGLE:
      for(int z = 0; z < third; z++) {
        windowenvelope[z] = (static_cast<float>(z) * oneDivThird);
        windowenvelope[z+third] = (1.0f - (static_cast<float>(z) * oneDivThird));
      }
      break;
    case WINDOW_ARROW:
      for(int z = 0; z < third; z++) {
        float p = static_cast<float>(z) * oneDivThird;
        p *= p;
        windowenvelope[z] = p;
        windowenvelope[z+third] = (1.0f - p);
      }
      break;
    case WINDOW_WEDGE:
      for(int z = 0; z < third; z++) {
        float const p = std::sqrt(static_cast<float>(z) * oneDivThird);
        windowenvelope[z] = p;
        windowenvelope[z+third] = (1.0f - p);
      }
      break;
    case WINDOW_COS:
      for(int z = 0; z < third; z++) {
        float const p = 0.5f * (-std::cos(dfx::math::kPi<float> * (static_cast<float>(z) * oneDivThird)) + 1.0f);
        windowenvelope[z] = p;
        windowenvelope[z+third] = (1.0f - p);
      }
      break;
  }
}


void PLUGIN::makepresets() {
  long i = 1;

  setpresetname(i, "atonal singing");
  setpresetparameter_i(i, P_BUFSIZE, 9);	// XXX is that 2^11 ?
  setpresetparameter_i(i, P_POINTSTYLE, POINT_FREQ);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_FREQ, 0.10112);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_REVERSI);
  i++;

  setpresetname(i, "robo sing (A)");
  setpresetparameter_i(i, P_BUFSIZE, 9);
  setpresetparameter_i(i, P_SHAPE, WINDOW_COS);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_DYDX);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_DYDX, 0.1250387420637675);//0.234);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SING);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SING, 1.0);
  setpresetparameter_i(i, P_POINTOP1, OP_FAST);
  setpresetparameter_f(i, P_OPPAR1S + OP_FAST, 0.9157304);
  i++;

  setpresetname(i, "sploop drums");
  setpresetparameter_i(i, P_BUFSIZE, 9);
  setpresetparameter_i(i, P_SHAPE, WINDOW_TRIANGLE);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_DYDX);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_DYDX, 0.5707532982591033);//0.528);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SING);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SING, 0.2921348);
  setpresetparameter_i(i, P_POINTOP1, OP_QUARTER);
  setpresetparameter_f(i, P_OPPAR1S + OP_QUARTER, 0.258427);
  setpresetparameter_i(i, P_POINTOP2, OP_DOUBLE);
  setpresetparameter_f(i, P_OPPAR2S + OP_DOUBLE, 0.5);
  i++;

  setpresetname(i, "loudest sing");
  setpresetparameter_i(i, P_BUFSIZE, 9);
  setpresetparameter_i(i, P_SHAPE, WINDOW_TRIANGLE);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_LEVEL);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_LEVEL, 0.280899);
  setpresetparameter_i(i, P_POINTOP2, OP_LONGPASS);
  setpresetparameter_f(i, P_OPPAR2S + OP_LONGPASS, 0.1404494);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SING);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SING, 0.8258427);
  i++;

  setpresetname(i, "slower");
  setpresetparameter_i(i, P_BUFSIZE, 13);
  setpresetparameter_i(i, P_SHAPE, WINDOW_COS);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_FREQ);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_FREQ, 0.3089887);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_FRIENDS);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_FRIENDS, 1.0);
  i++;

  setpresetname(i, "space chamber");
  setpresetparameter_i(i, P_BUFSIZE, 13);
  setpresetparameter_i(i, P_SHAPE, WINDOW_COS);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_FREQ);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_FREQ, 0.0224719);
  setpresetparameter_i(i, P_POINTOP2, OP_FAST);
  setpresetparameter_f(i, P_OPPAR2S + OP_FAST, 0.7247191);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SMOOTHIE);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SMOOTHIE, 0.5);
  i++;

  setpresetname(i, "robo sing (B)");
  setpresetparameter_i(i, P_BUFSIZE, 10);
  setpresetparameter_i(i, P_SHAPE, WINDOW_TRIANGLE);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_RANDOM);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_RANDOM, 0.0224719);
  setpresetparameter_i(i, P_POINTOP1, OP_LONGPASS);
  setpresetparameter_f(i, P_OPPAR1S + OP_LONGPASS, 0.1966292);
  setpresetparameter_i(i, P_POINTOP2, OP_FAST);
  setpresetparameter_f(i, P_OPPAR2S + OP_FAST, 1.0);
  setpresetparameter_i(i, P_POINTOP3, OP_FAST);
  setpresetparameter_f(i, P_OPPAR3S + OP_FAST, 1.0);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_POLYGON);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_POLYGON, 0.0);
  i++;
  
  setpresetname(i, "scrubby chorus");
  setpresetparameter_i(i, P_BUFSIZE, 13);
  setpresetparameter_i(i, P_SHAPE, WINDOW_ARROW);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_RANDOM);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_RANDOM, 0.9775281);
  setpresetparameter_i(i, P_POINTOP1, OP_LONGPASS);
  setpresetparameter_f(i, P_OPPAR1S + OP_LONGPASS, 0.5168539);
  setpresetparameter_i(i, P_POINTOP2, OP_FAST);
  setpresetparameter_f(i, P_OPPAR2S + OP_FAST, 0.0617978);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_FRIENDS);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_FRIENDS, 0.7303371);
  i++;

  setpresetname(i, "time shuffle echo (sux?)");
  setpresetparameter_i(i, P_BUFSIZE, 13);
  setpresetparameter_i(i, P_SHAPE, WINDOW_COS);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_DYDX);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_DYDX, 0.81);
  setpresetparameter_i(i, P_POINTOP1, OP_LONGPASS);
  setpresetparameter_f(i, P_OPPAR1S + OP_LONGPASS, 0.183);
  setpresetparameter_i(i, P_POINTOP2, OP_NONE);
  setpresetparameter_i(i, P_POINTOP3, OP_NONE);
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_SHUFFLE);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_SHUFFLE, 0.84);
  i++;

  setpresetname(i, "stranger");
  setpresetparameter_i(i, P_BUFSIZE, 13);
  setpresetparameter_i(i, P_SHAPE, WINDOW_TRIANGLE);
  setpresetparameter_i(i, P_POINTSTYLE, POINT_EXTNCROSS);
  setpresetparameter_f(i, P_POINTPARAMS + POINT_EXTNCROSS, 0.0);
  setpresetparameter_i(i, P_POINTOP1, OP_QUARTER);
  setpresetparameter_f(i, P_OPPAR1S + OP_QUARTER, 0.0);
  setpresetparameter_i(i, P_POINTOP2, OP_QUARTER);
  setpresetparameter_f(i, P_OPPAR2S + OP_QUARTER, 0.0);  
  setpresetparameter_i(i, P_POINTOP3, OP_QUARTER);
  setpresetparameter_f(i, P_OPPAR3S + OP_QUARTER, 0.0);    
  setpresetparameter_i(i, P_INTERPSTYLE, INTERP_FRIENDS);
  setpresetparameter_f(i, P_INTERPARAMS + INTERP_FRIENDS, 0.3876404);
  i++;
}
