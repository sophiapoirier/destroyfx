
/* DFX Transverb plugin by Tom 7 and Marc 3 */

#include "transverb.hpp"

#if TARGET_API_VST && TARGET_PLUGIN_HAS_GUI
	#ifndef __TRANSVERBEDITOR_H
	#include "transverbeditor.hpp"
	#endif
#endif



// these are macros that do boring entry point stuff for us
DFX_ENTRY(Transverb);
#if TARGET_PLUGIN_USES_DSPCORE
  DFX_CORE_ENTRY(TransverbDSP);
#endif



Transverb::Transverb(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, NUM_PARAMETERS, NUM_PRESETS) {

  initparameter_f(kBsize, "buffer size", 2700.0f, 333.0f, 1.0f, 3000.0f, kDfxParamCurve_linear, kDfxParamUnit_ms);
  initparameter_f(kDrymix, "dry mix", 1.0f, 1.0f, 0.0f, 1.0f, kDfxParamCurve_squared, kDfxParamUnit_lineargain);
  initparameter_f(kMix1, "1:mix", 1.0f, 1.0f, 0.0f, 1.0f, kDfxParamCurve_squared, kDfxParamUnit_lineargain);
  initparameter_f(kDist1, "1:dist", 0.90009f, 0.5f, 0.0f, 1.0f, kDfxParamCurve_linear, kDfxParamUnit_scalar);
  initparameter_d(kSpeed1, "1:speed", 0.0, 0.0, -3.0, 6.0, kDfxParamCurve_linear, kDfxParamUnit_octaves);
  initparameter_f(kFeed1, "1:feedback", 0.0f, 33.3f, 0.0f, 100.0f, kDfxParamCurve_linear, kDfxParamUnit_percent);
  initparameter_f(kMix2, "2:mix", 0.0f, 1.0f, 0.0f, 1.0f, kDfxParamCurve_squared, kDfxParamUnit_lineargain);
  initparameter_f(kDist2, "2:dist", 0.1f, 0.5f, 0.0f, 1.0f, kDfxParamCurve_linear, kDfxParamUnit_scalar);
  initparameter_d(kSpeed2, "2:speed", 1.0, 0.0, -3.0, 6.0, kDfxParamCurve_linear, kDfxParamUnit_octaves);
  initparameter_f(kFeed2, "2:feedback", 0.0f, 33.3f, 0.0f, 100.0f, kDfxParamCurve_linear, kDfxParamUnit_percent);
  initparameter_indexed(kQuality, "quality", ultrahifi, ultrahifi, numQualities);
  initparameter_b(kTomsound, "TOMSOUND", false, false);
  initparameter_indexed(kSpeed1mode, "1:speed mode", kFineMode, kFineMode, numSpeedModes);
  initparameter_indexed(kSpeed2mode, "2:speed mode", kFineMode, kFineMode, numSpeedModes);

  setparametervaluestring(kQuality, dirtfi, "dirt-fi");
  setparametervaluestring(kQuality, hifi, "hi-fi");
  setparametervaluestring(kQuality, ultrahifi, "ultra hi-fi");
  for (int i=kSpeed1mode; i <= kSpeed2mode; i += kSpeed2mode-kSpeed1mode)
  {
    setparametervaluestring(i, kFineMode, "fine");
    setparametervaluestring(i, kSemitoneMode, "semitone");
    setparametervaluestring(i, kOctaveMode, "octave");
  }

  settailsize_seconds(BUFFER_MAX * 0.001);

  #if TARGET_PLUGIN_USES_MIDI
    // since we don't use notes for any specialized control of Transverb, 
    // allow them to be assigned to control parameters via MIDI learn
    dfxsettings->setAllowPitchbendEvents(true);
    dfxsettings->setAllowNoteEvents(true);
  #endif

  setpresetname(0, PLUGIN_NAME_STRING);	// default preset name
  initPresets();


  #if TARGET_API_AUDIOUNIT
    // XXX is there a better way to do this?
    update_preset(0);	// make host see that current preset is 0
  #endif

  #if TARGET_API_VST
    #if TARGET_PLUGIN_HAS_GUI
      editor = new TransverbEditor(this);
    #endif
    #if TARGET_PLUGIN_USES_DSPCORE
      DFX_INIT_CORE(TransverbDSP);	// we need to manage DSP cores manually in VST
    #endif
  #endif

}

