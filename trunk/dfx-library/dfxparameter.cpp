/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our class for doing all kinds of fancy plugin parameter stuff.
written by Marc Poirier, October 2002
------------------------------------------------------------------------*/


#include "dfxparameter.h"

#include <stdlib.h>	// for malloc and free and RAND_MAX
#include <string.h>	// for strcpy
#include <math.h>


// these are twiddly values for when casting with decimal types
const float twiddle_f = 0.001f;
const double twiddle_d = 0.001;



#pragma mark _________init_________

//-----------------------------------------------------------------------------
DfxParam::DfxParam()
{
	name = NULL;
	name = (char*) malloc(sizeof(char) * DFX_PARAM_MAX_NAME_LENGTH);
	name[0] = 0;	// empty string

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
	valueType = kDfxParamValueType_undefined;
	unit = kDfxParamUnit_undefined;
	curve = kDfxParamCurve_undefined;
	curvespec = 1.0;
	changed = false;
	hidden = false;
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

	// release the custom unit name string, if there is one
	if (customUnitString != NULL)
		free(customUnitString);
	customUnitString = NULL;
}


//-----------------------------------------------------------------------------
void DfxParam::init(const char *initName, DfxParamValueType initType, 
                    DfxParamValue initValue, DfxParamValue initDefaultValue, 
                    DfxParamValue initMin, DfxParamValue initMax, 
                    DfxParamUnit initUnit, DfxParamCurve initCurve)
{
	// accept all of the incoming init values
	valueType = initType;
	value = oldValue = initValue;
	defaultValue = initDefaultValue;
	min = initMin;
	max = initMax;
	if (initName != NULL)
		strcpy(name, initName);
	curve = initCurve;
	unit = initUnit;
	if (unit == kDfxParamUnit_index)
		SetEnforceValueLimits(true);	// make sure not to go out of any array bounds
	changed = true;


	// do some checks to make sure that the min and max are not swapped 
	// and that the default value is between the min and max
	switch (initType)
	{
		case kDfxParamValueType_undefined:
			break;
		case kDfxParamValueType_float:
			if (min.f > max.f)
			{
				float swap = max.f;
				max.f = min.f;
				min.f = swap;
			}
			if ( (defaultValue.f > max.f)  || (defaultValue.f < min.f) )
				defaultValue.f = ((max.f - min.f) * 0.5f) + min.f;
			break;
		case kDfxParamValueType_double:
			if (min.d > max.d)
			{
				double swap = max.d;
				max.d = min.d;
				min.d = swap;
			}
			if ( (defaultValue.d > max.d)  || (defaultValue.d < min.d) )
				defaultValue.d = ((max.d - min.d) * 0.5) + min.d;
			break;
		case kDfxParamValueType_int:
			if (min.i > max.i)
			{
				long swap = max.i;
				max.i = min.i;
				min.i = swap;
			}
			if ( (defaultValue.i > max.i)  || (defaultValue.i < min.i) )
				defaultValue.i = ((max.i - min.i) / 2) + min.i;
			break;
		case kDfxParamValueType_uint:
			if (min.ui > max.ui)
			{
				unsigned long swap = max.ui;
				max.ui = min.ui;
				min.ui = swap;
			}
			if ( (defaultValue.ui > max.ui)  || (defaultValue.ui < min.ui) )
				defaultValue.ui = ((max.ui - min.ui) / 2) + min.ui;
			break;
		case kDfxParamValueType_boolean:
			min.f = false;
			max.f = true;
			if ( (defaultValue.b > max.b)  || (defaultValue.b < min.b) )
				defaultValue.b = false;
			break;
		case kDfxParamValueType_char:
			if (min.c > max.c)
			{
				char swap = max.c;
				max.c = min.c;
				min.c = swap;
			}
			if ( (defaultValue.c > max.c)  || (defaultValue.c < min.c) )
				defaultValue.c = ((max.c - min.c) / 2) + min.c;
			break;
		case kDfxParamValueType_uchar:
			if (min.uc > max.uc)
			{
				unsigned char swap = max.uc;
				max.uc = min.uc;
				min.uc = swap;
			}
			if ( (defaultValue.uc > max.uc)  || (defaultValue.uc < min.uc) )
				defaultValue.uc = ((max.uc - min.uc) / 2) + min.uc;
			break;
		default:
			break;
	}

	// now squeeze the current value within range, if necessary/desired
	limit();
}


