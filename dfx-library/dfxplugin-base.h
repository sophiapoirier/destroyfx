/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

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
	#define LOGIC_AU_PROPERTIES_AVAILABLE (__MAC_OS_X_VERSION_MAX_ALLOWED <= __MAC_10_9)
	#if LOGIC_AU_PROPERTIES_AVAILABLE
		#include <AudioUnit/LogicAUProperties.h>
	#endif

// using Steinberg's VST API
#elif defined(TARGET_API_VST)
	#if _WIN32
		#include <windows.h>
	#endif

	#include "audioeffectx.h"
	using TARGET_API_BASE_CLASS = AudioEffectX;
	using TARGET_API_BASE_INSTANCE_TYPE = audioMasterCallback;
	using TARGET_API_EDITOR_PARENT_INSTANCE_TYPE = AudioEffect*;
	// set numinputs and numoutputs if numchannels is defined
	#ifdef VST_NUM_CHANNELS
		#ifndef VST_NUM_INPUTS
			#define VST_NUM_INPUTS	VST_NUM_CHANNELS
		#endif
		#ifndef VST_NUM_OUTPUTS
			#define VST_NUM_OUTPUTS	VST_NUM_CHANNELS
		#endif
	#endif
	#ifdef __MACH__
		#include <CoreFoundation/CoreFoundation.h>
	#endif

// using Digidesign's RTAS/AudioSuite API
#elif defined(TARGET_API_RTAS)
	#ifdef TARGET_PLUGIN_USES_VSTGUI
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



//-----------------------------------------------------------------------------
#ifdef TARGET_API_AUDIOUNIT
	enum
	{
		kDfxErr_NoError = noErr,
		kDfxErr_ParamError = paramErr,
		kDfxErr_InitializationFailed = kAudioUnitErr_FailedInitialization,
		kDfxErr_InvalidParameter = kAudioUnitErr_InvalidParameter,
		kDfxErr_InvalidProperty = kAudioUnitErr_InvalidProperty,
		kDfxErr_InvalidPropertyValue = kAudioUnitErr_InvalidPropertyValue,
		kDfxErr_CannotDoInCurrentContext = kAudioUnitErr_CannotDoInCurrentContext
	};
#else
	enum
	{
		kDfxErr_NoError = 0,
		kDfxErr_ParamError = -50,
		kDfxErr_InitializationFailed = -10875,
		kDfxErr_InvalidParameter = -10878,
		kDfxErr_InvalidProperty = -10879,
		kDfxErr_InvalidPropertyValue = -10851,
		kDfxErr_CannotDoInCurrentContext = -10863
	};
#endif



#ifdef TARGET_API_RTAS

//-----------------------------------------------------------------------------
constexpr long kParameterShortNameMax_rtas = 4;  // XXX hack (this just happens to be what I've been doing)
constexpr long kParameterValueShortNameMax_rtas = 6;  // XXX hack

constexpr long kDFXParameterID_RTASMasterBypass = 1;
constexpr long kDFXParameterID_RTASOffset = kDFXParameterID_RTASMasterBypass + 1;
//-----------------------------------------------------------------------------
constexpr long DFX_ParameterID_ToRTAS(long inParameterID)
{
	return inParameterID + kDFXParameterID_RTASOffset;
}
//-----------------------------------------------------------------------------
constexpr long DFX_ParameterID_FromRTAS(long inParameterIndex_rtas)
{
	return inParameterIndex_rtas - kDFXParameterID_RTASOffset;
}

// XXX a hack to handle the fact that CPluginControl_Percent (annoyingly) 
// automatically converts to and from internal 0-1 values and external 0-100 values
constexpr double kDFX_RTASPercentScalar = 0.01;

#endif
