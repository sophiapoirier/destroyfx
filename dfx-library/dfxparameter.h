/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our class for doing all kinds of fancy plugin parameter stuff.
written by Marc Poirier, October 2002
------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
There are a number of things that we wanted our parameter system to support.  
There are a number of concepts from which we're working.  Here's an outline:

multiple variable types
	set/get values by type
		derive/accept
	set/get "generically"
	randomize values
current, previous, default, min, max
	init is not necessarily default
	min and max limits are not necessarily enforced
		exception:  index unit types
value distribution curves
	expand/contract
unit types
	value strings


multiple variable types:

This parameter system allows a parameter to operate using whichever 
variable type is most appropriate:  int, float, double, boolean, char, etc.  
The variable type that the parameter uses "natively" is stored in a 
DfxParamValueType type member called valueType.  
The actual storage of parameter values is done in a struct (DfxParamValue) 
that includes fields for each supported variable type.

You can get and set parameter values using entire DfxParamValue structs 
(in which case only the field of the struct for the native valueType 
actually affects the value) or using individual values of a particular 
variable type.  Rather than being restricted to setting and getting a 
parameter's value only in its native variable type, you can actually use 
any variable type and the parameter will internally interpret values 
appropriately given the parameter's native type.  The process of 
interpretation is done via the derive and accept routines.  
accept is used when setting a parameter value and derive is used when 
getting a parameter value.

Going even further than that, you can also set and get values "generically."  
By this I mean you can get and set using float values constrained within 
a 0.0 to 1.0 range.  Some plugin APIs only support handling parameters in 
this generic 0 to 1 fashion, so the generic set and get routines are 
available in order to interface with those APIs.  They also provide a 
convenient way to handle value distribution curves (more on that below).

Additionally, you can easily set a parameter's value randomly using the 
randomize routine.  This will randomize the current value in a way that 
makes sense for the parameter's range and native variable type, and 
which also takes into account the parameter's value distribution curve.

There are certain suffixes that are appended to function names to 
indicate how they operate.  You will see multiple variations of the 
same function with varying suffixes.  Most of the suffixes indicate 
that the functions are handling a specific variable type.  Those are:  
_f for float, _d for double, _i for long int, _ui for unsigned long int, 
and _b for boolean.  
The suffix _gen indicates that the function handles parameter values 
in the generic 0 to 1 float fashion.


parameter values:  current, previous, default, min, max:

Every parameter stores multiple values.  There is the current value, 
the previous value, the default value, the minimum value, and the 
maximum value.  The minimum and maximum are stored in the member 
variables min and max.  The default is stored in defaultValue.  The 
current value is stored in value and the previous value is stored in 
oldValue.  The current value is what the parameter value is right now 
and the previous value is what the value was during the previous 
audio processing buffer.  The min and max specify the recommended 
range of values for the parameter.  The default is what the parameter 
value should revert to when default settings are requested.

The default value is not necessarily the same as the initial value 
for the parameter.  A plugin may want to start off with a certain 
value for a parameter, but consider another value to be the default.  
That is why this init routines all take separate arguments for 
initial value and default values.

The recommended value range defined by min and max is not necessarily 
strictly enforced.  The SetEnforceValueLimits routine can be used to 
specify whether values should be strictly constrained to be within 
the min/max range.  The default behaviour is to not restrict values, 
with the exception of index unit types where it is assumed that out 
of range values would be a bad thing (going into unallocated memory 
of an array).  (more on parameter unit types below)


value distribution curves:

