/*-------------- by Marc Poirier  ][  February 2002 -------------*/

#ifndef __SCRUBBY_H
#include "scrubby.hpp"
#endif

#if TARGET_API_VST && TARGET_PLUGIN_HAS_GUI
	#ifndef __SCRUBBYEDITOR_H
	#include "scrubbyeditor.hpp"
	#endif
#endif


#pragma mark _________init_________

// this macro does boring entry point stuff for us
DFX_ENTRY(Scrubby);

//-----------------------------------------------------------------------------
// initializations & such
Scrubby::Scrubby(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, NUM_PRESETS)	// 29 parameters, 16 presets
{
	buffers = NULL;
	readPos = NULL;
	readStep = NULL;
	portamentoStep = NULL;
	movecount = NULL;
	seekcount = NULL;
	needResync = NULL;
	numBuffers = 0;
	pitchSteps = NULL;
	activeNotesTable = NULL;

	tempoRateTable = new TempoRateTable;


	// initialize the parameters
	long numTempoRates = tempoRateTable->getNumTempoRates();
	long unitTempoRateIndex = tempoRateTable->getNearestTempoRateIndex(1.0f);
	initparameter_f(kSeekRange, "seek range", 333.0f, 333.0f, 0.3f, 6000.0f, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_b(kFreeze, "freeze", false, false);
	initparameter_f(kSeekRate_abs, "seek rate (free)", 9.0f, 3.0f, 0.3f, 810.0f, kDfxParamUnit_hz, kDfxParamCurve_log);//kDfxParamCurve_cubed
	initparameter_indexed(kSeekRate_sync, "seek rate (sync)", unitTempoRateIndex, unitTempoRateIndex, numTempoRates);
	initparameter_f(kSeekRateRandMin_abs, "seek rate rand min (free)", 9.0f, 3.0f, 0.3f, 810.0f, kDfxParamUnit_hz, kDfxParamCurve_log);//kDfxParamCurve_cubed
	initparameter_indexed(kSeekRateRandMin_sync, "seek rate rand min (sync)", unitTempoRateIndex, unitTempoRateIndex, numTempoRates);
	initparameter_b(kTempoSync, "tempo sync", false, false);
	initparameter_f(kSeekDur, "seek duration", 100.0f, 100.0f, 3.0f, 100.0f, kDfxParamUnit_percent);	// percent of range
	initparameter_f(kSeekDurRandMin, "seek dur rand min", 100.0f, 100.0f, 3.0f, 100.0f, kDfxParamUnit_percent);	// percent of range
	initparameter_indexed(kSpeedMode, "speeds", kSpeedMode_robot, kSpeedMode_robot, kNumSpeedModes);
	initparameter_b(kSplitStereo, "stereo split", false, false);
	initparameter_b(kPitchConstraint, "pitch constraint", false, false);
	// default all notes to off (looks better on the GUI)
	// no, I changed my mind, at least leave 1 note on so that the user isn't 
	// confused the first time turning on pitch constraint & getting silence
	initparameter_b(kPitchStep0, "semi0 (unity/octave)", true, false);
	initparameter_b(kPitchStep1, "semi1 (minor 2nd)", false, false);
	initparameter_b(kPitchStep2, "semi2 (major 2nd)", false, false);
	initparameter_b(kPitchStep3, "semi3 (minor 3rd)", false, false);
	initparameter_b(kPitchStep4, "semi4 (major 3rd)", false, false);
	initparameter_b(kPitchStep5, "semi5 (perfect 4th)", false, false);
	initparameter_b(kPitchStep6, "semi6 (augmented 4th)", false, false);
	initparameter_b(kPitchStep7, "semi7 (perfect 5th)", false, false);
	initparameter_b(kPitchStep8, "semi8 (minor 6th)", false, false);
	initparameter_b(kPitchStep9, "semi9 (major 6th)", false, false);
	initparameter_b(kPitchStep10, "semi10 (minor 7th)", false, false);
	initparameter_b(kPitchStep11, "semi11 (major 7th)", false, false);
	initparameter_i(kOctaveMin, "octave minimum", -5, -5, -5, 0, kDfxParamUnit_strings, kDfxParamCurve_stepped);
	initparameter_i(kOctaveMax, "octave maximum", 7, 7, 0, 7, kDfxParamUnit_strings, kDfxParamCurve_stepped);
	initparameter_f(kTempo, "tempo", 120.0f, 120.0f, 39.0f, 480.0f, kDfxParamUnit_bpm);
	initparameter_b(kTempoAuto, "sync to host tempo", true, true);
	initparameter_f(kPredelay, "predelay", 0.0f, 50.0f, 0.0f, 100.0f, kDfxParamUnit_percent);	// percent of range

	// set the value strings for the sync rate parameters
	for (int i=0; i < numTempoRates; i++)
	{
		char *tname = tempoRateTable->getDisplay(i);
		setparametervaluestring(kSeekRate_sync, i, tname);
		setparametervaluestring(kSeekRateRandMin_sync, i, tname);
	}
	// set the value strings for the speed modes
	setparametervaluestring(kSpeedMode, kSpeedMode_robot, "robot");
	setparametervaluestring(kSpeedMode, kSpeedMode_dj, "DJ");
	// set the value strings for the octave range parameters
	char *octavename = (char*) malloc(DFX_PARAM_MAX_VALUE_STRING_LENGTH);
	for (int i=getparametermin_i(kOctaveMin)+1; i <= getparametermax_i(kOctaveMin); i++)
	{
		sprintf(octavename, "%d", i);
		setparametervaluestring(kOctaveMin, i, octavename);
	}
	setparametervaluestring(kOctaveMin, getparametermin_i(kOctaveMin), "no min");
	for (int i=getparametermin_i(kOctaveMax); i < getparametermax_i(kOctaveMax); i++)
	{
		if (i > 0)
			sprintf(octavename, "%+d", i);
		else
			sprintf(octavename, "%d", i);	// XXX is there a better way to do this?
		setparametervaluestring(kOctaveMax, i, octavename);
	}
	setparametervaluestring(kOctaveMax, getparametermax_i(kOctaveMax), "no max");
	free(octavename);


	settailsize_seconds(SEEK_RANGE_MAX * 0.001f / SEEK_RATE_MIN);

	setpresetname(0, "scub");	// default preset name
	initPresets();

	// since we don't use pitchbend for anything special, 
	// allow it be assigned to control parameters
	dfxsettings->setAllowPitchbendEvents(true);
	// can't load old VST-style settings
	dfxsettings->setLowestLoadableVersion(0x00010000);

	// give currentTempoBPS a value in case that's useful for a freshly opened GUI
	currentTempoBPS = getparameter_f(kTempo) / 60.0f;


	#if TARGET_API_VST && TARGET_PLUGIN_HAS_GUI
		editor = new ScrubbyEditor(this);
	#endif
}

//-------------------------------------------------------------------------
Scrubby::~Scrubby()
{
#if TARGET_API_VST
	// VST doesn't have initialize and cleanup methods like Audio Unit does, 
	// so we need to call this manually here
	do_cleanup();
#endif
}

//-------------------------------------------------------------------------
long Scrubby::initialize()
{
	// allocate memory for these arrays
	if (pitchSteps == NULL)
		pitchSteps = (bool*) malloc(NUM_PITCH_STEPS * sizeof(bool));
	if (activeNotesTable == NULL)
		activeNotesTable = (long*) malloc(NUM_PITCH_STEPS * sizeof(long));

	if ( (pitchSteps == NULL) || (activeNotesTable == NULL) )
		return kDfxErr_InitializationFailed;
	return kDfxErr_NoError;
}

//-------------------------------------------------------------------------
void Scrubby::cleanup()
{
	// deallocate the memory from these arrays
	if (pitchSteps != NULL)
		free(pitchSteps);
	pitchSteps = NULL;
	if (activeNotesTable != NULL)
		free(activeNotesTable);
	activeNotesTable = NULL;
}

//-------------------------------------------------------------------------
void Scrubby::reset()
{
	// reset these position trackers thingies & whatnot
	writePos = 0;

	keyboardWasPlayedByMidi = true;
	// delete any stored active notes
	for (int i=0; i < NUM_PITCH_STEPS; i++)
	{
		if (activeNotesTable[i] > 0)
			setparameter_b(i+kPitchStep0, false);
		activeNotesTable[i] = 0;
	}

sinecount = 0;
}

//-----------------------------------------------------------------------------
bool Scrubby::createbuffers()
{
	long oldmax = MAX_BUFFER;
	// the number of samples in the maximum seek range, 
	// dividing by the minimum seek rate for extra leeway while moving
	MAX_BUFFER = (long) (SEEK_RANGE_MAX * 0.001f * getsamplerate_f() / SEEK_RATE_MIN);
	MAX_BUFFER_FLOAT = (double)MAX_BUFFER;
	unsigned long oldnum = numBuffers;
	numBuffers = getnumoutputs();

	bool result1 = createbufferarray_f(&buffers, oldnum, oldmax, numBuffers, MAX_BUFFER);
	bool result2 = createbuffer_d(&readPos, oldnum, numBuffers);
	bool result3 = createbuffer_d(&readStep, oldnum, numBuffers);
	bool result4 = createbuffer_d(&portamentoStep, oldnum, numBuffers);
	bool result5 = createbuffer_i(&movecount, oldnum, numBuffers);
	bool result6 = createbuffer_i(&seekcount, oldnum, numBuffers);
	bool result7 = createbuffer_b(&needResync, oldnum, numBuffers);

	if (result1 && result2 && result3 && result4 && result5 && result6 && result7)
		return true;
	return false;
}

//-----------------------------------------------------------------------------
void Scrubby::releasebuffers()
{
	releasebufferarray_f(&buffers, numBuffers);
	releasebuffer_d(&readPos);
	releasebuffer_d(&readStep);
	releasebuffer_d(&portamentoStep);
	releasebuffer_i(&movecount);
	releasebuffer_i(&seekcount);
	releasebuffer_b(&needResync);

	numBuffers = 0;
}

//-----------------------------------------------------------------------------
void Scrubby::clearbuffers()
{
	// clear out the buffers
	clearbufferarray_f(buffers, numBuffers, MAX_BUFFER);
	clearbuffer_d(readPos, numBuffers, 0.001);
	clearbuffer_d(readStep, numBuffers, 1.0);
#if USE_LINEAR_ACCELERATION
	clearbuffer_d(portamentoStep, numBuffers, 0.0);
#else
	clearbuffer_d(portamentoStep, numBuffers, 1.0);
#endif
	clearbuffer_i(movecount, numBuffers);
	clearbuffer_i(seekcount, numBuffers);
	// some hosts may call reset when restarting playback
	clearbuffer_b(needResync, numBuffers, true);
}


#pragma mark _________presets_________

/*
//----------------------------------------------------------------------------- 
ScrubbyChunk::ScrubbyChunk(long numParameters, long numPrograms, long magic, AudioEffectX *effect)
	: VstChunk (numParameters, numPrograms, magic, effect)
{
	// start off with split CC automation of both range slider points
	seekRateDoubleAutomate = seekDurDoubleAutomate = false;
}

//----------------------------------------------------------------------------- 
// this gets called when Scrubby automates a parameter from CC messages.
// this is where we can link parameter automation for range slider points.
void ScrubbyChunk::doLearningAssignStuff(long tag, long eventType, long eventChannel, 
										long eventNum, long delta, long eventNum2, 
										long eventBehaviourFlags, 
										long data1, long data2, float fdata1, float fdata2)
{
	if ( getSteal() )
		return;

	switch (tag)
	{
		case kSeekRate:
			if (seekRateDoubleAutomate)
				assignParam(kSeekRateRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kSeekRateRandMin:
			if (seekRateDoubleAutomate)
				assignParam(kSeekRate, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kSeekDur:
			if (seekDurDoubleAutomate)
				assignParam(kSeekDurRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kSeekDurRandMin:
			if (seekDurDoubleAutomate)
				assignParam(kSeekDur, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		default:
			break;
	}
}

//----------------------------------------------------------------------------- 
void ScrubbyChunk::unassignParam(long tag)
{
	VstChunk::unassignParam(tag);

	switch (tag)
	{
		case kSeekRate:
			if (seekRateDoubleAutomate)
				VstChunk::unassignParam(kSeekRateRandMin);
			break;
		case kSeekRateRandMin:
			if (seekRateDoubleAutomate)
				VstChunk::unassignParam(kSeekRate);
			break;
		case kSeekDur:
			if (seekDurDoubleAutomate)
				VstChunk::unassignParam(kSeekDurRandMin);
			break;
		case kSeekDurRandMin:
			if (seekDurDoubleAutomate)
				VstChunk::unassignParam(kSeekDur);
			break;
		default:
			break;
	}
}
*/


//-------------------------------------------------------------------------
void Scrubby::initPresets()
{
	int i = 1;

	setpresetname(i, "happy machine");
	setpresetparameter_f(i, kSeekRange, 603.0f);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_f(i, kSeekRate_abs, 11.547f);
	setpresetparameter_f(i, kSeekRateRandMin_abs, 11.547f);
	setpresetparameter_b(i, kTempoSync, false);
	setpresetparameter_f(i, kSeekDur, 40.8f);
	setpresetparameter_f(i, kSeekDurRandMin, 40.8f);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_robot);
	setpresetparameter_b(i, kSplitStereo, false);
	setpresetparameter_b(i, kPitchConstraint, true);
	setpresetparameter_i(i, kOctaveMin, getparametermin_i(kOctaveMin));
	setpresetparameter_i(i, kOctaveMax, 1);
	setpresetparameter_b(i, kPitchStep0, true);
	setpresetparameter_b(i, kPitchStep1, false);
	setpresetparameter_b(i, kPitchStep2, false);
	setpresetparameter_b(i, kPitchStep3, false);
	setpresetparameter_b(i, kPitchStep4, false);
	setpresetparameter_b(i, kPitchStep5, false);
	setpresetparameter_b(i, kPitchStep6, false);
	setpresetparameter_b(i, kPitchStep7, true);
	setpresetparameter_b(i, kPitchStep8, false);
	setpresetparameter_b(i, kPitchStep9, false);
	setpresetparameter_b(i, kPitchStep10, false);
	setpresetparameter_b(i, kPitchStep11, false);
	i++;

	setpresetname(i, "fake chorus");
	setpresetparameter_f(i, kSeekRange, 3.0f);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_f(i, kSeekRate_abs, 18.0f);
	setpresetparameter_f(i, kSeekRateRandMin_abs, 18.0f);
	setpresetparameter_b(i, kTempoSync, false);
	setpresetparameter_f(i, kSeekDur, 100.0f);
	setpresetparameter_f(i, kSeekDurRandMin, 100.0f);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_robot);
	setpresetparameter_b(i, kSplitStereo, true);
	setpresetparameter_b(i, kPitchConstraint, false);
	i++;

	setpresetname(i, "broken turntable");
	setpresetparameter_f(i, kSeekRange, 3000.0f);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_f(i, kSeekRate_abs, 6.0f);
	setpresetparameter_f(i, kSeekRateRandMin_abs, 6.0f);
	setpresetparameter_b(i, kTempoSync, false);
	setpresetparameter_f(i, kSeekDur, 100.0f);
	setpresetparameter_f(i, kSeekDurRandMin, 100.0f);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_dj);
	setpresetparameter_b(i, kSplitStereo, false);
	setpresetparameter_b(i, kPitchConstraint, false);
	i++;

	setpresetname(i, "blib");
	setpresetparameter_f(i, kSeekRange, 270.0f);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_f(i, kSeekRate_abs, 420.0f);
	setpresetparameter_f(i, kSeekRateRandMin_abs, 7.2f);
	setpresetparameter_b(i, kTempoSync, false);
	setpresetparameter_f(i, kSeekDur, 57.0f);//5700.0f);
	setpresetparameter_f(i, kSeekDurRandMin, 30.0f);//3000.0f);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_robot);
	setpresetparameter_b(i, kSplitStereo, false);
	setpresetparameter_b(i, kPitchConstraint, false);
	i++;

