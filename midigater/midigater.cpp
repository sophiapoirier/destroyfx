/*------------------------------------------------------------------------
Copyright (C) 2001-2020  Sophia Poirier

This file is part of MIDI Gater.

MIDI Gater is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

MIDI Gater is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with MIDI Gater.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "midigater.h"

#include "dfxmisc.h"


//----------------------------------------------------------------------------- 
// constants
constexpr long kUnaffectedFadeDur = 18;
constexpr float kUnaffectedFadeStep = 1.0f / static_cast<float>(kUnaffectedFadeDur);

constexpr bool kUseLegato = false;


// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(MIDIGater)


//-----------------------------------------------------------------------------------------
// initializations
MIDIGater::MIDIGater(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, 1)
{
	initparameter_f(kAttackSlope, dfx::MakeParameterNames(dfx::kParameterNames_Attack), 3.0, 3.0, 0.0, 3000.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_f(kReleaseSlope, dfx::MakeParameterNames(dfx::kParameterNames_Release), 3.0, 3.0, 0.0, 3000.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_f(kVelocityInfluence, dfx::MakeParameterNames(dfx::kParameterNames_VelocityInfluence), 0.0, 1.0, 0.0, 1.0, DfxParam::Unit::Scalar);
	initparameter_f(kFloor, dfx::MakeParameterNames(dfx::kParameterNames_Floor), 0.0, 0.0, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Cubed);

	setpresetname(0, "push the button");  // default preset name

	setAudioProcessingMustAccumulate(true);  // only support accumulating output
	getmidistate().setResumedAttackMode(true);  // this enables the lazy note attack mode
	getmidistate().setEnvCurveType(DfxEnvelope::kCurveType_Cubed);

	registerSmoothedAudioValue(&mFloor);
}

//-----------------------------------------------------------------------------------------
void MIDIGater::reset()
{
	// reset the unaffected between-audio stuff
	mUnaffectedState = UnaffectedState::FadeIn;
	mUnaffectedFadeSamples = 0;
}

//-----------------------------------------------------------------------------------------
void MIDIGater::processparameters()
{
	mAttackSlope_Seconds = getparameter_f(kAttackSlope) * 0.001;
	mReleaseSlope_Seconds = getparameter_f(kReleaseSlope) * 0.001;
	mVelocityInfluence = getparameter_f(kVelocityInfluence);
	if (auto const value = getparameterifchanged_f(kFloor))
	{
		mFloor = *value;
	}

	constexpr float kNoDecay = 0.0f;
	constexpr float kFullSustain = 1.0f;
	getmidistate().setEnvParameters(mAttackSlope_Seconds, kNoDecay, kFullSustain, mReleaseSlope_Seconds);
}


//-----------------------------------------------------------------------------------------
void MIDIGater::processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames)
{
	auto const numChannels = getnumoutputs();
	auto numFramesToProcess = inNumFrames;  // for dividing up the block according to events


	// counter for the number of MIDI events this block
	// start at -1 because the beginning stuff has to happen
	long eventCount = -1;
	unsigned long currentBlockPosition = 0;

	// now we're ready to start looking at MIDI messages and processing sound and such
	do
	{
		// check for an upcoming event and decrease this block chunk size accordingly 
		// if there will be another event
		if ((eventCount + 1) >= getmidistate().getBlockEventCount())
		{
			numFramesToProcess = inNumFrames - currentBlockPosition;
		}
		// else this chunk goes to the end of the block
		else
		{
			numFramesToProcess = getmidistate().getBlockEvent(eventCount + 1).mOffsetFrames - currentBlockPosition;
		}

		// this means that 2 (or more) events occur simultaneously, 
		// so there's no need to do calculations during this round
		if (numFramesToProcess == 0)
		{
			eventCount++;
			// take in the effects of the next event
			getmidistate().heedEvents(eventCount, 0.0, kUseLegato, 1.0f, mVelocityInfluence);
			continue;
		}

		// test for whether or not all notes are off and unprocessed audio can be outputted
		bool noteActive = false;  // none yet for this chunk

		for (int noteCount = 0; noteCount < DfxMidi::kNumNotes; noteCount++)
		{
			// only go into the output processing cycle if a note is happening
			if (getmidistate().getNoteState(noteCount).mVelocity)
			{
				noteActive = true;  // we have a note
				for (unsigned long sampleCount = currentBlockPosition; sampleCount < (numFramesToProcess + currentBlockPosition); sampleCount++)
				{
					// see whether attack or release are active and fetch the output scalar
					float envAmp = getmidistate().processEnvelope(noteCount);  // the attack/release scalar
					envAmp *= getmidistate().getNoteState(noteCount).mNoteAmp;  // scale by key velocity
					for (unsigned long ch = 0; ch < numChannels; ch++)
					{
						outAudio[ch][sampleCount] += inAudio[ch][sampleCount] * envAmp;
					}
				}
			}
		}  // end of notes loop


		// we had notes this chunk, but the unaffected processing hasn't faded out, so change its state to fade-out
		if (noteActive && (mUnaffectedState == UnaffectedState::Flat))
		{
			mUnaffectedState = UnaffectedState::FadeOut;
		}

		// we can output unprocessed audio if no notes happened during this block chunk
		// or if the unaffected fade-out still needs to be finished
		if (!noteActive || (mUnaffectedState == UnaffectedState::FadeOut))
		{
			processUnaffected(inAudio, outAudio, numFramesToProcess, currentBlockPosition, numChannels);
		}

		eventCount++;
		// don't do the event processing below if there are no more events
		if (eventCount >= getmidistate().getBlockEventCount())
		{
			continue;
		}

		// jump our position value forward
		currentBlockPosition = getmidistate().getBlockEvent(eventCount).mOffsetFrames;

		// take in the effects of the next event
		getmidistate().heedEvents(eventCount, 0.0, kUseLegato, 1.0f, mVelocityInfluence);

	} while (eventCount < getmidistate().getBlockEventCount());
}

//-----------------------------------------------------------------------------------------
// this function outputs the unprocessed audio input between notes, if desired
void MIDIGater::processUnaffected(float const* const* inAudio, float* const* outAudio, 
								  unsigned long inNumFramesToProcess, unsigned long inOffsetFrames, unsigned long inNumChannels)
{
	auto const endPos = inNumFramesToProcess + inOffsetFrames;
	for (unsigned long sampleCount = inOffsetFrames; sampleCount < endPos; sampleCount++)
	{
		auto sampleAmp = mFloor.getValue();

		// this is the state when all notes just ended and the clean input first kicks in
		if (mUnaffectedState == UnaffectedState::FadeIn)
		{
			// linear fade-in
			sampleAmp = static_cast<float>(mUnaffectedFadeSamples) * kUnaffectedFadeStep * mFloor.getValue();
			mUnaffectedFadeSamples++;
			// go to the no-gain state if the fade-in is done
			if (mUnaffectedFadeSamples >= kUnaffectedFadeDur)
			{
				mUnaffectedState = UnaffectedState::Flat;
			}
		}
		// a note has just begun, so we need to hasily fade out the clean input audio
		else if (mUnaffectedState == UnaffectedState::FadeOut)
		{
			mUnaffectedFadeSamples--;
			// linear fade-out
			sampleAmp = static_cast<float>(mUnaffectedFadeSamples) * kUnaffectedFadeStep * mFloor.getValue();
			// get ready for the next time and exit this function if the fade-out is done
			if (mUnaffectedFadeSamples <= 0)
			{
				// ready for the next time
				mUnaffectedState = UnaffectedState::FadeIn;
				return;  // important!  leave this function or a new fade-in will begin
			}
		}

		for (unsigned long ch = 0; ch < inNumChannels; ch++)
		{
			outAudio[ch][sampleCount] += inAudio[ch][sampleCount] * sampleAmp;
		}

		incrementSmoothedAudioValues();
	}
}
