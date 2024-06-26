/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2024  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
This is where we connect the VST API to our DfxPlugin system.
------------------------------------------------------------------------*/

#include "dfxplugin.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <limits>
#include <optional>
#include <utility>

#include "dfxmath.h"


#pragma mark -
#pragma mark init
#pragma mark -

//-----------------------------------------------------------------------------
// this is called right before our plugin instance is destroyed
// XXX TODO: test and make sure that this is working in various hosts!
void DfxPlugin::close()
{
	do_PreDestructor();
}

//-----------------------------------------------------------------------------
// this gets called when the plugin is de-activated
void DfxPlugin::suspend()
{
}

//-----------------------------------------------------------------------------
// this gets called when the plugin is activated
void DfxPlugin::resume()
{
	updatesamplerate();

	bool const inputChannelCountChanged = !mLastInputChannelCount || (*mLastInputChannelCount != getnuminputs());
	mLastInputChannelCount = getnuminputs();
	bool const outputChannelCountChanged = !mLastOutputChannelCount || (*mLastOutputChannelCount != getnumoutputs());
	mLastOutputChannelCount = getnumoutputs();
	bool const sampleRateChanged = !mLastSampleRate || (*mLastSampleRate != getsamplerate());
	mLastSampleRate = getsamplerate();
	bool const maxFrameCountChanged = !mLastMaxFrameCount || (*mLastMaxFrameCount != getmaxframes());
	mLastMaxFrameCount = getmaxframes();

	// VST doesn't have initialize and cleanup methods like Audio Unit does, 
	// so we need to call this here
	if (!mIsInitialized)
	{
		do_initialize();
	}
	else if (inputChannelCountChanged || outputChannelCountChanged || sampleRateChanged || maxFrameCountChanged)
	{
		do_cleanup();
		do_initialize();
	}
	else
	{
		do_reset();  // else because do_initialize calls do_reset
	}

	// do these after calling do_reset, 
	// because the value for latency could change there
	setInitialDelay(static_cast<VstInt32>(getlatency_samples()));

	// it sure seems like we need to call ioChanged since we just
	// called setInitialDelay(). -tom7
	ioChanged();
}

//-------------------------------------------------------------------------
void DfxPlugin::setSampleRate(float newRate)
{
	TARGET_API_BASE_CLASS::setSampleRate(newRate);
	updatesamplerate();
}



#pragma mark -
#pragma mark info
#pragma mark -

//-----------------------------------------------------------------------------
// this tells the host to keep calling process() for the duration of one buffer 
// even if the audio input has ended
VstInt32 DfxPlugin::getGetTailSize()
{
	return static_cast<VstInt32>(gettailsize_samples());
}


