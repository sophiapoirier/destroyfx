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
------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
the following is provisional documentation (basically snippets of emails 
that I wrote to Tom):

the dspcore stuff:
This has to do with AU, basically.  In AU, the easiest way to make an 
effect is to use what are called "kernels" in AU-world, which are
classes that handle 1-in/1-out audio streams and have all of the state
variables necessary for DSP within them.  They fetch the parameter values
from the main plugin class, they get created and destroyed as needed, they
get an audio stream to process, and they just do their thing.  The
advantage is that you get a clear encapsulation of the DSP stuff and you
can handle an arbitrary number of channels with no extra work (this is
possible in AU, but not VST).  The only time when you wouldn't do this is
if there are channel dependencies (the effect needs to see more than one
channel at a time) or when you need (or just support) mismatched in/out
configs.  So that's why I made the DfxPluginCore class, that's why that
stuff is in there, because it's nice.  It's mostly handled for you in AU,
but I kind of hacked it into our VST implementation, too, so it should
work fine there.  Transverb now uses this stuff.

There are tons and tons of methods in the DfxPlugin and DfxParameter 
classes, but you actually don't have to ever touch hardly any of them.  
They are mostly all used just within the class (you'll notice that there 
are barely any virtual methods).

Regarding having a "whole lot of crap" when starting a new plugin, 
my hopes with this DfxPlugin stuff is that that is not the case anymore.  
For one thing, there are very few methods that you need to
implement (I think only 9 at most, and, depending on what you're doing, it
could be as few as 4), and within those methods, there's very little
default stuff that you need (as much as possible is handled in the
inherited classes).  If I had written a true stub, it probably would have
been about 15 lines of code.  But if you think it's still too much and you
can thin of any ways to make it even more compact, that's cool, do tell.
I think it's pretty good in that respect now, though.  Especially I hope
you'll like the DSPcore thing, it might look weird, but it can make
writing an effect pretty easy, since arbitrary numbers of channels are
handled for you.

Let me give you a little summary of the Theory Of Operation(tm):

In the constructor and destructor, you take care of whatever is needed for
specifying any properties, anything that other stuff depends on, etc.  You
can save creating/destroying stuff that is only needed for audio
processing for the initialize and cleanup methods.  Although in many
cases, you won't even need to implement those, it just depends on the
plugin in question.  One exception is audio buffers.  There are
additionally handy functions called createbuffers, clearbuffers, and
releasebuffers.  The base DfxPlugin class knows when to call these, so you
don't need to (not from initialize or cleanup or anything).  Note that
createbuffers is also recreate buffers.  In other words, it is called when
the sampling rate changes or when the number of channels to process
changes.  So your plugin's implementation of createbuffers should be aware
of what you currently have allocated and then check for sr or numchannels
changes, if the buffers have dependencies there, and then call
releasebuffers and then reallocate if necessary.  dfxplugin-stub has an
example of this.

When using DSPcore approach, the constructor and destructor of you DSP
class are essentially what initialize and cleanup are for a plugin that
doesn't use DSPCores.  Because of this, you do need to explicitly call
do_cleanup in the DSP class' destructor.

Immediately before processaudio (or just process for DSPcore) is called,
processparameters is called.  You should override it and, in there, fetch
the current parameter values into more usable variables and react to any
parameter changes that need special reacting to.  The reason for taking
this approach is to kind of at least minimize thread safety issues, and
also to avoid having to insert reaction code directly into the
setparameter call, and therefore reduce inefficient redundant reactions
(more than 1 per processing buffer).  And I had other reasons, too, that I
can't remember...

Also immediately before audio processing, MIDI events are collected and
sorted for you and useful tempo/time/location info is stuffed into a
DfxPlugin::TimeInfo struct mTimeInfo.

So the core routines are: constructor, destructor, processparameters, and
processaudio (or process for DSP core).  You might need initialize and
cleanup, and maybe the buffer routines if you use buffers, but that's it.
------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
 when impelementing plugins derived from this stuff, you must define the following:
PLUGIN_NAME_STRING
	a C string of the name of the plugin
PLUGIN_ID
	4-byte ID for the plugin
PLUGIN_VERSION
	4-byte version of the plugin
TARGET_PLUGIN_USES_MIDI
TARGET_PLUGIN_IS_INSTRUMENT
TARGET_PLUGIN_USES_DSPCORE
TARGET_PLUGIN_HAS_GUI
	1 or 0

 you must define one of these, and no more than one:
TARGET_API_AUDIOUNIT
TARGET_API_VST

 necessary for Audio Unit:
PLUGIN_ENTRY_POINT
	a C string of the base plugin class name with "Entry" appended

 necessary for VST:
VST_NUM_INPUTS
VST_NUM_OUTPUTS
or if they match, simply define
VST_NUM_CHANNELS
	integers representing how many inputs and outputs your plugin has

 optional for Audio Unit:
PLUGIN_DESCRIPTION_STRING
	a C string description of the plugin
PLUGIN_RES_ID
	component resource ID of the base plugin
PLUGIN_EDITOR_DESCRIPTION_STRING
	a C string description of the plugin editor
PLUGIN_EDITOR_ID
	4-byte ID for the plugin (will PLUGIN_ID if not defined)
PLUGIN_EDITOR_RES_ID
	component resource ID of the base plugin
------------------------------------------------------------------------*/

#pragma once


#include <atomic>
#include <memory>
#include <optional>
#include <variant>
#include <vector>


#ifdef TARGET_API_RTAS
	#include "CEffectGroup.h"
	#ifdef TARGET_API_AUDIOSUITE
		#include "CEffectProcessAS.h"
		using TARGET_API_BASE_CLASS = CEffectProcessAS;
	#else
		#include "CEffectProcessRTAS.h"
		using TARGET_API_BASE_CLASS = CEffectProcessRTAS;
	#endif
	using TARGET_API_BASE_INSTANCE_TYPE = void*;
	#ifdef TARGET_PLUGIN_USES_VSTGUI
		#include "CProcessType.h"
		#include "CTemplateNoUIView.h"
	#endif
	#if WINDOWS_VERSION
		#include "Mac2Win.h"
	#endif
#endif


// include our crucial shits

#include "dfxdefines.h"
#include "dfxparameter.h"
#include "dfxplugin-base.h"
#include "dfxpluginproperties.h"
#include "idfxsmoothedvalue.h"

