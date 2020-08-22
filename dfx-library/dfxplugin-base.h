/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier and Tom Murphy 7

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
------------------------------------------------------------------------*/

#pragma once


// should be pretty much implied:  
// if the plugin is an instrument, then it uses MIDI
#if TARGET_PLUGIN_IS_INSTRUMENT
	#ifndef TARGET_PLUGIN_USES_MIDI
		#define TARGET_PLUGIN_USES_MIDI 1
	#endif
#endif

// handle base header includes and class names for the target plugin API

#if (defined(TARGET_API_AUDIOUNIT) + defined(TARGET_API_VST) + defined(TARGET_API_RTAS)) != 1
   #error "you must define exactly one of TARGET_API_AUDIOUNIT, TARGET_API_VST, TARGET_API_RTAS"
#endif

// using Apple's Audio Unit API
#if defined(TARGET_API_AUDIOUNIT)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-parameter"
	#if TARGET_PLUGIN_IS_INSTRUMENT
		#include "MusicDeviceBase.h"
		using TARGET_API_BASE_CLASS = MusicDeviceBase;
		using TARGET_API_BASE_INSTANCE_TYPE = AudioComponentInstance;
	#elif TARGET_PLUGIN_USES_MIDI
		#include "AUMIDIEffectBase.h"
		using TARGET_API_BASE_CLASS = AUMIDIEffectBase;
		using TARGET_API_BASE_INSTANCE_TYPE = AudioComponentInstance;
	#else
		#include "AUEffectBase.h"
		using TARGET_API_BASE_CLASS = AUEffectBase;
		using TARGET_API_BASE_INSTANCE_TYPE = AudioComponentInstance;
	#endif
	#pragma clang diagnostic pop

	#if !TARGET_PLUGIN_IS_INSTRUMENT
		#define TARGET_API_CORE_CLASS	AUKernelBase
	#endif
	#define LOGIC_AU_PROPERTIES_AVAILABLE (MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_9)
	#if LOGIC_AU_PROPERTIES_AVAILABLE
		#include <AudioUnit/LogicAUProperties.h>
	#endif

// using Steinberg's VST2 API
#elif defined(TARGET_API_VST)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wshadow"
	#include "public.sdk/source/vst2.x/audioeffectx.h"
	#pragma clang diagnostic pop

	using TARGET_API_BASE_CLASS = AudioEffectX;
	using TARGET_API_BASE_INSTANCE_TYPE = audioMasterCallback;
	using TARGET_API_EDITOR_PARENT_INSTANCE_TYPE = AudioEffect*;

	// set numinputs and numoutputs if numchannels is defined
	#ifdef VST_NUM_CHANNELS
		#ifdef VST_NUM_INPUTS
			#error "do not #define VST_NUM_INPUTS if using VST_NUM_CHANNELS"
		#else
			#define VST_NUM_INPUTS	VST_NUM_CHANNELS
		#endif
		#ifdef VST_NUM_OUTPUTS
			#error "do not #define VST_NUM_OUTPUTS if using VST_NUM_CHANNELS"
		#else
			#define VST_NUM_OUTPUTS	VST_NUM_CHANNELS
		#endif
	#endif
	#if !defined(VST_NUM_INPUTS) || !defined(VST_NUM_OUTPUTS)
		#error "you must either #define both VST_NUM_INPUTS and VST_NUM_OUTPUTS, or VST_NUM_CHANNELS"
	#endif

	#ifdef __MACH__
		#include <TargetConditionals.h>
	#endif
	#if _WIN32
		#include <windows.h>
	#endif

// using Digidesign's RTAS/AudioSuite API
#elif defined(TARGET_API_RTAS)
	#if TARGET_PLUGIN_HAS_GUI
		#include "ITemplateProcess.h"
		using TARGET_API_EDITOR_PARENT_INSTANCE_TYPE = ITemplateProcess*;
	#else
		#ifdef TARGET_API_AUDIOSUITE
			using TARGET_API_EDITOR_PARENT_INSTANCE_TYPE = CEffectProcessAS*;
		#else
			using TARGET_API_EDITOR_PARENT_INSTANCE_TYPE = CEffectProcessRTAS*;
		#endif
	#endif

#endif
// end of target API check


// Now, sanity check defines. 

// Some preprocessor defines are meant to be used with "ifdef" (TARGET_API_*)
// and others are meant to be defined to 1/0. Test that these are configured
// as expected to avoid weirder errors down the line. Note that in GCC,
// defining -DTARGET_API_VST is the same as -DTARGET_API_VST=1, which is
// not the same as #define TARGET_API_VST inside a source file!
//
// The main failure mode is here is doing something like
// #ifdef TARGET_PLUGIN_IS_INSTRUMENT
// after
// #define TARGET_PLUGIN_IS_INSTRUMENT 0
// and unfortunately we can't protect against this except by searching for
// such mistakes. But we can at least be clear about how the macros should
// be used by enforcing it here.