Transverb::~Transverb() {
#if TARGET_API_VST
  // VST doesn't have initialize and cleanup methods like Audio Unit does, 
  // so we need to call this manually here
  do_cleanup();
#endif
}

long Transverb::initialize() {
  return 0;
}

void Transverb::cleanup() {
}



TransverbDSP::TransverbDSP(TARGET_API_CORE_INSTANCE_TYPE *inInstance)
  : DfxPluginCore(inInstance) {

  buf1 = 0;
  buf2 = 0;

  filter1 = new IIRfilter();
  filter2 = new IIRfilter();
  firCoefficients1 = (float*) malloc(NUM_FIR_TAPS * sizeof(float));
  firCoefficients2 = (float*) malloc(NUM_FIR_TAPS * sizeof(float));

  MAXBUF = 0;	// init to bogus value to "dirty" it

  do_reset();
}

TransverbDSP::~TransverbDSP() {

  if (filter1)
    delete filter1;
  filter1 = 0;
  if (filter2)
    delete filter2;
  filter2 = 0;

  if (firCoefficients1)
    free(firCoefficients1);
  firCoefficients1 = 0;
  if (firCoefficients2)
    free(firCoefficients2);
  firCoefficients2 = 0;

  // must call here because ~DfxPluginCore can't do this for us
  releasebuffers();
}

void TransverbDSP::reset() {
#if PRINT_FUNCTION_ALERTS
printf("calling TransverbDSP::reset()\n");
#endif

  writer = 0;
  read1 = read2 = 0.0;
  smoothcount1 = smoothcount2 = 0;
  lastr1val = lastr2val = 0.0f;
  filter1->reset();
  filter2->reset();
  speed1hasChanged = speed2hasChanged = true;
}

bool TransverbDSP::createbuffers() {
#if PRINT_FUNCTION_ALERTS
printf("calling TransverbDSP::createbuffers()\n");
#endif

  long oldmax = MAXBUF;
  MAXBUF = (int) (BUFFER_MAX * 0.001f * getsamplerate());

  bool result1 = createbuffer_f(&buf1, oldmax, MAXBUF);
  bool result2 = createbuffer_f(&buf2, oldmax, MAXBUF);

  if (!result1 && !result2)
    return false;

  return true;
}

void TransverbDSP::clearbuffers() {
#if PRINT_FUNCTION_ALERTS
printf("calling TransverbDSP::clearbuffers()\n");
#endif
  clearbuffer_f(buf1, MAXBUF);
  clearbuffer_f(buf2, MAXBUF);
}

void TransverbDSP::releasebuffers() {
#if PRINT_FUNCTION_ALERTS
printf("calling TransverbDSP::releasebuffers()\n");
#endif
  releasebuffer_f(&buf1);
  releasebuffer_f(&buf2);
}


void TransverbDSP::processparameters() {

  drymix = getparameter_f(kDrymix);
  bsize = (int) (getparameter_f(kBsize) * getsamplerate() * 0.001f);
  if (bsize > MAXBUF)   bsize = MAXBUF;
  mix1 = getparameter_f(kMix1);
  speed1 = pow(2.0, getparameter_d(kSpeed1));
  feed1 = getparameter_scalar(kFeed1);
  dist1 = getparameter_f(kDist1);
  mix2 = getparameter_f(kMix2);
  speed2 = pow(2.0, getparameter_d(kSpeed2));
  feed2 = getparameter_scalar(kFeed2);
  dist2 = getparameter_f(kDist2);
  quality = getparameter_i(kQuality);
  tomsound = getparameter_b(kTomsound);

  if (getparameterchanged(kBsize))
  {
    writer %= bsize;
    read1 = fmod(fabs(read1), (double)bsize);
    read2 = fmod(fabs(read2), (double)bsize);
  }

  if (getparameterchanged(kDist1))
    read1 = fmod(fabs((double)writer + (double)dist1 * (double)MAXBUF), (double)bsize);
  if (getparameterchanged(kSpeed1))
    speed1hasChanged = true;

  if (getparameterchanged(kDist2))
    read2 = fmod(fabs((double)writer + (double)dist2 * (double)MAXBUF), (double)bsize);
  if (getparameterchanged(kSpeed2))
    speed2hasChanged = true;

  if (getparameterchanged(kQuality) || getparameterchanged(kTomsound))
    speed1hasChanged = speed2hasChanged = true;
}



