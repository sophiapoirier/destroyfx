/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our parameter shit.
------------------------------------------------------------------------*/


#ifndef __DFXPARAMETER_H
#include "dfxparameter.h"
#endif

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
	enforceValueLimits = false;

	name = 0;
	name = (char*) malloc(sizeof(char) * DFX_PARAM_MAX_NAME_LENGTH);
	name[0] = 0;

	valueStrings = 0;
	#if TARGET_API_AUDIOUNIT
		valueCFStrings = 0;
	#endif

	// default this stuff empty-style
	valueType = kDfxParamValueType_undefined;
	unit = kDfxParamUnit_undefined;
	curve = kDfxParamCurve_undefined;
	curvespec = 1.0;
	changed = false;
}

//-----------------------------------------------------------------------------
DfxParam::~DfxParam()
{
	if (name)
		free(name);
	name = 0;

	if (valueStrings)
	{
		for (long i=0; i <= (max.i-min.i); i++)
		{
			if (valueStrings[i])
				free(valueStrings[i]);
			valueStrings[i] = 0;
		}
		free(valueStrings);
	}
	valueStrings = 0;

	#if TARGET_API_AUDIOUNIT
		if (valueCFStrings)
		{
			for (long i=0; i <= (max.i-min.i); i++)
			{
				if (valueCFStrings[i])
					CFRelease(valueCFStrings[i]);
				valueCFStrings[i] = 0;
			}
			free(valueCFStrings);
		}
		valueCFStrings = 0;
	#endif
}


//-----------------------------------------------------------------------------
void DfxParam::init(const char *initName, DfxParamValueType initType, 
						DfxParamValue initValue, DfxParamValue initDefaultValue, 
						DfxParamValue initMin, DfxParamValue initMax, 
						DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	valueType = initType;
	value = oldValue = initValue;
	defaultValue = initDefaultValue;
	min = initMin;
	max = initMax;
	if (initName)
		strcpy(name, initName);
	curve = initCurve;
	unit = initUnit;
	if ( (unit == kDfxParamUnit_strings) || (unit == kDfxParamUnit_index) )
		SetEnforceValueLimits(true);	// make sure not to go out of any array bounds
	changed = true;


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

	limit();

	// if we're using value strings, initialize
	if (unit == kDfxParamUnit_strings)
	{
		if (valueStrings)
			free(valueStrings);
		long numValueStrings = getmax_i() - getmin_i() + 1;
		valueStrings = (char**) malloc(numValueStrings * sizeof(char*));
		for (long i=0; i < numValueStrings; i++)
			valueStrings[i] = (char*) malloc(DFX_PARAM_MAX_VALUE_STRING_LENGTH * sizeof(char));

		#if TARGET_API_AUDIOUNIT
			if (valueCFStrings)
				free(valueCFStrings);
			// XXX release each CFString?
			valueCFStrings = (CFStringRef*) malloc(numValueStrings * sizeof(CFStringRef));
			for (long i=0; i < numValueStrings; i++)
				valueCFStrings[i] = 0;
		#endif

	}
}


//-----------------------------------------------------------------------------
void DfxParam::init_f(const char *initName, float initValue, float initDefaultValue, 
							float initMin, float initMax, 
							DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	DfxParamValue val, def, mn, mx;
	val.f = initValue;
	def.f = initDefaultValue;
	mn.f = initMin;
	mx.f = initMax;
	init(initName, kDfxParamValueType_float, val, def, mn, mx, initCurve, initUnit);
}
//-----------------------------------------------------------------------------
void DfxParam::init_d(const char *initName, double initValue, double initDefaultValue, 
							double initMin, double initMax, 
							DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	DfxParamValue val, def, mn, mx;
	val.d = initValue;
	def.d = initDefaultValue;
	mn.d = initMin;
	mx.d = initMax;
	init(initName, kDfxParamValueType_double, val, def, mn, mx, initCurve, initUnit);
}
//-----------------------------------------------------------------------------
void DfxParam::init_i(const char *initName, long initValue, long initDefaultValue, 
							long initMin, long initMax, 
							DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	DfxParamValue val, def, mn, mx;
	val.i = initValue;
	def.i = initDefaultValue;
	mn.i = initMin;
	mx.i = initMax;
	init(initName, kDfxParamValueType_int, val, def, mn, mx, initCurve, initUnit);
}
//-----------------------------------------------------------------------------
void DfxParam::init_ui(const char *initName, unsigned long initValue, unsigned long initDefaultValue, 
									unsigned long initMin, unsigned long initMax, 
									DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	DfxParamValue val, def, mn, mx;
	val.ui = initValue;
	def.ui = initDefaultValue;
	mn.ui = initMin;
	mx.ui = initMax;
	init(initName, kDfxParamValueType_uint, val, def, mn, mx, initCurve, initUnit);
}
//-----------------------------------------------------------------------------
void DfxParam::init_b(const char *initName, bool initValue, bool initDefaultValue, 
							DfxParamCurve initCurve, DfxParamUnit initUnit)
{
	DfxParamValue val, def, mn, mx;
	val.b = initValue;
	def.b = initDefaultValue;
	mn.b = false;
	mx.b = true;
	init(initName, kDfxParamValueType_boolean, val, def, mn, mx, initCurve, initUnit);
}

