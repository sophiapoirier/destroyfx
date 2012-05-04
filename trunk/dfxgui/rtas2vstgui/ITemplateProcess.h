#pragma once


#include "dfxparameter.h"
#include "dfxpluginproperties.h"


//-----------------------------------------------------------------------------
class ITemplateProcess
{
public:
	ITemplateProcess() {}
	virtual ~ITemplateProcess() {}

	virtual long SetControlValue(long aControlIndex, long aValue) = 0;
	virtual long GetControlValue(long aControlIndex, long * aValue) = 0;
	virtual long GetControlDefaultValue(long aControlIndex, long * aValue) = 0;

	virtual void setEditor(void * editor) = 0;

	virtual int ProcessTouchControl(long aControlIndex) = 0;
	virtual int ProcessReleaseControl(long aControlIndex) = 0;
	virtual void ProcessDoIdle() = 0;
	virtual void * ProcessGetModuleHandle() = 0;
	virtual short ProcessUseResourceFile() = 0;
	virtual void ProcessRestoreResourceFile(short resFile) = 0;

	// Destroy FX interface
	virtual long getnumparameters() = 0;
	virtual unsigned long getnumoutputs() = 0;
	virtual double getparameter_f(long inParameterID) = 0;
	virtual int64_t getparameter_i(long inParameterID) = 0;
	virtual bool getparameter_b(long inParameterID) = 0;
	virtual double getparametermin_f(long inParameterID) = 0;
	virtual double getparametermax_f(long inParameterID) = 0;
	virtual double getparameterdefault_f(long inParameterIndex) = 0;
	virtual void getparametername(long inParameterIndex, char * outText) = 0;
	virtual DfxParamValueType getparametervaluetype(long inParameterIndex) = 0;
	virtual DfxParamUnit getparameterunit(long inParameterIndex) = 0;
	virtual bool getparametervaluestring(long inParameterIndex, int64_t inStringIndex, char * outText) = 0;
	virtual void getparameterunitstring(long inParameterIndex, char * outText) = 0;
	virtual double expandparametervalue(long inParameterIndex, double inValue) = 0;
	virtual double contractparametervalue(long inParameterIndex, double inValue) = 0;
	virtual long dfx_GetPropertyInfo(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
										size_t & outDataSize, DfxPropertyFlags & outFlags) = 0;
	virtual long dfx_GetProperty(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
									void * outData) = 0;
	virtual long dfx_SetProperty(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
									const void * inData, size_t inDataSize) = 0;
};
