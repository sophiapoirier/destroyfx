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

This is a template for making a DfxPlugin.
------------------------------------------------------------------------*/

// The gratiutous use of preprocessor defines is to designate code that 
// you would use depending on which features or APIs you are supporting.  
// You wouldn't use all of that stuff in a single plugin.  
// In this example, it's just in order to show you all of the options.

#include "dfxplugin-stub.h"

#include <algorithm>
#include <cmath>


#pragma mark _________base_initializations_________

// these macros do boring entry point stuff for us
DFX_EFFECT_ENTRY(DfxStub);
#if TARGET_PLUGIN_USES_DSPCORE
	DFX_CORE_ENTRY(DxfStubDSP);
#endif

//-----------------------------------------------------------------------------
// initializations and such
// create and initialize everything that is needed for accessing/using 
// properties of the plugin here
// this is not the place to create or initialize stuff that is only needed 
// for audio processing (i.e. audio buffers, state variables)
DfxStub::DfxStub(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, kNumPresets)
{
// next, initialize your parameters
// note that, when you initialize a parameter using the initparameter 
// routines, every preset will automatically be set to those initial settings 
// that way you know that they are all starting off from the default settings
	// parameter ID, parameter name, init value, default value, min value, max value, curve, units
	initparameter_f(kFloatParam, {"decimal parameter"}, 90.0f, 33.3f, 1.0f, 999.0f, DfxParam::Unit::Generic, DfxParam::Curve::Linear);
	// parameter ID, parameter name, init value, default value, min value, max value, curve, units
	initparameter_i(kIntParam, {"int parameter"}, 9, 12, 3, 27, DfxParam::Unit::Generic);
	// parameter ID, parameter name, init value, default value, number of values
	initparameter_list(kIndexParam, {"list of items parameter"}, kIndexParamState3, kIndexParamState1, kNumIndexParamStates);
	// parameter ID, parameter name, init value, default value
	initparameter_b(kBooleanParam, {"forced buffer tempo sync"}, false, false);

// next you can set up any custom value displays for indexed parameters
	// set the value display strings for each state of the indexed parameter
	setparametervaluestring(kIndexParam, kIndexParamState1, "ultra");
	setparametervaluestring(kIndexParam, kIndexParamState2, "mega");
	setparametervaluestring(kIndexParam, kIndexParamState3, "turbo");


// now do whatever other initial setup that you need to do
	// set the tail time (time after audio input stops until output dies out) in seconds
	// if there is no tail, you don't need to call this
	settailsize_seconds(BUFFER_SIZE);
	// set the latency time (time from when audio input starts until output begins) in seconds
	// if there is no latency, you don't need to call this
	setlatency_seconds(BUFFER_SIZE);
	// or in samples, if that makes more sense for the particular plugin
//	settailsize_seconds(NUM_SAMPLES_TAIL);
//	setlatency_samples(NUM_SAMPLES_LATENCY);

	// also, if you want to specify the channel configurations that 
	// your plugin supports, then do it here
	// if you don't specify any, then it is assumed that the plugin 
	// can handle any number of channels, as long as there are 
	// the same number of inputs as there are outputs
//	addchannelconfig(number_of_inputs, number_of_outputs);


// now initialize the presets
	setpresetname(0, "default setting");  // default preset name, preset index 0
	// create the other built-in presets, if any
	// (optional, and not a virtual method, so call it whatever you want)
	initPresets();


// API-specific stuff below
#ifdef TARGET_API_VST
	#if TARGET_PLUGIN_HAS_GUI
	editor = new DfxStubEditor(this);
	#endif
#endif

}

void DfxStub::dfx_PostConstructor()
{
#if TARGET_PLUGIN_USES_MIDI
	// if you don't use notes or pithbend for any specialized control 
	// of your plugin, your can allow those types of events to be 
	// assigned to control parameters via MIDI learn
	// NOTE: configuration of the settings object must occur in 
	// this method override, the constructor is too early
	getsettings().setAllowPitchbendEvents(true);
	getsettings().setAllowNoteEvents(true);
#endif
}

//-------------------------------------------------------------------------
// do final cleanup here
DfxStub::~DfxStub()
{
}

//-------------------------------------------------------------------------
// create stuff that is only necessary for audio processing here
// (the triggering of creation and destruction of audio buffers 
// is handled for you, though)
// no need to initialize audio stuff, though; that is done in reset
// return 0 for success, otherwise return an error code
long DfxStub::initialize()
{
	return kDfxErr_NoError;
}

//-------------------------------------------------------------------------
// clean up stuff that is only necessary for audio processing here
// (the triggering of creation and destruction of audio buffers 
// is handled for you, though)
void DfxStub::cleanup()
{
}



#pragma mark _________DSP_initializations_________

#if TARGET_PLUGIN_USES_DSPCORE

//-------------------------------------------------------------------------
DfxStubDSP::DfxStubDSP(DfxPlugin* inInstance)
:	DfxPluginCore(inInstance)
{
}

//-------------------------------------------------------------------------
DfxStubDSP::~DfxStubDSP()
{
	// you must call here because ~DfxPluginCore can't do this for us
	releasebuffers();
}

#endif


//-------------------------------------------------------------------------
// this is where you can reset state variables and in general 
// set things up as though audio processing is just starting up
// (e.g. reset position trackers, clear IIR filter histories, etc.)
// you can assume that this will be called before any 
// audio processing ever happens and that things like 
// the current sampling rate and number of channels will be valid here
// (the triggering of creation and clearing of audio buffers is 
// handled for you, though)
#if TARGET_PLUGIN_USES_DSPCORE
void DfxStubDSP::reset()
{
	// reset this buffer position tracker to zero
	bufferpos = 0;

	// you can confidently access and use any of these values here
	double audio_sampling_rate = getsamplerate();
}