//-----------------------------------------------------------------------------
bool DfxParam::setvaluestring(long index, const char *inText)
{
	if (!ValueStringIndexIsValid(index))
		return false;

	if (inText == NULL)
		return false;

	long arrayIndex = index-getmin_i();
	strcpy(valueStrings[arrayIndex], inText);

	#if TARGET_API_AUDIOUNIT
		if (valueCFStrings[arrayIndex])
			CFRelease(valueCFStrings[arrayIndex]);	// XXX do this?
		valueCFStrings[arrayIndex] = CFStringCreateWithCString(NULL, inText, kCFStringEncodingMacRoman);//kCFStringEncodingASCII
	#endif

	return true;
}

//-----------------------------------------------------------------------------
bool DfxParam::getvaluestring(long index, char *outText)
{
	char *text = getvaluestring_ptr(index);

	if (text == NULL)
		return false;

	strcpy(outText, text);
	return true;
}

//-----------------------------------------------------------------------------
char * DfxParam::getvaluestring_ptr(long index)
{
	if (!ValueStringIndexIsValid(index))
		return 0;

	return valueStrings[index-getmin_i()];
}

//-----------------------------------------------------------------------------
bool DfxParam::ValueStringIndexIsValid(long index)
{
	if (unit != kDfxParamUnit_strings)
		return false;

	if ( (index < getmin_i()) || (index > getmax_i()) )
		return false;

	return true;
}



#pragma mark _________getting_values_________

//-----------------------------------------------------------------------------
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
bool DfxParam::derive_b(DfxParamValue inValue)
{
	switch (valueType)
	{
		case kDfxParamValueType_float:
		#if TARGET_API_AUDIOUNIT
			return (inValue.d != 0.0f);
		#else
			return (inValue.d > 0.5f);
		#endif
		case kDfxParamValueType_double:
		#if TARGET_API_AUDIOUNIT
			return (inValue.d != 0.0);
		#else
			return (inValue.d > 0.5);
		#endif
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
float DfxParam::contract(double realValue)
{
	double dvalue = realValue;
	double dmin = getmin_d(), dmax = getmax_d();
	double drange = dmax - dmin;

	switch (curve)
	{
		case kDfxParamCurve_linear:
			dvalue = (dvalue-dmin) / drange;
			break;
		case kDfxParamCurve_stepped:
			// XXX is this a good way to do this?
			dvalue = (dvalue-dmin) / drange;
			break;
		case kDfxParamCurve_sqrt:
			dvalue = (sqrt(dvalue) * drange) + dmin;
			break;
		case kDfxParamCurve_squared:
			dvalue = sqrt((dvalue-dmin) / drange);
			break;
		case kDfxParamCurve_cubed:
			dvalue = pow( (dvalue-dmin) / drange, 1.0/3.0 );
			break;
		case kDfxParamCurve_pow:
			dvalue = pow( (dvalue-dmin) / drange, 1.0/curvespec );
			break;
		case kDfxParamCurve_exp:
			dvalue = log(1.0-dmin+dvalue) / log(1.0-dmin+dmax);
			break;
		case kDfxParamCurve_log:
			dvalue = (log(dvalue/dmin) / log(2.0)) / (log(dmax/dmin) / log(2.0));
			break;
		case kDfxParamCurve_undefined:
		default:
			break;
	}

	return (float)dvalue;
}

//-----------------------------------------------------------------------------
float DfxParam::get_gen()
{
	return contract(get_d());
}



#pragma mark _________setting_values_________

//-----------------------------------------------------------------------------
void DfxParam::set(DfxParamValue newValue)
{
	value = newValue;
	limit();
	changed = true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
void DfxParam::accept_f(float inValue, DfxParamValue &outValue)
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
		#if TARGET_API_AUDIOUNIT
			outValue.b = (inValue != 0.0f) ? 1 : 0;
		#else
			outValue.b = (inValue > 0.5f) ? 1 : 0;
		#endif
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
}

//-----------------------------------------------------------------------------
void DfxParam::accept_d(double inValue, DfxParamValue &outValue)
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
		#if TARGET_API_AUDIOUNIT
			outValue.b = (inValue != 0.0) ? 1 : 0;
		#else
			outValue.b = (inValue > 0.5) ? 1 : 0;
		#endif
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
}

//-----------------------------------------------------------------------------
void DfxParam::accept_i(long inValue, DfxParamValue &outValue)
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
}