// Macros that we test with #ifdef or #if defined(), so it is not okay to
// #define them to 0.

#if defined(TARGET_API_VST) && TARGET_API_VST - 0 != 1
#error TARGET_API_VST should be defined to 1, if defined
#endif

#if defined(TARGET_API_AUDIOUNIT) && TARGET_API_AUDIOUNIT - 0 != 1
#error TARGET_API_AUDIOUNIT should be defined to 1, if defined
#endif

#if defined(TARGET_API_RTAS) && TARGET_API_RTAS - 0 != 1
#error TARGET_API_RTAS should be defined to 1, if defined
#endif

#if defined(TARGET_API_AUDIOSUITE) && TARGET_API_AUDIOSUITE - 0 != 1
#error TARGET_API_AUDIOSUITE should be defined to 1, if defined
#endif

#if defined(DFX_SUPPORT_OLD_VST_SETTINGS) && DFX_SUPPORT_OLD_VST_SETTINGS - 0 != 1
#error DFX_SUPPORT_OLD_VST_SETTINGS should be defined to 1, if defined
#endif


// Macros that we test with #if, and so should be defined to 1 or 0.

#if defined(TARGET_PLUGIN_USES_MIDI) && (0 - TARGET_PLUGIN_USES_MIDI - 1) == 1 
#error TARGET_PLUGIN_USES_MIDI should be defined to 0 or 1
#endif

#if defined(TARGET_PLUGIN_IS_INSTRUMENT) && (0 - TARGET_PLUGIN_IS_INSTRUMENT - 1) == 1 
#error TARGET_PLUGIN_IS_INSTRUMENT should be defined to 0 or 1
#endif

#if defined(TARGET_PLUGIN_USES_DSPCORE) && (0 - TARGET_PLUGIN_USES_DSPCORE - 1) == 1 
#error TARGET_PLUGIN_USES_DSPCORE should be defined to 0 or 1
#endif

#if defined(TARGET_PLUGIN_HAS_GUI) && (0 - TARGET_PLUGIN_HAS_GUI - 1) == 1 
#error TARGET_PLUGIN_HAS_GUI should be defined to 0 or 1
#endif

#if defined(TARGET_OS_MAC) && (0 - TARGET_OS_MAC - 1) == 1
#error TARGET_OS_MAC should be defined to 0 or 1
#endif

#if defined(TARGET_OS_WIN32) && (0 - TARGET_OS_WIN32 - 1) == 1 
#error TARGET_OS_WIN32 should be defined to 0 or 1
#endif

#if defined(WINDOWS) && (0 - WINDOWS - 1) == 1 
#error WINDOWS should be defined to 0 or 1
#endif

namespace dfx
{


//-----------------------------------------------------------------------------
#ifdef TARGET_API_AUDIOUNIT
enum
{
	kStatus_NoError = noErr,
	kStatus_ParamError = kAudio_ParamError,
	kStatus_InitializationFailed = kAudioUnitErr_FailedInitialization,
	kStatus_Uninitialized = kAudioUnitErr_Uninitialized,
	kStatus_InvalidParameter = kAudioUnitErr_InvalidParameter,
	kStatus_InvalidProperty = kAudioUnitErr_InvalidProperty,
	kStatus_InvalidPropertyValue = kAudioUnitErr_InvalidPropertyValue,
	kStatus_CannotDoInCurrentContext = kAudioUnitErr_CannotDoInCurrentContext
};
#else
enum
{
	kStatus_NoError = 0,
	kStatus_ParamError = -50,
	kStatus_InitializationFailed = -10875,
	kStatus_Uninitialized = -10867,
	kStatus_InvalidParameter = -10878,
	kStatus_InvalidProperty = -10879,
	kStatus_InvalidPropertyValue = -10851,
	kStatus_CannotDoInCurrentContext = -10863
};
#endif



#ifdef TARGET_API_RTAS

//-----------------------------------------------------------------------------
constexpr long kParameterShortNameMax_RTAS = 4;  // XXX hack (this just happens to be what I've been doing)
constexpr long kParameterValueShortNameMax_RTAS = 6;  // XXX hack

constexpr long kParameterID_RTASMasterBypass = 1;
constexpr long kParameterID_RTASOffset = kParameterID_RTASMasterBypass + 1;
//-----------------------------------------------------------------------------
constexpr long ParameterID_ToRTAS(long inParameterID)
{
	return inParameterID + kParameterID_RTASOffset;
}
//-----------------------------------------------------------------------------
constexpr long ParameterID_FromRTAS(long inParameterIndex_RTAS)
{
	return inParameterIndex_RTAS - kParameterID_RTASOffset;
}

// XXX a hack to handle the fact that CPluginControl_Percent (annoyingly) 
// automatically converts to and from internal 0-1 values and external 0-100 values
constexpr double kRTASPercentScalar = 0.01;

#endif


}  // namespace dfx
