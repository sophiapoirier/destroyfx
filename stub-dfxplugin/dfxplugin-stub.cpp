/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our unexciting, but informative, demonstration DfxPlugin.
written by Marc Poirier, October 2002
------------------------------------------------------------------------*/

// The gratiutous use of preprocessor defines is to designate code that 
// you would use depending on which features or APIs you are supporting.  
// You wouldn't use all of that stuff in a single plugin.  
// In this example, it's just in order to show you all of the options.

#include "dfxplugin-stub.hpp"


#pragma mark _________base_initializations_________

// these macros do boring entry point stuff for us
DFX_ENTRY(DfxStub);
#if TARGET_PLUGIN_USES_DSPCORE
	DFX_CORE_ENTRY(DxfStubDSP);
#endif

//-----------------------------------------------------------------------------
// initializations & such
// create and initialize everything that is needed for accessing/using 
// properties of the plugin here
// this is not the place to create or initialize stuff that is only needed 
// for audio processing (i.e. audio buffers, state variables)
DfxStub::DfxStub(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, NUM_PRESETS)
{
// first, null any audio buffer pointers that you have
	buffers = NULL;
	numbuffers = 0;
	buffersize = 0;


// next, initialize your parameters
// note that, when you initialize a parameter using the initparameter 
// routines, every preset will automatically be set to those initial settings 
// that way you know that they are all starting off from the default settings
	// parameter ID, parameter name, init value, default value, min value, max value, curve, units
	initparameter_f(kFloatParam, "decimal parameter", 90.0f, 33.3f, 1.0f, 999.0f, kDfxParamUnit_generic, kDfxParamCurve_linear);
	// parameter ID, parameter name, init value, default value, min value, max value, curve, units
	initparameter_i(kIntParam, "int parameter", 9, 12, 3, 27, kDfxParamUnit_index, kDfxParamCurve_stepped);
	// parameter ID, parameter name, init value, default value, number of values
	initparameter_indexed(kIndexParam, "indexed parameter", kIndexParamState3, kIndexParamState1, kNumIndexParamStates);
	// parameter ID, parameter name, init value, default value
	initparameter_b(kBooleanParam, "forced buffer tempo sync", false, false);

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

	#if TARGET_PLUGIN_USES_MIDI
		// if you don't use notes or pithbend for any specialized control 
		// of your plugin, your can allow those types of events to be 
		// assigned to control parameters via MIDI learn
		dfxsettings->setAllowPitchbendEvents(true);
		dfxsettings->setAllowNoteEvents(true);
	#endif


// now initialize the presets
	setpresetname(0, "default setting");	// default preset name, preset index 0
	// create the other built-in presets, if any
	// (optional, and not a virtual method, so call it whatever you want)
	initPresets();


// API-specific stuff below
	#ifdef TARGET_API_VST
		#if TARGET_PLUGIN_HAS_GUI
			editor = new DfxStubEditor(this);
		#endif
		#if TARGET_PLUGIN_USES_DSPCORE
			// we need to manage DSP cores manually in VST
			DFX_INIT_CORE(DfxStubDSP);
		#endif
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
DfxStubDSP::DfxStubDSP(TARGET_API_CORE_INSTANCE_TYPE *inInstance)
	: DfxPluginCore(inInstance)
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
	long number_of_inputs = getnuminputs();
	long number_of_outputs = getnumoutputs();
	double audio_sampling_rate = getsamplerate();
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
bool DfxStubDSP::createbuffers()
{
	long oldsize = buffersize;
	buffersize = (long) (getsamplerate() * BUFFER_SIZE_SECONDS);

	// if the sampling rate (& therefore the buffer size) has changed, 
	// then delete & reallocate the buffers according to the sampling rate
	bool success = createbuffer_f(&buffer, oldsize, buffersize);
	if (!success)
		return false;

	// we were successful if we reached this point
	return true;
}

#else
bool DfxStub::createbuffers()
{
	long oldsize = buffersize;
	buffersize = (long) (getsamplerate() * BUFFER_SIZE_SECONDS);
	unsigned long oldnum = numbuffers;
	numbuffers = getnumoutputs();

	// if the sampling rate (& therefore the buffer size) has changed, 
	// or if the number of channels to process has changed, 
	// then delete & reallocate the buffers according to the sampling rate
	bool success = createbufferarray_f(&buffers, oldnum, oldsize, numbuffers, buffersize);
	if (!success)
		return false;

	// we were successful if we reached this point
	return true;
}

//-------------------------------------------------------------------------
// deallocate your audio buffers here
// this will be called when necessary, handled by DfxPlugin
#if TARGET_PLUGIN_USES_DSPCORE
void DfxStubDSP::releasebuffers()
{
	releasebuffer_f(&buffer);
}

#else
void DfxStub::releasebuffers()
{
	releasebufferarray_f(&buffers, numbuffers);
	numbuffers = 0;
}
#endif

//-------------------------------------------------------------------------
// initialize the values in your audio buffers here
// this will be called when necessary, handled by DfxPlugin
#if TARGET_PLUGIN_USES_DSPCORE
void DfxStubDSP::clearbuffers()
{
	clearbuffer_f(buffer, buffersize);
}

#else
void DfxStub::clearbuffers()
{
	clearbufferarray_f(buffers, numbuffers, buffersize);
}
#endif



#pragma mark _________presets_________

//-------------------------------------------------------------------------
// you can assume that all values for every parameter in every preset 
// will be initialized to the default parameter values, as long as 
// you have initialized the parameters with the initparameter routines
void DfxStub::initPresets()
{
	int i = 1;

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
void DfxStubDSP::process(const float *in, float *out, unsigned long numSampleFrames, bool replacing) {
{
	// you might want to do this sort of pointer safety check 
	// if your plugin has audio buffers
	if (buffer == NULL)
	{
		// exit the loop if creation succeeded
		if ( createbuffers() )
			break;
		// or abort audio processing if the creation failed
		else return;
	}

	// do your audio processing here
}

#else
//-----------------------------------------------------------------------------
// passes an array of input streams, an array of output streams, 
// the number of sample frames in each stream, and whether to accumulate 
// output into the output stream (replacing=false) or replace output (replacing=true)
// (replacing is basically for VST)
void DfxStub::processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing)
{
	long numchannels = getnumoutputs();

	// you might want to do these sorts of pointer safety checks 
	// if your plugin has audio buffers
	if (numbuffers < numchannels)
		// there must have not been available memory or something (like WaveLab goofing up), 
		// so try to allocate buffers now
		createbuffers();
	for (long ch=0; ch < numchannels; ch++)
	{
		if (buffers[ch] == NULL)
		{
			// exit the loop if creation succeeded
			if ( createbuffers() )
				break;
			// or abort audio processing if the creation failed
			else return;
		}
	}

	// do your audio processing here

	// you can access all kinds of host time/tempo/location info 
	// from the timeinfo struct, which is automatically set before processing

	#if TARGET_PLUGIN_USES_MIDI
		// you can access MIDI events for this processing buffer from the 
		// midistuff->blockEvents array, using midistuff->numBlockEvents 
		// to see how many events there are for this processing block
	#endif
}
#endif