#if TARGET_PLUGIN_USES_MIDI
	#include "dfxmidi.h"
	#include "dfxsettings.h"
#endif




#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"


class DfxPluginCore;


#pragma mark _________DfxPlugin_________
//-----------------------------------------------------------------------------
class DfxPlugin : public TARGET_API_BASE_CLASS
#if defined(TARGET_API_RTAS) && defined(TARGET_PLUGIN_USES_VSTGUI)
, public ITemplateProcess
#endif
{
public:
	struct TimeInfo
	{
		double mTempo = 0.0, mTempo_BPS = 0.0;
		long mSamplesPerBeat = 0;
		bool mTempoIsValid = false;

		double mBeatPos = 0.0;
		bool mBeatPosIsValid = false;

		double mBarPos = 0.0;
		bool mBarPosIsValid = false;

		double mDenominator = 0.0, mNumerator = 0.0;
		bool mTimeSignatureIsValid = false;

		long mSamplesToNextBar = 0;
		bool mSamplesToNextBarIsValid = false;

		bool mPlaybackChanged = false;  // whether or not the playback state or position just changed
		bool mPlaybackIsOccurring = false;
	};

	// ***
	DfxPlugin(TARGET_API_BASE_INSTANCE_TYPE inInstance, long inNumParameters, long inNumPresets = 1);
	// ***
	virtual ~DfxPlugin() = default;

	void do_PostConstructor();
	// ***
	virtual void dfx_PostConstructor() {}
	void do_PreDestructor();
	// ***
	virtual void dfx_PreDestructor() {}

	long do_initialize();
	// ***
	virtual long initialize()
	{
		return dfx::kStatus_NoError;
	}
	void do_cleanup();
	// ***
	virtual void cleanup() {}
	void do_reset();
	// ***
	virtual void reset() {}

	// ***
	// override this if the plugin uses audio buffers
	// this will be called at the correct times
	// it may be called repeatedly when the audio stream format changes 
	// (sampling rate or number of channels), so it is expected that 
	// the implementation will check to see whether or not the 
	// buffers are already allocated and whether or not they need to 
	// be destroyed and reallocated in a different size
	virtual void createbuffers() {}
	// ***
	// override this if the plugin uses audio buffers and 
	// ever requires the buffer contents to be zeroed 
	// (like when the DSP state is reset)
	// this will be called at the correct times
	virtual void clearbuffers() {}
	// ***
	// override this if the plugin uses audio buffers
	// this will be called at the correct times
	// release any allocated audio buffers
	virtual void releasebuffers() {}

	// insures that processparameters (and perhaps other related stuff) 
	// is called at the correct moments
	void do_processparameters();
	// ***
	// override this to handle/accept parameter values immediately before 
	// processing audio and to react to parameter changes
	virtual void processparameters() {}
	// attend to things immediately before processing a block of audio
	void preprocessaudio();
	// attend to things immediately after processing a block of audio
	void postprocessaudio();
	// ***
	// do the audio processing (override with real stuff)
	// pass in arrays of float buffers for input and output ([channel][sample]), 
	// 
	virtual void processaudio(float const* const* inStreams, float* const* outStreams, unsigned long inNumFrames, 
							  bool replacing = true) {}

	auto getnumparameters() const noexcept
	{
		return static_cast<long>(mParameters.size());
	}
	auto getnumpresets() const noexcept
	{
		return static_cast<long>(mPresets.size());
	}
	bool parameterisvalid(long inParameterIndex) const noexcept
	{
		return (inParameterIndex >= 0) && (inParameterIndex < getnumparameters());
	}

	void initparameter_f(long inParameterIndex, char const* initName, double initValue, double initDefaultValue, 
						 double initMin, double initMax, 
						 DfxParam::Unit initUnit = DfxParam::Unit::Generic, 
						 DfxParam::Curve initCurve = DfxParam::Curve::Linear, 
						 char const* initCustomUnitString = nullptr);
	void initparameter_i(long inParameterIndex, char const* initName, int64_t initValue, int64_t initDefaultValue, 
						 int64_t initMin, int64_t initMax, 
						 DfxParam::Unit initUnit = DfxParam::Unit::Generic, 
						 DfxParam::Curve initCurve = DfxParam::Curve::Stepped, 
						 char const* initCustomUnitString = nullptr);
	void initparameter_b(long inParameterIndex, char const* initName, bool initValue, bool initDefaultValue, 
						 DfxParam::Unit initUnit = DfxParam::Unit::Generic);
	void initparameter_list(long inParameterIndex, char const* initName, int64_t initValue, int64_t initDefaultValue, 
							int64_t initNumItems, DfxParam::Unit initUnit = DfxParam::Unit::List, 
							char const* initCustomUnitString = nullptr);

	void setparameterusevaluestrings(long inParameterIndex, bool inMode = true)
	{
		if (parameterisvalid(inParameterIndex))
		{
			mParameters[inParameterIndex].setusevaluestrings(inMode);
		}
	}
	bool getparameterusevaluestrings(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getusevaluestrings() : false;
	}
	bool setparametervaluestring(long inParameterIndex, int64_t inStringIndex, char const* inText);
	bool getparametervaluestring(long inParameterIndex, int64_t inStringIndex, char* outText) const;
	void getparameterunitstring(long inParameterIndex, char* outText) const
	{
		if (parameterisvalid(inParameterIndex))
		{
			mParameters[inParameterIndex].getunitstring(outText);
		}
	}
	void setparametercustomunitstring(long inParameterIndex, char const* inText)
	{
		if (parameterisvalid(inParameterIndex))
		{
			mParameters[inParameterIndex].setcustomunitstring(inText);
		}
	}
	char const* getparametervaluestring_ptr(long inParameterIndex, int64_t inStringIndex) const;
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef getparametervaluecfstring(long inParameterIndex, int64_t inStringIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getvaluecfstring(inStringIndex) : nullptr;
	}
	virtual CFStringRef CopyParameterGroupName(UInt32 inParameterGroupID) const
	{
		return nullptr;
	}
#endif

	void setparameter(long inParameterIndex, DfxParam::Value inValue);
	void setparameter_f(long inParameterIndex, double inValue);
	void setparameter_i(long inParameterIndex, int64_t inValue);
	void setparameter_b(long inParameterIndex, bool inValue);
	void setparameter_gen(long inParameterIndex, double inValue);
	// ***
	virtual void randomizeparameter(long inParameterIndex);
	// ***
	virtual void randomizeparameters(bool writeAutomation = false);  // randomize all parameters at once
	void update_parameter(long inParameterIndex);
	void postupdate_parameter(long inParameterIndex);

	DfxParam::Value getparameter(long inParameterIndex) const;
	double getparameter_f(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].get_f() : 0.0;
	}
	int64_t getparameter_i(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].get_i() : 0;
	}
	bool getparameter_b(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].get_b() : false;
	}
	double getparameter_gen(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].get_gen() : 0.0;
	}
	// return a (hopefully) 0 to 1 scalar version of the parameter's current value
	double getparameter_scalar(long inParameterIndex) const;
	std::optional<double> getparameterifchanged_f(long inParameterIndex) const;
	std::optional<int64_t> getparameterifchanged_i(long inParameterIndex) const;
	std::optional<bool> getparameterifchanged_b(long inParameterIndex) const;
	std::optional<double> getparameterifchanged_scalar(long inParameterIndex) const;

	double getparametermin_f(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getmin_f() : 0.0;
	}
	int64_t getparametermin_i(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getmin_i() : 0;
	}
	double getparametermax_f(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getmax_f() : 0.0;
	}
	int64_t getparametermax_i(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getmax_i() : 0;
	}
	double getparameterdefault_f(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getdefault_f() : 0.0;
	}
	int64_t getparameterdefault_i(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getdefault_i() : 0;
	}
	bool getparameterdefault_b(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getdefault_b() : false;
	}

	void getparametername(long inParameterIndex, char* outText) const;
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef getparametercfname(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getcfname() : nullptr;
	}
