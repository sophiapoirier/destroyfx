/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our parameter shit.
------------------------------------------------------------------------*/


#ifndef __DFXPARAMETER_H
#define __DFXPARAMETER_H


#if TARGET_API_AUDIOUNIT
	#include <CoreFoundation/CFString.h>	// for CFString stuff
#endif


#define DFX_PARAM_MAX_NAME_LENGTH 64
#define DFX_PRESET_MAX_NAME_LENGTH 64
#define DFX_PARAM_MAX_VALUE_STRING_LENGTH 256



struct DfxParamValue {
	float f;
	double d;
	long i;
	unsigned long ui;
	unsigned char b;	// would be bool, but bool can be vary in byte size depending on the compiler
	char c;
	unsigned char uc;
};


enum DfxParamElement {
	kDfxParamElement_value,
	kDfxParamElement_min,
	kDfxParamElement_max,
	kDfxParamElement_default
};



enum DfxParamValueType {
	kDfxParamValueType_undefined,
	kDfxParamValueType_float,
	kDfxParamValueType_double,
	kDfxParamValueType_int,
	kDfxParamValueType_uint,
	kDfxParamValueType_boolean,
	kDfxParamValueType_char,
	kDfxParamValueType_uchar
};


enum DfxParamUnit {
	kDfxParamUnit_undefined,
	kDfxParamUnit_percent,	// typically 0-100
	kDfxParamUnit_portion,	// like percent, but typically 0-1
	kDfxParamUnit_lineargain,
	kDfxParamUnit_decibles,
	kDfxParamUnit_drywetmix,	// typically 0-100
	kDfxParamUnit_audiofreq,
	kDfxParamUnit_lfofreq,
	kDfxParamUnit_seconds,
	kDfxParamUnit_ms,
	kDfxParamUnit_samples,
	kDfxParamUnit_scalar,
	kDfxParamUnit_divisor,
	kDfxParamUnit_semitones,
	kDfxParamUnit_octaves,
	kDfxParamUnit_cents,
	kDfxParamUnit_notes,
	kDfxParamUnit_pan,	// typically -1 - +1
	kDfxParamUnit_bpm,
	kDfxParamUnit_beats,
	kDfxParamUnit_index,
	kDfxParamUnit_strings	// index, using array of custom text strings for modes/states/etc.
};


enum DfxParamCurve {
	kDfxParamCurve_undefined,
	kDfxParamCurve_linear,
	kDfxParamCurve_stepped,
	kDfxParamCurve_sqrt,
	kDfxParamCurve_squared,
	kDfxParamCurve_cubed,
	kDfxParamCurve_pow,
	kDfxParamCurve_exp,
	kDfxParamCurve_log
};



//-----------------------------------------------------------------------------
class DfxParam
{
public:
	DfxParam();
	virtual ~DfxParam();
	
	virtual void init(const char *initName, DfxParamValueType initType, 
						DfxParamValue initValue, DfxParamValue initDefaultValue, 
						DfxParamValue initMin, DfxParamValue initMax, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);

