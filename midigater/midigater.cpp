/*-------------- by Marc Poirier  ][  November 2001 -------------*/

#ifndef __MIDIGATER_H
#include "midigater.hpp"
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	#ifndef __MIDIGATEREDITOR_H
	#include "midigatereditor.hpp"
	#endif
#endif


// this macro does boring entry point stuff for us
DFX_ENTRY(MidiGater);

//-----------------------------------------------------------------------------------------
// initializations
MidiGater::MidiGater(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, 1)	// 3 parameters, 1 preset
{
	initparameter_f(kSlope, "slope", 3.0f, 3.0f, 0.0f, 3000.0f, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_f(kVelInfluence, "velocity influence", 0.0f, 1.0f, 0.0f, 1.0f, kDfxParamUnit_scalar);
	initparameter_f(kFloor, "floor", 0.0f, 0.0f, 0.0f, 1.0f, kDfxParamUnit_lineargain, kDfxParamCurve_cubed);

	setpresetname(0, "push the button");	// default preset name

	midistuff->setLazyAttack();	// this enables the lazy note attack mode


	#ifdef TARGET_API_VST
		canProcessReplacing(false);	// only support accumulating output
		#if TARGET_PLUGIN_HAS_GUI
			editor = new MidiGaterEditor(this);
		#endif
	#endif
}

//-----------------------------------------------------------------------------------------
MidiGater::~MidiGater()
{
	// nud
}

//-----------------------------------------------------------------------------------------
void MidiGater::reset()
{
	// reset the unaffected between audio stuff
	unaffectedState = unFadeIn;
	unaffectedFadeSamples = 0;
}

//-----------------------------------------------------------------------------------------
void MidiGater::processparameters()
{
	slope_seconds = getparameter_f(kSlope) * 0.001f;
	velInfluence = getparameter_f(kVelInfluence);
	floor = getparameter_f(kFloor);
}


//-----------------------------------------------------------------------------------------
void MidiGater::processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing)
{
	unsigned long numChannels = getnumoutputs();
	long numFramesToProcess = (signed)inNumFrames, totalSampleFrames = (signed)inNumFrames;	// for dividing up the block accoring to events
	float SAMPLERATE = getsamplerate_f();


	// clear the output buffer because we accumulate output into it
	if (replacing)
	{
		for (unsigned long cha=0; cha < numChannels; cha++)
		{
			for (unsigned long samp=0; samp < inNumFrames; samp++)
				out[cha][samp] = 0.0f;
		}
	}


	// counter for the number of MIDI events this block
	// start at -1 because the beginning stuff has to happen
	long eventcount = -1;
	long currentBlockPosition = 0;	// we are at sample 0

	// now we're ready to start looking at MIDI messages & processing sound & such
	do
	{
		// check for an upcoming event & decrease this block chunk size accordingly 
		// if there will be another event
		if ( (eventcount+1) >= midistuff->numBlockEvents )
			numFramesToProcess = totalSampleFrames - currentBlockPosition;
		// else this chunk goes to the end of the block
		else
			numFramesToProcess = midistuff->blockEvents[eventcount+1].delta - currentBlockPosition;

		// this means that 2 (or more) events occur simultaneously, 
		// so there's no need to do calculations during this round
		if (numFramesToProcess == 0)
		{
			eventcount++;
			// take in the effects of the next event
			midistuff->heedEvents(eventcount, SAMPLERATE, 0.0f, slope_seconds, 
									slope_seconds, false, 1.0f, velInfluence);
			continue;
		}

		// test for whether or not all notes are off & unprocessed audio can be outputted
		bool noNotes = true;	// none yet for this chunk

		for (int notecount=0; notecount < NUM_NOTES; notecount++)
		{
			// only go into the output processing cycle if a note is happening
			if (midistuff->noteTable[notecount].velocity)
			{
				noNotes = false;	// we have a note
				for (long samplecount = currentBlockPosition; 
						samplecount < numFramesToProcess+currentBlockPosition; samplecount++)
				{
					// see whether attack or release are active & fetch the output scalar
					float envAmp = midistuff->processEnvelope(false, notecount);	// the attack/release scalar
					envAmp *=  midistuff->noteTable[notecount].noteAmp;	// scale by key velocity
					for (unsigned long ch=0; ch < numChannels; ch++)
						out[ch][samplecount] += in[ch][samplecount] * envAmp;
				}
			}
		}	// end of notes loop


		// we had notes this chunk, but the unaffected processing hasn't faded out, so change its state to fade-out
		if ( !noNotes && (unaffectedState == unFlat) )
			unaffectedState = unFadeOut;

		// we can output unprocessed audio if no notes happened during this block chunk
		// or if the unaffected fade-out still needs to be finished
		if ( noNotes || (unaffectedState == unFadeOut) )
			processUnaffected(in, out, numFramesToProcess, currentBlockPosition, numChannels);

		eventcount++;
		// don't do the event processing below if there are no more events
		if (eventcount >= midistuff->numBlockEvents)
			continue;

		// jump our position value forward
		currentBlockPosition = midistuff->blockEvents[eventcount].delta;

		// take in the effects of the next event
		midistuff->heedEvents(eventcount, SAMPLERATE, 0.0f, slope_seconds, 
								slope_seconds, false, 1.0f, velInfluence);

	} while (eventcount < midistuff->numBlockEvents);
}

//-----------------------------------------------------------------------------------------
// this function outputs the unprocessed audio input between notes, if desired
void MidiGater::processUnaffected(const float **in, float **out, long numFramesToProcess, long offset, unsigned long numChannels)
{
	long endPos = numFramesToProcess + offset;
	for (long samplecount=offset; samplecount < endPos; samplecount++)
	{
		float sampleAmp = floor;

		// this is the state when all notes just ended & the clean input first kicks in
		if (unaffectedState == unFadeIn)
		{
			// linear fade-in
			sampleAmp = (float)unaffectedFadeSamples * UNAFFECTED_FADE_STEP * floor;
			unaffectedFadeSamples++;
			// go to the no-gain state if the fade-in is done
			if (unaffectedFadeSamples >= UNAFFECTED_FADE_DUR)
				unaffectedState = unFlat;
		}
		// a note has just begun, so we need to hasily fade out the clean input audio
		else if (unaffectedState == unFadeOut)
		{
			unaffectedFadeSamples--;
			// linear fade-out
			sampleAmp = (float)unaffectedFadeSamples * UNAFFECTED_FADE_STEP * floor;
			// get ready for the next time & exit this function if the fade-out is done
			if (unaffectedFadeSamples <= 0)
			{
				// ready for the next time
				unaffectedState = unFadeIn;
				return;	// important!  leave this function or a new fade-in will begin
			}
		}

		for (unsigned long ch=0; ch < numChannels; ch++)
			out[ch][samplecount] += in[ch][samplecount] * sampleAmp;
	}
}
