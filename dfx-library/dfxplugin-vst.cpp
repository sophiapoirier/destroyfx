/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
This is where we connect the VST API to our DfxPlugin system.
written by Sophia Poirier, October 2002
------------------------------------------------------------------------*/

#include "dfxplugin.h"

#include <stdio.h>



#pragma mark -
#pragma mark init
#pragma mark -

//-----------------------------------------------------------------------------
// this is called right before our plugin instance is destroyed
// XXX test and make sure that this is working in various hosts!
void DfxPlugin::close()
{
	dfxplugin_predestructor();
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
	needIdle();
	updatesamplerate();

	#if TARGET_PLUGIN_USES_MIDI
		#if !VST_FORCE_DEPRECATED
		wantEvents();
		#endif
	#endif

	// VST doesn't have initialize and cleanup methods like Audio Unit does, 
	// so we need to call this here
	if (!isinitialized)
		do_initialize();
	else
		do_reset();	// else because do_initialize calls do_reset

	#if TARGET_PLUGIN_USES_DSPCORE
		for (unsigned long i=0; i < getnumoutputs(); i++)
		{
			if (dspcores[i] != NULL)
				dspcores[i]->do_reset();
		}
	#endif

	// do these after calling do_reset, 
	// because the value for latency could change there
	setInitialDelay(getlatency_samples());
	setlatencychanged(false);	// reset this state
}

#if !VST_FORCE_DEPRECATED
//-------------------------------------------------------------------------
VstInt32 DfxPlugin::fxIdle()
{
	// I'm moving calls to ioChanged into the idle thread 
	// because it seems like it freaks out Fruity Loops
	if (getlatencychanged())
		ioChanged();
	setlatencychanged(false);

	return 1;
}
#endif

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
VstInt32 DfxPlugin::getTailSize()
{
	return gettailsize_samples();
}


