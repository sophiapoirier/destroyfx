/*------------------------------------------------------------------------
Copyright (C) 2001-2024  Sophia Poirier

This file is part of MIDI Gater.

MIDI Gater is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

MIDI Gater is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MIDI Gater.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "midigater.h"

#include <algorithm>

#include "dfxmath.h"
#include "dfxmisc.h"


//-----------------------------------------------------------------------------
// constants
constexpr auto kAmplitudeGateEnvelopeCurve = DfxEnvelope::kCurveType_Cubed;


// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(MIDIGater)


//-----------------------------------------------------------------------------------------
// initializations
MIDIGater::MIDIGater(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, 1)
{
	initparameter_f(kAttack, {dfx::kParameterNames_Attack}, 3., 3., 0., 3000., DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_f(kRelease, {dfx::kParameterNames_Release}, 30., 30., 0., 3000., DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_f(kVelocityInfluence, {dfx::kParameterNames_VelocityInfluence}, 0., 1., 0., 1., DfxParam::Unit::Scalar);
	initparameter_f(kFloor, {dfx::kParameterNames_Floor}, 0., 0., 0., 1., DfxParam::Unit::LinearGain, DfxParam::Curve::Cubed);
	initparameter_list(kGateMode, {"gate mode", "GateMod", "GatMod", "G8Md"}, kGateMode_Amplitude, kGateMode_Amplitude, kNumGateModes);

	setparametervaluestring(kGateMode, kGateMode_Amplitude, "amplitude");
	setparametervaluestring(kGateMode, kGateMode_Lowpass, "low-pass");

	setpresetname(0, "push the button");  // default preset name

	setInPlaceAudioProcessingAllowed(false);
	getmidistate().setResumedAttackMode(true);  // this enables the lazy note attack mode
	getmidistate().setEnvCurveType(kAmplitudeGateEnvelopeCurve);

	registerSmoothedAudioValue(mFloor);
}

//-----------------------------------------------------------------------------------------
void MIDIGater::initialize()
{
	std::ranges::fill(mLowpassGateFilters,
					  decltype(mLowpassGateFilters)::value_type(getnumoutputs(), dfx::IIRFilter(getsamplerate())));
}

//-----------------------------------------------------------------------------------------
void MIDIGater::cleanup()
{
	std::ranges::fill(mLowpassGateFilters, decltype(mLowpassGateFilters)::value_type{});
}

//-----------------------------------------------------------------------------------------
void MIDIGater::reset()
{
	resetFilters();
}

//-----------------------------------------------------------------------------------------
void MIDIGater::resetFilters()
{
	std::ranges::for_each(mLowpassGateFilters, [](auto& channelFilters)
	{
		std::ranges::for_each(channelFilters, [](auto& filter){ filter.reset(); });
	});
}

//-----------------------------------------------------------------------------------------
void MIDIGater::processparameters()
{
	if (getparameterchanged(kAttack) || getparameterchanged(kRelease))
	{
		auto const attack_seconds = getparameter_f(kAttack) * 0.001;
		auto const release_seconds = getparameter_f(kRelease) * 0.001;
		getmidistate().setEnvParameters(attack_seconds, release_seconds);
	}
	mVelocityInfluence = getparameter_f(kVelocityInfluence);
	if (auto const value = getparameterifchanged_f(kFloor))
	{
		mFloor = *value;
	}
	mGateMode = getparameter_i(kGateMode);
	if (getparameterchanged(kGateMode))
	{
		bool const isLPG = (mGateMode == kGateMode_Lowpass);
		getmidistate().setEnvCurveType(isLPG ? DfxEnvelope::kCurveType_Linear : kAmplitudeGateEnvelopeCurve);
		if (!isLPG)
		{
			resetFilters();
		}
	}
}


//-----------------------------------------------------------------------------------------
void MIDIGater::processaudio(float const* const* inAudio, float* const* outAudio, size_t inNumFrames)
{
	auto const numChannels = getnumoutputs();
	auto numFramesToProcess = inNumFrames;  // for dividing up the block according to events
	auto const filterSmoothingStride = dfx::math::GetFrequencyBasedSmoothingStride(getsamplerate());


	// add the "floor" audio input
	if (mFloor.isSmoothing() || (mFloor.getValue() > 0.0f))
	{
		for (size_t sampleIndex = 0; sampleIndex < inNumFrames; sampleIndex++)
		{
			for (size_t ch = 0; ch < numChannels; ch++)
			{
				outAudio[ch][sampleIndex] = inAudio[ch][sampleIndex] * mFloor.getValue();
			}
			mFloor.inc();
		}
	}
	// this is the correct state that smoothed floor should be in upon exiting this audio render
	auto const exitFloor = mFloor;


	constexpr float velocityCurve = 1.0f;

	// counter for the number of MIDI events this block
	// start at -1 because the beginning stuff has to happen
	long eventCount = -1;
	size_t currentBlockPosition = 0;

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

		// this means that two (or more) events occur simultaneously,
		// so there's no need to do calculations during this round
		if (numFramesToProcess == 0)
		{
			eventCount++;
			// take in the effects of the next event
			getmidistate().heedEvents(eventCount, velocityCurve, mVelocityInfluence);
			continue;
		}

		bool noteActive = false;  // test for whether any notes are are on in this chunk
		auto const entryFloor = mFloor;

		for (int noteCount = 0; noteCount < DfxMidi::kNumNotes; noteCount++)
		{
			auto& channelFilters = mLowpassGateFilters[noteCount];
			// only go into the output processing cycle if a note is happening
			if (getmidistate().isNoteActive(noteCount))
			{
				noteActive = true;  // we have a note
				mFloor = entryFloor;
				float postFilterAmp = 1.f;
				for (size_t sampleIndex = currentBlockPosition; sampleIndex < (numFramesToProcess + currentBlockPosition); sampleIndex++)
				{
					float noteAmp = getmidistate().getNoteAmplitude(noteCount);  // key velocity
					noteAmp *= 1.0f - mFloor.getValue();  // maximum note amplitude is scaled by what is above the floor
					if (mGateMode == kGateMode_Lowpass)
					{
						if (((sampleIndex - currentBlockPosition) % filterSmoothingStride) == 0)
						{
							dfx::IIRFilter::Coefficients filterCoef;
							std::tie(filterCoef, postFilterAmp) = getmidistate().processEnvelopeLowpassGate(noteCount);
							std::ranges::for_each(channelFilters, [&filterCoef](auto& filter)
							{
								filter.setCoefficients(filterCoef);
							});
						}
						else
						{
							getmidistate().processEnvelope(noteCount);  // to temporally progress the envelope's state
						}
						noteAmp *= postFilterAmp;
						for (size_t ch = 0; ch < numChannels; ch++)
						{
							outAudio[ch][sampleIndex] += channelFilters[ch].process(inAudio[ch][sampleIndex]) * noteAmp;
						}
					}
					else
					{
						// see whether attack or release are active and fetch the output scalar
						noteAmp *= getmidistate().processEnvelope(noteCount);  // scale by the attack/release envelope
						for (size_t ch = 0; ch < numChannels; ch++)
						{
							outAudio[ch][sampleIndex] += inAudio[ch][sampleIndex] * noteAmp;
						}
					}
					mFloor.inc();
				}
			}

			// could be because it already was inactive, or because we just completed articulation of the note
			if (!getmidistate().isNoteActive(noteCount))
			{
				std::ranges::for_each(channelFilters, [](auto& filter){ filter.reset(); });
			}
		}  // end of notes loop

		// catch up smoothing if no notes rendered during this chunk
		if (!noteActive)
		{
			mFloor.inc(numFramesToProcess);
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
		getmidistate().heedEvents(eventCount, velocityCurve, mVelocityInfluence);

	} while (eventCount < getmidistate().getBlockEventCount());


	mFloor = exitFloor;
}
