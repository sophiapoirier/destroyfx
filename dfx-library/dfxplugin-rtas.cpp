/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2009-2021  Sophia Poirier

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
This is where we connect the RTAS/AudioSuite API to our DfxPlugin system.
------------------------------------------------------------------------*/

#include "dfxplugin.h"

#include <algorithm>
#include <cmath>

#include "CEffectTypeAS.h"
#include "CEffectTypeRTAS.h"
#include "CPluginControl_LinearBoostCut.h"
#include "CPluginControl_LinearGain.h"
#include "CPluginControl_LinearPercent.h"
#include "CPluginControl_LinearQ.h"
#include "CPluginControl_LinearThld.h"
#include "CPluginControl_LinearTime.h"
#include "CPluginControl_List.h"
#include "CPluginControl_LogFrequency.h"
#include "CPluginControl_LogGain.h"
#include "CPluginControl_LogPercent.h"
#include "CPluginControl_LogQ.h"
#include "CPluginControl_LogTime.h"
#include "CPluginControl_LogTimeS.h"
#include "CPluginControl_OnOff.h"
#include "dfxmisc.h"
//#include "FicPluginEnums.h"
#include "PlugInUtils.h"

#if !TARGET_PLUGIN_HAS_GUI
	#include "CClippingView.h"
	#include "CGainMeterView.h"
	#include "CNoResourceView.h"
	#include "CPictBackgroundTextEditor.h"
	#include "CSliderControlEditor.h"
	#include "CStateTextControlEditor.h"
	#include "CStaticCustomText.h"
	#include "CTextCustomPopupEditor.h"
#endif



//-----------------------------------------------------------------------------
static OSType DFX_IterateAlphaNumericFourCharCode(OSType inPreviousCode);



#pragma mark -
#pragma mark init
#pragma mark -

//-----------------------------------------------------------------------------
void DfxPlugin::EffectInit()
{
	do_PostConstructor();

	AddParametersToList();

	if (IsAS())
	{
		mInputAudioStreams_as.assign(GetNumOutputs(), nullptr);
		mOutputAudioStreams_as.assign(GetNumOutputs(), nullptr);
	}

	do_initialize();

/*
long bypassControlIndex = 0;
ComponentResult bypassResult = GetMasterBypassControl(&bypassControlIndex);
CPluginControl_Discrete * bypassControl = dynamic_cast<CPluginControl_Discrete*>( GetControl(bypassControlIndex) );
char bypassName[32];
bypassControl->GetNameOfLength(bypassName, sizeof(bypassName)-1, 0);
fprintf(stderr, "%s IsAutomatable() = %s (error = %ld)\n", bypassName, bypassControl->IsAutomatable() ? "true" : "false", bypassResult);
*/
}

