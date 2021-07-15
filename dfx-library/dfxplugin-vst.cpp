/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2021  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/

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

	// VST doesn't have initialize and cleanup methods like Audio Unit does, 
	// so we need to call this here
	if (!mIsInitialized)
	{
		do_initialize();
	}
	else
	{
		do_reset();  // else because do_initialize calls do_reset
	}

	// do these after calling do_reset, 
	// because the value for latency could change there
	setInitialDelay(getlatency_samples());

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
	return gettailsize_samples();
}


//------------------------------------------------------------------------
bool DfxPlugin::getInputProperties(VstInt32 index, VstPinProperties* properties)
{
	if ((index >= 0) && (static_cast<unsigned long>(index) < getnuminputs()) && properties)
	{
		snprintf(properties->label, kVstMaxLabelLen, "%s input %d", PLUGIN_NAME_STRING, index + 1);
		snprintf(properties->shortLabel, kVstMaxShortLabelLen, "in %d", index + 1);
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
	if ((index >= 0) && (static_cast<unsigned long>(index) < getnumoutputs()) && properties)
	{
		snprintf(properties->label, kVstMaxLabelLen, "%s output %d", PLUGIN_NAME_STRING, index + 1);
		snprintf(properties->shortLabel, kVstMaxShortLabelLen, "out %d", index + 1);
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
	return getpluginversion();
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

	if (strcmp(text, "plugAsChannelInsert") == 0)
	{
		return 1;
	}
	if (strcmp(text, "plugAsSend") == 0)
	{
		return 1;
	}
	if (strcmp(text, "mixDryWet") == 0)
	{
		return 1;
	}
	if (strcmp(text, "fuctsoundz") == 0)
	{
		return 1;
	}
	if (strcmp(text, "receiveVstTimeInfo") == 0)
	{
		return 1;
	}
	// XXX this channels canDo stuff could be improved...
	if (getnumoutputs() > 1)
	{
		if (strcmp(text, "1in2out") == 0)
		{
			return 1;
		}
		if (getnuminputs() > 1)
		{
			if (strcmp(text, "2in2out") == 0)
			{
				return 1;
			}
		}
	}
	else
	{
		if (strcmp(text, "1in1out") == 0)
		{
			return 1;
		}
	}
	#if TARGET_PLUGIN_USES_MIDI
		if (strcmp(text, "receiveVstEvents") == 0)
		{
			return 1;
		}
		if (strcmp(text, "receiveVstMidiEvent") == 0)
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
	loadpreset(inProgramNum);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setProgramName(char* inName)
{
	if (inName)
	{
		assert(inName[0] != '\0');
		setpresetname(getProgram(), inName);
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

	if (presetisvalid(inIndex))
	{
		if (presetnameisvalid(inIndex))
		{
			vst_strncpy(outText, getpresetname(inIndex).c_str(), kVstMaxProgNameLen);
		}
		else
		{
			snprintf(outText, kVstMaxProgNameLen + 1, "default %" PRIi32, inIndex + 1);
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
	return mDfxSettings->restore(data, static_cast<size_t>(byteSize), isPreset);
}

#endif
// TARGET_PLUGIN_USES_MIDI



#pragma mark -
#pragma mark parameters
#pragma mark -

//-----------------------------------------------------------------------------
void DfxPlugin::setParameter(VstInt32 index, float value)
{
	setparameter_gen(index, value);
}

//-----------------------------------------------------------------------------
float DfxPlugin::getParameter(VstInt32 index)
{
	return getparameter_gen(index);
}

//-----------------------------------------------------------------------------
// titles of each parameter
void DfxPlugin::getParameterName(VstInt32 index, char* name)
{
	if (name)
	{
		// kVstMaxParamStrLen is absurdly short for parameter names at 8-characters, 
		// and given that exceding that length is common, hosts prepare for that, 
		// and even the Vst2Wrapper in the VST3 SDK acknowleges as much by defining 
		// the following "extended" constant to use instead
		constexpr size_t kVstExtMaxParamStrLen = 32;
		vst_strncpy(name, getparametername(index).c_str(), kVstExtMaxParamStrLen);
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

	if (getparameterusevaluestrings(index))
	{
		vst_strncpy(text, getparametervaluestring(index, getparameter_i(index)).value_or("").c_str(), kVstMaxParamStrLen);
		return;
	}

	switch (getparametervaluetype(index))
	{
		case DfxParam::ValueType::Float:
			snprintf(text, kVstMaxParamStrLen + 1, "%.3f", getparameter_f(index));
			break;
		case DfxParam::ValueType::Int:
			snprintf(text, kVstMaxParamStrLen + 1, "%" PRIi64, getparameter_i(index));
			break;
		case DfxParam::ValueType::Boolean:
			vst_strncpy(text, getparameter_b(index) ? "on" : "off", kVstMaxParamStrLen);
			break;
		default:
			assert(false);
			break;
	}
}

//-----------------------------------------------------------------------------
// unit of measure for each parameter
void DfxPlugin::getParameterLabel(VstInt32 index, char* label)
{
	if (label)
	{
		vst_strncpy(label, getparameterunitstring(index).c_str(), kVstMaxParamStrLen);
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

	// note that kVstMax* are specified as "number of characters excluding the null terminator", 
	// and vst_strncpy behaves accordingly and writes null at array[N], however the static arrays 
	// in struct types (like VstParameterProperties here) size the arrays by N, meaning that 
	// array[N] is out of range, so we call vst_strncpy in these cases passing N-1 as max length
	vst_strncpy(properties->label, getparametername(index).c_str(), kVstMaxLabelLen - 1);

	constexpr size_t recommendedShortLabelLength = 6;  // per the VST header comment
	static_assert(recommendedShortLabelLength < kVstMaxShortLabelLen);
	auto const shortName = getparametername(index, recommendedShortLabelLength);
	vst_strncpy(properties->shortLabel, shortName.c_str(), kVstMaxShortLabelLen - 1);

	auto const isVisible = [this](long parameterID)
	{
		return !hasparameterattribute(parameterID, DfxParam::kAttribute_Unused) && !hasparameterattribute(parameterID, DfxParam::kAttribute_Hidden);
	};
	if (isVisible(index))
	{
		properties->displayIndex = [index, isVisible]()
		{
			VstInt16 result = 0;
			for (VstInt16 parameterID = 0; parameterID <= index; parameterID++)
			{
				if (isVisible(parameterID))
				{
					assert(result < std::numeric_limits<decltype(result)>::max());
					result++;
				}
			}
			return result;
		}();
		properties->flags |= kVstParameterSupportsDisplayIndex;
	}

	switch (getparametervaluetype(index))
	{
		case DfxParam::ValueType::Boolean:
			properties->flags |= kVstParameterIsSwitch;
			break;
		case DfxParam::ValueType::Int:
			properties->minInteger = getparametermin_i(index);
			properties->maxInteger = getparametermax_i(index);
			properties->flags |= kVstParameterUsesIntegerMinMax;
			break;
		default:
			break;
	}

	if (auto const groupIndex = getparametergroup(index))
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
	preprocessaudio();

	for (unsigned long ch = 0; ch < getnuminputs(); ch++)
	{
		if (mInPlaceAudioProcessingAllowed)
		{
			mInputAudioStreams[ch] = inputs[ch];
		}
		else
		{
			std::copy_n(inputs[ch], dfx::math::ToIndex(sampleFrames), mInputOutOfPlaceAudioBuffers[ch].data());
		}
	}
	if (!mInPlaceAudioProcessingAllowed)
	{
		for (unsigned long ch = 0; ch < getnumoutputs(); ch++)
		{
			std::fill_n(outputs[ch], dfx::math::ToIndex(sampleFrames), 0.0f);
		}
	}

#if TARGET_PLUGIN_USES_DSPCORE
	for (unsigned long ch = 0; ch < getnumoutputs(); ch++)
	{
		if (mDSPCores[ch])
		{
			auto inputAudio = mInputAudioStreams[ch];
			if (asymmetricalchannels())
			{
				if (ch == 0)
				{
					std::copy_n(mInputAudioStreams[ch], sampleFrames, mAsymmetricalInputAudioBuffer.data());
				}
				inputAudio = mAsymmetricalInputAudioBuffer.data();
			}
			mDSPCores[ch]->process(inputAudio, outputs[ch], static_cast<unsigned long>(sampleFrames));
		}
	}
#else
	processaudio(mInputAudioStreams.data(), outputs, dfx::math::ToIndex<unsigned long>(sampleFrames));
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
	long newProgramNum {}, newProgramOffset = -1;

	for (VstInt32 i = 0; i < events->numEvents; i++)
	{
		// check to see if this event is MIDI; if no, then we try the for-loop again
		if (events->events[i]->type != kVstMidiType)
		{
			continue;
		}

		// cast the incoming event as a VstMidiEvent
		auto const midiEvent = reinterpret_cast<VstMidiEvent*>(events->events[i]);
		// address the midiData[4] string from the event to this temp data pointer
		auto const midiData = midiEvent->midiData;

		// save the channel number ...
		int const channel = midiData[0] & 0x0F;
		// ... and then wipe out the channel (lower 4 bits) for simplicity
		int const status = midiData[0] & 0xF0;
		int const byte1 = midiData[1] & 0x7F;
		int const byte2 = midiData[2] & 0x7F;
		auto const offsetFrames = dfx::math::ToIndex<unsigned long>(midiEvent->deltaFrames);  // timing offset

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
			if (midiEvent->deltaFrames >= newProgramOffset)
			{
				newProgramNum = byte1;
				newProgramOffset = midiEvent->deltaFrames;
			}
//			loadpreset(byte1);
		}
	}

	// change the plugin's program if a program change message was received
	if (newProgramOffset >= 0)
	{
		loadpreset(newProgramNum);
	}


	// tells the host to keep calling this function in the future; 0 means stop
	return 1;
}
#endif
// TARGET_PLUGIN_USES_MIDI