/*
	setpresetname(i, "");
	setpresetparameter_f(i, kSeekRange, f);
	setpresetparameter_b(i, kFreeze, );
	setpresetparameter_f(i, kSeekRate_abs, f);
	setpresetparameter_i(i, kSeekRate_sync, tempoRateTable->getNearestTempoRateIndex(f));
	setpresetparameter_f(i, kSeekRateRandMin_abs, f);
	setpresetparameter_i(i, kSeekRateRandMin_sync, tempoRateTable->getNearestTempoRateIndex(f));
	setpresetparameter_b(i, kTempoSync, );
	setpresetparameter_f(i, kSeekDur, f);
	setpresetparameter_f(i, kSeekDurRandMin, f);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_);
	setpresetparameter_b(i, kSlitStereo, );
	setpresetparameter_b(i, kPitchConstraint, );
	setpresetparameter_i(i, kOctaveMin, );
	setpresetparameter_i(i, kOctaveMax, );
	setpresetparameter_f(i, kTempo, f);
	setpresetparameter_b(i, kTempoAuto, );
	setpresetparameter_f(i, kPredelay, f);
	setpresetparameter_b(i, kPitchStep0, );
	setpresetparameter_b(i, kPitchStep1, );
	setpresetparameter_b(i, kPitchStep2, );
	setpresetparameter_b(i, kPitchStep3, );
	setpresetparameter_b(i, kPitchStep4, );
	setpresetparameter_b(i, kPitchStep5, );
	setpresetparameter_b(i, kPitchStep6, );
	setpresetparameter_b(i, kPitchStep7, );
	setpresetparameter_b(i, kPitchStep8, );
	setpresetparameter_b(i, kPitchStep9, );
	setpresetparameter_b(i, kPitchStep10, );
	setpresetparameter_b(i, kPitchStep11, );
	i++;
*/
}


