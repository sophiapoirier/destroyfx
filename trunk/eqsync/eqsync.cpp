/*------------------ by Marc Poirier  ][  January 2001 -----------------*/

#ifndef __EQSYNC_H
#include "eqsync.hpp"
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	#ifndef __EQSYNCEDITOR_H
	#include "eqsynceditor.hpp"
	#endif
#endif


// this macro does boring entry point stuff for us
DFX_ENTRY(EQsync);

//-----------------------------------------------------------------------------
EQsync::EQsync(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, 1)	// 9 parameters, 1 preset
{
	prevIn = NULL;
	prevprevIn = NULL;
	prevOut = NULL;
	prevprevOut = NULL;
	numBuffers = 0;

	tempoRateTable = new TempoRateTable;


	long numTempoRates = tempoRateTable->getNumTempoRates();
	long unitTempoRateIndex = tempoRateTable->getNearestTempoRateIndex(1.0f);
	initparameter_indexed(kRate_sync, "rate", unitTempoRateIndex, unitTempoRateIndex, numTempoRates, kDfxParamUnit_beats);
	initparameter_f(kSmooth, "smooth", 3.0f, 33.333f, 0.0f, 100.0f, kDfxParamUnit_percent);	// % of cycle
	initparameter_f(kTempo, "tempo", 120.0f, 120.0f, 39.0f, 480.0f, kDfxParamUnit_bpm);
	initparameter_b(kTempoAuto, "sync to host tempo", true, true);
//	for (long i=ka0; i <= kb2; i++)
//		initparameter_f(i, " ", 0.5f, 0.5f, 0.0f, 1.0f, kDfxParamUnit_generic);
	// okay, giving in and providing actual parameter names because Final Cut Pro folks say that it was causing problems...
	initparameter_f(ka0, "a0", 0.5f, 0.5f, 0.0f, 1.0f, kDfxParamUnit_generic);
	initparameter_f(ka1, "a1", 0.5f, 0.5f, 0.0f, 1.0f, kDfxParamUnit_generic);
	initparameter_f(ka2, "a2", 0.5f, 0.5f, 0.0f, 1.0f, kDfxParamUnit_generic);
	initparameter_f(kb1, "b1", 0.5f, 0.5f, 0.0f, 1.0f, kDfxParamUnit_generic);
	initparameter_f(kb2, "b2", 0.5f, 0.5f, 0.0f, 1.0f, kDfxParamUnit_generic);

	// set the value strings for the sync rate parameters
	for (long i=0; i < numTempoRates; i++)
		setparametervaluestring(kRate_sync, i, tempoRateTable->getDisplay(i));


	setpresetname(0, "with motors");	// default preset name

	// give currentTempoBPS a value in case that's useful for a freshly opened GUI
	currentTempoBPS = getparameter_f(kTempo) / 60.0f;


	#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
		editor = new EQsyncEditor(this);
	#endif
}

//-----------------------------------------------------------------------------------------
EQsync::~EQsync()
{
#ifdef TARGET_API_VST
	// VST doesn't have initialize and cleanup methods like Audio Unit does, 
	// so we need to call this manually here
	do_cleanup();
#endif
}

//-----------------------------------------------------------------------------------------
void EQsync::reset()
{
	cycleSamples = 1;
	smoothSamples = 1;
	smoothDur = 1;

	preva0 = preva1 = preva2 = prevb1 = prevb2 = 0.0f;
	cura0 = cura1 = cura2 = curb1 = curb2 = 0.0f;

	needResync = true;	// some hosts may call resume when restarting playback
}

//-----------------------------------------------------------------------------
bool EQsync::createbuffers()
{
	unsigned long oldnum = numBuffers;
	numBuffers = getnumoutputs();

	bool result1 = createbuffer_f(&prevIn, oldnum, numBuffers);
	bool result2 = createbuffer_f(&prevprevIn, oldnum, numBuffers);
	bool result3 = createbuffer_f(&prevOut, oldnum, numBuffers);
	bool result4 = createbuffer_f(&prevprevOut, oldnum, numBuffers);

	if (result1 && result2 && result3 && result4)
		return true;
	return false;
}

//-----------------------------------------------------------------------------
void EQsync::releasebuffers()
{
	releasebuffer_f(&prevIn);
	releasebuffer_f(&prevprevIn);
	releasebuffer_f(&prevOut);
	releasebuffer_f(&prevprevOut);
}

//-----------------------------------------------------------------------------
void EQsync::clearbuffers()
{
	clearbuffer_f(prevIn, numBuffers);
	clearbuffer_f(prevprevIn, numBuffers);
	clearbuffer_f(prevOut, numBuffers);
	clearbuffer_f(prevprevOut, numBuffers);
}