#else
void DfxStub::reset()
{
	// reset this buffer position tracker to zero
	bufferpos = 0;

	// you can confidently access and use any of these values here
	auto const number_of_inputs = getnuminputs();
	auto const number_of_outputs = getnumoutputs();
	auto const audio_sampling_rate = getsamplerate();
}
#endif

//-------------------------------------------------------------------------
// allocate your audio buffers here
// this will be called when necessary, handled by DfxPlugin
// return true on success, false on failure
// note that createbuffers may be called repeatedly (with no 
// releasebuffers called in between)
// createbuffers is called whenever the audio stream format changes 
// (sampling rate or number of channels), so it is expected that 
// any implementation will check to see whether or not the 
// buffers are already allocated and whether or not they need to 
// be destroyed and reallocated in a different size
#if TARGET_PLUGIN_USES_DSPCORE
void DfxStubDSP::createbuffers()
{
	auto const bufferSize = std::lround(getsamplerate() * kBufferSize_Seconds);

	// if the sampling rate (and therefore the buffer size) has changed, 
	// then delete and reallocate the buffers according to the sampling rate
	buffer.assign(bufferSize, 0.0f);
}

#else
void DfxStub::createbuffers()
{
	auto const bufferSize = std::lround(getsamplerate() * kBufferSize_Seconds);
	auto const numChannels = getnumoutputs();

	// if the sampling rate (and therefore the buffer size) has changed, 
	// or if the number of channels to process has changed, 
	// then delete and reallocate the buffers according to the sampling rate
	buffers.assign(numChannels);
	for (auto& buffer : buffers)
	{
		buffer.assign(bufferSize, 0.0f);
	}
}

//-------------------------------------------------------------------------
// deallocate your audio buffers here
// this will be called when necessary, handled by DfxPlugin
#if TARGET_PLUGIN_USES_DSPCORE
void DfxStubDSP::releasebuffers()
{
	buffer = {};
}

#else
void DfxStub::releasebuffers()
{
	buffers = {};
}
#endif

//-------------------------------------------------------------------------
// initialize the values in your audio buffers here
// this will be called when necessary, handled by DfxPlugin
#if TARGET_PLUGIN_USES_DSPCORE
void DfxStubDSP::clearbuffers()
{
	std::fill(buffer.begin(), buffer.end(), 0.0f);
}

#else
void DfxStub::clearbuffers()
{
	for (auto& buffer : buffers)
	{
		std::fill(buffer.begin(), buffer.end(), 0.0f);
	}
}
#endif



#pragma mark _________presets_________

//-------------------------------------------------------------------------
// you can assume that all values for every parameter in every preset 
// will be initialized to the default parameter values, as long as 
// you have initialized the parameters with the initparameter routines
void DfxStub::initPresets()
{
	long i = 1;

	setpresetname(i, "fancy preset 1");
	setpresetparameter_f(i, kFloatParam, 3.0f);
	setpresetparameter_i(i, kIntParam, 6);
	setpresetparameter_i(i, kIndexParam, 9);
	setpresetparameter_b(i, kBooleanParam, true);
	i++;

	setpresetname(i, "fancy preset 2");
	setpresetparameter_f(i, kFloatParam, 9.0f);
	setpresetparameter_i(i, kIntParam, 6);
	setpresetparameter_i(i, kIndexParam, 3);
	setpresetparameter_b(i, kBooleanParam, false);
	i++;

	// add more...
}



#pragma mark _________audio_processing_________

//-------------------------------------------------------------------------
// here you can fetch your current parameter values before doing audio processing
// you should use this opportunity to stick the current values into 
// private parameter value holders and react to any parameter changes 
// that need to be reacted to
// (this is automatically called before each processing buffer by DfxPlugin)
#if TARGET_PLUGIN_USES_DSPCORE
void DfxStubDSP::processparameters()
#else
void DfxStub::processparameters()
#endif
{
	// fetch the current values
	floatParam = getparameter_b(kFloatParam);
	intParam = getparameter_i(kIntParam);
	indexParam = getparameter_i(kIndexParam);
	booleanParam = getparameter_b(kBooleanParam);

	// this is how you can react to changes made to a parameter 
	// since the last processing buffer
	if (getparameterchanged(kIntParam))
	{
		// do what you gotta do
	}
}

#if TARGET_PLUGIN_USES_DSPCORE
//-----------------------------------------------------------------------------
// passes one stream of input audio, one stream of output audio, 
// the number of sample frames in each stream, and whether to accumulate 
// output into the output stream (replacing=false) or replace output (replacing=true)
// (replacing is basically for VST)
void DfxStubDSP::process(float const* inStream, float* outStream, unsigned long inNumFrames, bool replacing)
{
	// do your audio processing here
}

#else
//-----------------------------------------------------------------------------
// passes an array of input streams, an array of output streams, 
// the number of sample frames in each stream, and whether to accumulate 
// output into the output stream (replacing==false) or replace output (replacing==true)
// (replacing is basically for VST)
void DfxStub::processaudio(float const* const* in, float** out, unsigned long inNumFrames, bool replacing)
{
	// do your audio processing here

	// you can access all kinds of host time/tempo/location info 
	// from gettimeinfo(), which is automatically set before processing

#if TARGET_PLUGIN_USES_MIDI
	// you can access MIDI events for this processing buffer with 
	// getmidistate().getBlockEvent(), using getmidistate().getBlockEventCount() 
	// to see how many events there are for this processing block
#endif
}
#endif