#endif
	DfxParam::ValueType getparametervaluetype(long inParameterIndex) const;
	DfxParam::Unit getparameterunit(long inParameterIndex) const;
	bool getparameterchanged(long inParameterIndex) const;
	void setparameterchanged(long inParameterIndex, bool inChanged = true);
	bool getparametertouched(long inParameterIndex) const;
	void setparametertouched(long inParameterIndex, bool inTouched = true);
	DfxParam::Curve getparametercurve(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getcurve() : DfxParam::Curve::Linear;
	}
	void setparametercurve(long inParameterIndex, DfxParam::Curve inCurve)
	{
		if (parameterisvalid(inParameterIndex))
		{
			mParameters[inParameterIndex].setcurve(inCurve);
		}
	}
	double getparametercurvespec(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getcurvespec() : 0.0;
	}
	void setparametercurvespec(long inParameterIndex, double inCurveSpec)
	{
		if (parameterisvalid(inParameterIndex))
		{
			mParameters[inParameterIndex].setcurvespec(inCurveSpec);
		}
	}
	bool getparameterenforcevaluelimits(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].GetEnforceValueLimits() : false;
	}
	void setparameterenforcevaluelimits(long inParameterIndex, bool inMode)
	{
		if (parameterisvalid(inParameterIndex))
		{
			mParameters[inParameterIndex].SetEnforceValueLimits(inMode);
		}
	}
	DfxParam::Attribute getparameterattributes(long inParameterIndex) const
	{
		return parameterisvalid(inParameterIndex) ? mParameters[inParameterIndex].getattributes() : 0;
	}
	bool hasparameterattribute(long inParameterIndex, DfxParam::Attribute inFlag) const;
	void setparameterattributes(long inParameterIndex, DfxParam::Attribute inFlags)
	{
		if (parameterisvalid(inParameterIndex))
		{
			mParameters[inParameterIndex].setattributes(inFlags);
		}
	}
	void addparameterattributes(long inParameterIndex, DfxParam::Attribute inFlags)
	{
		if (parameterisvalid(inParameterIndex))
		{
			mParameters[inParameterIndex].setattributes(inFlags | mParameters[inParameterIndex].getattributes());
		}
	}

	// convenience methods for expanding and contracting parameter values 
	// using the min/max/curvetype/curvespec/etc. settings of a given parameter
	double expandparametervalue(long inParameterIndex, double genValue) const;
	double contractparametervalue(long inParameterIndex, double realValue) const;

	// whether or not the index is a valid preset
	bool presetisvalid(long inPresetIndex) const;
	// whether or not the index is a valid preset with a valid name
	bool presetnameisvalid(long inPresetIndex) const;
	// load the settings of a preset
	virtual bool loadpreset(long inPresetIndex);
	// set a parameter value in all of the empty (no name) presets 
	// to the current value of that parameter
	void initpresetsparameter(long inParameterIndex);
	// set the text of a preset name
	void setpresetname(long inPresetIndex, char const* inText);
	// get a copy of the text of a preset name
	void getpresetname(long inPresetIndex, char* outText) const;
	// get a pointer to the text of a preset name
	char const* getpresetname_ptr(long inPresetIndex) const;
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef getpresetcfname(long inPresetIndex) const;
#endif
	auto getcurrentpresetnum() const noexcept
	{
		return mCurrentPresetNum;
	}
	void setpresetparameter(long inPresetIndex, long inParameterIndex, DfxParam::Value inValue);
	void setpresetparameter_f(long inPresetIndex, long inParameterIndex, double inValue);
	void setpresetparameter_i(long inPresetIndex, long inParameterIndex, int64_t inValue);
	void setpresetparameter_b(long inPresetIndex, long inParameterIndex, bool inValue);
	void setpresetparameter_gen(long inPresetIndex, long inParameterIndex, double inValue);
	void update_preset(long inPresetIndex);
	DfxParam::Value getpresetparameter(long inPresetIndex, long inParameterIndex) const;
	double getpresetparameter_f(long inPresetIndex, long inParameterIndex) const;


	virtual long dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
									 size_t& outDataSize, dfx::PropertyFlags& outFlags)
	{
		return dfx::kStatus_InvalidProperty;
	}
	virtual long dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
								 void* outData)
	{
		return dfx::kStatus_InvalidProperty;
	}
	virtual long dfx_SetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
								 void const* inData, size_t inDataSize)
	{
		return dfx::kStatus_InvalidProperty;
	}
	long dfx_GetNumPluginProperties()
	{
		return dfx::kPluginProperty_NumProperties + dfx_GetNumAdditionalPluginProperties();
	}
	virtual long dfx_GetNumAdditionalPluginProperties()
	{
		return 0;
	}


	// get the current audio sampling rate
	double getsamplerate() const noexcept
	{
		return DfxPlugin::mSampleRate;
	}
	// convenience wrapper of getsamplerate to get float type value
	float getsamplerate_f() const
	{
		return static_cast<float>(DfxPlugin::mSampleRate);
	}
	// change the current audio sampling rate
	void setsamplerate(double inSampleRate);
	// force a refetching of the sample rate from the host
	void updatesamplerate();

	// react to a change in the number of audio channels
	void updatenumchannels();
	// return the number of audio inputs
	unsigned long getnuminputs();
	// return the number of audio outputs
	unsigned long getnumoutputs();

	// get the TimeInfo struct with the latest time info values
	TimeInfo const& gettimeinfo() const noexcept
	{
		return mTimeInfo;
	}

	// add an audio input/output configuration to the array of i/o configurations
	void addchannelconfig(short inNumInputChannels, short inNumOutputChannels);

	void setlatency_samples(long inSamples, dfx::NotificationPolicy inNotificationPolicy = dfx::NotificationPolicy::Sync);
	void setlatency_seconds(double inSeconds, dfx::NotificationPolicy inNotificationPolicy = dfx::NotificationPolicy::Sync);
	long getlatency_samples() const;
	double getlatency_seconds() const;
	void postupdate_latency();

	void settailsize_samples(long inSamples, dfx::NotificationPolicy inNotificationPolicy = dfx::NotificationPolicy::Sync);
	void settailsize_seconds(double inSeconds, dfx::NotificationPolicy inNotificationPolicy = dfx::NotificationPolicy::Sync);
	long gettailsize_samples() const;
	double gettailsize_seconds() const;
	void postupdate_tailsize();

	void setAudioProcessingMustAccumulate(bool inMode);

	void getpluginname(char* outText) const;
	long getpluginversion() const;

	void registerSmoothedAudioValue(dfx::ISmoothedValue* smoothedValue, DfxPluginCore* owner = nullptr);
	void unregisterAllSmoothedAudioValues(DfxPluginCore* owner);
	void incrementSmoothedAudioValues(DfxPluginCore* owner = nullptr);

	void do_idle();
	virtual void idle() {}