	void init_f(const char *initName, float initValue, float initDefaultValue, 
						float initMin, float initMax, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void init_d(const char *initName, double initValue, double initDefaultValue, 
						double initMin, double initMax, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void init_i(const char *initName, long initValue, long initDefaultValue, 
						long initMin, long initMax, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void init_ui(const char *initName, unsigned long initValue, unsigned long initDefaultValue, 
						unsigned long initMin, unsigned long initMax, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void init_b(const char *initName, bool initValue, bool initDefaultValue, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);

	bool ValueStringIndexIsValid(long index);
	bool setvaluestring(long index, const char *inText);
	bool getvaluestring(long index, char *outText);
	char * getvaluestring_ptr(long index);
#if TARGET_API_AUDIOUNIT
	CFStringRef * getvaluecfstrings()
		{	return valueCFStrings;	}
#endif

	void set(DfxParamValue newValue);
	void set_f(float newValue);
	void set_d(double newValue);
	void set_i(long newValue);
	void set_b(bool newValue);
	// set with a generic 0...1 float value
	void set_gen(float genValue);
	// get current value scaled into a generic 0...1 float value
	float get_gen();

	DfxParamValue get()
		{	return value;	}
	float get_f()
		{	return derive_f(value);	}
	double get_d()
		{	return derive_d(value);	}
	long get_i()
		{	return derive_i(value);	}
	unsigned long get_ui()
		{	return derive_ui(value);	}
	bool get_b()
		{	return derive_b(value);	}
	char get_c()
		{	return derive_c(value);	}
	unsigned char get_uc()
		{	return derive_uc(value);	}

	DfxParamValue getmin()
		{	return min;	}
	float getmin_f()
		{	return derive_f(min);	}
	double getmin_d()
		{	return derive_d(min);	}
	long getmin_i()
		{	return derive_i(min);	}
	bool getmin_b()
		{	return derive_b(min);	}

	DfxParamValue getmax()
		{	return max;	}
	float getmax_f()
		{	return derive_f(max);	}
	double getmax_d()
		{	return derive_d(max);	}
	long getmax_i()
		{	return derive_i(max);	}
	bool getmax_b()
		{	return derive_b(max);	}

	DfxParamValue getdefault()
		{	return defaultValue;	}
	float getdefault_f()
		{	return derive_f(defaultValue);	}
	long getdefault_i()
		{	return derive_i(defaultValue);	}
	bool getdefault_b()
		{	return derive_b(defaultValue);	}

	// figure out a DfxParamValue as a certain type
	float derive_f(DfxParamValue inValue);
	double derive_d(DfxParamValue inValue);
	long derive_i(DfxParamValue inValue);
	unsigned long derive_ui(DfxParamValue inValue);
	bool derive_b(DfxParamValue inValue);
	char derive_c(DfxParamValue inValue);
	unsigned char derive_uc(DfxParamValue inValue);

	// set DfxParamValue with a value of a specific type
	void accept_f(float inValue, DfxParamValue &outValue);
	void accept_d(double inValue, DfxParamValue &outValue);
	void accept_i(long inValue, DfxParamValue &outValue);
	void accept_b(bool inValue, DfxParamValue &outValue);

	// expand and contract routines for setting and getting values generically
	double expand(float genValue);
	float contract(double realValue);

	bool limit();
	void SetEnforceValueLimits(bool newRule)
		{	enforceValueLimits = newRule;	limit();	}
	bool GetEnforceValueLimits()
		{	return enforceValueLimits;	}

	void setvaluetype(DfxParamValueType newType);
	DfxParamValueType getvaluetype()
		{	return valueType;	}

	void setunit(DfxParamUnit newUnit);
	DfxParamUnit getunit()
		{	return unit;	}

	void setcurvespec(double newcurvespec)
		{	curvespec = newcurvespec;	}
	double getcurvespec()
		{	return curvespec;	}

	void setchanged(bool newChanged = true);
	bool getchanged()
		{	return changed;	}

	virtual DfxParamValue randomize();

	void getname(char *text);


protected:
	// when this is enabled, out of range values are "bounced" into range
	bool enforceValueLimits;
	char *name;
	DfxParamValue value, defaultValue, min, max, oldValue;
	DfxParamValueType valueType;
	DfxParamUnit unit;
	DfxParamCurve curve;
	double curvespec;	// special specification, like the exponent in kDfxParamCurve_pow
	char **valueStrings;	// an array of strings for kDfxParamUnit_strings
	bool changed;	// indicates if the value has changed

	#if TARGET_API_AUDIOUNIT
		CFStringRef *valueCFStrings;
	#endif
};



// XXX gotta obsolete this stuff...
#define onOffTest(fvalue)   ((fvalue) > 0.5f)
#define paramRangeScaled(value,min,max)   ( ((value) * ((max)-(min))) + (min) )
#define paramRangeUnscaled(value,min,max)   ( ((value)-(min)) / ((max)-(min)) )
#define paramRangeSquaredScaled(value,min,max)   ( ((value)*(value) * ((max)-(min))) + (min) )
#define paramRangeSquaredUnscaled(value,min,max)   ( sqrtf(((value)-(min)) / ((max)-(min))) )
#define paramRangeCubedScaled(value,min,max)   ( ((value)*(value)*(value) * ((max)-(min))) + (min) )
#define paramRangeCubedUnscaled(value,min,max)   ( powf(((value)-(min)) / ((max)-(min)), 1.0f/3.0f) )
#define paramRangePowScaled(value,min,max,power)   ( (powf((value),(power)) * ((max)-(min))) + (min) )
#define paramRangePowUnscaled(value,min,max,power)   ( powf(((value)-(min)) / ((max)-(min)), 1.0f/(power)) )
#define paramRangeExpScaled(value,min,max)   ( expf(logf((max)-(min)+1.0f)*(value)) + (min) - 1.0f )
#define paramRangeExpUnscaled(value,min,max)   ( logf(1.0f-(min)+(value)) / logf(1.0f-(min)+(max)) )
#define paramSteppedScaled(value,numSteps)   ( (long)((value) * ((float)(numSteps)-0.01f)) )
#define paramSteppedUnscaled(step,numSteps)   ( (float)(step) / ((float)((numSteps)-1)) )
#define paramRangeIntScaled(value,min,max)   ( (long)((value) * ((float)((max)-(min)+1)-0.01f)) + (min) )
#define paramRangeIntUnscaled(step,min,max)   ( (float)((step)-(min)) / (float)((max)-(min)) )
// scale logarithmicly from 20 Hz to 20 kHz
#define paramFrequencyScaled(value)   (20.0f * powf(2.0f, (value) * 9.965784284662088765571752446703612804412841796875f))
#define paramFrequencyUnscaled(value)   ( (logf((value)/20.0f)/logf(2.0f)) / 9.965784284662088765571752446703612804412841796875f )



//-----------------------------------------------------------------------------
class DfxPreset
{
public:
	DfxPreset();
	~DfxPreset();
	// call this immediately after the constructor (because new[] can't take arguments)
	void PostConstructor(long inNumParameters);

//	void setvalue(long parameterIndex, DfxParamValue newValue);
//	DfxParamValue getvalue(long parameterIndex);
	void setname(const char *inText);
	void getname(char *outText);
	char * getname_ptr();
	#if TARGET_API_AUDIOUNIT
		CFStringRef getcfname()
			{	return cfname;	}
	#endif

	DfxParamValue *values;

protected:
	char *name;
	long numParameters;
	#if TARGET_API_AUDIOUNIT
		CFStringRef cfname;
	#endif
};



#endif