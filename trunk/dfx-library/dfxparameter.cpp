/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for doing all kinds of fancy plugin parameter stuff.
written by Sophia Poirier, October 2002
------------------------------------------------------------------------*/


#include "dfxparameter.h"

#include <stdlib.h>	// for malloc and free and RAND_MAX
#include <string.h>	// for strcpy
#include <math.h>


// this is a twiddly value for when casting with decimal types
const double twiddle = 0.001;



#pragma mark -
#pragma mark init
#pragma mark -

//-----------------------------------------------------------------------------
DfxParam::DfxParam()
{
	name = NULL;
	name = (char*) malloc(sizeof(char) * DFX_PARAM_MAX_NAME_LENGTH);
	name[0] = 0;	// empty string
	#ifdef TARGET_API_AUDIOUNIT
		cfname = NULL;
	#endif

	// these are null until it's shown that we'll actually need them
	valueStrings = NULL;
	#ifdef TARGET_API_AUDIOUNIT
		valueCFStrings = NULL;
	#endif
	numAllocatedValueStrings = 0;
	customUnitString = NULL;

	// default to not using any custom value display
	useValueStrings = false;

	// default to allowing values outside of the min/max range
	enforceValueLimits = false;

	// default this stuff empty-style
	valueType = kDfxParamValueType_float;
	unit = kDfxParamUnit_generic;
	curve = kDfxParamCurve_linear;
	curvespec = 1.0;
	changed = false;
	touched = false;
	attributes = 0;
}

//-----------------------------------------------------------------------------
DfxParam::~DfxParam()
{
	// release the parameter value strings, if any
	releaseValueStrings();

	// release the parameter name
	if (name != NULL)
		free(name);
	name = NULL;
	#ifdef TARGET_API_AUDIOUNIT
		if (cfname != NULL)
			CFRelease(cfname);
		cfname = NULL;
	#endif

	// release the custom unit name string, if there is one
	if (customUnitString != NULL)
		free(customUnitString);
	customUnitString = NULL;
}


//-----------------------------------------------------------------------------
void DfxParam::init(const char * inName, DfxParamValueType inType, 
                    DfxParamValue inInitialValue, DfxParamValue inDefaultValue, 
                    DfxParamValue inMinValue, DfxParamValue inMaxValue, 
                    DfxParamUnit inUnit, DfxParamCurve inCurve)
{
	// accept all of the incoming init values
	valueType = inType;
	value = oldValue = inInitialValue;
	defaultValue = inDefaultValue;
	min = inMinValue;
	max = inMaxValue;
	if (inName != NULL)
	{
		strncpy(name, inName, DFX_PARAM_MAX_NAME_LENGTH);
		name[DFX_PARAM_MAX_NAME_LENGTH-1] = 0;
		#ifdef TARGET_API_AUDIOUNIT
			cfname = CFStringCreateWithCString(kCFAllocatorDefault, inName, kDFX_DefaultCStringEncoding);
		#endif
	}
	curve = inCurve;
	unit = inUnit;
	if (unit == kDfxParamUnit_index)
		SetEnforceValueLimits(true);	// make sure not to go out of any array bounds
	changed = true;


	// do some checks to make sure that the min and max are not swapped 
	// and that the default value is between the min and max
	switch (inType)
	{
		case kDfxParamValueType_float:
			if (min.f > max.f)
			{
				double swap = max.f;
				max.f = min.f;
				min.f = swap;
			}
			if ( (defaultValue.f > max.f)  || (defaultValue.f < min.f) )
				defaultValue.f = ((max.f - min.f) * 0.5) + min.f;
			break;
		case kDfxParamValueType_int:
			if (min.i > max.i)
			{
				int64_t swap = max.i;
				max.i = min.i;
				min.i = swap;
			}
			if ( (defaultValue.i > max.i)  || (defaultValue.i < min.i) )
				defaultValue.i = ((max.i - min.i) / 2) + min.i;
			break;
		case kDfxParamValueType_boolean:
			min.f = false;
			max.f = true;
			if ( (defaultValue.b > max.b)  || (defaultValue.b < min.b) )
				defaultValue.b = false;
			break;
		default:
			break;
	}

	// now squeeze the current value within range, if necessary/desired
	limit();
}