#if TARGET_PLUGIN_USES_MIDI
	// handlers for the types of MIDI events that we support
	virtual void handlemidi_noteon(int inChannel, int inNote, int inVelocity, unsigned long inOffsetFrames);
	virtual void handlemidi_noteoff(int inChannel, int inNote, int inVelocity, unsigned long inOffsetFrames);
	virtual void handlemidi_allnotesoff(int inChannel, unsigned long inOffsetFrames);
	virtual void handlemidi_channelaftertouch(int inChannel, int inValue, unsigned long inOffsetFrames);
	virtual void handlemidi_pitchbend(int inChannel, int inValueLSB, int inValueMSB, unsigned long inOffsetFrames);
	virtual void handlemidi_cc(int inChannel, int inControllerNum, int inValue, unsigned long inOffsetFrames);
	virtual void handlemidi_programchange(int inChannel, int inProgramNum, unsigned long inOffsetFrames);

	/* - - - - - - - - - hooks for DfxSettings - - - - - - - - - */
	//
	// although this method can theoretically be called multiple times,
	// its return value must stay the same for the lifetime of a plugin instance
	virtual size_t settings_sizeOfExtendedData() const noexcept { return 0; }
	//
	// these allow for additional constructor or destructor stuff, if necessary
	virtual void settings_init() {}
	virtual void settings_cleanup() {}
	//
	// these can be overridden to store and restore more data during 
	// save() and restore() respectively
	// the data pointers point to the start of the extended data 
	// sections of the settings data
	virtual void settings_saveExtendedData(void* outData, bool isPreset) {}
	virtual void settings_restoreExtendedData(void const* inData, size_t storedExtendedDataSize, 
											  long dataVersion, bool isPreset) {}
	//
	// this can be overridden to react when parameter values are 
	// restored from settings that are loaded during restore()
	// for example, you might need to remap certain parameter values 
	// when loading settings stored by older versions of your plugin 
	// (that's why settings data version is one of the function arguments; 
	// it's the version number of the plugin that created the data)
	// (presetNum of -1 indicates that we're just working with the current 
	// state of the plugin)
	virtual void settings_doChunkRestoreSetParameterStuff(long tag, float value, long dataVersion, long presetNum = -1) {}
	//
	// these can be overridden to do something and extend the MIDI event processing
	virtual void settings_doLearningAssignStuff(long tag, dfx::MidiEventType eventType, long eventChannel, 
												long eventNum, unsigned long offsetFrames, long eventNum2 = 0, 
												dfx::MidiEventBehaviorFlags eventBehaviorFlags = dfx::kMidiEventBehaviorFlag_None, 
												long data1 = 0, long data2 = 0, 
												float fdata1 = 0.0f, float fdata2 = 0.0f) {}
	virtual void settings_doMidiAutomatedSetParameterStuff(long tag, float value, unsigned long offsetFrames) {}
	// HACK: the return type is overloaded for this purpose, contains more data than used in this context
	virtual std::optional<dfx::ParameterAssignment> settings_getLearningAssignData(long inParameterIndex) const
	{
		return {};
	}
#endif

	// handling of AU properties specific to Logic
#if defined(TARGET_API_AUDIOUNIT) && LOGIC_AU_PROPERTIES_AVAILABLE
	UInt32 getSupportedLogicNodeOperationMode() const noexcept
	{
		return mSupportedLogicNodeOperationMode;
	}
	void setSupportedLogicNodeOperationMode(UInt32 inMode) noexcept
	{
		mSupportedLogicNodeOperationMode = inMode;
	}
	UInt32 getCurrentLogicNodeOperationMode() const noexcept
	{
		return mCurrentLogicNodeOperationMode;
	}
	virtual void setCurrentLogicNodeOperationMode(UInt32 inMode) noexcept
	{
		mCurrentLogicNodeOperationMode = inMode;
	}
	bool isLogicNodeEndianReversed() const noexcept
	{
		return ((getCurrentLogicNodeOperationMode() & kLogicAUNodeOperationMode_NodeEnabled) 
				&& (getCurrentLogicNodeOperationMode() & kLogicAUNodeOperationMode_EndianSwap));
	}
