/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2011  Sophia Poirier

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

#ifndef __DFXPLUGIN_BASE_H
#define __DFXPLUGIN_BASE_H


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
	#if TARGET_PLUGIN_IS_INSTRUMENT
		#include "MusicDeviceBase.h"
		typedef MusicDeviceBase TARGET_API_BASE_CLASS;
		typedef AudioComponentInstance TARGET_API_BASE_INSTANCE_TYPE;
	#elif TARGET_PLUGIN_USES_MIDI
		#include "AUMIDIEffectBase.h"
		typedef AUMIDIEffectBase TARGET_API_BASE_CLASS;
		typedef AudioComponentInstance TARGET_API_BASE_INSTANCE_TYPE;
	#else
		#include "AUEffectBase.h"
		typedef AUEffectBase TARGET_API_BASE_CLASS;
		typedef AudioComponentInstance TARGET_API_BASE_INSTANCE_TYPE;
	#endif
	#if !TARGET_PLUGIN_IS_INSTRUMENT
		#define TARGET_API_CORE_CLASS	AUKernelBase
	#endif
	#include <AudioUnit/LogicAUProperties.h>

// using Steinberg's VST API
#elif defined(TARGET_API_VST)
	#if _WIN32
		#include <windows.h>
	#endif

	#include "audioeffectx.h"
//	#include <stdio.h>
//	#include <stdlib.h>
//	#include <math.h>
	typedef AudioEffectX	TARGET_API_BASE_CLASS;
	typedef audioMasterCallback	TARGET_API_BASE_INSTANCE_TYPE;
	typedef AudioEffect *	TARGET_API_EDITOR_PARENT_INSTANCE_TYPE;
	// set numinputs and numoutputs if numchannels is defined
	#ifdef VST_NUM_CHANNELS
		#ifndef VST_NUM_INPUTS
			#define VST_NUM_INPUTS	VST_NUM_CHANNELS
		#endif
		#ifndef VST_NUM_OUTPUTS
			#define VST_NUM_OUTPUTS	VST_NUM_CHANNELS
		#endif
	#endif
	// XXX for when we may still need to build with pre-2.4 VST SDKs
	#if !VST_2_4_EXTENSIONS
		#define VstInt32	long
		#define VstIntPtr	long
	#endif
	#ifdef __MACH__
		#include <CoreFoundation/CoreFoundation.h>
	#endif

// using Digidesign's RTAS/AudioSuite API
#elif defined(TARGET_API_RTAS)
	#ifdef TARGET_PLUGIN_USES_VSTGUI
		#include "ITemplateProcess.h"
	#endif
	#ifdef TARGET_PLUGIN_USES_VSTGUI
		typedef ITemplateProcess *	TARGET_API_EDITOR_PARENT_INSTANCE_TYPE;
	#else
		#ifdef TARGET_API_AUDIOSUITE
			typedef CEffectProcessAS *	TARGET_API_EDITOR_PARENT_INSTANCE_TYPE;
		#else
			typedef CEffectProcessRTAS *	TARGET_API_EDITOR_PARENT_INSTANCE_TYPE;
		#endif
	#endif

#endif
// end of target API check



//-----------------------------------------------------------------------------
//class DfxPluginCore;
#ifndef TARGET_API_AUDIOUNIT
//	class AUKernelBase;
#endif
//class DfxPreset;

#ifdef TARGET_API_AUDIOUNIT
	enum {
		kDfxErr_NoError = noErr,
		kDfxErr_ParamError = paramErr,
		kDfxErr_InitializationFailed = kAudioUnitErr_FailedInitialization,
		kDfxErr_InvalidParameter = kAudioUnitErr_InvalidParameter,
		kDfxErr_InvalidProperty = kAudioUnitErr_InvalidProperty,
		kDfxErr_InvalidPropertyValue = kAudioUnitErr_InvalidPropertyValue,
		kDfxErr_CannotDoInCurrentContext = kAudioUnitErr_CannotDoInCurrentContext
	};
#else
	enum {
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
const long kParameterShortNameMax_rtas = 4;	// XXX hack (this just happens to be what I've been doing)
const long kParameterValueShortNameMax_rtas = 6;	// XXX hack

const long kDFXParameterID_RTASMasterBypass = 1;
const long kDFXParameterID_RTASOffset = kDFXParameterID_RTASMasterBypass + 1;
//-----------------------------------------------------------------------------
inline long DFX_ParameterID_ToRTAS(long inParameterID)
{
	return inParameterID + kDFXParameterID_RTASOffset;
}
//-----------------------------------------------------------------------------
inline long DFX_ParameterID_FromRTAS(long inParameterIndex_rtas)
{
	return inParameterIndex_rtas - kDFXParameterID_RTASOffset;
}

// XXX a hack to handle the fact that CPluginControl_Percent (annoyingly) 
// automatically converts to and from internal 0-1 values and external 0-100 values
const double kDFX_RTASPercentScalar = 0.01;

#endif



#endif
