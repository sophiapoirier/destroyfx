/*-------------- by Marc Poirier  ][  November 2001 -------------*/

#include "midigater.hpp"

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	#include "midigatereditor.hpp"
#endif


//----------------------------------------------------------------------------- 
// constants
const long kUnaffectedFadeDur = 18;
const float kUnaffectedFadeStep = 1.0f / (float)kUnaffectedFadeDur;

const bool kUseNiceAudioFades = true;
const bool kUseLegato = false;

// these are the 3 states of the unaffected audio input between notes
enum
{
	unFadeIn,
	unFlat,
	unFadeOut
};


// this macro does boring entry point stuff for us
DFX_ENTRY(MidiGater)


//-----------------------------------------------------------------------------------------
// initializations
MidiGater::MidiGater(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, 1)	// 4 parameters, 1 preset
{
	initparameter_f(kAttackSlope, "attack", 3.0, 3.0, 0.0, 3000.0, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_f(kReleaseSlope, "release", 3.0, 3.0, 0.0, 3000.0, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_f(kVelInfluence, "velocity influence", 0.0, 1.0, 0.0, 1.0, kDfxParamUnit_scalar);
	initparameter_f(kFloor, "floor", 0.0, 0.0, 0.0, 1.0, kDfxParamUnit_lineargain, kDfxParamCurve_cubed);

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
void MidiGater::reset()
{
	// reset the unaffected between-audio stuff
	unaffectedState = unFadeIn;
	unaffectedFadeSamples = 0;
}

//-----------------------------------------------------------------------------------------
void MidiGater::processparameters()
{
	attackSlope_seconds = getparameter_f(kAttackSlope) * 0.001;
	releaseSlope_seconds = getparameter_f(kReleaseSlope) * 0.001;
	velInfluence = getparameter_f(kVelInfluence);
	floor = getparameter_f(kFloor);
}


//-----------------------------------------------------------------------------------------
void MidiGater::processaudio(const float ** inAudio, float ** outAudio, unsigned long inNumFrames, bool inReplacing)
{
	unsigned long numChannels = getnumoutputs();
	long numFramesToProcess = (signed)inNumFrames, totalSampleFrames = (signed)inNumFrames;	// for dividing up the block accoring to events


#ifndef TARGET_API_VST
	// clear the output buffer because we accumulate output into it
	for (unsigned long cha=0; cha < numChannels; cha++)
	{
		for (unsigned long samp=0; samp < inNumFrames; samp++)
			outAudio[cha][samp] = 0.0f;
	}
#endif


	// counter for the number of MIDI events this block
	// start at -1 because the beginning stuff has to happen
	long eventcount = -1;
	long currentBlockPosition = 0;	// we are at sample 0

	// now we're ready to start looking at MIDI messages and processing sound and such
	do
	{
		// check for an upcoming event and decrease this block chunk size accordingly 
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
			midistuff->heedEvents(eventcount, getsamplerate_f(), 0.0f, attackSlope_seconds, 
									releaseSlope_seconds, kUseLegato, 1.0f, velInfluence);
			continue;
		}

		// test for whether or not all notes are off and unprocessed audio can be outputted
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
					// see whether attack or release are active and fetch the output scalar
					float envAmp = midistuff->processEnvelope(kUseNiceAudioFades, notecount);	// the attack/release scalar
					envAmp *=  midistuff->noteTable[notecount].noteAmp;	// scale by key velocity
					for (unsigned long ch=0; ch < numChannels; ch++)
						outAudio[ch][samplecount] += inAudio[ch][samplecount] * envAmp;
				}
			}
		}	// end of notes loop


		// we had notes this chunk, but the unaffected processing hasn't faded out, so change its state to fade-out
		if ( !noNotes && (unaffectedState == unFlat) )
			unaffectedState = unFadeOut;

		// we can output unprocessed audio if no notes happened during this block chunk
		// or if the unaffected fade-out still needs to be finished
		if ( noNotes || (unaffectedState == unFadeOut) )
			processUnaffected(inAudio, outAudio, numFramesToProcess, currentBlockPosition, numChannels);

		eventcount++;
		// don't do the event processing below if there are no more events
		if (eventcount >= midistuff->numBlockEvents)
			continue;

		// jump our position value forward
		currentBlockPosition = midistuff->blockEvents[eventcount].delta;

		// take in the effects of the next event
		midistuff->heedEvents(eventcount, getsamplerate_f(), 0.0f, attackSlope_seconds, 
								releaseSlope_seconds, kUseLegato, 1.0f, velInfluence);

	} while (eventcount < midistuff->numBlockEvents);
}

//-----------------------------------------------------------------------------------------
// this function outputs the unprocessed audio input between notes, if desired
void MidiGater::processUnaffected(const float ** inAudio, float ** outAudio, long inNumFramesToProcess, long inOffset, unsigned long numChannels)
{
	long endPos = inNumFramesToProcess + inOffset;
	for (long samplecount=inOffset; samplecount < endPos; samplecount++)
	{
		float sampleAmp = floor;

		// this is the state when all notes just ended and the clean input first kicks in
		if (unaffectedState == unFadeIn)
		{
			// linear fade-in
			sampleAmp = (float)unaffectedFadeSamples * kUnaffectedFadeStep * floor;
			unaffectedFadeSamples++;
			// go to the no-gain state if the fade-in is done
			if (unaffectedFadeSamples >= kUnaffectedFadeDur)
				unaffectedState = unFlat;
		}
		// a note has just begun, so we need to hasily fade out the clean input audio
		else if (unaffectedState == unFadeOut)
		{
			unaffectedFadeSamples--;
			// linear fade-out
			sampleAmp = (float)unaffectedFadeSamples * kUnaffectedFadeStep * floor;
			// get ready for the next time and exit this function if the fade-out is done
			if (unaffectedFadeSamples <= 0)
			{
				// ready for the next time
				unaffectedState = unFadeIn;
				return;	// important!  leave this function or a new fade-in will begin
			}
		}

		for (unsigned long ch=0; ch < numChannels; ch++)
			outAudio[ch][samplecount] += inAudio[ch][samplecount] * sampleAmp;
	}
}