#endif


protected:
#if TARGET_PLUGIN_USES_MIDI
	DfxMidi& getmidistate() noexcept
	{
		return mMidiState;
	}
	DfxMidi const& getmidistate() const noexcept
	{
		return mMidiState;
	}
	DfxSettings& getsettings()
	{
		return *mDfxSettings;
	}
	DfxSettings const& getsettings() const
	{
		return *mDfxSettings;
	}
#endif
	auto sampleRateChanged() const noexcept
	{
		return mSampleRateChanged;
	}
	auto hostCanDoTempo() const noexcept
	{
		return mHostCanDoTempo;
	}


private:
#ifdef TARGET_API_AUDIOUNIT
	// the Audio Unit API already has an i/o configurations structure
	using ChannelConfig = AUChannelInfo;
#else
	// immitate AUChannelInfo from the Audio Unit API for other APIs
	struct ChannelConfig
	{
		short inChannels = 0;
		short outChannels = 0;
	};
#endif

	std::vector<DfxParam> mParameters;
	std::vector<DfxPreset> mPresets;

	std::vector<ChannelConfig> mChannelconfigs;

	TimeInfo mTimeInfo;
	bool mHostCanDoTempo = false;

	double mSampleRate = 0.0;
	bool mSampleRateChanged = false;

	unsigned long mNumInputs = 0, mNumOutputs = 0;

#if TARGET_PLUGIN_USES_MIDI
	DfxMidi mMidiState;
	std::unique_ptr<DfxSettings> mDfxSettings;
#endif

	long mCurrentPresetNum = 0;

#if TARGET_PLUGIN_USES_DSPCORE && !defined(TARGET_API_AUDIOUNIT)
	std::vector<std::unique_ptr<DfxPluginCore>> mDSPCores;  // we have to handle this ourselves because VST can't
#endif

#ifdef TARGET_API_AUDIOUNIT
	bool mAUElementsHaveBeenCreated = false;
	// array of float pointers to input and output audio buffers, 
	// just for the sake of making processaudio(float**, float**, etc.) possible
	std::vector<float const*> mInputAudioStreams_au;
	std::vector<float*> mOutputAudioStreams_au;

	void UpdateInPlaceProcessingState();
	#if LOGIC_AU_PROPERTIES_AVAILABLE
	UInt32 mSupportedLogicNodeOperationMode = kLogicAUNodeOperationMode_FullSupport;
	UInt32 mCurrentLogicNodeOperationMode = 0;
	#endif
#endif

#ifdef TARGET_API_VST
	bool mIsInitialized = false;
#endif

	// try to get musical tempo/time/location information from the host
	void processtimeinfo();

	std::variant<long, double> mLatency {0l};
	std::atomic<bool> mLatencyChanged {false};
	std::variant<long, double> mTailSize {0l};
	std::atomic<bool> mTailSizeChanged {false};
	bool mAudioProcessingAccumulatingOnly = false;
	bool mAudioIsRendering = false;
	std::vector<std::pair<dfx::ISmoothedValue*, DfxPluginCore*>> mSmoothedAudioValues;
	bool mIsFirstRenderSinceReset = false;

#ifdef TARGET_API_RTAS
	void AddParametersToList();
	bool mMasterBypass_rtas = false;
	std::vector<float*> mInputAudioStreams_as;
	std::vector<float*> mOutputAudioStreams_as;
	std::vector<float> mZeroAudioBuffer;
#ifdef TARGET_PLUGIN_USES_VSTGUI
	GrafPtr mMainPort = nullptr;
	std::unique_ptr<ITemplateCustomUI> mCustomUI_p;
	Rect mPIWinRect;
	CTemplateNoUIView* mNoUIView_p = nullptr;
	short mLeftOffset = 0, mTopOffset = 0;
	void* mModuleHandle_p = nullptr;
#endif
#endif


// overridden virtual methods from inherited API base classes
public:

#ifdef TARGET_API_AUDIOUNIT
	void PostConstructor() override;
	void PreDestructor() override;
	OSStatus Initialize() override;
	void Cleanup() override;
	OSStatus Reset(AudioUnitScope inScope, AudioUnitElement inElement) override;

	#if TARGET_PLUGIN_IS_INSTRUMENT
	OSStatus Render(AudioUnitRenderActionFlags& ioActionFlags, 
					AudioTimeStamp const& inTimeStamp, UInt32 inFramesToProcess) override;
	#else
	OSStatus ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags, 
								AudioBufferList const& inBuffer, AudioBufferList& outBuffer, 
								UInt32 inFramesToProcess) override;
	#endif
	#if TARGET_PLUGIN_USES_DSPCORE
	AUKernelBase* NewKernel() override;
	DfxPluginCore* getplugincore(unsigned long channel) const;
	#endif

	OSStatus GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
							 AudioUnitScope inScope, AudioUnitElement inElement, 
							 UInt32& outDataSize, Boolean& outWritable) override;
	OSStatus GetProperty(AudioUnitPropertyID inPropertyID, 
						 AudioUnitScope inScope, AudioUnitElement inElement, 
						 void* outData) override;
	OSStatus SetProperty(AudioUnitPropertyID inPropertyID, 
						 AudioUnitScope inScope, AudioUnitElement inElement, 
						 void const* inData, UInt32 inDataSize) override;

	OSStatus Version() override;
	UInt32 SupportedNumChannels(AUChannelInfo const** outInfo) override;
	Float64 GetLatency() override;
	Float64 GetTailTime() override;
	bool SupportsTail() override
	{
		return true;
	}
	CFURLRef CopyIconLocation() override;

	OSStatus GetParameterInfo(AudioUnitScope inScope, 
							  AudioUnitParameterID inParameterID, 
							  AudioUnitParameterInfo& outParameterInfo) override;
	OSStatus GetParameterValueStrings(AudioUnitScope inScope, 
									  AudioUnitParameterID inParameterID, CFArrayRef* outStrings) override;
	OSStatus SetParameter(AudioUnitParameterID inParameterID, 
						  AudioUnitScope inScope, AudioUnitElement inElement, 
						  Float32 inValue, UInt32 inBufferOffsetInFrames) override;

	OSStatus ChangeStreamFormat(AudioUnitScope inScope, 
								AudioUnitElement inElement, 
								CAStreamBasicDescription const& inPrevFormat, 
								CAStreamBasicDescription const& inNewFormat) override;

	OSStatus SaveState(CFPropertyListRef* outData) override;
	OSStatus RestoreState(CFPropertyListRef inData) override;
	OSStatus GetPresets(CFArrayRef* outData) const override;
	OSStatus NewFactoryPresetSet(AUPreset const& inNewFactoryPreset) override;

	#if TARGET_PLUGIN_USES_MIDI
	OSStatus HandleNoteOn(UInt8 inChannel, UInt8 inNoteNumber, UInt8 inVelocity, UInt32 inStartFrame) override;
	OSStatus HandleNoteOff(UInt8 inChannel, UInt8 inNoteNumber, UInt8 inVelocity, UInt32 inStartFrame) override;
	OSStatus HandleAllNotesOff(UInt8 inChannel) override;
	OSStatus HandleControlChange(UInt8 inChannel, UInt8 inController, UInt8 inValue, UInt32 inStartFrame) override;
	OSStatus HandlePitchWheel(UInt8 inChannel, UInt8 inPitchLSB, UInt8 inPitchMSB, UInt32 inStartFrame) override;
	OSStatus HandleChannelPressure(UInt8 inChannel, UInt8 inValue, UInt32 inStartFrame) override;
	OSStatus HandleProgramChange(UInt8 inChannel, UInt8 inProgramNum) override;
	OSStatus HandlePolyPressure(UInt8 inChannel, UInt8 inKey, UInt8 inValue, UInt32 inStartFrame) override
	{
		return noErr;
	}
	OSStatus HandleResetAllControllers(UInt8 inChannel) override
	{
		return noErr;
	}
	OSStatus HandleAllSoundOff(UInt8 inChannel) override
	{
		return noErr;
	}
	#endif
	#if TARGET_PLUGIN_IS_INSTRUMENT
	OSStatus PrepareInstrument(MusicDeviceInstrumentID inInstrument) override;
	OSStatus ReleaseInstrument(MusicDeviceInstrumentID inInstrument) override;
	OSStatus StartNote(MusicDeviceInstrumentID inInstrument,
					   MusicDeviceGroupID inGroupID, NoteInstanceID* outNoteInstanceID, 
					   UInt32 inOffsetSampleFrame, MusicDeviceNoteParams const& inParams) override;
	OSStatus StopNote(MusicDeviceGroupID inGroupID, 
					  NoteInstanceID inNoteInstanceID, UInt32 inOffsetSampleFrame) override;

	// this is a convenience function swiped from AUEffectBase, but not included in MusicDeviceBase
	Float64 GetSampleRate()
	{
		return GetOutput(0)->GetStreamFormat().mSampleRate;
	}
	// this is handled by AUEffectBase, but not in MusicDeviceBase
	bool StreamFormatWritable(AudioUnitScope inScope, AudioUnitElement inElement) override
	{
		return IsInitialized() ? false : true;
	}
	bool CanScheduleParameters() const override
	{
		return true;
	}
	#endif
	#if TARGET_PLUGIN_HAS_GUI
	int GetNumCustomUIComponents() override;
	void GetUIComponentDescs(ComponentDescription* inDescArray) override;
	#endif
#endif
// end of Audio Unit API methods


#ifdef TARGET_API_VST
	void close() override;

	void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) override;
	#if !VST_FORCE_DEPRECATED
	void process(float** inputs, float** outputs, VstInt32 sampleFrames) override;
	#endif

	void suspend() override;
	void resume() override;
	void setSampleRate(float newRate) override;

	VstInt32 getTailSize() override;
	// XXX there is a typo in the VST SDK header files, so in case it gets corrected, 
	// make this call the properly-named version
	VstInt32 getGetTailSize() override { return getTailSize(); }
	bool getInputProperties(VstInt32 index, VstPinProperties* properties) override;
	bool getOutputProperties(VstInt32 index, VstPinProperties* properties) override;

	void setProgram(VstInt32 inProgramNum) override;
	void setProgramName(char* inName) override;
	void getProgramName(char* outText) override;
	bool getProgramNameIndexed(VstInt32 inCategory, VstInt32 inIndex, char* outText) override;

	void setParameter(VstInt32 index, float value) override;
	float getParameter(VstInt32 index) override;
	void getParameterName(VstInt32 index, char* name) override;
	void getParameterDisplay(VstInt32 index, char* text) override;
	void getParameterLabel(VstInt32 index, char* label) override;

	bool getEffectName(char* outText) override;
	VstInt32 getVendorVersion() override;
	bool getVendorString(char* outText) override;
	bool getProductString(char* outText) override;

	VstInt32 canDo(char* text) override;

	#if TARGET_PLUGIN_USES_MIDI
	VstInt32 processEvents(VstEvents* events) override;
	VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override;
	VstInt32 getChunk(void** data, bool isPreset) override;
	#endif

	// DFX supplementary VST methods
	#if TARGET_PLUGIN_USES_DSPCORE
	DfxPluginCore* getplugincore(unsigned long channel) const
	{
		if (channel >= mDSPCores.size())
		{
			return nullptr;
		}
		return mDSPCores[channel].get();
	}
	#endif
#endif
// end of VST API methods


#ifdef TARGET_API_RTAS
	void Free() override;
	ComponentResult ResetPlugInState() override;
	ComponentResult Prime(Boolean inPriming) override;
	void UpdateControlValueInAlgorithm(long inParameterIndex) override;
	ComponentResult IsControlAutomatable(long inControlIndex, short* outItIs) override;
	ComponentResult GetControlNameOfLength(long inParameterIndex, char* outName, long inNameLength, OSType inControllerType, FicBoolean* outReverseHighlight) override;
	ComponentResult GetValueString(long inParameterIndex, long inValue, StringPtr outValueString, long inMaxLength) override;
	ComponentResult SetChunk(OSType inChunkID, SFicPlugInChunk* chunk) override;
	void DoTokenIdle() override;
	CPlugInView* CreateCPlugInView() override;
#ifdef TARGET_API_AUDIOSUITE
	void SetViewOrigin(Point anOrigin) override;