//------------------------------------------------------------------------
bool DfxPlugin::getInputProperties(VstInt32 index, VstPinProperties* properties)
{
	if ((index >= 0) && (dfx::math::ToIndex(index) < getnuminputs()) && properties)
	{
		std::snprintf(properties->label, kVstMaxLabelLen, "%s input %d", PLUGIN_NAME_STRING, index + 1);
		std::snprintf(properties->shortLabel, kVstMaxShortLabelLen, "in %d", index + 1);
		properties->flags = kVstPinIsActive;
		if (getnuminputs() == 2)
		{
			properties->flags |= kVstPinIsStereo;
		}
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool DfxPlugin::getOutputProperties(VstInt32 index, VstPinProperties* properties)
{
	if ((index >= 0) && (dfx::math::ToIndex(index) < getnumoutputs()) && properties)
	{
		std::snprintf(properties->label, kVstMaxLabelLen, "%s output %d", PLUGIN_NAME_STRING, index + 1);
		std::snprintf(properties->shortLabel, kVstMaxShortLabelLen, "out %d", index + 1);
		properties->flags = kVstPinIsActive;
		if (getnumoutputs() == 2)
		{
			properties->flags |= kVstPinIsStereo;
		}
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// plugin identifying infos

bool DfxPlugin::getEffectName(char* outText)
{
	if (!outText)
	{
		return false;
	}
	vst_strncpy(outText, getpluginname().c_str(), kVstMaxEffectNameLen);
	return true;
}

VstInt32 DfxPlugin::getVendorVersion()
{
	return static_cast<VstInt32>(getpluginversion());
}

bool DfxPlugin::getVendorString(char* outText)
{
	if (!outText)
	{
		return false;
	}
	vst_strncpy(outText, PLUGIN_CREATOR_NAME_STRING, kVstMaxVendorStrLen);
	return true;
}

bool DfxPlugin::getProductString(char* outText)
{
	if (!outText)
	{
		return false;
	}
	vst_strncpy(outText, PLUGIN_COLLECTION_NAME, kVstMaxProductStrLen);
	return true;
}

//-----------------------------------------------------------------------------
// this just tells the host what this plugin can do
VstInt32 DfxPlugin::canDo(char* text)
{
	if (!text)
	{
		return -1;
	}

	std::string const stdText(text);
	if (stdText == "plugAsChannelInsert")
	{
		return 1;
	}
	if (stdText == "plugAsSend")
	{
		return 1;
	}
	if (stdText == "mixDryWet")
	{
		return 1;
	}
	if (stdText == "fuctsoundz")
	{
		return 1;
	}
	if (stdText == "receiveVstTimeInfo")
	{
		return 1;
	}
	// XXX this channels canDo stuff could be improved...
	if (getnumoutputs() > 1)
	{
		if (stdText == "1in2out")
		{
			return 1;
		}
		if (getnuminputs() > 1)
		{
			if (stdText == "2in2out")
			{
				return 1;
			}
		}
	}
	else
	{
		if (stdText == "1in1out")
		{
			return 1;
		}
	}
	#if TARGET_PLUGIN_USES_MIDI
		if (stdText == "receiveVstEvents")
		{
			return 1;
		}
		if (stdText == "receiveVstMidiEvent")
		{
			return 1;
		}
	#endif

	return -1;  // explicitly can't do; 0 => don't know
}


#pragma mark -
#pragma mark programs
#pragma mark -

//-----------------------------------------------------------------------------
void DfxPlugin::setProgram(VstInt32 inProgramNum)
{
	if (inProgramNum >= 0)
	{
		loadpreset(dfx::math::ToIndex(inProgramNum));
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setProgramName(char* inName)
{
	if (inName)
	{
		assert(inName[0] != '\0');
		setpresetname(dfx::math::ToIndex(getProgram()), inName);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::getProgramName(char* outText)
{
	getProgramNameIndexed({}, getProgram(), outText);
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getProgramNameIndexed(VstInt32 /*inCategory*/, VstInt32 inIndex, char* outText)
{
	if (!outText)
	{
		return false;
	}

	if ((inIndex >= 0) && presetisvalid(dfx::math::ToIndex(inIndex)))
	{
		if (presetnameisvalid(dfx::math::ToIndex(inIndex)))
		{
			vst_strncpy(outText, getpresetname(dfx::math::ToIndex(inIndex)).c_str(), kVstMaxProgNameLen);
		}
		else
		{
			std::snprintf(outText, kVstMaxProgNameLen + 1, "default %" PRIi32, inIndex + 1);
		}
		return true;
	}
	return false;
}

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
VstInt32 DfxPlugin::getChunk(void** data, bool isPreset)
{
	// The VST API awkwardly makes the plugin allocate this buffer and
	// maintain it, but the lifetime isn't totally clear ("You can
	// savely release it in next suspend/resume call." -audioeffect.cpp.
	// But can there be multiple calls to getChunk in the meantime?) We
	// assume not, and just keep a buffer (mLastChunk) with the last
	// saved chunk.
	mLastChunk = mDfxSettings->save(isPreset);
	if (data != nullptr)
	{
		*data = mLastChunk.data();
	}
	return static_cast<VstInt32>(mLastChunk.size());
}

//-----------------------------------------------------------------------------
// notes from Charlie Steinberg:
// "host gives you data and its bytesize (as it got from your getChunk() way
// back then, when it saved it).
// <data> is only valid during this call.
// you may want to check the bytesize, and certainly should maintain a version."
VstInt32 DfxPlugin::setChunk(void* data, VstInt32 byteSize, bool isPreset)
{
	assert(byteSize >= 0);
	auto const result = mDfxSettings->restore(data, static_cast<size_t>(byteSize), isPreset);
	if (!isPreset)
	{
		// some hosts (like FL Studio) only load the active program from a bank after setChunk
		// if it differs from the current program, and others (like REAPER) load it before setChunk,
		// so explicitly apply parameters from at least the current program after loading a bank
		// (the host can always still load a different program afterwards)
		setProgram(getProgram());
	}
	return result;
}

#endif
// TARGET_PLUGIN_USES_MIDI



#pragma mark -
#pragma mark parameters
#pragma mark -

//-----------------------------------------------------------------------------
void DfxPlugin::setParameter(VstInt32 index, float value)
{
	setparameter_gen(dfx::ParameterID_FromVST(index), value);
}

//-----------------------------------------------------------------------------
float DfxPlugin::getParameter(VstInt32 index)
{
	return getparameter_gen(dfx::ParameterID_FromVST(index));
}

//-----------------------------------------------------------------------------
// titles of each parameter
void DfxPlugin::getParameterName(VstInt32 index, char* name)
{
	if (name)
	{
		// kVstMaxParamStrLen is absurdly short for parameter names at 8-characters, 
		// and given that exceding that length is common, hosts prepare for that, 
		// and even the Vst2Wrapper in the VST3 SDK acknowledges as much by defining 
		// the following "extended" constant to use instead
		constexpr size_t kVstExtMaxParamStrLen = 32;
		vst_strncpy(name, getparametername(dfx::ParameterID_FromVST(index)).c_str(), kVstExtMaxParamStrLen);
	}
}

//-----------------------------------------------------------------------------
// numerical display of each parameter's gradations
void DfxPlugin::getParameterDisplay(VstInt32 index, char* text)
{
	if (!text)
	{
		return;
	}

	auto const parameterID = dfx::ParameterID_FromVST(index);
	if (getparameterusevaluestrings(parameterID))
	{
		vst_strncpy(text, getparametervaluestring(parameterID, getparameter_i(parameterID)).value_or("").c_str(), kVstMaxParamStrLen);
		return;
	}

	switch (getparametervaluetype(parameterID))
	{
		case DfxParam::Value::Type::Float:
			std::snprintf(text, kVstMaxParamStrLen + 1, "%.3f", getparameter_f(parameterID));
			break;
		case DfxParam::Value::Type::Int:
			std::snprintf(text, kVstMaxParamStrLen + 1, "%" PRIi64, getparameter_i(parameterID));
			break;
		case DfxParam::Value::Type::Boolean:
			vst_strncpy(text, getparameter_b(parameterID) ? "on" : "off", kVstMaxParamStrLen);
			break;
		default:
			std::unreachable();
	}
}

//-----------------------------------------------------------------------------
// unit of measure for each parameter
void DfxPlugin::getParameterLabel(VstInt32 index, char* label)
{
	if (label)
	{
		vst_strncpy(label, getparameterunitstring(dfx::ParameterID_FromVST(index)).c_str(), kVstMaxParamStrLen);
	}
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getParameterProperties(VstInt32 index, VstParameterProperties* properties)
{
	if (!properties)
	{
		return false;
	}
	*properties = {};
	auto const parameterID = dfx::ParameterID_FromVST(index);

	// note that kVstMax* are specified as "number of characters excluding the null terminator", 
	// and vst_strncpy behaves accordingly and writes null at array[N], however the static arrays 
	// in struct types (like VstParameterProperties here) size the arrays by N, meaning that 
	// array[N] is out of range, so we call vst_strncpy in these cases passing N-1 as max length
	vst_strncpy(properties->label, getparametername(parameterID).c_str(), kVstMaxLabelLen - 1);

	constexpr size_t recommendedShortLabelLength = 6;  // per the VST header comment
	static_assert(recommendedShortLabelLength < kVstMaxShortLabelLen);
	auto const shortName = getparametername(parameterID, recommendedShortLabelLength);
	vst_strncpy(properties->shortLabel, shortName.c_str(), kVstMaxShortLabelLen - 1);

	auto const isVisible = [this](dfx::ParameterID parameterID)
	{
		return !hasparameterattribute(parameterID, DfxParam::kAttribute_Unused) && !hasparameterattribute(parameterID, DfxParam::kAttribute_Hidden);
	};
	if (isVisible(parameterID))
	{
		properties->displayIndex = [parameterID, isVisible]
		{
			VstInt16 result = 0;
			for (dfx::ParameterID i = 0; i <= parameterID; i++)
			{
				if (isVisible(i))
				{
					assert(result < std::numeric_limits<decltype(result)>::max());
					result++;
				}
			}
			return result;
		}();
		properties->flags |= kVstParameterSupportsDisplayIndex;
	}

	switch (getparametervaluetype(parameterID))
	{
		case DfxParam::Value::Type::Boolean:
			properties->flags |= kVstParameterIsSwitch;
			break;
		case DfxParam::Value::Type::Int:
			properties->minInteger = static_cast<VstInt32>(getparametermin_i(parameterID));
			properties->maxInteger = static_cast<VstInt32>(getparametermax_i(parameterID));
			properties->flags |= kVstParameterUsesIntegerMinMax;
			break;
		default:
			break;
	}

	if (auto const groupIndex = getparametergroup(parameterID))
	{
		auto const downcastWithValidation = [](auto& destination, auto const& source)
		{
			using FromT [[maybe_unused]] = std::decay_t<decltype(source)>;
			using ToT = std::decay_t<decltype(destination)>;
			assert(source <= static_cast<FromT>(std::numeric_limits<ToT>::max()));
			destination = static_cast<ToT>(source);
		};

		constexpr size_t baseCategoryIndex = 1;
		downcastWithValidation(properties->category, baseCategoryIndex + *groupIndex);
		auto const& group = mParameterGroups.at(*groupIndex);
		vst_strncpy(properties->categoryLabel, group.first.c_str(), kVstMaxCategLabelLen - 1);
		downcastWithValidation(properties->numParametersInCategory, group.second.size());
		properties->flags |= kVstParameterSupportsDisplayCategory;
	}

	return true;
}



#pragma mark -
#pragma mark DSP
#pragma mark -

//-----------------------------------------------------------------------------------------
void DfxPlugin::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
	preprocessaudio(dfx::math::ToUnsigned(sampleFrames));

	for (size_t ch = 0; ch < getnuminputs(); ch++)
	{
		if (mInPlaceAudioProcessingAllowed)
		{
			mInputAudioStreams[ch] = inputs[ch];
		}
		else
		{
			assert(std::ssize(mInputOutOfPlaceAudioBuffers.front()) >= sampleFrames);
			std::copy_n(inputs[ch], dfx::math::ToUnsigned(sampleFrames), mInputOutOfPlaceAudioBuffers[ch].data());
		}
	}
	if (!mInPlaceAudioProcessingAllowed)
	{
		for (size_t ch = 0; ch < getnumoutputs(); ch++)
		{
			std::fill_n(outputs[ch], dfx::math::ToUnsigned(sampleFrames), 0.f);
		}
	}

#if TARGET_PLUGIN_USES_DSPCORE
	for (size_t ch = 0; ch < getnumoutputs(); ch++)
	{
		if (mDSPCores[ch])
		{
			std::span inputAudio(mInputAudioStreams[ch], dfx::math::ToUnsigned(sampleFrames));
			if (asymmetricalchannels())
			{
				if (ch == 0)
				{
					assert(std::ssize(mAsymmetricalInputAudioBuffer) >= sampleFrames);
					std::copy_n(mInputAudioStreams[ch], dfx::math::ToUnsigned(sampleFrames), mAsymmetricalInputAudioBuffer.data());
				}
				inputAudio = std::span(mAsymmetricalInputAudioBuffer).subspan(0, dfx::math::ToUnsigned(sampleFrames));
			}
			mDSPCores[ch]->process(inputAudio, {outputs[ch], dfx::math::ToUnsigned(sampleFrames)});
		}
	}
#else
	processaudio(mInputAudioStreams, {outputs, getnumoutputs()}, dfx::math::ToUnsigned(sampleFrames));
#endif

	postprocessaudio();
}



#pragma mark -
#pragma mark MIDI
#pragma mark -

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------------------
// note:  it is possible and allowable for this method to be called more than once per audio processing block
VstInt32 DfxPlugin::processEvents(VstEvents* events)
{
	size_t newProgramIndex {};
	std::optional<int> newProgramOffset;

	for (VstInt32 i = 0; i < events->numEvents; i++)
	{
		// check to see if this event is MIDI; if no, then we try the for-loop again
		if (events->events[i]->type != kVstMidiType)
		{
			continue;
		}

		// cast the incoming event as a VstMidiEvent
		auto const midiEvent = reinterpret_cast<VstMidiEvent const*>(events->events[i]);
		// address the midiData[4] string from the event to this temp data pointer
		auto const midiData = midiEvent->midiData;

		// save the channel number ...
		int const channel = midiData[0] & 0x0F;
		// ... and then wipe out the channel (lower 4 bits) for simplicity
		int const status = midiData[0] & 0xF0;
		int const byte1 = midiData[1] & 0x7F;
		int const byte2 = midiData[2] & 0x7F;
		auto const offsetFrames = dfx::math::ToUnsigned(midiEvent->deltaFrames);  // timing offset

		// looking at notes   (0x9* is Note On status ~ 0x8* is Note Off status)
		if ((status == DfxMidi::kStatus_NoteOn) || (status == DfxMidi::kStatus_NoteOff))
		{
			// note-off received
			if ((status == DfxMidi::kStatus_NoteOff) || (byte2 == 0))
			{
				handlemidi_noteoff(channel, byte1, byte2, offsetFrames);
			}
			// note-on received
			else
			{
				handlemidi_noteon(channel, byte1, byte2, offsetFrames);
			}
		}

		else if (status == DfxMidi::kStatus_ChannelAftertouch)
		{
			handlemidi_channelaftertouch(channel, byte1, offsetFrames);
		}

		// looking at pitchbend   (0xE* is pitchbend status)
		else if (status == DfxMidi::kStatus_PitchBend)
		{
			handlemidi_pitchbend(channel, byte1, byte2, offsetFrames);
		}

		// continuous controller
		else if (status == DfxMidi::kStatus_CC)
		{
			// all notes off
			if (byte1 == DfxMidi::kCC_AllNotesOff)
			{
				handlemidi_allnotesoff(channel, offsetFrames);
			}
			else
			{
				handlemidi_cc(channel, byte1, byte2, offsetFrames);
			}
		}

		// program change
		else if (status == DfxMidi::kStatus_ProgramChange)
		{
//			handlemidi_programchange(channel, byte1, offsetFrames);
			// XXX maybe this is really what we want to do with these?
			if (!newProgramOffset || (midiEvent->deltaFrames >= *newProgramOffset))
			{
				newProgramIndex = dfx::math::ToIndex(byte1);
				newProgramOffset = midiEvent->deltaFrames;
			}
//			loadpreset(byte1);
		}
	}

	// change the plugin's program if a program change message was received
	if (newProgramOffset)
	{
		loadpreset(newProgramIndex);
	}


	// tells the host to keep calling this function in the future; 0 means stop
	return 1;
}
#endif
// TARGET_PLUGIN_USES_MIDI