//--------- presets --------

void Transverb::initPresets() {

	int i = 1;

	setpresetname(i, "phaser up");
	setpresetparameter_f(i, kBsize, 48.687074827f);
	setpresetparameter_f(i, kDrymix, 0.45f);
	setpresetparameter_f(i, kMix1, 0.5f);
	setpresetparameter_f(i, kDist1, 0.9f);
	setpresetparameter_d(i, kSpeed1, 0.048406605/12.0);
	setpresetparameter_f(i, kFeed1, 67.0f);
	setpresetparameter_f(i, kMix2, 0.0f);
	setpresetparameter_f(i, kDist2, 0.0f);
	setpresetparameter_d(i, kSpeed2, getparametermin_d(kSpeed2));
	setpresetparameter_f(i, kFeed2, 0.0f);
	setpresetparameter_i(i, kQuality, ultrahifi);
	setpresetparameter_b(i, kTomsound, false);
	i++;

	setpresetname(i, "phaser down");
	setpresetparameter_f(i, kBsize, 27.0f);
	setpresetparameter_f(i, kDrymix, 0.45f);
	setpresetparameter_f(i, kMix1, 0.5f);
	setpresetparameter_f(i, kDist1, 0.0f);
	setpresetparameter_d(i, kSpeed1, -0.12/12.0);//-0.048542333f/12.0f);
	setpresetparameter_f(i, kFeed1, 76.0f);
	setpresetparameter_f(i, kMix2, 0.0f);
	setpresetparameter_f(i, kDist2, 0.0f);
	setpresetparameter_d(i, kSpeed2, getparametermin_d(kSpeed2));
	setpresetparameter_f(i, kFeed2, 0.0f);
	setpresetparameter_i(i, kQuality, ultrahifi);
	setpresetparameter_b(i, kTomsound, false);
	i++;

	setpresetname(i, "aquinas");
	setpresetparameter_f(i, kBsize, 2605.1f);
	setpresetparameter_f(i, kDrymix, 0.6276f);
	setpresetparameter_f(i, kMix1, 1.0f);
	setpresetparameter_f(i, kDist1, 0.993f);
	setpresetparameter_d(i, kSpeed1, -4.665660556);
	setpresetparameter_f(i, kFeed1, 54.0f);
	setpresetparameter_f(i, kMix2, 0.757f);
	setpresetparameter_f(i, kDist2, 0.443f);
	setpresetparameter_d(i, kSpeed2, 2.444534569);
	setpresetparameter_f(i, kFeed2, 46.0f);
	setpresetparameter_i(i, kQuality, ultrahifi);
	setpresetparameter_b(i, kTomsound, false);
	i++;

	setpresetname(i, "glup drums");
	setpresetparameter_f(i, kBsize, 184.27356f);
	setpresetparameter_f(i, kDrymix, 0.157f);
	setpresetparameter_f(i, kMix1, 1.0f);
	setpresetparameter_f(i, kDist1, 0.0945f);
	setpresetparameter_d(i, kSpeed1, -8.0/12.0);
	setpresetparameter_f(i, kFeed1, 97.6f);
	setpresetparameter_f(i, kMix2, 0.0f);
	setpresetparameter_f(i, kDist2, 0.197f);
	setpresetparameter_d(i, kSpeed2, 0.978195651);
	setpresetparameter_f(i, kFeed2, 0.0f);
	setpresetparameter_i(i, kQuality, ultrahifi);
	setpresetparameter_b(i, kTomsound, false);
	i++;

	setpresetname(i, "space invaders");
	setpresetparameter_d(i, kSpeed1, -0.23/12.0);
	setpresetparameter_f(i, kFeed1, 73.0f);
	setpresetparameter_f(i, kDist1, 0.1857f);
	setpresetparameter_d(i, kSpeed2, 4.3/12.0);
	setpresetparameter_f(i, kFeed2, 41.0f);
	setpresetparameter_f(i, kDist2, 0.5994f);
	setpresetparameter_f(i, kBsize, 16.8f);
	setpresetparameter_f(i, kDrymix, 0.000576f);
	setpresetparameter_f(i, kMix1, 1.0f);
	setpresetparameter_f(i, kMix2, 0.1225f);
	setpresetparameter_i(i, kQuality, ultrahifi);
	setpresetparameter_b(i, kTomsound, false);
	i++;

/*
	setpresetparameter_d(i, kSpeed1, );
	setpresetparameter_f(i, kFeed1, f);
	setpresetparameter_f(i, kDist1, 0.0f);
	setpresetparameter_d(i, kSpeed2, );
	setpresetparameter_f(i, kFeed2, f);
	setpresetparameter_f(i, kDist2, 0.0f);
	setpresetparameter_f(i, kBsize, f);
	setpresetparameter_f(i, kDrymix, 0.f);
	setpresetparameter_f(i, kMix1, 0.f);
	setpresetparameter_f(i, kMix2, 0.f);
	setpresetparameter_i(i, kQuality, );
	setpresetparameter_b(i, kTomsound, );
	setpresetname(i, "");
	i++;
*/

	// special randomizing "preset"
	setpresetname(numPresets-1, "random");
};

