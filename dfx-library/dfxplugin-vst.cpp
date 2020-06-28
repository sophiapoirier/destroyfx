/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2019  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
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
#include <stdio.h>
#include <cinttypes>


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

#if TARGET_PLUGIN_USES_MIDI
	#if !VST_FORCE_DEPRECATED
	wantEvents();
	#endif
#endif

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

	// This function was removed in r1157.
	// setlatencychanged(false);  // reset this state
	// ... but it sure seems like we need to call ioChanged since we just
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
	// get the name into a temp string buffer that we know is large enough
	char tempName[dfx::kParameterNameMaxLength];
	getpluginname(tempName);
	// then make sure to only copy as much as the name C string can hold
	vst_strncpy(outText, tempName, kVstMaxEffectNameLen);
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

	// (shouldn't we be returning 0 by default? -tom7)
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
		setpresetname(TARGET_API_BASE_CLASS::getProgram(), inName);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::getProgramName(char* outText)
{
	if (!outText)
	{
		return;
	}

	const VstInt32 vstPresetIndex = TARGET_API_BASE_CLASS::getProgram();

	if (presetisvalid(vstPresetIndex))
	{
		if (presetnameisvalid(vstPresetIndex))
		{
			char tempName[dfx::kPresetNameMaxLength];
			getpresetname(vstPresetIndex, tempName);
			vst_strncpy(outText, tempName, kVstMaxProgNameLen);
		}
		else
		{
			snprintf(outText, kVstMaxProgNameLen, "default %" PRIi32, vstPresetIndex + 1);
		}
	}
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getProgramNameIndexed(VstInt32 inCategory, VstInt32 inIndex, char* outText)
{
	if (!outText)
	{
		return false;
	}

	if (presetisvalid(inIndex))
	{
		if (presetnameisvalid(inIndex))
		{
			char tempName[dfx::kPresetNameMaxLength];
			getpresetname(inIndex, tempName);
			vst_strncpy(outText, tempName, kVstMaxProgNameLen);
		}
		else
		{
			snprintf(outText, kVstMaxProgNameLen, "default %d", inIndex+1);
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
		getparametername(index, name);  // XXX kVstMaxParamStrLen
	}
}

//-----------------------------------------------------------------------------
// numerical display of each parameter's gradiations
void DfxPlugin::getParameterDisplay(VstInt32 index, char* text)
{
	if (!text)
	{
		return;
	}

	if (getparameterusevaluestrings(index))
	{
		char tempValueString[dfx::kParameterValueStringMaxLength];
		getparametervaluestring(index, getparameter_i(index), tempValueString);
		vst_strncpy(text, tempValueString, kVstMaxParamStrLen);
		return;
	}

	switch (getparametervaluetype(index))
	{
		case DfxParam::ValueType::Float:
			snprintf(text, kVstMaxParamStrLen, "%.3f", getparameter_f(index));
			break;
		case DfxParam::ValueType::Int:
			snprintf(text, kVstMaxParamStrLen, "%" PRIi64, getparameter_i(index));
			break;
		case DfxParam::ValueType::Boolean:
			vst_strncpy(text, getparameter_b(index) ? "on" : "off", kVstMaxParamStrLen);
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
// unit of measure for each parameter
void DfxPlugin::getParameterLabel(VstInt32 index, char* label)
{
	if (label)
	{
		char tempLabel[dfx::kParameterUnitStringMaxLength];
		getparameterunitstring(index, tempLabel);
		vst_strncpy(label, tempLabel, kVstMaxParamStrLen);
	}
}



#pragma mark -
#pragma mark dsp
#pragma mark -

//-----------------------------------------------------------------------------------------
void DfxPlugin::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
	preprocessaudio();

#if TARGET_PLUGIN_USES_DSPCORE
	for (unsigned long i = 0; i < getnumoutputs(); i++)
	{
		if (mDSPCores[i])
		{
			mDSPCores[i]->process(inputs[i], outputs[i], static_cast<unsigned long>(sampleFrames));
		}
	}
#else
	processaudio(const_cast<float const**>(inputs), outputs, static_cast<unsigned long>(sampleFrames));
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
		auto const offsetFrames = static_cast<unsigned long>(std::max(midiEvent->deltaFrames, VstInt32(0)));  // timing offset

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
