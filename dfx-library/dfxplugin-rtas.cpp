/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2009-2012  Sophia Poirier

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
This is where we connect the RTAS/AudioSuite API to our DfxPlugin system.
------------------------------------------------------------------------*/

#include "dfxplugin.h"

#include "CEffectTypeRTAS.h"
#include "CEffectTypeAS.h"
//#include "FicPluginEnums.h"
#include "PlugInUtils.h"

#include "CPluginControl_List.h"
#include "CPluginControl_OnOff.h"
#include "CPluginControl_LinearPercent.h"
#include "CPluginControl_LogPercent.h"
#include "CPluginControl_LinearTime.h"
#include "CPluginControl_LogTime.h"
#include "CPluginControl_LogTimeS.h"
#include "CPluginControl_LogFrequency.h"
#include "CPluginControl_LinearGain.h"
#include "CPluginControl_LogGain.h"
#include "CPluginControl_LinearBoostCut.h"
#include "CPluginControl_LinearThld.h"
#include "CPluginControl_LogQ.h"
#include "CPluginControl_LinearQ.h"

#ifndef TARGET_PLUGIN_USES_VSTGUI
	#include "CNoResourceView.h"
	#include "CStaticCustomText.h"
	#include "CGainMeterView.h"
	#include "CClippingView.h"
	#include "CPictBackgroundTextEditor.h"
	#include "CSliderControlEditor.h"
	#include "CStateTextControlEditor.h"
	#include "CTextCustomPopupEditor.h"
#endif



#pragma mark -
#pragma mark init
#pragma mark -