//-----------------------------------------------------------------------------
bool Transverb::loadpreset(long index)
{
	if ( !presetisvalid(index) )
		return false;

	if (strcmp(getpresetname_ptr(index), "random") == 0)
	{
		randomizeParameters(false);
		return true;
	}

	return DfxPlugin::loadpreset(index);
}

//--------- presets (end) --------



/* this randomizes the values of all of Transverb's parameters, sometimes in smart ways */
void Transverb::randomizeParameters(bool writeAutomation)
{
// randomize the first 7 parameters

	for (long i=0; (i < kDrymix); i++)
	{
		// make slow speeds more probable (for fairer distribution)
		if ( (i == kSpeed1) || (i == kSpeed1) )
		{
			double temprand = randDouble();
			if (temprand < 0.5)
				temprand = getparametermin_d(i) * temprand*2.0;
			else
				temprand = getparametermax_d(i) * ((temprand - 0.5) * 2.0);
			setparameter_d(i, temprand);
		}
		// make smaller buffer sizes more probable (because they sound better)
		else if (i == kBsize)
			setparameter_gen( kBsize, powf((randFloat()*0.93f)+0.07f, 1.38f) );
		else
			randomizeparameter(i);
	}


// do fancy mix level randomization

	// store the current total gain sum
	float mixSum = getparameter_f(kDrymix) + getparameter_f(kMix1) + getparameter_f(kMix2);

	// randomize the mix parameters
	float newDrymix = randFloat();
	float newMix1 = randFloat();
	float newMix2 = randFloat();
	// square them all for squared gain scaling
	newDrymix *= newDrymix;
	newMix1 *= newMix1;
	newMix2 *= newMix2;
	// calculate a scalar to make up for total gain changes
	float mixScalar = mixSum / (newDrymix + newMix1 + newMix2);

	// apply the scalar to the new mix parameter values
	newDrymix *= mixScalar;
	newMix1 *= mixScalar;
	newMix2 *= mixScalar;

	// clip the the mix values at 1.0 so that we don't get mega-feedback blasts
	newDrymix = (newDrymix > 1.0f) ? 1.0f : newDrymix;
	newMix1 = (newMix1 > 1.0f) ? 1.0f : newMix1;
	newMix2 = (newMix2 > 1.0f) ? 1.0f : newMix2;

	// set the new randomized mix parameter values as the new values
	setparameter_f(kDrymix, newDrymix);
	setparameter_f(kMix1, newMix1);
	setparameter_f(kMix2, newMix2);


// randomize the state parameters

	// make higher qualities more probable (happen 4/5 of the time)
	setparameter_i( kQuality, ((rand()%5)+1)%3 );
	// make TOMSOUND less probable (only 1/3 of the time)
	setparameter_b( kTomsound, (bool) ((rand()%3)%2) );


	#if TARGET_API_VST
		if (writeAutomation)
		{
			for (long i=0; i < kSpeed1mode; i++)
				setParameterAutomated(i, getparameter_gen(i));
		}
	#endif

	// display the mix parameter changes
//	printf(mixData, "drymix = %.3f,   mix1 = %.3f,   mix2 = %.3f,   sum = %.6f\n", newDrymix, newMix1, newMix2, newDrymix+newMix1+newMix2);
}
