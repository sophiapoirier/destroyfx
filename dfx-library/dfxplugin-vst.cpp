/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
This is where we connect the VST API to our DfxPlugin system.
written by Marc Poirier, October 2002
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif

#include <stdio.h>



#pragma mark _________init_________

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
	setInitialDelay(getlatency_samples());
	updatesamplerate();

	latencychanged = false;

	#if TARGET_PLUGIN_USES_MIDI
		wantEvents();
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
}

//-------------------------------------------------------------------------
long DfxPlugin::fxIdle()
{
	// I'm moving calls to ioChanged into the idle thread 
	// because it seems like it freaks out Fruity Loops
	if (latencychanged)
		ioChanged();
	latencychanged = false;

	return 1;
}

//-------------------------------------------------------------------------
void DfxPlugin::setSampleRate(float newRate)
{
	TARGET_API_BASE_CLASS::setSampleRate(newRate);
	updatesamplerate();
}



#pragma mark _________info_________

//-----------------------------------------------------------------------------
// this tells the host to keep calling process() for the duration of one buffer 
// even if the audio input has ended
long DfxPlugin::getTailSize()
{
	return gettailsize_samples();
}


//------------------------------------------------------------------------
bool DfxPlugin::getInputProperties(long index, VstPinProperties* properties)
{
	if ( (index >= 0) && (index < numInputs) )
	{
		sprintf(properties->label, "%s input %ld", PLUGIN_NAME_STRING, index+1);
		sprintf(properties->shortLabel, "in %ld", index+1);
		properties->flags = kVstPinIsActive;
		if (numInputs == 2)
			properties->flags |= kVstPinIsStereo;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool DfxPlugin::getOutputProperties(long index, VstPinProperties *properties)
{
	if ( (index >= 0) && (index < numOutputs) )
	{
		sprintf (properties->label, "%s output %ld", PLUGIN_NAME_STRING, index+1);
		sprintf (properties->shortLabel, "out %ld", index+1);
		properties->flags = kVstPinIsActive;
		if (numOutputs == 2)
			properties->flags |= kVstPinIsStereo;
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Destroy FX infos

bool DfxPlugin::getEffectName(char *name) {
	getpluginname(name);	// name max 32 char
	return true; }

long DfxPlugin::getVendorVersion() {
	return PLUGIN_VERSION; }

bool DfxPlugin::getErrorText(char *text) {
	strcpy (text, "U FUCT UP");	// max 256 char
	return true; }

bool DfxPlugin::getVendorString(char *text) {
	strcpy (text, DESTROYFX_NAME_STRING);	// a string identifying the vendor (max 64 char)
	return true; }

bool DfxPlugin::getProductString(char *text) {
	// a string identifying the product name (max 64 char)
	strcpy (text, "Super Destroy FX bipolar VST plugin pack");
	return true; }

//-----------------------------------------------------------------------------
// this just tells the host what this plugin can do
long DfxPlugin::canDo(char* text)
{
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


#pragma mark _________programs_________

//-----------------------------------------------------------------------------
void DfxPlugin::setProgram(long programNum)
{
	loadpreset(programNum);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setProgramName(char *name)
{
	setpresetname(TARGET_API_BASE_CLASS::getProgram(), name);
}

//-----------------------------------------------------------------------------
void DfxPlugin::getProgramName(char *name)
{
	long vstpresetnum = TARGET_API_BASE_CLASS::getProgram();

	if (presetisvalid(vstpresetnum))
	{
		if ( presetnameisvalid(vstpresetnum) )
			getpresetname(vstpresetnum, name);
		else
			sprintf(name, "default %ld", vstpresetnum+1);
	}
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getProgramNameIndexed(long category, long index, char *name)
{
	if (presetisvalid(index))
	{
		if ( presetnameisvalid(index) )
			getpresetname(index, name);
		else
			sprintf(name, "default %ld", index+1);
		return true;
	}
	else return false;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::copyProgram(long destination)
{
	long vstpresetnum = TARGET_API_BASE_CLASS::getProgram();

	if ( presetisvalid(destination) && presetisvalid(vstpresetnum) )
	{
		if (destination != vstpresetnum)	// only copy it if it's a different program slot
		{
			for (long i=0; i < numParameters; i++)
				setpresetparameter(destination, i, getpresetparameter(vstpresetnum, i));
			setpresetname(destination, getpresetname_ptr(vstpresetnum));
		}
		return true;
	}
	return false;
}

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
// note:  don't ever return 0 or Logic crashes
long DfxPlugin::getChunk(void **data, bool isPreset)
{
	long outsize = (signed long) dfxsettings->save(data, isPreset);
	return (outsize < 1) ? 1 : outsize;
}

//-----------------------------------------------------------------------------
// notes from Charlie Steinberg:
// "host gives you data and its bytesize (as it got from your getChunk() way
// back then, when it saved it).
// <data> is only valid during this call.
// you may want to check the bytesize, and certainly should maintain a version."
long DfxPlugin::setChunk(void *data, long byteSize, bool isPreset)
{
	return dfxsettings->restore(data, (unsigned)byteSize, isPreset);
}

#endif
// TARGET_PLUGIN_USES_MIDI



#pragma mark _________parameters_________

//-----------------------------------------------------------------------------
void DfxPlugin::setParameter(long index, float value)
{
	setparameter_gen(index, value);
}

//-----------------------------------------------------------------------------
float DfxPlugin::getParameter(long index)
{
	return getparameter_gen(index);
}

//-----------------------------------------------------------------------------
// titles of each parameter
void DfxPlugin::getParameterName(long index, char *name)
{
	getparametername(index, name);
}

//-----------------------------------------------------------------------------
// numerical display of each parameter's gradiations
void DfxPlugin::getParameterDisplay(long index, char *text)
{
	if (getparameterusevaluestrings(index))
	{
		getparametervaluestring(index, getparameter_i(index), text);
		return;
	}

	switch (getparametervaluetype(index))
	{
		case kDfxParamValueType_float:
			sprintf(text, "%.3f", getparameter_f(index));
			break;
		case kDfxParamValueType_double:
			sprintf(text, "%.3lf", getparameter_d(index));
			break;
		case kDfxParamValueType_int:
			sprintf(text, "%ld", getparameter_i(index));
			break;
		case kDfxParamValueType_uint:
			sprintf(text, "%ld", getparameter_ui(index));
			break;
		case kDfxParamValueType_boolean:
			if (getparameter_b(index))
				strcpy(text, "on");
			else
				strcpy(text, "off");
			break;
		case kDfxParamValueType_char:
			sprintf(text, "%d", getparameter_c(index));
			break;
		case kDfxParamValueType_uchar:
			sprintf(text, "%d", getparameter_uc(index));
			break;
		case kDfxParamValueType_undefined:
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
// unit of measure for each parameter
void DfxPlugin::getParameterLabel(long index, char *label)
{
	getparameterunitstring(index, label);
}



#pragma mark _________dsp_________

//-----------------------------------------------------------------------------------------
void DfxPlugin::process(float **inputs, float **outputs, long sampleFrames)
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

//-----------------------------------------------------------------------------------------
void DfxPlugin::processReplacing(float **inputs, float **outputs, long sampleFrames)
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



#pragma mark _________MIDI_________

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------------------
long DfxPlugin::processEvents(VstEvents* events)
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
		VstMidiEvent *midiEvent = (VstMidiEvent*)events->events[i];
		// address the midiData[4] string from the event to this temp data pointer
		char *midiData = midiEvent->midiData;

		// save the channel number ...
		int channel = midiData[0] & 0x0F;
		// ... & then wipe out the channel (lower 4 bits) for simplicity
		int status = midiData[0] & 0xF0;
		int byte1 = midiData[1] & 0x7F;
		int byte2 = midiData[2] & 0x7F;
		int frameOffset = midiEvent->deltaFrames;	// timing offset

		// looking at notes   (0x9* is Note On status ~ 0x8* is Note Off status)
		if ( (status == kMidiNoteOn) || (status == kMidiNoteOff) )
		{
			// note-off received
			if ( (status == kMidiNoteOff) || ((midiData[2] & 0x7F) == 0) )
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
			if (midiData[1] == kMidiCC_AllNotesOff)
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




// boring Windows main stuff
#if WIN32
	void *hInstance;
	BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
	{
		hInstance = hInst;
		return 1;
	}
#endif