//-----------------------------------------------------------------------------
void DfxPlugin::EffectInit()
{
	dfx_PostConstructor();

	AddParametersToList();

	if ( IsAS() )
	{
		inputAudioStreams_as = (float**) malloc(GetNumOutputs() * sizeof(*inputAudioStreams_as));
		outputAudioStreams_as = (float**) malloc(GetNumOutputs() * sizeof(*outputAudioStreams_as));
		for (SInt32 ch=0; ch < GetNumOutputs(); ch++)
		{
			if (inputAudioStreams_as != NULL)
				inputAudioStreams_as[ch] = NULL;
			if (outputAudioStreams_as != NULL)
				outputAudioStreams_as[ch] = NULL;
		}
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
	dfx_PreDestructor();

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

#ifdef TARGET_PLUGIN_USES_VSTGUI
	if (mCustomUI_p != NULL)
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
	const OSType masterBypassFourCharID = 'bypa';

	AddControl( new CPluginControl_OnOff(masterBypassFourCharID, "Master Bypass\nM Bypass\nMByp\nByp", false, true) );
	DefineMasterBypassControlIndex(kDFXParameterID_RTASMasterBypass);

	OSType paramFourCharID = 0;
	for (long i=0; i < numParameters; i++)
	{
		if (! parameterisvalid(i) )
			continue;	// XXX eh actually maybe we need to add a parameter for every index?

		const double paramMin_f = getparametermin_f(i), paramMax_f = getparametermax_f(i), paramDefault_f = getparameterdefault_f(i);
		const int64_t paramMin_i = getparametermin_i(i), paramMax_i = getparametermax_i(i), paramDefault_i = getparameterdefault_i(i);
		char paramName[DFX_PARAM_MAX_NAME_LENGTH];
		paramName[0] = 0;
		getparametername(i, paramName);

		paramFourCharID = DFX_IterateAlphaNumericFourCharCode(paramFourCharID);
		// make sure to skip over the ID already used for master bypass
		if (paramFourCharID == masterBypassFourCharID)
			paramFourCharID = DFX_IterateAlphaNumericFourCharCode(paramFourCharID);
		const bool paramAutomatable = (getparameterattributes(i) & (kDfxParamAttribute_hidden|kDfxParamAttribute_unused)) ? false : true;
		const int numCurvedSteps = 1000;
		const double stepSize_default = (paramMax_f - paramMin_f) / (double)numCurvedSteps;
		const DfxParamCurve paramCurve = getparametercurve(i);
		const double paramCurveSpec = getparametercurvespec(i);
		const bool paramIsLog = (paramCurve == kDfxParamCurve_log) || (paramCurve == kDfxParamCurve_exp);

		if ( getparameterusevaluestrings(i) )
		{
			std::vector<std::string> valueStringsVector;
			for (int64_t stringIndex=paramMin_i; stringIndex <= paramMax_i; stringIndex++)
			{
				const char * valueString = getparametervaluestring_ptr(i, stringIndex);
				if (valueString != NULL)
					valueStringsVector.push_back( std::string(valueString) );
			}
			AddControl( new CPluginControl_List(paramFourCharID, paramName, valueStringsVector, paramDefault_i - paramMin_i, paramAutomatable) );
		}

		else if (getparametervaluetype(i) == kDfxParamValueType_int)
		{
			AddControl( new CPluginControl_Discrete(paramFourCharID, paramName, paramMin_i, paramMax_i, paramDefault_i, paramAutomatable) );
		}

		else if (getparametervaluetype(i) == kDfxParamValueType_boolean)
		{
			AddControl( new CPluginControl_OnOff(paramFourCharID, paramName, getparameterdefault_b(i), paramAutomatable) );
		}

		else
		{
			switch ( getparameterunit(i) )
			{
				case kDfxParamUnit_percent:
				case kDfxParamUnit_drywetmix:
					if (paramIsLog)
						AddControl( new CPluginControl_LogPercent(paramFourCharID, paramName, paramMin_f*kDFX_RTASPercentScalar, paramMax_f*kDFX_RTASPercentScalar, numCurvedSteps, paramDefault_f*kDFX_RTASPercentScalar, paramAutomatable) );
					else
						AddControl( new CPluginControl_LinearPercent(paramFourCharID, paramName, paramMin_f*kDFX_RTASPercentScalar, paramMax_f*kDFX_RTASPercentScalar, stepSize_default*kDFX_RTASPercentScalar, paramDefault_f*kDFX_RTASPercentScalar, paramAutomatable) );
					break;

				case kDfxParamUnit_ms:
					if (paramIsLog)
						AddControl( new CPluginControl_LogTime(paramFourCharID, paramName, paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
					else
						AddControl( new CPluginControl_LinearTime(paramFourCharID, paramName, paramMin_f, paramMax_f, stepSize_default, paramDefault_f, paramAutomatable) );
					break;

				case kDfxParamUnit_seconds:
					// XXX not actually really sure whether CPluginControl_TimeS is for seconds? or maybe samples?
					AddControl( new CPluginControl_LogTimeS(paramFourCharID, paramName, paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
					break;

				case kDfxParamUnit_hz:
					if (paramCurve == kDfxParamCurve_log)
						AddControl( new CPluginControl_LogFrequency(paramFourCharID, paramName, paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
					else
						AddControl( new CPluginControl_DfxCurvedFrequency(paramFourCharID, paramName, paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable, paramCurve, paramCurveSpec) );
					break;

				case kDfxParamUnit_decibles:
					// XXX or perhaps use CPluginControl_Level, or split the two out from each other?
					AddControl( new CPluginControl_LinearBoostCut(paramFourCharID, paramName, paramMin_f, paramMax_f, stepSize_default, paramDefault_f, paramAutomatable) );
					break;

				case kDfxParamUnit_lineargain:
					if (paramIsLog)
						AddControl( new CPluginControl_LogGain(paramFourCharID, paramName, paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
					else
						AddControl( new CPluginControl_LinearGain(paramFourCharID, paramName, paramMin_f, paramMax_f, stepSize_default, paramDefault_f, paramAutomatable) );
					break;

				default:
					{
						if (paramIsLog)
							AddControl( new CPluginControl_Log(paramFourCharID, paramName, paramMin_f, paramMax_f, numCurvedSteps, paramDefault_f, paramAutomatable) );
						else
							AddControl( new CPluginControl_Linear(paramFourCharID, paramName, paramMin_f, paramMax_f, stepSize_default, paramDefault_f, paramAutomatable) );
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
	if (inNameLength <= kParameterShortNameMax_rtas)
	{
		char * shortNameString = NULL;
		if (inParameterIndex == kDFXParameterID_RTASMasterBypass)
			shortNameString = "Bypass";	// any shortened version of this is fine
		else
			shortNameString = GetParameterShortName( DFX_ParameterID_FromRTAS(inParameterIndex) );
		if (shortNameString != NULL)
		{
			strncpy(outName, shortNameString, inNameLength);
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
	if (inMaxLength <= kParameterValueShortNameMax_rtas)
	{
		const char * shortValueString = GetParameterValueShortString(DFX_ParameterID_FromRTAS(inParameterIndex), inValue);
		if (shortValueString != NULL)
		{
			strncpy((char*)(outValueString+1), shortValueString, inMaxLength);
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
	if (inParameterIndex == kDFXParameterID_RTASMasterBypass)
	{
		mMasterBypass_rtas = dynamic_cast<CPluginControl_Discrete*>(GetControl(inParameterIndex))->GetDiscrete();
		return;
	}

	inParameterIndex = DFX_ParameterID_FromRTAS(inParameterIndex);
	if (! parameterisvalid(inParameterIndex) )
		return;

	switch ( getparametervaluetype(inParameterIndex) )
	{
		case kDfxParamValueType_float:
			parameters[inParameterIndex].set_f( GetParameter_f_FromRTAS(inParameterIndex) );
			break;
		case kDfxParamValueType_int:
			parameters[inParameterIndex].set_i( GetParameter_i_FromRTAS(inParameterIndex) );
			break;
		case kDfxParamValueType_boolean:
			parameters[inParameterIndex].set_b( GetParameter_b_FromRTAS(inParameterIndex) );
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
double DfxPlugin::GetParameter_f_FromRTAS(long inParameterID)
{
	double resultValue = dynamic_cast<CPluginControl_Continuous*>(GetControl(DFX_ParameterID_ToRTAS(inParameterID)))->GetContinuous();
	if ( (getparameterunit(inParameterID) == kDfxParamUnit_percent) || (getparameterunit(inParameterID) == kDfxParamUnit_drywetmix) )
		resultValue /= kDFX_RTASPercentScalar;
	return resultValue;
}

//-----------------------------------------------------------------------------
int64_t DfxPlugin::GetParameter_i_FromRTAS(long inParameterID)
{
	int64_t resultValue = dynamic_cast<CPluginControl_Discrete*>(GetControl(DFX_ParameterID_ToRTAS(inParameterID)))->GetDiscrete();
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

	// XXX test this first, since DFX_ParameterID_FromRTAS() makes it an invalid ID
	if (inControlIndex == kDFXParameterID_RTASMasterBypass)
	{
		*outItIs = 1;
		return noErr;
	}

	inControlIndex = DFX_ParameterID_FromRTAS(inControlIndex);
	if (! parameterisvalid(inControlIndex) )
		return paramErr;

	if ( getparameterattributes(inControlIndex) & (kDfxParamAttribute_unused|kDfxParamAttribute_hidden) )
		*outItIs = 0;
	else
		*outItIs = 1;
	return noErr;
}

#ifndef TARGET_API_AUDIOSUITE

#include "SliderConversions.h"
#include "PlugInAssert.h"

//-----------------------------------------------------------------------------
CPluginControl_DfxCurved::CPluginControl_DfxCurved(OSType id, const char * name, double min, double max, 
		int numSteps, double defaultValue, bool isAutomatable, 
		DfxParamCurve inCurve, double inCurveSpec, 
		const PrefixDictionaryEntry * begin, const PrefixDictionaryEntry * end)
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
	continuous = DFX_ContractParameterValue(continuous, GetMin(), GetMax(), mCurve, mCurveSpec);
	return DoubleToLongControl(continuous, 0.0, 1.0);
}

//-----------------------------------------------------------------------------
double CPluginControl_DfxCurved::ConvertControlToContinuous(long control) const
{
	double continous = LongControlToDouble(control, 0.0, 1.0);
	return DFX_ExpandParameterValue(continous, GetMin(), GetMax(), mCurve, mCurveSpec);
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
UInt32 DfxPlugin::ProcessAudio(bool inIsMasterBypassed)
{
	if (! IsAS() )
		return 0;

	long totalInputSamples = 0;	// total number of input samples in one input buffer

	// use the mono channel input sample number (guaranteed to be connected)
	if (GetInputConnection(0) != NULL)
		totalInputSamples = GetInputConnection(0)->mNumSamplesInBuf;

	for (SInt32 ch=0; ch < GetNumOutputs(); ch++)
	{
		// XXX what to do if there are no valid connections for the channel?
		inputAudioStreams_as[ch] = NULL;
		outputAudioStreams_as[ch] = NULL;

		DAEConnectionPtr outputConnection = GetOutputConnection(ch);
		if (outputConnection != NULL)	// if no valid connection, don't do anything
		{
			outputAudioStreams_as[ch] = (float*)(outputConnection->mBuffer);

			DAEConnectionPtr inputConnection = GetInputConnection(ch);
			if (inputConnection != NULL)	// have a valid input connection
				inputAudioStreams_as[ch] = (float*)(inputConnection->mBuffer);
			else	// no input connection; use default value of zero
			{
				// (re)allocate the zero audio buffer if it is not currently large enough for this rendering slice
				if (numZeroAudioBufferSamples < totalInputSamples)
				{
					if (zeroAudioBuffer != NULL)
						free(zeroAudioBuffer);
					numZeroAudioBufferSamples = totalInputSamples;
					zeroAudioBuffer = (float*) malloc(numZeroAudioBufferSamples * sizeof(*zeroAudioBuffer));
					if (zeroAudioBuffer != NULL)
					{
						for (long i=0; i < numZeroAudioBufferSamples; i++)
							zeroAudioBuffer[i] = 0.0f;
					}
				}
				inputAudioStreams_as[ch] = zeroAudioBuffer;
			}
			
			if (inIsMasterBypassed)
			{
				for (long i=0; i < totalInputSamples; i++)
					outputAudioStreams_as[ch][i] = inputAudioStreams_as[ch][i];
			}
			// do the sample number adjustment
			outputConnection->mNumSamplesInBuf = totalInputSamples;
			if (inputConnection != NULL)
				inputConnection->mNumSamplesInBuf = 0;
		}
	}

	if (!inIsMasterBypassed)
		RenderAudio(inputAudioStreams_as, outputAudioStreams_as, totalInputSamples);

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

	SInt32 channel;
	long samp;

	// RTAS clip monitoring
	for (channel=0; (channel < GetNumInputs()) && !fClipped; channel++)
	{
		if (inAudioStreams[channel] == NULL)	// XXX possible in AudioSuite
			continue;
		for (samp=0; samp < inNumFramesToProcess; samp++)
		{
			if (fabsf(inAudioStreams[channel][samp]) > 1.0f)
			{
				fClipped = true;
				break;
			}
		}
	}

	for (channel=0; channel < GetNumOutputs(); channel++)
	{
		if ( (inAudioStreams[channel] == NULL) || (outAudioStreams[channel] == NULL) )	// XXX possible in AudioSuite
			continue;
		if (mMasterBypass_rtas)
		{
			SInt32 inputChannelIndex = (GetNumInputs() < GetNumOutputs()) ? (GetNumInputs() - 1) : channel;
			for (samp=0; samp < inNumFramesToProcess; samp++)
				outAudioStreams[channel][samp] = inAudioStreams[inputChannelIndex][samp];
		}
		else
		{
#if TARGET_PLUGIN_USES_DSPCORE
			if (dspcores[channel] != NULL)
				dspcores[channel]->do_process(inAudioStreams[channel], outAudioStreams[channel], (unsigned)inNumFramesToProcess, true);
#endif
		}
	}
#if !TARGET_PLUGIN_USES_DSPCORE
	if (!mMasterBypass_rtas)
		processaudio((const float**)inAudioStreams, outAudioStreams, (unsigned)inNumFramesToProcess, true);
#endif

	// RTAS clip monitoring
	for (channel=0; (channel < GetNumOutputs()) && !fClipped; channel++)
	{
		if (outAudioStreams[channel] == NULL)	// XXX possible in AudioSuite
			continue;
		for (samp=0; samp < inNumFramesToProcess; samp++)
		{
			if (fabsf(outAudioStreams[channel][samp]) > 1.0f)
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
#ifdef TARGET_PLUGIN_USES_VSTGUI
	CNoResourceView * ui = NULL;
	try
	{
		if (mCustomUI_p == NULL)
		{
			mCustomUI_p = CreateCTemplateCustomUI(this);
			mCustomUI_p->GetRect( &(mPIWinRect.left), &(mPIWinRect.top), &(mPIWinRect.right), &(mPIWinRect.bottom) );
		}

		ui = new CNoResourceView;
#if PT_SDK_VERSION < 0x08000000
		const long viewSizeOffset = 1;	// XXX +1 because of a bug where CNoResourceView::SetSize() subtracts 1 from width and height
#else
		const long viewSizeOffset = 0;	// the bug is fixed in the Pro Tools 8 SDK
#endif
		ui->SetSize(mPIWinRect.right+viewSizeOffset, mPIWinRect.bottom+viewSizeOffset);

		mNoUIView_p = (CTemplateNoUIView *) ui->AddView2("!NoUIView[('NoID')]", 0, 0, mPIWinRect.right, mPIWinRect.bottom, false);

		if (mNoUIView_p != NULL)
			mNoUIView_p->SetCustomUI(mCustomUI_p);
	}
	catch (...)
	{
		if (mCustomUI_p != NULL)
			delete mCustomUI_p;
		mCustomUI_p = NULL;
		if (ui != NULL)
			delete ui;
		ui = NULL;
	}

	return ui;

#else
	OSType meterFourCharID = 0;
	OSType meterIDs[EffectLayerDef::MAX_NUM_CONNECTIONS];
	for (UInt32 i=0; i < EffectLayerDef::MAX_NUM_CONNECTIONS; i++)
	{
		meterFourCharID = DFX_IterateAlphaNumericFourCharCode(meterFourCharID);
		meterIDs[i] = meterFourCharID;
	}

	OSType clipIDs[EffectLayerDef::MAX_NUM_CONNECTIONS];
	for (UInt32 i=0; i < EffectLayerDef::MAX_NUM_CONNECTIONS; i++)
	{
		meterFourCharID = DFX_IterateAlphaNumericFourCharCode(meterFourCharID);
		clipIDs[i] = meterFourCharID;
	}

	return CreateMinimalGUI(this, meterIDs, clipIDs);
#endif
}

#ifdef TARGET_PLUGIN_USES_VSTGUI

//-----------------------------------------------------------------------------
void DfxPlugin::GetViewRect(Rect * outViewRect)
{
	if (mCustomUI_p == NULL)
		mCustomUI_p = CreateCTemplateCustomUI(this);
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
		if (mCustomUI_p != NULL)
		{
#if WINDOWS_VERSION
			Rect aRect;
			GetPortBounds(inPort, &aRect);
			if ( ((aRect.left <= 0) && (aRect.top <= 0)) && ((mLeftOffset == 0) && (mTopOffset == 0)) )
			{
				mLeftOffset = -aRect.left;
				mTopOffset = -aRect.top;
			}
#endif

			void * windowPtr = NULL;
			#if WINDOWS_VERSION
				windowPtr = (void*) ASI_GethWnd((WindowPtr)mMainPort);
			#elif MAC_VERSION
				windowPtr = (void*) GetWindowFromPort(mMainPort);
			#endif
			mCustomUI_p->Open(windowPtr, mLeftOffset, mTopOffset);

			if (mNoUIView_p != NULL)
				mNoUIView_p->SetEnable(true);

#if WINDOWS_VERSION
			mCustomUI_p->Draw(mPIWinRect.left, mPIWinRect.top, mPIWinRect.right, mPIWinRect.bottom);
#endif
		}
	}
	else
	{
		if (mCustomUI_p != NULL)
			mCustomUI_p->Close();
		if (mNoUIView_p != NULL)
			mNoUIView_p->SetEnable(false);
	}
	return;
}

//-----------------------------------------------------------------------------
long DfxPlugin::SetControlValue(long inControlIndex, long inValue)
{
	return (long)CProcess::SetControlValue(inControlIndex, inValue);
}

//-----------------------------------------------------------------------------
long DfxPlugin::GetControlValue(long inControlIndex, long * outValue)
{
	return (long)CProcess::GetControlValue(inControlIndex, outValue);
}

//-----------------------------------------------------------------------------
long DfxPlugin::GetControlDefaultValue(long inControlIndex, long * outValue)
{
	return (long)CProcess::GetControlDefaultValue(inControlIndex, outValue);
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::UpdateControlGraphic(long inControlIndex, long inValue)
{
	if (mCustomUI_p != NULL)
		return mCustomUI_p->UpdateGraphicControl(inControlIndex, inValue);
	else
		return noErr;
}

//-----------------------------------------------------------------------------
int DfxPlugin::ProcessTouchControl(long inControlIndex)
{
	return (int)CProcess::TouchControl(inControlIndex);
}

//-----------------------------------------------------------------------------
int DfxPlugin::ProcessReleaseControl(long inControlIndex)
{
	return (int)CProcess::ReleaseControl(inControlIndex);
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
	if (inControlIndex == kDFXParameterID_RTASMasterBypass)
		CProcess::SetControlHighliteInfo(inControlIndex, inIsHighlighted, inColor);

	if (mCustomUI_p != NULL)
		mCustomUI_p->SetControlHighlight(inControlIndex, inIsHighlighted, inColor);

	return noErr;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::ChooseControl(Point inLocalCoord, long * outControlIndex)
{
	if (outControlIndex != NULL)
	{
		*outControlIndex = 0;
		if (mCustomUI_p != NULL)
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

#endif
// TARGET_PLUGIN_USES_VSTGUI



#pragma mark -
#pragma mark EffectGroup
#pragma mark -

#ifndef TARGET_API_AUDIOSUITE

extern CEffectProcess * DFX_NewEffectProcess();
extern CEffectProcess * DFX_NewEffectProcessAS();

#if PT_SDK_VERSION < 0x08000000
enum {
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
	DefinePlugInNamesAndVersion(PLUGIN_NAME_STRING, DFX_CompositePluginVersionNumberValue());

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
	for (int i=(int)ePlugIn_StemFormat_FirstExplicitChoice-1; i <= (int)ePlugIn_StemFormat_LastExplicitChoice; i++)
	{
		effectTypeFourCharID = DFX_IterateAlphaNumericFourCharCode(effectTypeFourCharID);
		CEffectType * effectType;
		if (i < (int)ePlugIn_StemFormat_FirstExplicitChoice)
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

#ifdef TARGET_PLUGIN_USES_VSTGUI
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
#ifdef TARGET_PLUGIN_USES_VSTGUI
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
OSType DFX_IterateAlphaNumericFourCharCode(OSType inPreviousCode)
{
	if (inPreviousCode)
	{
		OSType resultValue = inPreviousCode + 1;
		const char * bytesPointer = (char*)(&resultValue);
		while (true)
		{
			bool isValidCode = true;
			for (size_t i=0; i < sizeof(resultValue); i++)
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