//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with float variable type
void DfxParam::init_f(const char * inName, double inInitialValue, double inDefaultValue, 
							double inMinValue, double inMaxValue, 
							DfxParamUnit inUnit, DfxParamCurve inCurve)
{
	DfxParamValue val, def, mn, mx;
	val.f = inInitialValue;
	def.f = inDefaultValue;
	mn.f = inMinValue;
	mx.f = inMaxValue;
	init(inName, kDfxParamValueType_float, val, def, mn, mx, inUnit, inCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with int variable type
void DfxParam::init_i(const char * inName, int64_t inInitialValue, int64_t inDefaultValue, 
							int64_t inMinValue, int64_t inMaxValue, 
							DfxParamUnit inUnit, DfxParamCurve inCurve)
{
	DfxParamValue val, def, mn, mx;
	val.i = inInitialValue;
	def.i = inDefaultValue;
	mn.i = inMinValue;
	mx.i = inMaxValue;
	init(inName, kDfxParamValueType_int, val, def, mn, mx, inUnit, inCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with boolean variable type
void DfxParam::init_b(const char * inName, bool inInitialValue, bool inDefaultValue, 
							DfxParamUnit inUnit)
{
	DfxParamValue val, def, mn, mx;
	val.b = inInitialValue;
	def.b = inDefaultValue;
	mn.b = false;
	mx.b = true;
	init(inName, kDfxParamValueType_boolean, val, def, mn, mx, inUnit, kDfxParamCurve_linear);
}

//-----------------------------------------------------------------------------
void DfxParam::releaseValueStrings()
{
	// release the parameter value strings, if any
	if (valueStrings != NULL)
	{
		for (int64_t i=0; i < numAllocatedValueStrings; i++)
		{
			if (valueStrings[i] != NULL)
				free(valueStrings[i]);
			valueStrings[i] = NULL;
		}
		free(valueStrings);
	}
	valueStrings = NULL;

	#ifdef TARGET_API_AUDIOUNIT
		// release the CFString versions of the parameter value strings, if any
		if (valueCFStrings != NULL)
		{
			for (int64_t i=0; i < numAllocatedValueStrings; i++)
			{
				if (valueCFStrings[i] != NULL)
					CFRelease(valueCFStrings[i]);
				valueCFStrings[i] = NULL;
			}
			free(valueCFStrings);
		}
		valueCFStrings = NULL;
	#endif
}

//-----------------------------------------------------------------------------
// set a value string's text contents
void DfxParam::setusevaluestrings(bool inNewMode)
{
	useValueStrings = inNewMode;

	// if we're using value strings, initialize
	if (inNewMode)
	{
		// release any value strings arrays that may have previously been allocated
		releaseValueStrings();

		// determine how many items there are in the array from the parameter value range
		numAllocatedValueStrings = getmax_i() - getmin_i() + 1;
		valueStrings = (char**) malloc(numAllocatedValueStrings * sizeof(char*));
		for (int64_t i=0; i < numAllocatedValueStrings; i++)
		{
			valueStrings[i] = (char*) malloc(DFX_PARAM_MAX_VALUE_STRING_LENGTH * sizeof(char));
			valueStrings[i][0] = 0;	// default to empty strings
		}

		#ifdef TARGET_API_AUDIOUNIT
			valueCFStrings = (CFStringRef*) malloc(numAllocatedValueStrings * sizeof(CFStringRef));
			for (int64_t i=0; i < numAllocatedValueStrings; i++)
				valueCFStrings[i] = NULL;
		#endif

		SetEnforceValueLimits(true);	// make sure not to go out of any array bounds
	}
}

//-----------------------------------------------------------------------------
// set a value string's text contents
bool DfxParam::setvaluestring(int64_t inIndex, const char * inText)
{
	if ( !ValueStringIndexIsValid(inIndex) )
		return false;

	if (inText == NULL)
		return false;

	// the actual index of the array is the incoming index 
	// minus the parameter's minimum value
	int64_t arrayIndex = inIndex - getmin_i();
	strncpy(valueStrings[arrayIndex], inText, DFX_PARAM_MAX_VALUE_STRING_LENGTH);
	valueStrings[arrayIndex][DFX_PARAM_MAX_VALUE_STRING_LENGTH-1] = 0;

	#ifdef TARGET_API_AUDIOUNIT
		if (valueCFStrings[arrayIndex] != NULL)
			CFRelease(valueCFStrings[arrayIndex]);
		// convert the incoming text to a CFString
		valueCFStrings[arrayIndex] = CFStringCreateWithCString(kCFAllocatorDefault, inText, kDFX_DefaultCStringEncoding);
	#endif

	return true;
}

//-----------------------------------------------------------------------------
// get a copy of the contents of a specific value string
bool DfxParam::getvaluestring(int64_t inIndex, char * outText)
{
	char * text = getvaluestring_ptr(inIndex);

	if (text == NULL)
		return false;

	strcpy(outText, text);
	return true;
}

//-----------------------------------------------------------------------------
// get a copy of the pointer to a specific value string
char * DfxParam::getvaluestring_ptr(int64_t inIndex)
{
	if ( !ValueStringIndexIsValid(inIndex) )
		return NULL;

	return valueStrings[inIndex - getmin_i()];
}

//-----------------------------------------------------------------------------
// safety check for an index into the value strings array
bool DfxParam::ValueStringIndexIsValid(int64_t inIndex)
{
	if ( !useValueStrings )
		return false;
	if (valueStrings == NULL)
		return false;
	if ( (inIndex < getmin_i()) || (inIndex > getmax_i()) )
		return false;
	// XXX should I rethink this one?
	if (valueStrings[inIndex - getmin_i()] == NULL)
		return false;

	return true;
}



#pragma mark -
#pragma mark gettingvalues
#pragma mark -

//-----------------------------------------------------------------------------
// figure out the value of a DfxParamValue as float type value
// perform type conversion if float is not the parameter's "native" type
double DfxParam::derive_f(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			return inValue.f;
		case kDfxParamValueType_int:
			return (double) inValue.i;
		case kDfxParamValueType_boolean:
			return (inValue.b != 0) ? 1.0 : 0.0;
		default:
			return 0.0;
	}
}

//-----------------------------------------------------------------------------
// figure out the value of a DfxParamValue as int type value
// perform type conversion if int is not the parameter's "native" type
int64_t DfxParam::derive_i(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			if (inValue.f < 0.0)
				return (int64_t) (inValue.f - twiddle);
			else
				return (int64_t) (inValue.f + twiddle);
		case kDfxParamValueType_int:
			return inValue.i;
		case kDfxParamValueType_boolean:
			return (inValue.b != 0) ? 1 : 0;
		default:
			return 0;
	}
}

//-----------------------------------------------------------------------------
// figure out the value of a DfxParamValue as boolean type value
// perform type conversion if boolean is not the parameter's "native" type
bool DfxParam::derive_b(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			return DBOOL(inValue.f);
		case kDfxParamValueType_int:
			return (inValue.i != 0);
		case kDfxParamValueType_boolean:
			return (inValue.b != 0);
		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// take a real parameter value and contract it to a generic 0.0 to 1.0 float value
// this takes into account the parameter curve
// XXX this is being obsoleted by the non-class contractparametervalue() function
double DfxParam::contract(double inLiteralValue)
{
	return contractparametervalue(inLiteralValue, getmin_f(), getmax_f(), curve, curvespec);
}

//-----------------------------------------------------------------------------
// take a real parameter value and contract it to a generic 0.0 to 1.0 float value
// this takes into account the parameter curve
double contractparametervalue(double inLiteralValue, double minValue, double maxValue, DfxParamCurve curveType, double curveSpec)
{
	double valueRange = maxValue - minValue;
	static const double oneDivThree = 1.0 / 3.0;
	static const double logTwo = log(2.0);

	switch (curveType)
	{
		case kDfxParamCurve_linear:
			return (inLiteralValue-minValue) / valueRange;
		case kDfxParamCurve_stepped:
			// XXX is this a good way to do this?
			return (inLiteralValue-minValue) / valueRange;
		case kDfxParamCurve_sqrt:
			return (sqrt(inLiteralValue) * valueRange) + minValue;
		case kDfxParamCurve_squared:
			return sqrt((inLiteralValue-minValue) / valueRange);
		case kDfxParamCurve_cubed:
			return pow( (inLiteralValue-minValue) / valueRange, oneDivThree );
		case kDfxParamCurve_pow:
			return pow( (inLiteralValue-minValue) / valueRange, 1.0/curveSpec );
		case kDfxParamCurve_exp:
			return log(1.0-minValue+inLiteralValue) / log(1.0-minValue+maxValue);
		case kDfxParamCurve_log:
			return (log(inLiteralValue/minValue) / logTwo) / (log(maxValue/minValue) / logTwo);
		default:
			break;
	}

	return inLiteralValue;
}

//-----------------------------------------------------------------------------
// get the parameter's current value scaled into a generic 0...1 float value
double DfxParam::get_gen()
{
	return contractparametervalue(get_f(), getmin_f(), getmax_f(), curve, curvespec);
}



#pragma mark -
#pragma mark setting values
#pragma mark -

//-----------------------------------------------------------------------------
// set a DfxParamValue with a value of a float type
// perform type conversion if float is not the parameter's "native" type
bool DfxParam::accept_f(double inValue, DfxParamValue & outValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			outValue.f = inValue;
			break;
		case kDfxParamValueType_int:
			{
				int64_t oldvalue = outValue.i;
				if (inValue < 0.0)
					outValue.i = (int64_t) (inValue - twiddle);
				else
					outValue.i = (int64_t) (inValue + twiddle);
				if (outValue.i == oldvalue)
					return false;
			}
			break;
		case kDfxParamValueType_boolean:
			{
				unsigned char oldvalue = outValue.b;
				outValue.b = DBOOL(inValue) ? 1 : 0;
				if (outValue.b == oldvalue)
					return false;
			}
			break;
		default:
			return false;
	}

	return true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
// set a DfxParamValue with a value of a int type
// perform type conversion if int is not the parameter's "native" type
bool DfxParam::accept_i(int64_t inValue, DfxParamValue & outValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			outValue.f = (double)inValue;
			break;
		case kDfxParamValueType_int:
			{
				int64_t oldvalue = outValue.i;
				outValue.i = inValue;
				if (outValue.i == oldvalue)
					return false;
			}
			break;
		case kDfxParamValueType_boolean:
			{
				unsigned char oldvalue = outValue.b;
				outValue.b = (inValue == 0) ? 0 : 1;
				if (outValue.b == oldvalue)
					return false;
			}
			break;
		default:
			return false;
	}

	return true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
// set a DfxParamValue with a value of a boolean type
// perform type conversion if boolean is not the parameter's "native" type
bool DfxParam::accept_b(bool inValue, DfxParamValue & outValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			outValue.f = (inValue ? 1.0 : 0.0);
			break;
		case kDfxParamValueType_int:
			{
				int64_t oldvalue = outValue.i;
				outValue.i = (inValue ? 1 : 0);
				if (outValue.i == oldvalue)
					return false;
			}
			break;
		case kDfxParamValueType_boolean:
			{
				unsigned char oldvalue = outValue.b;
				outValue.b = (inValue ? 1 : 0);
				if (outValue.b == oldvalue)
					return false;
			}
			break;
		default:
			return false;
	}

	return true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
// take a generic 0.0 to 1.0 float value and expand it to a real parameter value
// this takes into account the parameter curve
double DfxParam::expand(double inGenValue)
{
	return expandparametervalue(inGenValue, getmin_f(), getmax_f(), curve, curvespec);
}

//-----------------------------------------------------------------------------
// take a generic 0.0 to 1.0 float value and expand it to a real parameter value
// this takes into account the parameter curve
double expandparametervalue(double inGenValue, double minValue, double maxValue, DfxParamCurve curveType, double curveSpec)
{
	double valueRange = maxValue - minValue;
	static const double logTwoInv = 1.0 / log(2.0);

	switch (curveType)
	{
		case kDfxParamCurve_linear:
			return (inGenValue * valueRange) + minValue;
		case kDfxParamCurve_stepped:
			{
				double tempval = (inGenValue * valueRange) + minValue;
				if (tempval < 0.0)
					tempval -= twiddle;
				else
					tempval += twiddle;
				// XXX is this a good way to do this?
				return (double) ((int64_t)tempval);
			}
		case kDfxParamCurve_sqrt:
			return (sqrt(inGenValue) * valueRange) + minValue;
		case kDfxParamCurve_squared:
			return (inGenValue*inGenValue * valueRange) + minValue;
		case kDfxParamCurve_cubed:
			return (inGenValue*inGenValue*inGenValue * valueRange) + minValue;
		case kDfxParamCurve_pow:
			return (pow(inGenValue, curveSpec) * valueRange) + minValue;
		case kDfxParamCurve_exp:
			return exp( log(valueRange+1.0) * inGenValue ) + minValue - 1.0;
		case kDfxParamCurve_log:
			return minValue * pow( 2.0, inGenValue * log(maxValue/minValue)*logTwoInv );
		default:
			break;
	}

	return inGenValue;
}

//-----------------------------------------------------------------------------
// set the parameter's current value using a DfxParamValue
void DfxParam::set(DfxParamValue inNewValue)
{
	value = inNewValue;
	limit();
	setchanged(true);	// XXX do this smarter?
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using a float type value
void DfxParam::set_f(double inNewValue)
{
	bool changed_1 = accept_f(inNewValue, value);
	bool changed_2 = limit();
	if (changed_1 || changed_2)
		setchanged(true);
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using an int type value
void DfxParam::set_i(int64_t inNewValue)
{
	bool changed_1 = accept_i(inNewValue, value);
	bool changed_2 = limit();
	if (changed_1 || changed_2)
		setchanged(true);
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using a boolean type value
void DfxParam::set_b(bool inNewValue)
{
	bool changed_1 = accept_b(inNewValue, value);
	bool changed_2 = limit();
	if (changed_1 || changed_2)
		setchanged(true);
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the parameter's current value with a generic 0...1 float value
void DfxParam::set_gen(double inGenValue)
{
	set_f( expandparametervalue(inGenValue, getmin_f(), getmax_f(), curve, curvespec) );
}



#pragma mark -
#pragma mark handling values
#pragma mark -

//-----------------------------------------------------------------------------
// randomize the current parameter value
// this takes into account the parameter curve
DfxParamValue DfxParam::randomize()
{
	changed = true;	// XXX do this smarter?
	settouched(true);

	switch (valueType)
	{
		case kDfxParamValueType_float:
			set_gen( (double)rand() / (double)RAND_MAX );
			break;
		case kDfxParamValueType_int:
			value.i = (rand() % ((max.i-min.i)+1)) + min.i;
			break;
		case kDfxParamValueType_boolean:
			// but we don't really need to worry about the curve for boolean values
			value.b = (rand() % 2) ? true : false;
			break;
		default:
			break;
	}

	// just in case this ever expedites things
	return value;
}

//-----------------------------------------------------------------------------
// limits the current value to be within the parameter's min/max range
// returns true if the value was altered, false if not
bool DfxParam::limit()
{
	if (!enforceValueLimits)
		return false;

	switch (valueType)
	{
		case kDfxParamValueType_float:
			if (value.f > max.f)
				value.f = max.f;
			else if (value.f < min.f)
				value.f = min.f;
			else return false;
			break;
		case kDfxParamValueType_int:
			if (value.i > max.i)
				value.i = max.i;
			else if (value.i < min.i)
				value.i = min.i;
			else return false;
			break;
		case kDfxParamValueType_boolean:
			if (value.b > max.b)
				value.b = max.b;
			else if (value.b < min.b)
				value.b = min.b;
			else return false;
			break;
		default:
			return false;
	}

	// if we reach this point, then the value was changed, so return true
	changed = true;
	settouched(true);
	return true;
}



#pragma mark -
#pragma mark state
#pragma mark -

//-----------------------------------------------------------------------------
// set the property indicating whether the parameter value has changed
void DfxParam::setchanged(bool inChanged)
{
	// XXX this is when we stuff the current value away as the old value (?)
	if (!inChanged)
		oldValue = value;

	changed = inChanged;
}



#pragma mark -
#pragma mark info
#pragma mark -

//-----------------------------------------------------------------------------
// get a copy of the text of the parameter name
void DfxParam::getname(char * outText)
{
	if ( (name != NULL) && (outText != NULL) )
		strcpy(outText, name);
}

//-----------------------------------------------------------------------------
// get a text string of the unit type
void DfxParam::getunitstring(char * outText)
{
	if (outText == NULL)
		return;

	switch (unit)
	{
		case kDfxParamUnit_generic:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_quantity:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_percent:
			strcpy(outText, "%%");
			break;
		case kDfxParamUnit_portion:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_lineargain:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_decibles:
			strcpy(outText, "dB");
			break;
		case kDfxParamUnit_drywetmix:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_hz:
			strcpy(outText, "Hz");
			break;
		case kDfxParamUnit_seconds:
			strcpy(outText, "seconds");
			break;
		case kDfxParamUnit_ms:
			strcpy(outText, "ms");
			break;
		case kDfxParamUnit_samples:
			strcpy(outText, "samples");
			break;
		case kDfxParamUnit_scalar:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_divisor:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_exponent:
			strcpy(outText, "exponent");
			break;
		case kDfxParamUnit_semitones:
			strcpy(outText, "semitones");
			break;
		case kDfxParamUnit_octaves:
			strcpy(outText, "octaves");
			break;
		case kDfxParamUnit_cents:
			strcpy(outText, "cents");
			break;
		case kDfxParamUnit_notes:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_pan:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_bpm:
			strcpy(outText, "bpm");
			break;
		case kDfxParamUnit_beats:
			strcpy(outText, "per beat");
			break;
		case kDfxParamUnit_index:
			strcpy(outText, "");
			break;
		case kDfxParamUnit_custom:
			if (customUnitString == NULL)
				strcpy(outText, "");
			else
				strcpy(outText, customUnitString);
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
// set the text for a custom unit type display
void DfxParam::setcustomunitstring(const char * inText)
{
	if (inText == NULL)
		return;

	// allocate for the custom unit type string if we haven't yet
	if (customUnitString == NULL)
		customUnitString = (char*) malloc(DFX_PARAM_MAX_UNIT_STRING_LENGTH * sizeof(char));

	strncpy(customUnitString, inText, DFX_PARAM_MAX_UNIT_STRING_LENGTH);
	customUnitString[DFX_PARAM_MAX_UNIT_STRING_LENGTH-1] = 0;
}






#pragma mark -
#pragma mark DfxPreset
#pragma mark -

//-----------------------------------------------------------------------------
DfxPreset::DfxPreset()
{
	// use PostConstructor to allocate parameters
	values = NULL;
	numParameters = 0;

	name = NULL;
	name = (char*) malloc(DFX_PRESET_MAX_NAME_LENGTH * sizeof(char));
	name[0] = 0;

	#ifdef TARGET_API_AUDIOUNIT
		cfname = NULL;
	#endif
}

//-----------------------------------------------------------------------------
DfxPreset::~DfxPreset()
{
	if (values != NULL)
		free(values);
	values = NULL;
	if (name != NULL)
		free(name);
	name = NULL;

	#ifdef TARGET_API_AUDIOUNIT
		if (cfname != NULL)
			CFRelease(cfname);
		cfname = NULL;
	#endif
}

//-----------------------------------------------------------------------------
// call this immediately after the constructor (because new[] can't take arguments)
void DfxPreset::PostConstructor(long inNumParameters)
{
	numParameters = inNumParameters;
	if (values != NULL)
		free(values);
	values = (DfxParamValue*) malloc(numParameters * sizeof(DfxParamValue));
}

/*
//-----------------------------------------------------------------------------
void setvalue(long inParameterIndex, DfxParamValue inNewValue)
{
	if ( (values != NULL) && ((inParameterIndex >= 0) && (inParameterIndex < numParameters)) )
		values[inParameterIndex] = inNewValue;
}

//-----------------------------------------------------------------------------
DfxParamValue getvalue(long inParameterIndex)
{
	if ( (values != NULL) && ((inParameterIndex >= 0) && (inParameterIndex < numParameters)) )
		return values[inParameterIndex];
	else
	{
		DfxParamValue dummy = {0};
		return dummy;
	}
}
*/

//-----------------------------------------------------------------------------
void DfxPreset::setname(const char * inText)
{
	if (inText == NULL)
		return;

	strncpy(name, inText, DFX_PRESET_MAX_NAME_LENGTH);
	name[DFX_PRESET_MAX_NAME_LENGTH-1] = 0;

	#ifdef TARGET_API_AUDIOUNIT
		if (strlen(inText) > 0)
		{
			if (cfname != NULL)
				CFRelease(cfname);
			cfname = CFStringCreateWithCString(kCFAllocatorDefault, inText, kDFX_DefaultCStringEncoding);
		}
	#endif
}

//-----------------------------------------------------------------------------
void DfxPreset::getname(char * outText)
{
	if (outText != NULL)
		strcpy(outText, name);
}

//-----------------------------------------------------------------------------
char * DfxPreset::getname_ptr()
{
	return name;
}