//-----------------------------------------------------------------------------
void EQsync::processparameters()
{
	rate = tempoRateTable->getScalar(getparameter_i(kRate_sync));
	smooth = getparameter_scalar(kSmooth);
	userTempo = getparameter_f(kTempo);
	useHostTempo = getparameter_b(kTempoAuto);
	a0 = getparameter_f(ka0);
	a1 = getparameter_f(ka1);
	a2 = getparameter_f(ka2);
	b1 = getparameter_f(kb1);
	b2 = getparameter_f(kb2);

	if (getparameterchanged(kRate_sync))
		needResync = true;	// make sure the cycles match up if the tempo rate has changed
}


//-----------------------------------------------------------------------------
void EQsync::processaudio(const float **inputs, float **outputs, unsigned long inNumFrames, bool replacing)
{
	unsigned long numChannels = getnumoutputs();
	bool eqchanged = false;

	// . . . . . . . . . . . tempo stuff . . . . . . . . . . . . .
	// calculate the tempo at the current processing buffer
	if ( useHostTempo && hostCanDoTempo && timeinfo.tempoIsValid )	// get the tempo from the host
	{
		currentTempoBPS = timeinfo.tempo_bps;
		// check if audio playback has just restarted & reset buffer stuff if it has (for measure sync)
		if (timeinfo.playbackChanged)
		{
			needResync = true;
			cycleSamples = 0;
		}
	}
	else	// get the tempo from the user parameter
	{
		currentTempoBPS = userTempo / 60.0f;
		needResync = false;	// we don't want it true if we're not syncing to host tempo
	}
	long latestCycleDur = (long) ( (getsamplerate_f()/currentTempoBPS) / rate );


	for (unsigned long samplecount=0; samplecount < inNumFrames; samplecount++)
	{
		cycleSamples--;	// decrement our EQ cycle counter
		if (cycleSamples <= 0)
		{
			// calculate the lengths of the next cycle & smooth portion
			cycleSamples = latestCycleDur;
			// see if we need to adjust this cycle so that an EQ change syncs with the next measure
			if (needResync)
				cycleSamples = timeinfo.samplesToNextBar % cycleSamples;
			needResync = false;	// set it false now that we're done with it
			
			smoothSamples = (long) ((float)cycleSamples*smooth);
			// if smoothSamples is 0, make smoothDur = 1 to avoid dividing by zero later on
			smoothDur = (smoothSamples <= 0) ? 1 : smoothSamples;

			// refill the "previous" filter parameter containers
			preva0 = a0;
			preva1 = a1;
			preva2 = a2;
			prevb1 = b1;
			prevb2 = b2;

			// generate "current" filter parameter values
			cura0 = randFloat();
			cura1 = randFloat();
			cura2 = randFloat();
			curb1 = randFloat();
			curb2 = randFloat();

			eqchanged = true;
		}

		// fade from the previous filter parameter values to the current ones
		if (smoothSamples >= 0)
		{
			//calculate the changing scalars for the two EQ settings
			float smoothIn = ((float)(smoothDur-smoothSamples)) / ((float)smoothDur);
			float smoothOut = ((float)smoothSamples) / ((float)smoothDur);

			a0 = (cura0*smoothIn) + (preva0*smoothOut);
			a1 = (cura1*smoothIn) + (preva1*smoothOut);
			a2 = (cura2*smoothIn) + (preva2*smoothOut);
			b1 = (curb1*smoothIn) + (prevb1*smoothOut);
			b2 = (curb2*smoothIn) + (prevb2*smoothOut);

			smoothSamples--;	// & decrement the counter
			eqchanged = true;
		}

//.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

		for (unsigned long ch=0; ch < numChannels; ch++)
		{
			// audio output section -- outputs the latest sample
			float inval = inputs[ch][samplecount];	// because Cubase inserts are goofy
			float outval = ( (inval*a0) + (prevIn[ch]*a1) + (prevprevIn[ch]*a2) 
							- (prevOut[ch]*b1) - (prevprevOut[ch]*b2) );
			undenormalize(outval);

			#ifdef TARGET_API_VST
				if (!replacing)
					outval += outputs[ch][samplecount];
			#endif
			// ...doing the complex filter thing here
			outputs[ch][samplecount] = outval;

			// update the previous sample holders & increment the i/o streams
			prevprevIn[ch] = prevIn[ch];
			prevIn[ch] = inval;
			prevprevOut[ch] = prevOut[ch];
			prevOut[ch] = outval;
		}

	} //end main while loop


	if (eqchanged)
	{
		setparameter_f(ka0, a0);
		setparameter_f(ka1, a1);
		setparameter_f(ka2, a2);
		setparameter_f(kb1, b1);
		setparameter_f(kb2, b2);
		for (long i=ka0; i <= kb2; i++)
			postupdate_parameter(i);	// inform listeners of change
	}
}