//-----------------------------------------------------------------------------
void DfxPlugin::Free()
{
	do_cleanup();
	do_PreDestructor();

	TARGET_API_BASE_CLASS::Free();
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::ResetPlugInState()
{
	do_reset();
	return noErr;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::Prime(Boolean inPriming)
{
	if (inPriming)
		do_reset();
	return noErr;
}

//-----------------------------------------------------------------------------
void DfxPlugin::DoTokenIdle()
{
	CEffectProcess::DoTokenIdle();

#if TARGET_PLUGIN_HAS_GUI
	if (mCustomUI_p)
	{
//		if ( !IsAS() || (IsAS() && (GetASPreviewState() == previewState_Off)) )
		mCustomUI_p->Idle();
	}
#endif
}



#pragma mark -
#pragma mark parameters
#pragma mark -

//-----------------------------------------------------------------------------
void DfxPlugin::AddParametersToList()
{
	constexpr OSType globalBypassFourCharID = FOURCC('b', 'y', 'p', 'a');

	AddControl(new CPluginControl_OnOff(globalBypassFourCharID, "Global Bypass\nG Bypass\nGByp\nByp", false, true));
	DefineMasterBypassControlIndex(dfx::kParameterID_RTASGlobalBypass);

	OSType paramFourCharID = 0;
	for (long i = 0; i < getnumparameters(); i++)
	{
		if (!parameterisvalid(i))
		{
			continue;	// XXX eh actually maybe we need to add a parameter for every index?
		}

		auto const paramMin_f = getparametermin_f(i), paramMax_f = getparametermax_f(i), paramDefault_f = getparameterdefault_f(i);
		auto const paramMin_i = getparametermin_i(i), paramMax_i = getparametermax_i(i), paramDefault_i = getparameterdefault_i(i);
		auto const paramName = getparametername(i);

		paramFourCharID = DFX_IterateAlphaNumericFourCharCode(paramFourCharID);
		// make sure to skip over the ID already used for global bypass
		if (paramFourCharID == globalBypassFourCharID)
		{
			paramFourCharID = DFX_IterateAlphaNumericFourCharCode(paramFourCharID);
		}
		bool const paramAutomatable = (getparameterattributes(i) & (DfxParam::kAttribute_Hidden | DfxParam::kAttribute_Unused)) ? false : true;
		constexpr int numCurvedSteps = 1000;
		double const stepSize_default = (paramMax_f - paramMin_f) / (double)numCurvedSteps;
		auto const paramCurve = getparametercurve(i);
		auto const paramCurveSpec = getparametercurvespec(i);
		bool const paramIsLog = (paramCurve == DfxParam::Curve::Log) || (paramCurve == DfxParam::Curve::Exp);

		if ( getparameterusevaluestrings(i) )
		{
			std::vector<std::string> valueStringsVector;
			for (int64_t stringIndex=paramMin_i; stringIndex <= paramMax_i; stringIndex++)
			{
				if (auto const valueString = getparametervaluestring(i, stringIndex))
				{
					valueStringsVector.push_back(*valueString);
				}
			}
			AddControl( new CPluginControl_List(paramFourCharID, paramName.c_str(), valueStringsVector, paramDefault_i - paramMin_i, paramAutomatable) );
		}

		else if (getparametervaluetype(i) == DfxParam::ValueType::Int)
		{
			AddControl( new CPluginControl_Discrete(paramFourCharID, paramName.c_str(), paramMin_i, paramMax_i, paramDefault_i, paramAutomatable) );
		}

		else if (getparametervaluetype(i) == DfxParam::ValueType::Boolean)
		{
			AddControl( new CPluginControl_OnOff(paramFourCharID, paramName.c_str(), getparameterdefault_b(i), paramAutomatable) );
		}

		else
		{
			switch ( getparameterunit(i) )
			{
				case DfxParam::Unit::Percent:
				case DfxParam::Unit::DryWetMix:
					if (paramIsLog)
						AddControl( new CPluginControl_LogPercent(paramFourCharID, paramName.c_str(), paramMin_f * dfx::kRTASPercentScalar, paramMax_f * dfx::kRTASPercentScalar, numCurvedSteps, paramDefault_f * dfx::kRTASPercentScalar, paramAutomatable) );
					else
						AddControl( new CPluginControl_LinearPercent(paramFourCharID, paramName.c_str(), paramMin_f * dfx::kRTASPercentScalar, paramMax_f * dfx::kRTASPercentScalar, stepSize_default * dfx::kRTASPercentScalar, paramDefault_f * dfx::kRTASPercentScalar, paramAutomatable) );
					break;

				case DfxParam::Unit::MS:
					if (paramIsLog)
						AddControl( new CPluginControl_LogTime(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
					else
						AddControl( new CPluginControl_LinearTime(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, stepSize_default, paramDefault_f, paramAutomatable) );
					break;

				case DfxParam::Unit::Seconds:
					// XXX not actually really sure whether CPluginControl_TimeS is for seconds? or maybe samples?
					AddControl( new CPluginControl_LogTimeS(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
					break;

				case DfxParam::Unit::Hz:
					if (paramCurve == DfxParam::Curve::Log)
						AddControl( new CPluginControl_LogFrequency(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
					else
						AddControl( new CPluginControl_DfxCurvedFrequency(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable, paramCurve, paramCurveSpec) );
					break;

				case DfxParam::Unit::Decibles:
					// XXX or perhaps use CPluginControl_Level, or split the two out from each other?
					AddControl( new CPluginControl_LinearBoostCut(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, stepSize_default, paramDefault_f, paramAutomatable) );
					break;

				case DfxParam::Unit::LinearGain:
					if (paramIsLog)
						AddControl( new CPluginControl_LogGain(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
					else
						AddControl( new CPluginControl_LinearGain(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, stepSize_default, paramDefault_f, paramAutomatable) );
					break;

				default:
					{
						if (paramIsLog)
							AddControl( new CPluginControl_Log(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
						else
							AddControl( new CPluginControl_Linear(paramFourCharID, paramName.c_str(), paramMin_f, paramMax_f, stepSize_default, paramDefault_f, paramAutomatable) );
					}
					break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// XXX implement
ComponentResult DfxPlugin::GetControlNameOfLength(long inParameterIndex, char * outName, long inNameLength, OSType inControllerType, FicBoolean * outReverseHighlight)
{
	if (outName == NULL)
		return paramErr;

/*
	if (inNameLength <= dfx::kParameterShortNameMax_RTAS)
	{
		char * shortNameString = NULL;
		if (inParameterIndex == dfx::kParameterID_RTASGlobalBypass)
			shortNameString = "Bypass";	// any shortened version of this is fine
		else
			shortNameString = GetParameterShortName( dfx::ParameterID_FromRTAS(inParameterIndex) );
		if (shortNameString != NULL)
		{
			strlcpy(outName, shortNameString, inNameLength + 1);
			outName[inNameLength] = 0;
			if (outReverseHighlight != NULL)
				*outReverseHighlight = false;	// XXX assume control is not to be highlighted
			return noErr;
		}
	}
*/

	return TARGET_API_BASE_CLASS::GetControlNameOfLength(inParameterIndex, outName, inNameLength, inControllerType, outReverseHighlight);
}

//-----------------------------------------------------------------------------
// XXX implement
ComponentResult DfxPlugin::GetValueString(long inParameterIndex, long inValue, StringPtr outValueString, long inMaxLength)
{
	if (outValueString == NULL)
		return paramErr;

/*
	if (inMaxLength <= dfx::kParameterValueShortNameMax_rtas)
	{
		auto const shortValueString = GetParameterValueShortString(dfx::ParameterID_FromRTAS(inParameterIndex), inValue);
		if (shortValueString != NULL)
		{
			strlcpy((char*)(outValueString + 1), shortValueString, inMaxLength + 1);
			outValueString[0] = ((signed)strlen(shortValueString) > inMaxLength) ? inMaxLength : strlen(shortValueString);
			return noErr;
		}
	}
*/

	return TARGET_API_BASE_CLASS::GetValueString(inParameterIndex, inValue, outValueString, inMaxLength);
}

//-----------------------------------------------------------------------------
void DfxPlugin::UpdateControlValueInAlgorithm(long inParameterIndex)
{
	if (inParameterIndex == dfx::kParameterID_RTASGlobalBypass)
	{
		mGlobalBypass_rtas = dynamic_cast<CPluginControl_Discrete*>(GetControl(inParameterIndex))->GetDiscrete();
		return;
	}

	inParameterIndex = dfx::ParameterID_FromRTAS(inParameterIndex);
	if (! parameterisvalid(inParameterIndex) )
		return;

	switch ( getparametervaluetype(inParameterIndex) )
	{
		case DfxParam::ValueType::Float:
			parameters[inParameterIndex].set_f( GetParameter_f_FromRTAS(inParameterIndex) );
			break;
		case DfxParam::ValueType::Int:
			parameters[inParameterIndex].set_i( GetParameter_i_FromRTAS(inParameterIndex) );
			break;
		case DfxParam::ValueType::Boolean:
			parameters[inParameterIndex].set_b( GetParameter_b_FromRTAS(inParameterIndex) );
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
double DfxPlugin::GetParameter_f_FromRTAS(long inParameterID)
{
	double resultValue = dynamic_cast<CPluginControl_Continuous*>(GetControl(dfx::ParameterID_ToRTAS(inParameterID)))->GetContinuous();
	if ( (getparameterunit(inParameterID) == DfxParam::Unit::Percent) || (getparameterunit(inParameterID) == DfxParam::Unit::DryWetMix) )
		resultValue /= dfx::kRTASPercentScalar;
	return resultValue;
}

//-----------------------------------------------------------------------------
int64_t DfxPlugin::GetParameter_i_FromRTAS(long inParameterID)
{
	int64_t resultValue = dynamic_cast<CPluginControl_Discrete*>(GetControl(dfx::ParameterID_ToRTAS(inParameterID)))->GetDiscrete();
	if ( getparameterusevaluestrings(inParameterID) )
		resultValue += getparametermin_i(inParameterID);
	return resultValue;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::GetParameter_b_FromRTAS(long inParameterID)
{
	return (GetParameter_i_FromRTAS(inParameterID) != 0);
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::IsControlAutomatable(long inControlIndex, short * outItIs)
{
	if (outItIs == NULL)
		return paramErr;

	// XXX test this first, since dfx::ParameterID_FromRTAS() makes it an invalid ID
	if (inControlIndex == dfx::kParameterID_RTASGlobalBypass)
	{
		*outItIs = 1;
		return noErr;
	}

	inControlIndex = dfx::ParameterID_FromRTAS(inControlIndex);
	if (! parameterisvalid(inControlIndex) )
		return paramErr;

	if ( getparameterattributes(inControlIndex) & (DfxParam::kAttribute_Unused | DfxParam::kAttribute_Hidden) )
		*outItIs = 0;
	else
		*outItIs = 1;
	return noErr;
}

#ifndef TARGET_API_AUDIOSUITE

#include "SliderConversions.h"
#include "PlugInAssert.h"

//-----------------------------------------------------------------------------
CPluginControl_DfxCurved::CPluginControl_DfxCurved(OSType id, char const* name, double min, double max, 
		int numSteps, double defaultValue, bool isAutomatable, 
		DfxParam::Curve inCurve, double inCurveSpec, 
		PrefixDictionaryEntry const* begin, PrefixDictionaryEntry const* end)
:	CPluginControl_Continuous(id, name, min, max, defaultValue, isAutomatable, begin, end),
	mCurve(inCurve), mCurveSpec(inCurveSpec), mNumSteps(numSteps)
{
	PIASSERT(numSteps != 0);	// mNumSteps==0 can cause problems with devide-by-zero in PT
	SetValue( GetDefaultValue() );
}

//-----------------------------------------------------------------------------
long CPluginControl_DfxCurved::GetNumSteps() const
{
	return mNumSteps;
}

//-----------------------------------------------------------------------------
long CPluginControl_DfxCurved::ConvertContinuousToControl(double continuous) const
{
	continuous = DfxParam::contract(continuous, GetMin(), GetMax(), mCurve, mCurveSpec);
	return DoubleToLongControl(continuous, 0.0, 1.0);
}

//-----------------------------------------------------------------------------
double CPluginControl_DfxCurved::ConvertControlToContinuous(long control) const
{
	double continous = LongControlToDouble(control, 0.0, 1.0);
	return DfxParam::expand(continous, GetMin(), GetMax(), mCurve, mCurveSpec);
}

#endif	// !TARGET_API_AUDIOSUITE



#pragma mark -
#pragma mark state
#pragma mark -

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::SetChunk(OSType inChunkID, SFicPlugInChunk * chunk)
{
	return TARGET_API_BASE_CLASS::SetChunk(inChunkID, chunk);
}



#pragma mark -
#pragma mark dsp
#pragma mark -

#ifdef TARGET_API_AUDIOSUITE
//-----------------------------------------------------------------------------
UInt32 DfxPlugin::ProcessAudio(bool inIsGlobalBypassed)
{
	if (!IsAS())
	{
		return 0;
	}

	long totalInputSamples = 0;  // total number of input samples in one input buffer

	// use the mono channel input sample number (guaranteed to be connected)
	if (GetInputConnection(0) != NULL)
	{
		totalInputSamples = GetInputConnection(0)->mNumSamplesInBuf;
	}

	for (SInt32 ch = 0; ch < GetNumOutputs(); ch++)
	{
		// XXX what to do if there are no valid connections for the channel?
		mInputAudioStreams_as[ch] = NULL;
		mOutputAudioStreams_as[ch] = NULL;

		DAEConnectionPtr outputConnection = GetOutputConnection(ch);
		if (outputConnection != NULL)	// if no valid connection, don't do anything
		{
			mOutputAudioStreams_as[ch] = (float*)(outputConnection->mBuffer);

			DAEConnectionPtr inputConnection = GetInputConnection(ch);
			if (inputConnection != NULL)	// have a valid input connection
				mInputAudioStreams_as[ch] = (float*)(inputConnection->mBuffer);
			else	// no input connection; use default value of zero
			{
				// (re)allocate the zero audio buffer if it is not currently large enough for this rendering slice
				if (static_cast<long>(mZeroAudioBuffer.size()) < totalInputSamples)
				{
					mZeroAudioBuffer.assign(totalInputSamples, 0.0f);
				}
				mInputAudioStreams_as[ch] = mZeroAudioBuffer.data();
			}
			
			if (inIsGlobalBypassed)
			{
				for (long i = 0; i < totalInputSamples; i++)
					mOutputAudioStreams_as[ch][i] = mInputAudioStreams_as[ch][i];
			}
			// do the sample number adjustment
			outputConnection->mNumSamplesInBuf = totalInputSamples;
			if (inputConnection != NULL)
				inputConnection->mNumSamplesInBuf = 0;
		}
	}

	if (!inIsGlobalBypassed)
	{
		RenderAudio(mInputAudioStreams_as.data(), mOutputAudioStreams_as.data(), totalInputSamples);
	}

	// Get the current number of samples analyzed and pass this info 
	// back to the DAE application so it knows how much we've processed.  
	// This is a global position keeper, it should be incremented by the 
	// number of samples processed on a single track, not the total processed 
	// by all tracks.
	return totalInputSamples;
}
#endif

//-----------------------------------------------------------------------------
void DfxPlugin::RenderAudio(float ** inAudioStreams, float ** outAudioStreams, long inNumFramesToProcess)
{
	preprocessaudio();

	// RTAS clip monitoring
	for (SInt32 channel = 0; (channel < GetNumInputs()) && !fClipped; channel++)
	{
		mInputAudioStreams[[channel]] = nullptr;
		if (!inAudioStreams[channel])	// XXX possible in AudioSuite
		{
			continue;
		}
		for (long samp = 0; (samp < inNumFramesToProcess) && !fClipped; samp++)
		{
			if (std::fabs(inAudioStreams[channel][samp]) > 1.0f)
			{
				fClipped = true;
				break;
			}
		}
		if (mInPlaceAudioProcessingAllowed)
		{
			mInputAudioStreams[channel] = inAudioStreams[channel];
		}
		else
		{
			std::copy_n(inAudioStreams[channel], dfx::math::ToIndex(inNumFramesToProcess), mInputOutOfPlaceAudioBuffers[channel].data());
		}
	}

	for (SInt32 channel = 0; channel < GetNumOutputs(); channel++)
	{
		if (!inAudioStreams[channel] || !outAudioStreams[channel])  // XXX possible in AudioSuite
		{
			continue;
		}
		if (mGlobalBypass_rtas)
		{
			SInt32 const inputChannelIndex = (GetNumInputs() < GetNumOutputs()) ? (GetNumInputs() - 1) : channel;
			std::copy_n(mInputAudioStreams[inputChannelIndex], dfx::math::ToIndex(inNumFramesToProcess), outAudioStreams[channel]);
		}
		else
		{
			if (!mInPlaceAudioProcessingAllowed)
			{
				std::fill_n(outAudioStreams[channel], dfx::math::ToIndex(inNumFramesToProcess), 0.0f);
			}
#if TARGET_PLUGIN_USES_DSPCORE
			if (mDSPCores[channel])
			{
				auto inputAudio = mInputAudioStreams[channel];
				if (asymmetricalchannels())
				{
					if (channel == 0)
					{
						std::copy_n(mInputAudioStreams[channel], sampleFrames, mAsymmetricalInputAudioBuffer.data());
					}
					inputAudio = mAsymmetricalInputAudioBuffer.data();
				}
				mDSPCores[channel]->process(inputAudio, outAudioStreams[channel], (unsigned)inNumFramesToProcess);
			}
#endif
		}
	}
#if !TARGET_PLUGIN_USES_DSPCORE
	if (!mGlobalBypass_rtas)
	{
		processaudio(const_cast<float const* const*>(mInputAudioStreams.data()), const_cast<float* const*>(outAudioStreams), (unsigned)inNumFramesToProcess);
	}
#endif

	// RTAS clip monitoring
	for (SInt32 channel = 0; (channel < GetNumOutputs()) && !fClipped; channel++)
	{
		if (outAudioStreams[channel] == NULL)  // XXX possible in AudioSuite
		{
			continue;
		}
		for (long samp = 0; samp < inNumFramesToProcess; samp++)
		{
			if (std::fabs(outAudioStreams[channel][samp]) > 1.0f)
			{
				fClipped = true;
				break;
			}
		}
	}

	postprocessaudio();
}



#pragma mark -
#pragma mark MIDI
#pragma mark -

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------------------
#endif
// TARGET_PLUGIN_USES_MIDI




#pragma mark -
#pragma mark GUI
#pragma mark -

//-----------------------------------------------------------------------------
CPlugInView * DfxPlugin::CreateCPlugInView()
{
#if TARGET_PLUGIN_HAS_GUI
	CNoResourceView * ui = NULL;
	try
	{
		if (!mCustomUI_p)
		{
			mCustomUI_p.reset(CreateCTemplateCustomUI(this));
			mCustomUI_p->GetRect( &(mPIWinRect.left), &(mPIWinRect.top), &(mPIWinRect.right), &(mPIWinRect.bottom) );
		}

		ui = new CNoResourceView;
#if PT_SDK_VERSION < 0x08000000
		constexpr long viewSizeOffset = 1;	// XXX +1 because of a bug where CNoResourceView::SetSize() subtracts 1 from width and height
#else
		constexpr long viewSizeOffset = 0;	// the bug is fixed in the Pro Tools 8 SDK
#endif
		ui->SetSize(mPIWinRect.right+viewSizeOffset, mPIWinRect.bottom+viewSizeOffset);

		mNoUIView_p = (CTemplateNoUIView *) ui->AddView2("!NoUIView[('NoID')]", 0, 0, mPIWinRect.right, mPIWinRect.bottom, false);

		if (mNoUIView_p != NULL)
			mNoUIView_p->SetCustomUI(mCustomUI_p.get());
	}
	catch (...)
	{
		mCustomUI_p.reset();
		if (ui != NULL)
			delete ui;
		ui = NULL;
	}

	return ui;

#else
	OSType meterFourCharID = 0;
	OSType meterIDs[EffectLayerDef::MAX_NUM_CONNECTIONS];
	for (UInt32 i = 0; i < EffectLayerDef::MAX_NUM_CONNECTIONS; i++)
	{
		meterFourCharID = DFX_IterateAlphaNumericFourCharCode(meterFourCharID);
		meterIDs[i] = meterFourCharID;
	}

	OSType clipIDs[EffectLayerDef::MAX_NUM_CONNECTIONS];
	for (UInt32 i = 0; i < EffectLayerDef::MAX_NUM_CONNECTIONS; i++)
	{
		meterFourCharID = DFX_IterateAlphaNumericFourCharCode(meterFourCharID);
		clipIDs[i] = meterFourCharID;
	}

	return CreateMinimalGUI(this, meterIDs, clipIDs);
#endif
}

#if TARGET_PLUGIN_HAS_GUI

//-----------------------------------------------------------------------------
void DfxPlugin::GetViewRect(Rect * outViewRect)
{
	if (!mCustomUI_p)
		mCustomUI_p.reset(CreateCTemplateCustomUI(this));
	if (outViewRect == NULL)
		return;

	mCustomUI_p->GetRect( &(mPIWinRect.left), &(mPIWinRect.top), &(mPIWinRect.right), &(mPIWinRect.bottom) );
	outViewRect->left = mPIWinRect.left;
	outViewRect->top = mPIWinRect.top;
	outViewRect->right = mPIWinRect.right;
	outViewRect->bottom = mPIWinRect.bottom;
}

//-----------------------------------------------------------------------------
void DfxPlugin::SetViewPort(GrafPtr inPort)
{
	mMainPort = inPort;
	CEffectProcess::SetViewPort(mMainPort);

	if (mMainPort != NULL)
	{
		if (mCustomUI_p)
		{
#if TARGET_OS_WIN32
			Rect aRect;
			GetPortBounds(inPort, &aRect);
			if ( ((aRect.left <= 0) && (aRect.top <= 0)) && ((mLeftOffset == 0) && (mTopOffset == 0)) )
			{
				mLeftOffset = -aRect.left;
				mTopOffset = -aRect.top;
			}
#endif

			void * windowPtr = NULL;
			#if TARGET_OS_WIN32
				windowPtr = (void*) ASI_GethWnd((WindowPtr)mMainPort);
			#elif TARGET_OS_MAC
				windowPtr = (void*) GetWindowFromPort(mMainPort);
			#endif
			mCustomUI_p->Open(windowPtr, mLeftOffset, mTopOffset);

			if (mNoUIView_p != NULL)
				mNoUIView_p->SetEnable(true);

#if TARGET_OS_WIN32
			mCustomUI_p->Draw(mPIWinRect.left, mPIWinRect.top, mPIWinRect.right, mPIWinRect.bottom);
#endif
		}
	}
	else
	{
		if (mCustomUI_p)
			mCustomUI_p->Close();
		if (mNoUIView_p != NULL)
			mNoUIView_p->SetEnable(false);
	}
	return;
}

//-----------------------------------------------------------------------------
long DfxPlugin::SetControlValue(long inControlIndex, long inValue)
{
	return static_cast<long>(CProcess::SetControlValue(inControlIndex, inValue));
}

//-----------------------------------------------------------------------------
long DfxPlugin::GetControlValue(long inControlIndex, long * outValue)
{
	return static_cast<long>(CProcess::GetControlValue(inControlIndex, outValue));
}

//-----------------------------------------------------------------------------
long DfxPlugin::GetControlDefaultValue(long inControlIndex, long * outValue)
{
	return static_cast<long>(CProcess::GetControlDefaultValue(inControlIndex, outValue));
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::UpdateControlGraphic(long inControlIndex, long inValue)
{
	if (mCustomUI_p)
		return mCustomUI_p->UpdateGraphicControl(inControlIndex, inValue);
	else
		return noErr;
}

//-----------------------------------------------------------------------------
int DfxPlugin::ProcessTouchControl(long inControlIndex)
{
	return static_cast<int>(CProcess::TouchControl(inControlIndex));
}

//-----------------------------------------------------------------------------
int DfxPlugin::ProcessReleaseControl(long inControlIndex)
{
	return static_cast<int>(CProcess::ReleaseControl(inControlIndex));
}

//-----------------------------------------------------------------------------
void DfxPlugin::ProcessDoIdle()
{
	this->DoIdle(NULL);

	gProcessGroup->YieldCriticalSection();
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::SetControlHighliteInfo(long inControlIndex, short inIsHighlighted, short inColor)
{
	if (inControlIndex == dfx::kParameterID_RTASGlobalBypass)
	{
		CProcess::SetControlHighliteInfo(inControlIndex, inIsHighlighted, inColor);
	}

	if (mCustomUI_p)
	{
		mCustomUI_p->SetControlHighlight(inControlIndex, inIsHighlighted, inColor);
	}

	return noErr;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::ChooseControl(Point inLocalCoord, long * outControlIndex)
{
	if (outControlIndex != NULL)
	{
		*outControlIndex = 0;
		if (mCustomUI_p)
			mCustomUI_p->GetControlIndexFromPoint(inLocalCoord.h, inLocalCoord.v, outControlIndex);
		return noErr;
	}
	else
		return paramErr;
}

#ifdef TARGET_API_AUDIOSUITE
//-----------------------------------------------------------------------------
// override this method to improve UI compatibility with non-Pro Tools hosts (like Media Composer)
void DfxPlugin::SetViewOrigin(Point anOrigin)
{
	TARGET_API_BASE_CLASS::SetViewOrigin(anOrigin);

	Rect ourPlugInViewRect;
	fOurPlugInView->GetRect(&ourPlugInViewRect);

	mLeftOffset = ourPlugInViewRect.left;
	mTopOffset = ourPlugInViewRect.top;
}
#endif

#endif  // TARGET_PLUGIN_HAS_GUI



#pragma mark -
#pragma mark EffectGroup
#pragma mark -

#ifndef TARGET_API_AUDIOSUITE

extern CEffectProcess* DFX_NewEffectProcess();
extern CEffectProcess* DFX_NewEffectProcessAS();

#if PT_SDK_VERSION < 0x08000000
enum
{
	plugInGestalt_SupportsRTElastic = pluginGestalt_WantsLoadSettingsFileCallback + 1,
	plugInGestalt_RequiresPitchData,
	pluginGestalt_WantsStateReset,
	plugInGestalt_SupportsControlChangesInThread,
	pluginGestalt_UseSmallPreviewBuffer
};
#endif

//-----------------------------------------------------------------------------
CProcessGroupInterface * CProcessGroup::CreateProcessGroup()
{
	return new DfxEffectGroup;
}

//-----------------------------------------------------------------------------
DfxEffectGroup::DfxEffectGroup()
{
	DefineManufacturerNamesAndID(PLUGIN_CREATOR_NAME_STRING"\n", PLUGIN_CREATOR_ID);
	DefinePlugInNamesAndVersion(PLUGIN_NAME_STRING, dfx::CompositePluginVersionNumberValue());

	AddGestalt(pluginGestalt_IsCacheable);
#if PT_SDK_VERSION >= 0x08000000	// XXX otherwise compiling with earlier SDK (stupidly) results in an assert
	AddGestalt(plugInGestalt_SupportsControlChangesInThread);
#endif
}

//-----------------------------------------------------------------------------
// XXX fix to honor channel configuration
void DfxEffectGroup::CreateEffectTypes()
{
	OSType effectTypeFourCharID = 0;
//	for (UInt32 i=1; i <= EffectLayerDef::MAX_NUM_CONNECTIONS; i++)
//	int maxNumChannels = SurroundFormatToNumChannels(ePlugIn_StemFormat_LastExplicitChoice);
//	for (int i=1; i <= maxNumChannels; i++)
	for (int i = static_cast<int>(ePlugIn_StemFormat_FirstExplicitChoice) - 1; i <= static_cast<int>(ePlugIn_StemFormat_LastExplicitChoice); i++)
	{
		effectTypeFourCharID = DFX_IterateAlphaNumericFourCharCode(effectTypeFourCharID);
		CEffectType * effectType;
		if (i < static_cast<int>(ePlugIn_StemFormat_FirstExplicitChoice))
		{
			effectType = new CEffectTypeAS(effectTypeFourCharID, PLUGIN_ID, PLUGIN_CATEGORY_RTAS);
//			effectType->AddGestalt(pluginGestalt_MultiInputModeOnly);	// XXX I see no reason to require this?
//			effectType->DefineGenericNumInputsAndOutputs(EffectLayerDef::MAX_NUM_CONNECTIONS, EffectLayerDef::MAX_NUM_CONNECTIONS);
effectType->DefineGenericNumInputsAndOutputs(2, 2);	// XXX hack for Lowender
			effectType->RemoveGestalt(pluginGestalt_SupportsStemFormats);	// XXX not allowed to support StemFormats when NOT doing MultiInputModeOnly
		}
		else
		{
if (i > ePlugIn_StemFormat_Stereo) break;	// XXX hack for Lowender, still need to implement real channel config handle
			effectType = new CEffectTypeRTAS(effectTypeFourCharID, PLUGIN_ID, PLUGIN_CATEGORY_RTAS);
			effectType->DefineStemFormats((EPlugIn_StemFormat)i, (EPlugIn_StemFormat)i);
		}
		dfx_AddEffectType(effectType);
	}
}

//-----------------------------------------------------------------------------
void DfxEffectGroup::Initialize()
{
	CEffectGroup::Initialize();

#if TARGET_PLUGIN_HAS_GUI
	CCustomView::AddNewViewProc("NoUIView", CreateCTemplateNoUIView);
#else
	// CGainMeterView doesn't have a RegisterView() method, so we have to do this stuff instead:
	CCustomView::AddNewViewProc("GainMeterView", CreateCGainMeterView);
	CCustomView::AddKeyword("labels", kConstantElem, CGainMeterView::kLabelKeyword);

	// necessary for the "minimal" generic GUI
	CCustomView::AddNewViewProc("ClippingView", CreateCClippingView);
	CStaticCustomText::RegisterView();
	CPictBackgroundTextEditor::RegisterView();
	CCustomView::AddNewViewProc("Slider", CreateCSliderControlEditor);
	CCustomView::AddNewViewProc("StateTextControlEditor", CreateCStateTextControlEditor);
	CCustomView::AddNewViewProc("TextCustomPopupEditor", CreateCTextCustomPopupEditor);
#endif
}

//-----------------------------------------------------------------------------
void DfxEffectGroup::dfx_AddEffectType(CEffectType * inEffectType)
{
	inEffectType->DefineTypeNames(PLUGIN_NAME_STRING);
	inEffectType->DefineSampleRateSupport(eSupports48kAnd96kAnd192k);
	inEffectType->AddGestalt(pluginGestalt_CanBypass);
	inEffectType->AddGestalt(pluginGestalt_SupportsClipMeter);
#if TARGET_PLUGIN_HAS_GUI
	inEffectType->AddGestalt(pluginGestalt_DoesNotUseDigiUI);
#endif

	long digiPluginType = 0;
	ComponentResult result = inEffectType->GetPlugInType(&digiPluginType);
	if (result == noErr)
	{
		switch (digiPluginType)
		{
			case pluginType_Host:	// RTAS
				inEffectType->AttachEffectProcessCreator(DFX_NewEffectProcess);
				inEffectType->AddGestalt(pluginGestalt_SupportsVariableQuanta);
//#if PT_SDK_VERSION >= 0x08000000	// XXX otherwise our ResetPlugInState callback doesn't actually get called
				inEffectType->AddGestalt(pluginGestalt_WantsStateReset);
//#endif
				break;
			case pluginType_ASP:	// AudioSuite
				inEffectType->AttachEffectProcessCreator(DFX_NewEffectProcessAS);
				inEffectType->AddGestalt(pluginGestalt_UseSmallPreviewBuffer);
				break;
			default:
				break;
		}
	}

	AddEffectType(inEffectType);
}

#endif
// !TARGET_API_AUDIOSUITE



#pragma mark -
#pragma mark utilities
#pragma mark -

#ifndef TARGET_API_AUDIOSUITE

//-----------------------------------------------------------------------------
// XXX This is a hack to generate an alpha-numeric four-char-codes for RTAS parameter 
// initialization cuz, if they are not alpha-numeric, then Digidesign's GUI library 
// chokes on them and fails to CreateMinimalGUI() (and possibly other things).  
// note:  This can only generate 14,776,336 values (about 1/290 the full 32-bit value range).
// also note:  If that maximum range of output values is exceeded, output values will wrap back around.
static OSType DFX_IterateAlphaNumericFourCharCode(OSType inPreviousCode)
{
	if (inPreviousCode)
	{
		OSType resultValue = inPreviousCode + 1;
		auto const bytesPointer = reinterpret_cast<char*>(&resultValue);
		while (true)
		{
			bool isValidCode = true;
			for (size_t i = 0; i < sizeof(resultValue); i++)
			{
				if (! isalnum(bytesPointer[i]) )
				{
					isValidCode = false;
					break;
				}
			}
			if (isValidCode)
				return resultValue;
			else
				resultValue++;
		}
	}
	else
		return '0000';
}

#endif
// !TARGET_API_AUDIOSUITE