//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with float variable type
void DfxParam::init_f(const char *initName, float initValue, float initDefaultValue, 
							float initMin, float initMax, 
							DfxParamUnit initUnit, DfxParamCurve initCurve)
{
	DfxParamValue val, def, mn, mx;
	val.f = initValue;
	def.f = initDefaultValue;
	mn.f = initMin;
	mx.f = initMax;
	init(initName, kDfxParamValueType_float, val, def, mn, mx, initUnit, initCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with double variable type
void DfxParam::init_d(const char *initName, double initValue, double initDefaultValue, 
							double initMin, double initMax, 
							DfxParamUnit initUnit, DfxParamCurve initCurve)
{
	DfxParamValue val, def, mn, mx;
	val.d = initValue;
	def.d = initDefaultValue;
	mn.d = initMin;
	mx.d = initMax;
	init(initName, kDfxParamValueType_double, val, def, mn, mx, initUnit, initCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with int variable type
void DfxParam::init_i(const char *initName, long initValue, long initDefaultValue, 
							long initMin, long initMax, 
							DfxParamUnit initUnit, DfxParamCurve initCurve)
{
	DfxParamValue val, def, mn, mx;
	val.i = initValue;
	def.i = initDefaultValue;
	mn.i = initMin;
	mx.i = initMax;
	init(initName, kDfxParamValueType_int, val, def, mn, mx, initUnit, initCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with unsigned int variable type
void DfxParam::init_ui(const char *initName, unsigned long initValue, unsigned long initDefaultValue, 
									unsigned long initMin, unsigned long initMax, 
									DfxParamUnit initUnit, DfxParamCurve initCurve)
{
	DfxParamValue val, def, mn, mx;
	val.ui = initValue;
	def.ui = initDefaultValue;
	mn.ui = initMin;
	mx.ui = initMax;
	init(initName, kDfxParamValueType_uint, val, def, mn, mx, initUnit, initCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with boolean variable type
void DfxParam::init_b(const char *initName, bool initValue, bool initDefaultValue, 
							DfxParamUnit initUnit)
{
	DfxParamValue val, def, mn, mx;
	val.b = initValue;
	def.b = initDefaultValue;
	mn.b = false;
	mx.b = true;
	init(initName, kDfxParamValueType_boolean, val, def, mn, mx, initUnit, kDfxParamCurve_linear);
}

//-----------------------------------------------------------------------------
void DfxParam::releaseValueStrings()
{
	// release the parameter value strings, if any
	if (valueStrings != NULL)
	{
		for (long i=0; i < numAllocatedValueStrings; i++)
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
			for (long i=0; i < numAllocatedValueStrings; i++)
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
void DfxParam::setusevaluestrings(bool newMode)
{
	useValueStrings = newMode;

	// if we're using value strings, initialize
	if (newMode)
	{
		// release any value strings arrays that may have previously been allocated
		releaseValueStrings();

		// determine how many items there are in the array from the parameter value range
		numAllocatedValueStrings = getmax_i() - getmin_i() + 1;
		valueStrings = (char**) malloc(numAllocatedValueStrings * sizeof(char*));
		for (long i=0; i < numAllocatedValueStrings; i++)
		{
			valueStrings[i] = (char*) malloc(DFX_PARAM_MAX_VALUE_STRING_LENGTH * sizeof(char));
			valueStrings[i][0] = 0;	// default to empty strings
		}

		#ifdef TARGET_API_AUDIOUNIT
			valueCFStrings = (CFStringRef*) malloc(numAllocatedValueStrings * sizeof(CFStringRef));
			for (long i=0; i < numAllocatedValueStrings; i++)
				valueCFStrings[i] = NULL;
		#endif

		SetEnforceValueLimits(true);	// make sure not to go out of any array bounds
	}
}

//-----------------------------------------------------------------------------
// set a value string's text contents
bool DfxParam::setvaluestring(long index, const char *inText)
{
	if (!ValueStringIndexIsValid(index))
		return false;

	if (inText == NULL)
		return false;

	// the actual index of the array is the incoming index 
	// minus the parameter's minimum value
	long arrayIndex = index-getmin_i();
	strcpy(valueStrings[arrayIndex], inText);

	#ifdef TARGET_API_AUDIOUNIT
		if (valueCFStrings[arrayIndex] != NULL)
			CFRelease(valueCFStrings[arrayIndex]);	// XXX do this?
		// convert the incoming text to a CFString
		valueCFStrings[arrayIndex] = CFStringCreateWithCString(kCFAllocatorDefault, inText, CFStringGetSystemEncoding());
	#endif

	return true;
}

//-----------------------------------------------------------------------------
// get a copy of the contents of a specific value string
bool DfxParam::getvaluestring(long index, char *outText)
{
	char *text = getvaluestring_ptr(index);

	if (text == NULL)
		return false;

	strcpy(outText, text);
	return true;
}

//-----------------------------------------------------------------------------
// get a copy of the pointer to a specific value string
char * DfxParam::getvaluestring_ptr(long index)
{
	if (!ValueStringIndexIsValid(index))
		return 0;

	return valueStrings[index-getmin_i()];
}

//-----------------------------------------------------------------------------
// safety check for an index into the value strings array
bool DfxParam::ValueStringIndexIsValid(long index)
{
	if ( !useValueStrings )
		return false;
	if (valueStrings == NULL)
		return false;
	if ( (index < getmin_i()) || (index > getmax_i()) )
		return false;
	if (valueStrings[index] == NULL)
		return false;

	return true;
}



#pragma mark _________getting_values_________

//-----------------------------------------------------------------------------
// figure out the value of a DfxParamValue as float type value
// perform type conversion if float is not the parameter's "native" type
float DfxParam::derive_f(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			return inValue.f;
		case kDfxParamValueType_double:
			return (float) inValue.d;
		case kDfxParamValueType_int:
			return (float) inValue.i;
		case kDfxParamValueType_uint:
			return (float) inValue.ui;
		case kDfxParamValueType_boolean:
			return (inValue.b != 0) ? 1.0f : 0.0f;
		case kDfxParamValueType_char:
			return (float) inValue.c;
		case kDfxParamValueType_uchar:
			return (float) inValue.uc;
		case kDfxParamValueType_undefined:
		default:
			return 0.0f;
	}
}

//-----------------------------------------------------------------------------
// figure out the value of a DfxParamValue as double type value
// perform type conversion if double is not the parameter's "native" type
double DfxParam::derive_d(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			return (double) inValue.f;
		case kDfxParamValueType_double:
			return inValue.d;
		case kDfxParamValueType_int:
			return (double) inValue.i;
		case kDfxParamValueType_uint:
			return (double) inValue.ui;
		case kDfxParamValueType_boolean:
			return (inValue.b != 0) ? 1.0 : 0.0;
		case kDfxParamValueType_char:
			return (double) inValue.c;
		case kDfxParamValueType_uchar:
			return (double) inValue.uc;
		case kDfxParamValueType_undefined:
		default:
			return 0.0;
	}
}

//-----------------------------------------------------------------------------
// figure out the value of a DfxParamValue as int type value
// perform type conversion if int is not the parameter's "native" type
long DfxParam::derive_i(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			if (inValue.d < 0.0f)
				return (long) (inValue.f - twiddle_f);
			else
				return (long) (inValue.f + twiddle_f);
		case kDfxParamValueType_double:
			if (inValue.d < 0.0)
				return (long) (inValue.d - twiddle_d);
			else
				return (long) (inValue.d + twiddle_d);
		case kDfxParamValueType_int:
			return inValue.i;
		case kDfxParamValueType_uint:
			return (signed) inValue.ui;
		case kDfxParamValueType_boolean:
			return (inValue.b != 0) ? 1 : 0;
		case kDfxParamValueType_char:
			return (long) inValue.c;
		case kDfxParamValueType_uchar:
			return (long) inValue.uc;
		case kDfxParamValueType_undefined:
		default:
			return 0;
	}
}

//-----------------------------------------------------------------------------
// figure out the value of a DfxParamValue as unsigned int type value
// perform type conversion if unsigned int is not the parameter's "native" type
unsigned long DfxParam::derive_ui(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			if (inValue.d < 0.0f)
				return 0;
			else
				return (unsigned long) (inValue.f + twiddle_f);
		case kDfxParamValueType_double:
			if (inValue.d < 0.0)
				return 0;
			else
				return (unsigned long) (inValue.d + twiddle_d);
		case kDfxParamValueType_int:
			if (inValue.i < 0)
				return 0;
			else
				return (unsigned) inValue.i;
		case kDfxParamValueType_uint:
			return inValue.ui;
		case kDfxParamValueType_boolean:
			return (inValue.b != 0) ? 1 : 0;
		case kDfxParamValueType_char:
			if (inValue.c < 0)
				return 0;
			else
				return (unsigned long) inValue.c;
		case kDfxParamValueType_uchar:
			return (unsigned long) inValue.uc;
		case kDfxParamValueType_undefined:
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
			return FBOOL(inValue.f);
		case kDfxParamValueType_double:
			return DBOOL(inValue.d);
		case kDfxParamValueType_int:
			return (inValue.i != 0);
		case kDfxParamValueType_uint:
			return (inValue.ui != 0);
		case kDfxParamValueType_boolean:
			return (inValue.b != 0);
		case kDfxParamValueType_char:
			return (inValue.c != 0);
		case kDfxParamValueType_uchar:
			return (inValue.uc != 0);
		case kDfxParamValueType_undefined:
		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// figure out the value of a DfxParamValue as char type value
// perform type conversion if char is not the parameter's "native" type
char DfxParam::derive_c(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			if (inValue.d < 0.0f)
				return (char) (inValue.f - twiddle_f);
			else
				return (char) (inValue.f + twiddle_f);
		case kDfxParamValueType_double:
			if (inValue.d < 0.0)
				return (char) (inValue.d - twiddle_d);
			else
				return (char) (inValue.d + twiddle_d);
		case kDfxParamValueType_int:
			return (char) inValue.i;
		case kDfxParamValueType_uint:
			return (char) inValue.ui;
		case kDfxParamValueType_boolean:
			return (inValue.b != 0) ? 1 : 0;
		case kDfxParamValueType_char:
			return inValue.c;
		case kDfxParamValueType_uchar:
			return (signed char) inValue.uc;
		case kDfxParamValueType_undefined:
		default:
			return 0;
	}
}

//-----------------------------------------------------------------------------
// figure out the value of a DfxParamValue as unsigned char type value
// perform type conversion if unsigned char is not the parameter's "native" type
unsigned char DfxParam::derive_uc(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			if (inValue.d < 0.0f)
				return 0;
			else
				return (unsigned char) (inValue.f + twiddle_f);
		case kDfxParamValueType_double:
			if (inValue.d < 0.0)
				return 0;
			else
				return (unsigned char) (inValue.d + twiddle_d);
		case kDfxParamValueType_int:
			if (inValue.i < 0)
				return 0;
			else
				return (unsigned char) inValue.i;
		case kDfxParamValueType_uint:
			return (unsigned char) inValue.ui;
		case kDfxParamValueType_boolean:
			return (inValue.b != 0) ? 1 : 0;
		case kDfxParamValueType_char:
			if (inValue.c < 0)
				return 0;
			else
				return (unsigned char) inValue.c;
		case kDfxParamValueType_uchar:
			return inValue.uc;
		case kDfxParamValueType_undefined:
		default:
			return 0;
	}
}

//-----------------------------------------------------------------------------
// take a real parameter value and contract it to a generic 0.0 to 1.0 float value
// this takes into account the parameter curve
// XXX this is being obsoleted by the non-class contractparametervalue() function
float DfxParam::contract(double realValue)
{
	return (float) contractparametervalue(realValue, getmin_d(), getmax_d(), curve, curvespec);
}

//-----------------------------------------------------------------------------
// take a real parameter value and contract it to a generic 0.0 to 1.0 float value
// this takes into account the parameter curve
double contractparametervalue(double realValue, double minValue, double maxValue, DfxParamCurve curveType, double curveSpec)
{
	double valueRange = maxValue - minValue;

	switch (curveType)
	{
		case kDfxParamCurve_linear:
			return (realValue-minValue) / valueRange;
		case kDfxParamCurve_stepped:
			// XXX is this a good way to do this?
			return (realValue-minValue) / valueRange;
		case kDfxParamCurve_sqrt:
			return (sqrt(realValue) * valueRange) + minValue;
		case kDfxParamCurve_squared:
			return sqrt((realValue-minValue) / valueRange);
		case kDfxParamCurve_cubed:
			return pow( (realValue-minValue) / valueRange, 1.0/3.0 );
		case kDfxParamCurve_pow:
			return pow( (realValue-minValue) / valueRange, 1.0/curveSpec );
		case kDfxParamCurve_exp:
			return log(1.0-minValue+realValue) / log(1.0-minValue+maxValue);
		case kDfxParamCurve_log:
			return (log(realValue/minValue) / log(2.0)) / (log(maxValue/minValue) / log(2.0));
		case kDfxParamCurve_undefined:
		default:
			break;
	}

	return realValue;
}

//-----------------------------------------------------------------------------
// get the parameter's current value scaled into a generic 0...1 float value
float DfxParam::get_gen()
{
	return (float) contractparametervalue(get_d(), getmin_d(), getmax_d(), curve, curvespec);
}



#pragma mark _________setting_values_________

//-----------------------------------------------------------------------------
// set a DfxParamValue with a value of a float type
// perform type conversion if float is not the parameter's "native" type
bool DfxParam::accept_f(float inValue, DfxParamValue &outValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			outValue.f = inValue;
			break;
		case kDfxParamValueType_double:
			outValue.d = (double) inValue;
			break;
		case kDfxParamValueType_int:
			if (inValue < 0.0f)
				outValue.i = (long) (inValue - twiddle_f);
			else
				outValue.i = (long) (inValue + twiddle_f);
			break;
		case kDfxParamValueType_uint:
			if (inValue < 0.0f)
				outValue.ui = 0;
			else
				outValue.ui = (unsigned long) (inValue + twiddle_f);
			break;
		case kDfxParamValueType_boolean:
			outValue.b = FBOOL(inValue) ? 1 : 0;
			break;
		case kDfxParamValueType_char:
			if (inValue < 0.0f)
				outValue.c = (char) (inValue - twiddle_f);
			else
				outValue.c = (char) (inValue + twiddle_f);
			break;
		case kDfxParamValueType_uchar:
			if (inValue < 0.0f)
				outValue.uc = 0;
			else
				outValue.uc = (unsigned char) (inValue + twiddle_f);
			break;
		case kDfxParamValueType_undefined:
		default:
			break;
	}

	return true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
// set a DfxParamValue with a value of a double type
// perform type conversion if double is not the parameter's "native" type
bool DfxParam::accept_d(double inValue, DfxParamValue &outValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			outValue.f = (float) inValue;
			break;
		case kDfxParamValueType_double:
			outValue.d = inValue;
			break;
		case kDfxParamValueType_int:
			if (inValue < 0.0)
				outValue.i = (long) (inValue - twiddle_d);
			else
				outValue.i = (long) (inValue + twiddle_d);
			break;
		case kDfxParamValueType_uint:
			if (inValue < 0.0)
				outValue.ui = 0;
			else
				outValue.ui = (unsigned long) (inValue + twiddle_d);
			break;
		case kDfxParamValueType_boolean:
			outValue.b = DBOOL(inValue) ? 1 : 0;
			break;
		case kDfxParamValueType_char:
			if (inValue < 0.0)
				outValue.c = (char) (inValue - twiddle_d);
			else
				outValue.c = (char) (inValue + twiddle_d);
			break;
		case kDfxParamValueType_uchar:
			if (inValue < 0.0)
				outValue.uc = 0;
			else
				outValue.uc = (unsigned char) (inValue + twiddle_d);
			break;
		case kDfxParamValueType_undefined:
		default:
			break;
	}

	return true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
// set a DfxParamValue with a value of a int type
// perform type conversion if int is not the parameter's "native" type
bool DfxParam::accept_i(long inValue, DfxParamValue &outValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			outValue.f = (float)inValue;
			break;
		case kDfxParamValueType_double:
			outValue.d = (double)inValue;
			break;
		case kDfxParamValueType_int:
			outValue.i = inValue;
			break;
		case kDfxParamValueType_uint:
			outValue.ui = (inValue < 0) ? 0 : (unsigned)inValue;
			break;
		case kDfxParamValueType_boolean:
			outValue.b = (inValue == 0) ? 0 : 1;
			break;
		case kDfxParamValueType_char:
			outValue.c = (char)inValue;
			break;
		case kDfxParamValueType_uchar:
			outValue.uc = (inValue < 0) ? 0 : (unsigned char)inValue;
			break;
		case kDfxParamValueType_undefined:
		default:
			break;
	}

	return true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
// set a DfxParamValue with a value of a boolean type
// perform type conversion if boolean is not the parameter's "native" type
bool DfxParam::accept_b(bool inValue, DfxParamValue &outValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
			outValue.f = (inValue ? 1.0f : 0.0f);
			break;
		case kDfxParamValueType_double:
			outValue.d = (inValue ? 1.0 : 0.0);
			break;
		case kDfxParamValueType_int:
			outValue.i = (inValue ? 1 : 0);
			break;
		case kDfxParamValueType_uint:
			outValue.ui = (inValue ? 1 : 0);
			break;
		case kDfxParamValueType_boolean:
			outValue.b = (inValue ? 1 : 0);
			break;
		case kDfxParamValueType_char:
			outValue.c = (inValue ? 1 : 0);
			break;
		case kDfxParamValueType_uchar:
			outValue.uc = (inValue ? 1 : 0);
			break;
		case kDfxParamValueType_undefined:
		default:
			break;
	}

	return true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
// take a generic 0.0 to 1.0 float value and expand it to a real parameter value
// this takes into account the parameter curve
double DfxParam::expand(float genValue)
{
	return expandparametervalue((double)genValue, getmin_d(), getmax_d(), curve, curvespec);
}

//-----------------------------------------------------------------------------
// take a generic 0.0 to 1.0 float value and expand it to a real parameter value
// this takes into account the parameter curve
double expandparametervalue(double genValue, double minValue, double maxValue, DfxParamCurve curveType, double curveSpec)
{
	double valueRange = maxValue - minValue;

	switch (curveType)
	{
		case kDfxParamCurve_linear:
			return (genValue * valueRange) + minValue;
		case kDfxParamCurve_stepped:
			// XXX is this a good way to do this?
			return (double) ( (long) ((genValue * valueRange) + minValue + twiddle_d) );
		case kDfxParamCurve_sqrt:
			return (sqrt(genValue) * valueRange) + minValue;
		case kDfxParamCurve_squared:
			return (genValue*genValue * valueRange) + minValue;
		case kDfxParamCurve_cubed:
			return (genValue*genValue*genValue * valueRange) + minValue;
		case kDfxParamCurve_pow:
			return (pow(genValue, curveSpec) * valueRange) + minValue;
		case kDfxParamCurve_exp:
			return exp( log(valueRange+1.0) * genValue ) + minValue - 1.0;
		case kDfxParamCurve_log:
			return minValue * pow( 2.0, genValue * log(maxValue/minValue)/log(2.0) );
		case kDfxParamCurve_undefined:
		default:
			break;
	}

	return genValue;
}

//-----------------------------------------------------------------------------
// set the parameter's current value using a DfxParamValue
void DfxParam::set(DfxParamValue newValue)
{
	value = newValue;
	limit();
	setchanged(true);	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
// set the current parameter value using a float type value
void DfxParam::set_f(float newValue)
{
	bool changed_1 = accept_f(newValue, value);
	bool changed_2 = limit();
	if (changed_1 || changed_2)
		setchanged(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using a double type value
void DfxParam::set_d(double newValue)
{
	bool changed_1 = accept_d(newValue, value);
	bool changed_2 = limit();
	if (changed_1 || changed_2)
		setchanged(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using an int type value
void DfxParam::set_i(long newValue)
{
	bool changed_1 = accept_i(newValue, value);
	bool changed_2 = limit();
	if (changed_1 || changed_2)
		setchanged(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using a boolean type value
void DfxParam::set_b(bool newValue)
{
	bool changed_1 = accept_b(newValue, value);
	bool changed_2 = limit();
	if (changed_1 || changed_2)
		setchanged(true);
}

//-----------------------------------------------------------------------------
// set the parameter's current value with a generic 0...1 float value
void DfxParam::set_gen(float genValue)
{
	set_d( expandparametervalue((double)genValue, getmin_d(), getmax_d(), curve, curvespec) );
}



#pragma mark _________handling_values_________

//-----------------------------------------------------------------------------
// randomize the current parameter value
// this takes into account the parameter curve
DfxParamValue DfxParam::randomize()
{
	changed = true;	// do this smarter?

	switch (valueType)
	{
		case kDfxParamValueType_float:
		case kDfxParamValueType_double:
		case kDfxParamValueType_int:
//			value.i = (rand() % (long)(max.i-min.i)) + min.i;
		case kDfxParamValueType_uint:
//			value.ui = (unsigned)(rand() % (max.ui-min.ui)) + min.ui;
		case kDfxParamValueType_char:
//			value.c = (char) ((rand() % (long)(max.c-min.c)) + (long)min.c);
		case kDfxParamValueType_uchar:
//			value.uc = (unsigned char) ((rand() % (long)(max.uc-min.uc)) + (long)min.uc);
			// using set_gen accounts for value distribution curves
			set_gen( (float)rand() / (float)RAND_MAX );
			break;
		case kDfxParamValueType_boolean:
			// but we don't really need to worry about the curve for boolean values
			value.b = (rand() % 2) ? true : false;
			break;
		case kDfxParamValueType_undefined:
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
		case kDfxParamValueType_double:
			if (value.d > max.d)
				value.d = max.d;
			else if (value.d < min.d)
				value.d = min.d;
			else return false;
			break;
		case kDfxParamValueType_int:
			if (value.i > max.i)
				value.i = max.i;
			else if (value.i < min.i)
				value.i = min.i;
			else return false;
			break;
		case kDfxParamValueType_uint:
			if (value.ui > max.ui)
				value.ui = max.ui;
			else if (value.ui < min.ui)
				value.ui = min.ui;
			else return false;
			break;
		case kDfxParamValueType_boolean:
			if (value.b > max.b)
				value.b = max.b;
			else if (value.b < min.b)
				value.b = min.b;
			else return false;
			break;
		case kDfxParamValueType_char:
			if (value.c > max.c)
				value.c = max.c;
			else if (value.c < min.c)
				value.c = min.c;
			else return false;
			break;
		case kDfxParamValueType_uchar:
			if (value.uc > max.uc)
				value.uc = max.uc;
			else if (value.uc < min.uc)
				value.uc = min.uc;
			else return false;
			break;
		case kDfxParamValueType_undefined:
		default:
			return false;
	}

	// if we reach this point, then the value was changed, so return true
	changed = true;
	return true;
}



#pragma mark _________state_________

//-----------------------------------------------------------------------------
// set the property indicating whether the parameter value has changed
void DfxParam::setchanged(bool newChanged)
{
	// XXX this is when we stuff the current value away as the old value (?)
	if (newChanged == false)
		oldValue = value;

	changed = newChanged;
}

//-----------------------------------------------------------------------------
// set/get the property indicating whether the parameter is only for internal use
void DfxParam::sethidden(bool newHide)
{
	hidden = newHide;
}



#pragma mark _________info_________

//-----------------------------------------------------------------------------
// get a copy of the text of the parameter name
void DfxParam::getname(char *outText)
{
	if (name && outText)
		strcpy(outText, name);
}

//-----------------------------------------------------------------------------
// get a text string of the unit type
void DfxParam::getunitstring(char *outText)
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
		case kDfxParamUnit_undefined:
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
// set the text for a custom unit type display
void DfxParam::setcustomunitstring(const char *inText)
{
	if (inText == NULL)
		return;

	// allocate for the custom unit type string if we haven't yet
	if (customUnitString == NULL)
		customUnitString = (char*) malloc(DFX_PARAM_MAX_UNIT_STRING_LENGTH * sizeof(char));

	strcpy(customUnitString, inText);
}






#pragma mark _________DfxPreset_________

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
void setvalue(long parameterIndex, DfxParamValue newValue)
{
	if ( (values != NULL) && ((parameterIndex >= 0) && (parameterIndex < numParameters)) )
		values[parameterIndex] = newValue;
}

//-----------------------------------------------------------------------------
DfxParamValue getvalue(long parameterIndex)
{
	if ( (values != NULL) && ((parameterIndex >= 0) && (parameterIndex < numParameters)) )
		return values[parameterIndex];
	else
	{
		DfxParamValue dummy;
		memset(&dummy, 0, sizeof(DfxParamValue));
		return dummy;
	}
}
*/

//-----------------------------------------------------------------------------
void DfxPreset::setname(const char *inText)
{
	if (inText == NULL)
		return;

	strcpy(name, inText);

	#ifdef TARGET_API_AUDIOUNIT
		if (strlen(inText) > 0)
		{
			if (cfname != NULL)
				CFRelease(cfname);	// XXX do this?
			cfname = CFStringCreateWithCString(kCFAllocatorDefault, inText, CFStringGetSystemEncoding());
		}
	#endif
}

//-----------------------------------------------------------------------------
void DfxPreset::getname(char *outText)
{
	if (outText != NULL)
		strcpy(outText, name);
}

//-----------------------------------------------------------------------------
char * DfxPreset::getname_ptr()
{
	return name;
}