#endif

	// AU->RTAS glue convenience functions
	double GetParameter_f_FromRTAS(long inParameterID);
	int64_t GetParameter_i_FromRTAS(long inParameterID);
	bool GetParameter_b_FromRTAS(long inParameterID);

	#ifdef TARGET_PLUGIN_USES_VSTGUI
	void SetViewPort(GrafPtr inPort) override;
	void GetViewRect(Rect* outViewRect) override;
	long SetControlValue(long inControlIndex, long inValue) override;
	long GetControlValue(long inControlIndex, long* aValue) override;
	long GetControlDefaultValue(long inControlIndex, long* outValue) override;
	ComponentResult UpdateControlGraphic(long inControlIndex, long inValue);
	ComponentResult SetControlHighliteInfo(long inControlIndex, short inIsHighlighted, short inColor);
	ComponentResult ChooseControl(Point inLocalCoord, long* outControlIndex);

	void setEditor(void* inEditor) override
	{
		mCustomUI_p = static_cast<ITemplateCustomUI*>(inEditor);
	}
	int ProcessTouchControl(long inControlIndex) override;
	int ProcessReleaseControl(long inControlIndex) override;
	void ProcessDoIdle() override;
	void* ProcessGetModuleHandle() override
	{
		return mModuleHandle_p;
	}
	short ProcessUseResourceFile() override
	{
		return fProcessType->GetProcessGroup()->UseResourceFile();
	}
	void ProcessRestoreResourceFile(short resFile) override
	{
		fProcessType->GetProcessGroup()->RestoreResourceFile(resFile);
	}

	// silliness needing for RTAS<->VSTGUI connecting
//	virtual float GetParameter_f(long inParameterID) const { return GetParameter(inParameterID); }
	#endif
#endif
// end of RTAS/AudioSuite API methods


protected:
#ifdef TARGET_API_RTAS
	void EffectInit() override;
#ifdef TARGET_API_AUDIOSUITE
	UInt32 ProcessAudio(bool inIsMasterBypassed) override;
#endif
	void RenderAudio(float** inAudioStreams, float** outAudioStreams, long inNumFramesToProcess) override;
#endif
};



#pragma mark _________DfxPluginCore_________

#if TARGET_PLUGIN_USES_DSPCORE
//-----------------------------------------------------------------------------
// Audio Unit must override NewKernel() to implement this
class DfxPluginCore
#ifdef TARGET_API_CORE_CLASS
: public TARGET_API_CORE_CLASS
#endif
{
public:
	explicit DfxPluginCore(DfxPlugin* inDfxPlugin)
	:
		#ifdef TARGET_API_CORE_CLASS
		TARGET_API_CORE_CLASS(inDfxPlugin), 
		#endif
		mDfxPlugin(inDfxPlugin)
	{
	}

	virtual ~DfxPluginCore()
	{
		mDfxPlugin->unregisterAllSmoothedAudioValues(this);
	}

	void dfxplugincore_postconstructor()
	{
		do_reset();
	}

	virtual void process(float const* inStream, float* outStream, unsigned long inNumFrames, bool replacing = true) = 0;
	void do_reset()
	{
		createbuffers();
		clearbuffers();
		reset();
	}
	virtual void reset() {}
	virtual void processparameters() {}

	// ***
	virtual void createbuffers() {}
	// ***
	virtual void clearbuffers() {}
	// ***
	virtual void releasebuffers() {}

	auto const getplugin() const noexcept
	{
		return mDfxPlugin;
	}
	double getsamplerate() const noexcept
	{
		return mDfxPlugin->getsamplerate();
	}
	float getsamplerate_f() const
	{
		return mDfxPlugin->getsamplerate_f();
	}
//	DfxParam::Value getparameter(long inParameterIndex) const { return mDfxPlugin->getparameter(inParameterIndex); }
	double getparameter_f(long inParameterIndex) const
	{
		return mDfxPlugin->getparameter_f(inParameterIndex);
	}
	int64_t getparameter_i(long inParameterIndex) const
	{
		return mDfxPlugin->getparameter_i(inParameterIndex);
	}
	bool getparameter_b(long inParameterIndex) const
	{
		return mDfxPlugin->getparameter_b(inParameterIndex);
	}
	double getparameter_scalar(long inParameterIndex) const
	{
		return mDfxPlugin->getparameter_scalar(inParameterIndex);
	}
	double getparameter_gen(long inParameterIndex) const
	{
		return mDfxPlugin->getparameter_gen(inParameterIndex);
	}
	auto getparameterifchanged_f(long inParameterIndex) const
	{
		return mDfxPlugin->getparameterifchanged_f(inParameterIndex);
	}
	auto getparameterifchanged_i(long inParameterIndex) const
	{
		return mDfxPlugin->getparameterifchanged_i(inParameterIndex);
	}
	auto getparameterifchanged_b(long inParameterIndex) const
	{
		return mDfxPlugin->getparameterifchanged_b(inParameterIndex);
	}
	auto getparameterifchanged_scalar(long inParameterIndex) const
	{
		return mDfxPlugin->getparameterifchanged_scalar(inParameterIndex);
	}
	double getparametermin_f(long inParameterIndex) const
	{
		return mDfxPlugin->getparametermin_f(inParameterIndex);
	}
	int64_t getparametermin_i(long inParameterIndex) const
	{
		return mDfxPlugin->getparametermin_i(inParameterIndex);
	}
	double getparametermax_f(long inParameterIndex) const
	{
		return mDfxPlugin->getparametermax_f(inParameterIndex);
	}
	int64_t getparametermax_i(long inParameterIndex) const
	{
		return mDfxPlugin->getparametermax_i(inParameterIndex);
	}
	bool getparameterchanged(long inParameterIndex) const
	{
		return mDfxPlugin->getparameterchanged(inParameterIndex);
	}
	bool getparametertouched(long inParameterIndex) const
	{
		return mDfxPlugin->getparametertouched(inParameterIndex);
	}
	void registerSmoothedAudioValue(dfx::ISmoothedValue* smoothedValue)
	{
		mDfxPlugin->registerSmoothedAudioValue(smoothedValue, this);
		smoothedValue->setSampleRate(getsamplerate());
	}
	void incrementSmoothedAudioValues()
	{
		mDfxPlugin->incrementSmoothedAudioValues(this);
	}


private:
	DfxPlugin* const mDfxPlugin;


public:

#ifdef TARGET_API_AUDIOUNIT
	void Process(Float32 const* in, Float32* out, UInt32 inNumFrames, UInt32 inNumChannels, bool& ioSilence) override
	{
		process(in, out, inNumFrames, true);
		ioSilence = false;
	}
	void Reset() override
	{
		do_reset();
	}
#endif

};
#endif  // TARGET_PLUGIN_USES_DSPCORE