//-----------------------------------------------------------------------------
void DfxParam::accept_b(bool inValue, DfxParamValue &outValue)
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
}

//-----------------------------------------------------------------------------
// take a generic 0.0 to 1.0 float value and expand it to a real parameter value
double DfxParam::expand(float genValue)
{
	double dvalue = (double) genValue;
	double dmin = getmin_d(), dmax = getmax_d();
	double drange = dmax - dmin;

	switch (curve)
	{
		case kDfxParamCurve_linear:
			dvalue = (dvalue * drange) + dmin;
			break;
		case kDfxParamCurve_stepped:
			// XXX is this a good way to do this?
			dvalue = (double) ( (long) ((dvalue * drange) + dmin + twiddle_d) );
			break;
		case kDfxParamCurve_sqrt:
			dvalue = (sqrt(dvalue) * drange) + dmin;
			break;
		case kDfxParamCurve_squared:
			dvalue = (dvalue*dvalue * drange) + dmin;
			break;
		case kDfxParamCurve_cubed:
			dvalue = (dvalue*dvalue*dvalue * drange) + dmin;
			break;
		case kDfxParamCurve_pow:
			dvalue = (pow(dvalue, curvespec) * drange) + dmin;
			break;
		case kDfxParamCurve_exp:
			dvalue = exp( log(drange+1.0) * dvalue ) + dmin - 1.0;
			break;
		case kDfxParamCurve_log:
			dvalue = dmin * pow( 2.0, dvalue * log(dmax/dmin)/log(2.0) );
			break;
		case kDfxParamCurve_undefined:
		default:
			break;
	}

	return dvalue;
}

//-----------------------------------------------------------------------------
void DfxParam::set_f(float newValue)
{
	accept_f(newValue, value);
	limit();
	changed = true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
void DfxParam::set_d(double newValue)
{
	accept_d(newValue, value);
	limit();
	changed = true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
void DfxParam::set_i(long newValue)
{
	accept_i(newValue, value);
	limit();
	changed = true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
void DfxParam::set_b(bool newValue)
{
	accept_b(newValue, value);
	limit();
	changed = true;	// XXX do this smarter?
}

//-----------------------------------------------------------------------------
void DfxParam::set_gen(float genValue)
{
	set_d(expand(genValue));
}



#pragma mark _________handling_values_________

//-----------------------------------------------------------------------------
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
			// using set_gen accounts for scaling curves
			set_gen( (float)rand() / (float)RAND_MAX );
			break;
		case kDfxParamValueType_boolean:
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
// limits the value to be within the parameter's range
// returns true if the value was altered, false if not
bool DfxParam::limit()
{
	if (!enforceValueLimits)
		return false;

	// remember the old value in order to later see if it changed
	DfxParamValue oldValue = value;

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
void DfxParam::setchanged(bool newChanged)
{
	// XXX this is when we stuff the current value away as the old value (?)
	if (newChanged == false)
		oldValue = value;

	changed = newChanged;
}



#pragma mark _________info_________

//-----------------------------------------------------------------------------
void DfxParam::getname(char *text)
{
	if (name && text)
		strcpy(text, name);
}






#pragma mark _________DfxPreset_________

//-----------------------------------------------------------------------------
DfxPreset::DfxPreset()
{
	values = 0;
	numParameters = 0;	// for now

	name = 0;
	name = (char*) malloc(DFX_PRESET_MAX_NAME_LENGTH * sizeof(char));
	name[0] = 0;

	#if TARGET_API_AUDIOUNIT
		cfname = 0;
	#endif
}

//-----------------------------------------------------------------------------
DfxPreset::~DfxPreset()
{
	if (values)
		free(values);
	values = 0;
	if (name)
		free(name);
	name = 0;

	#if TARGET_API_AUDIOUNIT
		if (cfname)
			CFRelease(cfname);
		cfname = 0;
	#endif
}

//-----------------------------------------------------------------------------
// call this immediately after the constructor (because new[] can't take arguments)
void DfxPreset::PostConstructor(long inNumParameters)
{
	numParameters = inNumParameters;
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

	#if TARGET_API_AUDIOUNIT
		if (strlen(inText) > 0)
		{
			if (cfname)
				CFRelease(cfname);	// XXX do this?
			cfname = CFStringCreateWithCString(NULL, inText, kCFStringEncodingMacRoman);//kCFStringEncodingASCII
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