#pragma mark _________parameters_________

//-------------------------------------------------------------------------
void Scrubby::processparameters()
{
	seekRangeSeconds = getparameter_f(kSeekRange) * 0.001f;
	freeze = getparameter_b(kFreeze);
	seekRateHz = getparameter_f(kSeekRate_abs);
	seekRateIndex = getparameter_i(kSeekRate_sync);
	seekRateSync = tempoRateTable->getScalar(seekRateIndex);
	seekRateRandMinHz = getparameter_f(kSeekRateRandMin_abs);
	seekRateRandMinIndex = getparameter_i(kSeekRateRandMin_sync);
	seekRateRandMinSync = tempoRateTable->getScalar(seekRateRandMinIndex);
	tempoSync = getparameter_b(kTempoSync);
	seekDur = getparameter_scalar(kSeekDur);
	seekDurRandMin = getparameter_scalar(kSeekDurRandMin);
	speedMode = getparameter_i(kSpeedMode);
	splitStereo = getparameter_b(kSplitStereo);
	pitchConstraint = getparameter_b(kPitchConstraint);
	octaveMin = getparameter_i(kOctaveMin);
	octaveMax = getparameter_i(kOctaveMax);
	userTempo = getparameter_f(kTempo);
	useHostTempo = getparameter_b(kTempoAuto);
	if (pitchSteps != NULL)
	{
		for (int i=0; i < NUM_PITCH_STEPS; i++)
			pitchSteps[i] = getparameter_b(i+kPitchStep0);
	}

	// get the "generic" values of these parameters for randomization
	seekRateHz_gen = getparameter_gen(kSeekRate_abs);
	seekRateRandMinHz_gen = getparameter_gen(kSeekRateRandMin_abs);

	bool tempNeedResync = false;
	if (getparameterchanged(kSeekRate_sync))
		// make sure the cycles match up if the tempo rate has changed
		tempNeedResync = true;
	if ( getparameterchanged(kTempoSync) && tempoSync )
		// set needResync true if tempo sync mode has just been switched on
		tempNeedResync = true;
	if (getparameterchanged(kPredelay))
	{
		// tell the host what the length of delay compensation should be
		setlatency_seconds(seekRangeSeconds * getparameter_scalar(kPredelay));
		#if TARGET_API_VST
			// this tells the host to call a suspend()-resume() pair, 
			// which updates initialDelay value
			latencychanged = true;
		#endif
	}
	for (int i=0; i < NUM_PITCH_STEPS; i++)
	{
		if (getparameterchanged(i+kPitchStep0))
			// reset the associated note in the notes table; manual changes override MIDI
			activeNotesTable[i] = 0;
	}
	if (needResync != NULL)
	{
		for (unsigned long i=0; i < numBuffers; i++)
			needResync[i] = tempNeedResync;
	}

	if (tempoSync)
		useSeekRateRandMin = (seekRateRandMinSync < seekRateSync);
	else
		useSeekRateRandMin = (seekRateRandMinHz < seekRateHz);
	useSeekDurRandMin = (seekDurRandMin < seekDur);
}