#pragma clang diagnostic pop



#pragma mark _________DfxEffectGroup_________

#ifdef TARGET_API_RTAS

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class DfxEffectGroup : public CEffectGroup
{
public:
	DfxEffectGroup();

protected:
	void CreateEffectTypes() override;
	void Initialize() override;

private:
	void dfx_AddEffectType(CEffectType* inEffectType);
};


#include "CPluginControl_Continuous.h"
#include "CPluginControl_Frequency.h"

#if WINDOWS_VERSION
	#pragma warning( disable : 4250 ) // function being inherited through dominance
#endif


//-----------------------------------------------------------------------------
class CPluginControl_DfxCurved : virtual public CPluginControl_Continuous
{
public:
	CPluginControl_DfxCurved(OSType id, char const* name, double min, double max, 
							 int numSteps, double defaultValue, bool isAutomatable, 
							 DfxParam::Curve inCurve, double inCurveSpec = 1.0, 
							 PrefixDictionaryEntry const* begin = cStandardPrefixDictionary + ePrefixOffset_no_prefix, 
							 PrefixDictionaryEntry const* end = cStandardPrefixDictionary + ePrefixOffset_End);

	// CPluginControl overrides
	long GetNumSteps() const override;

	// CPluginControl_Continuous overrides
	long ConvertContinuousToControl(double continuous) const override;
	double ConvertControlToContinuous(long control) const override;

private:
	DfxParam::Curve const mCurve;
	double const mCurveSpec;
	int const mNumSteps;
};


//-----------------------------------------------------------------------------
class CPluginControl_DfxCurvedFrequency : public CPluginControl_Frequency, public CPluginControl_DfxCurved
{
public:
	CPluginControl_DfxCurvedFrequency(OSType id, char const* name, double min, double max, 
									  int numSteps, double defaultValue, bool isAutomatable, 
									  DfxParam::Curve inCurve, double inCurveSpec = 1.0, 
									  PrefixDictionaryEntry const* begin = cStandardPrefixDictionary + ePrefixOffset_no_prefix, 
									  PrefixDictionaryEntry const* end = cStandardPrefixDictionary + ePrefixOffset_End)
	:
		CPluginControl_Continuous(id, name, min, max, defaultValue, isAutomatable, begin, end),
		CPluginControl_Frequency(id, name, min, max, defaultValue, isAutomatable, begin, end),
		CPluginControl_DfxCurved(id, name, min, max, numSteps, defaultValue, isAutomatable, 
			inCurve, inCurveSpec, begin, end)
	{}

private:
	CPluginControl_DfxCurvedFrequency(CPluginControl_DfxCurvedFrequency const&);
	CPluginControl_DfxCurvedFrequency& operator=(CPluginControl_DfxCurvedFrequency const&);
};

#endif	// TARGET_API_RTAS






//-----------------------------------------------------------------------------
// plugin entry point macros and defines and stuff

#ifdef TARGET_API_AUDIOUNIT

#if TARGET_PLUGIN_IS_INSTRUMENT
	#define DFX_EFFECT_ENTRY(PluginClass)   AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, PluginClass)
#elif TARGET_PLUGIN_USES_MIDI
	#define DFX_EFFECT_ENTRY(PluginClass)   AUDIOCOMPONENT_ENTRY(AUMIDIEffectFactory, PluginClass)
#else
	#define DFX_EFFECT_ENTRY(PluginClass)   AUDIOCOMPONENT_ENTRY(AUBaseFactory, PluginClass)
#endif

	#if TARGET_PLUGIN_USES_DSPCORE
		#define DFX_CORE_ENTRY(PluginCoreClass)					\
			AUKernelBase* DfxPlugin::NewKernel()				\
			{													\
				auto const core = new PluginCoreClass(this);	\
				if (core)										\
				{												\
					core->dfxplugincore_postconstructor();		\
				}												\
				return core;									\
			}
//	#else
//		AUKernelBase* DfxPlugin::NewKernel()
//		{	return TARGET_API_BASE_CLASS::NewKernel();	}
		#define DFX_INIT_CORE(PluginCoreClass)
	#endif

#else

	// we need to manage the DSP cores manually in APIs other than AU
	// call this in the plugin's constructor if it uses DSP cores for processing
	#if TARGET_PLUGIN_USES_DSPCORE
		// DFX_CORE_ENTRY is not useful for APIs other than AU, so it is defined as nothing
		#define DFX_CORE_ENTRY(PluginCoreClass)
		#define DFX_INIT_CORE(PluginCoreClass)   										\
			for (unsigned long corecount = 0; corecount < getnumoutputs(); corecount++)	\
			{																			\
				mDSPCores[corecount] = std::make_unique<PluginCoreClass>(this);			\
				if (mDSPCores[corecount])												\
				{																		\
					mDSPCores[corecount]->dfxplugincore_postconstructor();				\
				}																		\
			}
	#endif

#endif



#ifdef TARGET_API_VST

	#define DFX_EFFECT_ENTRY(PluginClass)										\
		AudioEffect* createEffectInstance(audioMasterCallback inAudioMaster)	\
		{																		\
			DfxPlugin* effect = nullptr;										\
			try																	\
			{																	\
				effect = new PluginClass(inAudioMaster);						\
			}																	\
			catch (...)															\
			{																	\
				return nullptr;													\
			}																	\
			if (effect)															\
			{																	\
				effect->do_PostConstructor();									\
			}																	\
			return effect;														\
		}

#endif



#ifdef TARGET_API_RTAS

	#ifdef TARGET_API_AUDIOSUITE
		#define DFX_NewEffectProcess	DFX_NewEffectProcessAS
	#endif
	#define DFX_EFFECT_ENTRY(PluginClass)			\
		CEffectProcess* DFX_NewEffectProcess()		\
		{											\
			try										\
			{										\
				return new PluginClass(nullptr);	\
			}										\
			catch (...)								\
			{										\
				return nullptr;						\
			}										\
		}

#endif