A parameter has a value distributation curve, specified by the 
member variable called curve which is of type DfxParamCurve.  
The curve is a recommendation for how to distribute values along 
a parameter control (like a slider on a plugin's graphical interface).  
It indicates whether or not certain parts of the value range should 
be emphasized more than other parts.  This can be useful for units 
like Hz and gain, where our perceptions of change (pitch, dB) are 
not linear in correspondance with linear changes in Hz or a gain 
scalar.

Parameters don't handle their value distribution curves for you, 
for the most part.  The exceptions are when setting or getting 
values "generically" and when randomizing values.  When handling 
"generic" (0.0 to 1.0 float) parameter values, the values are 
weighted according to the distribution curve before being expanded 
and de-weighted after being contracted.  When randomizing a 
parameter, the probability is weighted according to the parameter's 
value distribution curve.  If a graphical interface control (or 
something else) wants to respect a parameter's value distribution 
curve, the easiest way is probably to set and get the parameter 
value generically, and that way the weighting will be handled 
automatically (internally).

The weighting and de-weighting of values according to the value 
distribution curve are handled by the expand and contract routines 
(respectively).  expand takes a generic value and outputs a double 
value which has been weighted according to the curve and expanded 
into the parameter's value range.  contract takes a double value 
on input which is somewhere in the parameter's value range, 
de-weights it according to the curve, and outputs a generic 
(0.0 to 1.0) float value.  0.0 to 1.0 value ranges work very 
nicely for most value distribution curves (pow, sqrt, etc. 
operations all will give output that is still within the 0 to 1 
range when fed input within the 0 to 1 range), so that's why 
generic values are used in the expand and contract routines.

For certain curve types, it might be necessary to provide extra 
curve specification information, and that's what the member variable 
called curvespec is for (for example, the value of the exponent 
for kDfxParamCurve_pow).  The routines getcurvespec and setcurvespec 
can be used to get and set the value of a parameter's curvespec.


unit types:

A parameter has a unit type, specified by the member variable called 
unit which is of type DfxParamUnit.  DfxParamUnit is an enum of common 
parameter unit types (Hz, ms, gain, percent, etc.).

There are a couple of special unit types.  kDfxParamUnit_index indicates 
that the parameter values represent integer index values for an array.  
kDfxParamUnit_custom indicates that there is a custom text string with 
a name for the unit type.  setcustomunitstring is used to set the 
text for the custom unit name, and getunitstring will get it (as well 
as text names for any other non-custom unit types).

Settings useValueStrings true indicates that there is an array of text 
strings that should be used for displaying the parameter's value for a 
given index value.  There are routines for getting and setting the text 
for these value strings (setvaluestring, getvaluestring, 
getvaluestring_ptr, etc.), and setusevaluestrings is used to set this 
property.  When setusevaluestrings(true) is called, memory is allocated 
for the value strings.
------------------------------------------------------------------------*/


#ifndef __DFXPARAMETER_H
#define __DFXPARAMETER_H


#include "dfxdefines.h"

#ifdef TARGET_API_AUDIOUNIT
	#include <CoreFoundation/CFString.h>	// for CFString stuff
#endif



// the DfxParamValue struct holds every type of supported value variable type
// all values (current, min, max, default) are stored in these
struct DfxParamValue {
	float f;
	double d;
	long i;
	unsigned long ui;
	unsigned char b;	// would be bool, but bool can vary in byte size depending on the compiler
	DfxParamValue() {}	// suppress compiler warnings
};
typedef struct DfxParamValue DfxParamValue;


// these are the different variable types that a parameter can 
// declare as its "native" type
typedef enum {
	kDfxParamValueType_undefined,
	kDfxParamValueType_float,
	kDfxParamValueType_double,
	kDfxParamValueType_int,
	kDfxParamValueType_uint,
	kDfxParamValueType_boolean,
} DfxParamValueType;


// these are the different value unit types that a parameter can have
typedef enum {
	kDfxParamUnit_undefined,
	kDfxParamUnit_generic,
	kDfxParamUnit_quantity,
	kDfxParamUnit_percent,	// typically 0-100
	kDfxParamUnit_portion,	// like percent, but typically 0-1
	kDfxParamUnit_lineargain,
	kDfxParamUnit_decibles,
	kDfxParamUnit_drywetmix,	// typically 0-100
	kDfxParamUnit_hz,
	kDfxParamUnit_seconds,
	kDfxParamUnit_ms,
	kDfxParamUnit_samples,
	kDfxParamUnit_scalar,
	kDfxParamUnit_divisor,
	kDfxParamUnit_exponent,
	kDfxParamUnit_semitones,
	kDfxParamUnit_octaves,
	kDfxParamUnit_cents,
	kDfxParamUnit_notes,
	kDfxParamUnit_pan,	// typically -1 - +1
	kDfxParamUnit_bpm,
	kDfxParamUnit_beats,
	kDfxParamUnit_index,	// this indicates that the parameter value is an index into some array
//	kDfxParamUnit_strings,	// index, using array of custom text strings for modes/states/etc.
	kDfxParamUnit_custom	// with a text string lable for custom display
} DfxParamUnit;


// these are the different value distribution curves that a parameter can have
// this is useful if you need to give greater emphasis to some parts of the value range
typedef enum {
	kDfxParamCurve_undefined,
	kDfxParamCurve_linear,
	kDfxParamCurve_stepped,
	kDfxParamCurve_sqrt,
	kDfxParamCurve_squared,
	kDfxParamCurve_cubed,
	kDfxParamCurve_pow,	// requires curvespec to be defined as the value of the exponent
	kDfxParamCurve_exp,
	kDfxParamCurve_log
} DfxParamCurve;



//-----------------------------------------------------------------------------
class DfxParam
{
public:
	DfxParam();
	virtual ~DfxParam();
	
	// initialize a parameter with values, value types, curve types, etc.
	void init(const char * initName, DfxParamValueType initType, 
					DfxParamValue initValue, DfxParamValue initDefaultValue, 
					DfxParamValue initMin, DfxParamValue initMax, 
					DfxParamUnit initUnit = kDfxParamUnit_undefined, 
					DfxParamCurve initCurve = kDfxParamCurve_linear);
	// the rest of these are just convenience wrappers for initializing with a certain variable type
	void init_f(const char * initName, float initValue, float initDefaultValue, 
					float initMin, float initMax, 
					DfxParamUnit initUnit = kDfxParamUnit_undefined, 
					DfxParamCurve initCurve = kDfxParamCurve_linear);
	void init_d(const char * initName, double initValue, double initDefaultValue, 
					double initMin, double initMax, 
					DfxParamUnit initUnit = kDfxParamUnit_undefined, 
					DfxParamCurve initCurve = kDfxParamCurve_linear);
	void init_i(const char * initName, long initValue, long initDefaultValue, 
					long initMin, long initMax, 
					DfxParamUnit initUnit = kDfxParamUnit_undefined, 
					DfxParamCurve initCurve = kDfxParamCurve_stepped);
	void init_ui(const char * initName, unsigned long initValue, unsigned long initDefaultValue, 
					unsigned long initMin, unsigned long initMax, 
					DfxParamUnit initUnit = kDfxParamUnit_undefined, 
					DfxParamCurve initCurve = kDfxParamCurve_linear);
	void init_b(const char * initName, bool initValue, bool initDefaultValue, 
					DfxParamUnit initUnit = kDfxParamUnit_undefined);

	// release memory for the value strings arrays
	void releaseValueStrings();
	// set/get whether or not to use an array of strings for custom value display
	void setusevaluestrings(bool newMode=true);
	bool getusevaluestrings()
		{	return useValueStrings;	}
	// safety check for an index into the value strings array
	bool ValueStringIndexIsValid(long index);
	// set a value string's text contents
	bool setvaluestring(long index, const char * inText);
	// get a copy of the contents of a specific value string...
	bool getvaluestring(long index, char * outText);
	// ...or get a copy of the pointer to the value string
	char * getvaluestring_ptr(long index);
#ifdef TARGET_API_AUDIOUNIT
	// get a pointer to the array of CFString value strings
	CFStringRef * getvaluecfstrings()
		{	return valueCFStrings;	}
#endif

	// set the parameter's current value
	void set(DfxParamValue newValue);
	void set_f(float newValue);
	void set_d(double newValue);
	void set_i(long newValue);
	void set_b(bool newValue);
	// set the current value with a generic 0...1 float value
	void set_gen(float genValue);

	// get the parameter's current value
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
	// get the current value scaled into a generic 0...1 float value
	float get_gen();

	// get the parameter's minimum value
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

	// get the parameter's maximum value
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

	// get the parameter's default value
	DfxParamValue getdefault()
		{	return defaultValue;	}
	float getdefault_f()
		{	return derive_f(defaultValue);	}
	long getdefault_i()
		{	return derive_i(defaultValue);	}
	bool getdefault_b()
		{	return derive_b(defaultValue);	}

	// figure out the value of a DfxParamValue as a certain variable type
	// perform type conversion if the desired variable type is not "native"
	float derive_f(DfxParamValue inValue);
	double derive_d(DfxParamValue inValue);
	long derive_i(DfxParamValue inValue);
	unsigned long derive_ui(DfxParamValue inValue);
	bool derive_b(DfxParamValue inValue);

	// set a DfxParamValue with a value of a specific type
	// perform type conversion if the incoming variable type is not "native"
	bool accept_f(float inValue, DfxParamValue &outValue);
	bool accept_d(double inValue, DfxParamValue &outValue);
	bool accept_i(long inValue, DfxParamValue &outValue);
	bool accept_b(bool inValue, DfxParamValue &outValue);

	// expand and contract routines for setting and getting values generically
	// these take into account the parameter curve
	double expand(float genValue);
	float contract(double realValue);

	// clip the current parameter value to the min/max range
	// returns true if the value was altered, false otherwise
	bool limit();
	// set/get the property stating whether or not to automatically clip values into range
	void SetEnforceValueLimits(bool newRule)
		{	enforceValueLimits = newRule;	limit();	}
	bool GetEnforceValueLimits()
		{	return enforceValueLimits;	}

	// get a copy of the text of the parameter name
	void getname(char * outText);

	// set/get the variable type of the parameter values
	void setvaluetype(DfxParamValueType newType);
	DfxParamValueType getvaluetype()
		{	return valueType;	}

	// set/get the unit type of the parameter
	void setunit(DfxParamUnit newUnit);
	DfxParamUnit getunit()
		{	return unit;	}
	void getunitstring(char * outText);
	void setcustomunitstring(const char * inText);

	// set/get the value distribution curve
	void setcurve(DfxParamCurve newcurve)
		{	curve = newcurve;	}
	DfxParamCurve getcurve()
		{	return curve;	}

	// set/get possibly-necessary extra specification about the value distribution curve
	void setcurvespec(double newcurvespec)
		{	curvespec = newcurvespec;	}
	double getcurvespec()
		{	return curvespec;	}

	// set/get the property indicating whether the parameter is only for internal use
	void sethidden(bool newHide = true);
	bool gethidden()
		{	return hidden;	}

	// set/get the property indicating whether the parameter value has changed
	void setchanged(bool newChanged = true);
	bool getchanged()
		{	return changed;	}

	// randomize the current value of the parameter
	virtual DfxParamValue randomize();



protected:

	// when this is enabled, out of range values are "bounced" into range
	bool enforceValueLimits;
	char * name;
	DfxParamValue value, defaultValue, min, max, oldValue;
	DfxParamValueType valueType;	// the variable type of the parameter values
	DfxParamUnit unit;	// the unit type of the parameter
	DfxParamCurve curve;	// the shape of the distribution of parameter values
	double curvespec;	// special specification, like the exponent in kDfxParamCurve_pow
	bool useValueStrings;	// whether or not to use an array of custom strings to display the parameter's value
	char ** valueStrings;	// an array of strings for when useValueStrings is true
	long numAllocatedValueStrings;	// just to remember how many we allocated
	char * customUnitString;	// a text string display for parameters using custom unit types
	bool changed;	// indicates if the value has changed
	bool hidden;	// indicates if the parameter might be only for internal use

	#ifdef TARGET_API_AUDIOUNIT
		// array of CoreFoundation-style versions of the indexed value strings
		CFStringRef * valueCFStrings;
	#endif

};
// end of DfxParam class definition






// prototypes for parameter value mapping utility functions
double expandparametervalue(double genValue, double minValue, double maxValue, DfxParamCurve curveType, double curveSpec);
double contractparametervalue(double realValue, double minValue, double maxValue, DfxParamCurve curveType, double curveSpec);






//-----------------------------------------------------------------------------
// this is a very simple class for preset stuff
// most of the fancy stuff is in the DfxParameter class
class DfxPreset
{
public:
	DfxPreset();
	~DfxPreset();
	// call this immediately after the constructor (because new[] can't take arguments)
	void PostConstructor(long inNumParameters);

//	void setvalue(long parameterIndex, DfxParamValue newValue);
//	DfxParamValue getvalue(long parameterIndex);
	void setname(const char * inText);
	void getname(char * outText);
	char * getname_ptr();
	#ifdef TARGET_API_AUDIOUNIT
		CFStringRef getcfname()
			{	return cfname;	}
	#endif

	DfxParamValue * values;

protected:
	char * name;
	long numParameters;
	#ifdef TARGET_API_AUDIOUNIT
		CFStringRef cfname;
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



#endif