//------------------------------------------------------------------------
bool DfxPlugin::getInputProperties(VstInt32 index, VstPinProperties * properties)
{
	if ( (index >= 0) && ((unsigned long)index < getnuminputs()) && (properties != NULL) )
	{
		sprintf(properties->label, "%s input %d", PLUGIN_NAME_STRING, index+1);
		sprintf(properties->shortLabel, "in %d", index+1);
		properties->flags = kVstPinIsActive;
		if (getnuminputs() == 2)
			properties->flags |= kVstPinIsStereo;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool DfxPlugin::getOutputProperties(VstInt32 index, VstPinProperties * properties)
{
	if ( (index >= 0) && ((unsigned long)index < getnumoutputs()) && (properties != NULL) )
	{
		sprintf (properties->label, "%s output %d", PLUGIN_NAME_STRING, index+1);
		sprintf (properties->shortLabel, "out %d", index+1);
		properties->flags = kVstPinIsActive;
		if (getnumoutputs() == 2)
			properties->flags |= kVstPinIsStereo;
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Destroy FX infos

bool DfxPlugin::getEffectName(char * name)
{
	if (name == NULL)
		return false;
	// get the name into a temp string buffer that we know is large enough
	char tempname[DFX_PARAM_MAX_NAME_LENGTH];
	getpluginname(tempname);
	// then make sure to only copy as much as the name C string can hold
	strncpy(name, tempname, 32);	// name max 32 characters XXX kVstMaxEffectNameLen
	// in case the parameter name was 32 or longer, 
	// make sure that the name string is terminated
	name[32-1] = 0;
	return true;
}

VstInt32 DfxPlugin::getVendorVersion()
{
	return PLUGIN_VERSION;
}

bool DfxPlugin::getVendorString(char * text)
{
	if (text == NULL)
		return false;
	// a string identifying the vendor (max 64 characters)
	strcpy(text, DESTROYFX_NAME_STRING);	// XXX kVstMaxVendorStrLen
	return true;
}

bool DfxPlugin::getProductString(char * text)
{
	if (text == NULL)
		return false;
	// a string identifying the product name (max 64 characters)
	strcpy(text, "Super Destroy FX bipolar VST plugin pack");	// XXX kVstMaxProductStrLen
	return true;
}

//-----------------------------------------------------------------------------
// this just tells the host what this plugin can do
VstInt32 DfxPlugin::canDo(char * text)
{
	if (text == NULL)
		return -1;

	if (strcmp(text, "plugAsChannelInsert") == 0)
		return 1;
	if (strcmp(text, "plugAsSend") == 0)
		return 1;
	if (strcmp(text, "mixDryWet") == 0)
		return 1;
	if (strcmp(text, "fuctsoundz") == 0)
		return 1;
	if (strcmp(text, "receiveVstTimeInfo") == 0)
		return 1;
	// XXX this channels canDo stuff could be improved...
	if (getnumoutputs() > 1)
	{
		if (strcmp(text, "1in2out") == 0)
			return 1;
		if (getnuminputs() > 1)
		{
			if (strcmp(text, "2in2out") == 0)
				return 1;
		}
	}
	else
	{
		if (strcmp(text, "1in1out") == 0)
			return 1;
	}
	#if TARGET_PLUGIN_USES_MIDI
		if (strcmp(text, "receiveVstEvents") == 0)
			return 1;
		if (strcmp(text, "receiveVstMidiEvent") == 0)
			return 1;
	#endif

	return -1;	// explicitly can't do; 0 => don't know
}


#pragma mark -
#pragma mark programs
#pragma mark -

//-----------------------------------------------------------------------------
void DfxPlugin::setProgram(VstInt32 programNum)
{
	loadpreset(programNum);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setProgramName(char * name)
{
	if (name != NULL)
		setpresetname(TARGET_API_BASE_CLASS::getProgram(), name);
}

//-----------------------------------------------------------------------------
void DfxPlugin::getProgramName(char * name)
{
	if (name == NULL)
		return;

	long vstpresetnum = TARGET_API_BASE_CLASS::getProgram();

	if (presetisvalid(vstpresetnum))
	{
		if ( presetnameisvalid(vstpresetnum) )
			getpresetname(vstpresetnum, name);	// XXX kVstMaxProgNameLen
		else
			sprintf(name, "default %ld", vstpresetnum+1);
	}
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getProgramNameIndexed(VstInt32 category, VstInt32 index, char * name)
{
	if (name == NULL)
		return false;

	if (presetisvalid(index))
	{
		if ( presetnameisvalid(index) )
			getpresetname(index, name);	// XXX kVstMaxProgNameLen
		else
			sprintf(name, "default %d", index+1);
		return true;
	}
	else return false;
}

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
// note:  don't ever return 0 or Logic crashes
VstInt32 DfxPlugin::getChunk(void ** data, bool isPreset)
{
	VstInt32 outsize = (VstInt32) dfxsettings->save(data, isPreset);
	return (outsize < 1) ? 1 : outsize;
}

//-----------------------------------------------------------------------------
// notes from Charlie Steinberg:
// "host gives you data and its bytesize (as it got from your getChunk() way
// back then, when it saved it).
// <data> is only valid during this call.
// you may want to check the bytesize, and certainly should maintain a version."
VstInt32 DfxPlugin::setChunk(void * data, VstInt32 byteSize, bool isPreset)
{
	return dfxsettings->restore(data, (unsigned)byteSize, isPreset);
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
void DfxPlugin::getParameterName(VstInt32 index, char * name)
{
	if (name != NULL)
		getparametername(index, name);	// XXX kVstMaxParamStrLen
}

//-----------------------------------------------------------------------------
// numerical display of each parameter's gradiations
void DfxPlugin::getParameterDisplay(VstInt32 index, char * text)
{
	if (text == NULL)
		return;

	if (getparameterusevaluestrings(index))
	{
		getparametervaluestring(index, getparameter_i(index), text);	// XXX kVstMaxParamStrLen
		return;
	}

	switch (getparametervaluetype(index))
	{
		case kDfxParamValueType_float:
			sprintf(text, "%.3lf", getparameter_f(index));
			break;
		case kDfxParamValueType_int:
			sprintf(text, "%ld", getparameter_i(index));
			break;
		case kDfxParamValueType_boolean:
			if (getparameter_b(index))
				strcpy(text, "on");
			else
				strcpy(text, "off");
			break;
		case kDfxParamValueType_undefined:
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
// unit of measure for each parameter
void DfxPlugin::getParameterLabel(VstInt32 index, char * label)
{
	if (label != NULL)
		getparameterunitstring(index, label);	// XXX kVstMaxParamStrLen
}



#pragma mark -
#pragma mark dsp
#pragma mark -

//-----------------------------------------------------------------------------------------
void DfxPlugin::processReplacing(float ** inputs, float ** outputs, VstInt32 sampleFrames)
{
	preprocessaudio();

#if TARGET_PLUGIN_USES_DSPCORE
	for (unsigned long i=0; i < getnumoutputs(); i++)
	{
		if (dspcores[i] != NULL)
			dspcores[i]->do_process(inputs[i], outputs[i], (unsigned)sampleFrames, true);
	}
#else
	processaudio((const float**)inputs, outputs, (unsigned)sampleFrames, true);
#endif

	postprocessaudio();
}

#if !VST_FORCE_DEPRECATED
//-----------------------------------------------------------------------------------------
void DfxPlugin::process(float ** inputs, float ** outputs, VstInt32 sampleFrames)
{
	preprocessaudio();

#if TARGET_PLUGIN_USES_DSPCORE
	for (unsigned long i=0; i < getnumoutputs(); i++)
	{
		if (dspcores[i] != NULL)
			dspcores[i]->do_process(inputs[i], outputs[i], (unsigned)sampleFrames, false);
	}
#else
	processaudio((const float**)inputs, outputs, (unsigned)sampleFrames, false);
#endif

	postprocessaudio();
}
#endif



#pragma mark -
#pragma mark MIDI
#pragma mark -

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------------------
VstInt32 DfxPlugin::processEvents(VstEvents* events)
{
	long newProgramNum, newProgramOffset = -1;


// note:  This function depends on the base plugin class to zero numBlockEvents 
// at the end of each processing block & does not do that itself because it is both 
// prossible & allowable for processEvents() to be called more than once per block.

	for (long i = 0; i < events->numEvents; i++)
	{
		// check to see if this event is MIDI; if no, then we try the for-loop again
		if ( ((events->events[i])->type) != kVstMidiType )
			continue;

		// cast the incoming event as a VstMidiEvent
		VstMidiEvent * midiEvent = (VstMidiEvent*)events->events[i];
		// address the midiData[4] string from the event to this temp data pointer
		char * midiData = midiEvent->midiData;

		// save the channel number ...
		int channel = midiData[0] & 0x0F;
		// ... & then wipe out the channel (lower 4 bits) for simplicity
		int status = midiData[0] & 0xF0;
		int byte1 = midiData[1] & 0x7F;
		int byte2 = midiData[2] & 0x7F;
		long frameOffset = midiEvent->deltaFrames;	// timing offset

		// looking at notes   (0x9* is Note On status ~ 0x8* is Note Off status)
		if ( (status == kMidiNoteOn) || (status == kMidiNoteOff) )
		{
			// note-off received
			if ( (status == kMidiNoteOff) || (byte2 == 0) )
				handlemidi_noteoff(channel, byte1, byte2, frameOffset);
			// note-on received
			else
				handlemidi_noteon(channel, byte1, byte2, frameOffset);
		}

		// looking at pitchbend   (0xE* is pitchbend status)
		else if (status == kMidiPitchbend)
			handlemidi_pitchbend(channel, byte1, byte2, frameOffset);

		// continuous controller
		else if (status == kMidiCC)
		{
			// all notes off
			if (byte1 == kMidiCC_AllNotesOff)
				handlemidi_allnotesoff(channel, frameOffset);
			else
				handlemidi_cc(channel, byte1, byte2, frameOffset);
		}

		// program change
		else if (status == kMidiProgramChange)
		{
//			handlemidi_programchange(channel, byte1, frameOffset);
			// XXX maybe this is really what we want to do with these?
			if (frameOffset >= newProgramOffset)
			{
				newProgramNum = byte1;
				newProgramOffset = frameOffset;
			}
//			loadpreset(byte1);
		}

	}

	// change the plugin's program if a program change message was received
	if (newProgramOffset >= 0)
		loadpreset(newProgramNum);


	// tells the host to keep calling this function in the future; 0 means stop
	return 1;
}
#endif
// TARGET_PLUGIN_USES_MIDI




/// this is handled in vstplugmain.cpp in the VST 2.4 SDK and higher
#if !VST_2_4_EXTENSIONS
/* entry point for Windows. All we need to do is set the global
   instance pointers. We save two copies because vstgui stupidly wants
   it to be a "void *", where our own GUI stuff avoids downcasts by
   using the right type, HINSTANCE. */
#if WIN32

/* vstgui needs void *, not HINSTANCE */
void * hInstance;
HINSTANCE instance;

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved) {
  hInstance = instance = hInst;
  return 1;
}

#endif
#endif
