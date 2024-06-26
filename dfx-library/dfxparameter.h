/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2024  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for doing all kinds of fancy plugin parameter stuff.
------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
There are a number of things that we wanted our parameter system to support.  
There are a number of concepts from which we're working.  Here's an outline:

multiple value types
	set/get values by type
		derive/accept
	set/get "generically"
current, previous, default, min, max
	init is not necessarily default
	min and max limits are not necessarily enforced
		exception:  index unit types
value distribution curves
	expand/contract
unit types
	value strings


multiple value types:

This parameter system allows a parameter to operate using whichever 
value type is most appropriate:  integer, float, boolean, etc.  
The actual storage of parameter values is done in a variant (DfxParam::Value) 
that includes fields for each supported value type.

You can get and set parameter values using entire DfxParam::Value variants 
or using individual values of a particular value type.  Rather than 
being restricted to setting and getting a parameter's value only in its 
native value type, you can actually use any value type and the 
parameter will internally interpret values appropriately given the 
parameter's native type.  The process of interpretation is done via the 
derive and accept routines.  accept is used when setting a parameter value 
and derive is used when getting a parameter value.

Going even further than that, you can also set and get values "generically."  
By this I mean you can get and set using float values constrained within 
a 0.0 to 1.0 range.  Some plugin APIs only support handling parameters in 
this generic 0 to 1 fashion, so the generic set and get routines are 
available in order to interface with those APIs.  They also provide a 
convenient way to handle value distribution curves (more on that below).

There are certain suffixes that are appended to function names to 
indicate how they operate.  You will see multiple variations of the 
same function with varying suffixes.  Most of the suffixes indicate 
that the functions are handling a specific value type.  Those are:  
_f for 64-bit float, _i for 64-bit signed integer, and _b for boolean.  
The suffix _gen indicates that the function handles parameter values 
in the generic 0 to 1 float fashion.


parameter values:  current, previous, default, min, max:

Every parameter stores multiple values.  There is the current value, 
the previous value, the default value, the minimum value, and the 
maximum value.  The minimum and maximum are stored in the member 
variables min and max.  The default is stored in mDefaultValue.  The 
current value is stored in mValue.  The min and max specify the 
recommended range of values for the parameter.  The default is what 
the parameter value should revert to when default settings are 
requested.

The default value is not necessarily the same as the initial value 
for the parameter.  A plugin may want to start off with a certain 
value for a parameter, but consider another value to be the default.  
That is why this init routines all take separate arguments for 
initial value and default values.

The recommended value range defined by min and max is not necessarily 
strictly enforced.  The SetEnforceValueLimits routine can be used to 
specify whether values should be strictly constrained to be within 
the min/max range.  The default behavior is to not restrict values, 
with the exception of index unit types where it is assumed that out 
of range values would be a bad thing (going out of array bounds).   
(more on parameter unit types below)


value distribution curves:

A parameter has a value distributation curve, specified by the 
member variable called mCurve which is of type DfxParam::Curve.  
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
called mCurveSpec is for (for example, the value of the exponent 
for DfxParam::Curve::Pow).  The routines getcurvespec and setcurvespec 
can be used to get and set the value of a parameter's curve-spec.


unit types:

A parameter has a unit type, specified by the member variable called 
mUnit which is of type DfxParam::Unit.  DfxParam::Unit is an enum of common 
parameter unit types (Hz, ms, gain, percent, etc.).

There are a couple of special unit types.  DfxParam::Unit::List indicates 
that the parameter values represent integer index values for an array.  
DfxParam::Unit::Custom indicates that there is a custom text string with 
a name for the unit type.  setcustomunitstring is used to set the 
text for the custom unit name, and getunitstring will get it (as well 
as text names for any other non-custom unit types).

setusevaluestrings(true) indicates that there is an array of text 
strings that should be used for displaying the parameter's value for a 
given index value.  There are routines for getting and setting the text 
for these value strings (setvaluestring, getvaluestring, etc.), and 
setusevaluestrings is used to set this property.
------------------------------------------------------------------------*/

#pragma once


#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "dfx-base.h"
#include "dfxmisc.h"

#ifdef TARGET_API_AUDIOUNIT
	#include <CoreFoundation/CFString.h>
#endif



//-----------------------------------------------------------------------------
class DfxParam
{
public:
	// this is a twiddly value for when casting with decimal types
	static constexpr double kIntegerPadding = 0.001;

#ifdef TARGET_API_AUDIOUNIT
	static constexpr CFStringEncoding kDefaultCStringEncoding = kCFStringEncodingMacRoman;
#endif

	// Value holds every supported value type
	// all parameter values (current, min, max, default) are stored in these
	struct Value : public std::variant<double, int64_t, bool>
	{
		// these are the different representation types that a parameter can 
		// declare as its "native" value type
		enum class Type : uint32_t
		{
			Float,
			Int,
			Boolean
		};

		using Parent = std::variant<double, int64_t, bool>;
		using Parent::variant;
		using Parent::operator=;

		constexpr auto get_f() const
		{
			return std::get<double>(*this);
		}
		constexpr auto get_i() const
		{
			return std::get<int64_t>(*this);
		}
		constexpr auto get_b() const
		{
			return std::get<bool>(*this);
		}
		Type gettype() const noexcept;
	};

	// these are the different value unit types that a parameter can have
	enum class Unit : uint32_t
	{
		Generic,
		Percent,  // typically 0-100
		LinearGain,
		Decibles,
		DryWetMix,  // typically 0-100
		Hz,
		Seconds,
		MS,
		Samples,
		Scalar,
		Divisor,
		Exponent,
		Semitones,
		Octaves,
		Cents,
		Notes,
		Pan,  // typically -1 - +1
		BPM,
		Beats,
		List,  // this indicates that the parameter value is an index into some array
//		Strings,  // index, using array of custom text strings for modes/states/etc.
		Custom  // with a text string lable for custom display
	};

	// these are the different value distribution curves that a parameter can have
	// this is useful if you need to give greater emphasis to some parts of the value range
	enum class Curve
	{
		Linear,
		Stepped,
		SquareRoot,
		Squared,
		Cubed,
		Pow,  // requires curve-spec to be defined as the value of the exponent
		Exp,
		Log
	};

	// some miscellaneous parameter attributes
	// they are stored as flags in a bit-mask thingy
	enum : uint32_t
	{
		kAttribute_Hidden = 1,  // should not be revealed to the user
		kAttribute_Unused = 1 << 1,  // isn't being used at all (a place-holder?); don't reveal to the host or anyone
		kAttribute_OmitFromRandomizeAll = 1 << 2  // should not be included when randomizing all parameters
	};
	using Attribute = uint32_t;


	DfxParam() = default;

	// initialize a parameter with values, value types, curve types, etc.
	// the longest name in the array of names is assumed to be the full name, 
	// and any additional are used to match for short name requests. The names
	// must all be different lengths. Recommended short lengths to support
	// (common in control surfaces): 6, 4, 7
	void init_f(std::vector<std::string_view> const& inNames, 
				double inInitialValue, double inDefaultValue, 
				double inMinValue, double inMaxValue, 
				Unit inUnit = Unit::Generic, 
				Curve inCurve = Curve::Linear);
	void init_i(std::vector<std::string_view> const& inNames, 
				int64_t inInitialValue, int64_t inDefaultValue, 
				int64_t inMinValue, int64_t inMaxValue, 
				Unit inUnit = Unit::Generic, 
				Curve inCurve = Curve::Stepped);
	void init_b(std::vector<std::string_view> const& inNames, 
				bool inInitialValue, bool inDefaultValue, 
				Unit inUnit = Unit::Generic);

	// set/get whether or not to use an array of strings for custom value display
	void setusevaluestrings(bool inMode = true);
	bool getusevaluestrings() const noexcept
	{
		return !mValueStrings.empty();
	}
	// set a value string's text contents
	bool setvaluestring(int64_t inIndex, std::string_view inText);
	// get a copy of the contents of a specific value string
	std::optional<std::string> getvaluestring(int64_t inIndex) const;
#ifdef TARGET_API_AUDIOUNIT
	// get a pointer to the array of CFString value strings
	CFStringRef getvaluecfstring(int64_t inIndex) const
	{
		return mValueCFStrings.at(inIndex).get();
	}
#endif

	// set the parameter's current value
	void set(Value inValue);
	void set_f(double inValue);
	void set_i(int64_t inValue);
	void set_b(bool inValue);
	// set the current value with a generic 0...1 float value
	void set_gen(double inGenValue);
	// set the current value without flagging change or touch or range check
	// (intended for when a plugin generates the change itself)
	void setquietly_f(double inValue);
	void setquietly_i(int64_t inValue);
	void setquietly_b(bool inValue);

	// get the parameter's current value
	Value get() const noexcept
	{
		return mValue;
	}
	double get_f() const noexcept
	{
		return derive_f(mValue);
	}
	int64_t get_i() const
	{
		return derive_i(mValue);
	}
	bool get_b() const noexcept
	{
		return derive_b(mValue);
	}
	// get the current value scaled into a generic 0...1 float value
	double get_gen() const;

	// get the parameter's minimum value
	Value getmin() const noexcept
	{
		return mMinValue;
	}
	double getmin_f() const noexcept
	{
		return derive_f(mMinValue);
	}
	int64_t getmin_i() const
	{
		return derive_i(mMinValue);
	}
	bool getmin_b() const noexcept
	{
		return derive_b(mMinValue);
	}

	// get the parameter's maximum value
	Value getmax() const noexcept
	{
		return mMaxValue;
	}
	double getmax_f() const noexcept
	{
		return derive_f(mMaxValue);
	}
	int64_t getmax_i() const
	{
		return derive_i(mMaxValue);
	}
	bool getmax_b() const noexcept
	{
		return derive_b(mMaxValue);
	}

	// get the parameter's default value
	Value getdefault() const noexcept
	{
		return mDefaultValue;
	}
	double getdefault_f() const noexcept
	{
		return derive_f(mDefaultValue);
	}
	int64_t getdefault_i() const
	{
		return derive_i(mDefaultValue);
	}
	bool getdefault_b() const noexcept
	{
		return derive_b(mDefaultValue);
	}

	// extract the active value of a Value as a scalar type
	// (perform type conversion if the desired output type is not the contained type)
	static double derive_f(Value inValue) noexcept;
	static int64_t derive_i(Value inValue);
	static bool derive_b(Value inValue) noexcept;

	// produce a Value holding the "native" type coerced from a value of a scalar type
	// (perform type conversion if the incoming value type is not "native")
	Value coerce_f(double inValue) const;
	Value coerce_i(int64_t inValue) const noexcept;
	Value coerce_b(bool inValue) const noexcept;

	// expand and contract routines for setting and getting values generically
	// these take into account the parameter curve
	double expand(double inGenValue) const;
	static double expand(double inGenValue, double inMinValue, double inMaxValue, DfxParam::Curve inCurveType, double inCurveSpec = 1.0);
	double contract(double inLiteralValue) const;
	static double contract(double inLiteralValue, double inMinValue, double inMaxValue, DfxParam::Curve inCurveType, double inCurveSpec = 1.0);

	// set/get the property stating whether or not to automatically clip values into range
	void SetEnforceValueLimits(bool inMode);
	bool GetEnforceValueLimits() const noexcept
	{
		return mEnforceValueLimits;
	}

	// get a copy of the text of the parameter name
	std::string getname() const
	{
		return mName;
	}
	// get a copy of the parameter name up to a desired maximum number of characters
	std::string getname(size_t inMaxLength) const;
#ifdef TARGET_API_AUDIOUNIT
	// get a pointer to the CFString version of the parameter name
	CFStringRef getcfname() const noexcept
	{
		return mCFName.get();
	}
#endif

	// get the value type of the parameter values
	Value::Type getvaluetype() const noexcept
	{
		// relying upon default value to avoid atomic overhead with current value
		return mDefaultValue.gettype();
	}

	// set/get the unit type of the parameter
	//void setunit(Unit inNewUnit);
	Unit getunit() const noexcept
	{
		return mUnit;
	}
	std::string getunitstring() const;
	void setcustomunitstring(std::string_view inText);

	// set/get the value distribution curve
	void setcurve(Curve inNewCurve) noexcept
	{
		mCurve = inNewCurve;
	}
	Curve getcurve() const noexcept
	{
		return mCurve;
	}
	// set/get possibly-necessary extra specification about the value distribution curve
	void setcurvespec(double inNewCurveSpec) noexcept
	{
		mCurveSpec = inNewCurveSpec;
	}
	double getcurvespec() const noexcept
	{
		return mCurveSpec;
	}

	// set/get various parameter attribute bits
	void setattributes(Attribute inFlags) noexcept
	{
		mAttributes = inFlags;
	}
	Attribute getattributes() const noexcept
	{
		return mAttributes;
	}
	void addattributes(Attribute inFlags) noexcept
	{
		mAttributes |= inFlags;
	}
	void removeattributes(Attribute inFlags) noexcept
	{
		mAttributes &= ~inFlags;
	}

	// set/get the property indicating whether the parameter value has changed
	// returns preceding state
	bool setchanged(bool inChanged) noexcept;
	bool getchanged() const noexcept
	{
		return mChanged;
	}
	// set/get the property indicating whether the parameter value has been set for any reason (regardless of whether the new value differed)
	// returns preceding state
	bool settouched(bool inTouched) noexcept
	{
		return mTouched.exchange(inTouched);
	}
	bool gettouched() const noexcept
	{
		return mTouched;
	}


private:
	void init(std::vector<std::string_view> const& inNames, 
			  Value inInitialValue, Value inDefaultValue, 
			  Value inMinValue, Value inMaxValue, 
			  Unit inUnit = Unit::Generic, 
			  Curve inCurve = Curve::Linear);

	void initNames(std::vector<std::string_view> const& inNames);

	// apply a value of a scalar type to the current value
	// (perform type conversion if the incoming value type is not "native")
	// returns whether the provided Value changed upon storing the scalar value
	bool accept_f(double inValue);
	bool accept_i(int64_t inValue) noexcept;
	bool accept_b(bool inValue) noexcept;

	// clip the current parameter value within the min/max range
	[[nodiscard]] double limit_f(double inValue) const;
	[[nodiscard]] int64_t limit_i(int64_t inValue) const noexcept;

	// safety check for an index into the value strings array
	bool ValueStringIndexIsValid(int64_t inIndex) const;

	// when this is enabled, out of range values are "bounced" into range
	bool mEnforceValueLimits = false;  // default to allowing values outside of the min/max range
	std::string mName;
	std::vector<std::string> mShortNames;
	dfx::LockFreeAtomic<Value> mValue {};
	Value mDefaultValue {}, mMinValue {}, mMaxValue {};
	Unit mUnit = Unit::Generic;  // the unit type of the parameter
	Curve mCurve = Curve::Linear;  // the shape of the distribution of parameter values
	double mCurveSpec = 1.0;  // special specification, like the exponent in Curve::Pow
	std::vector<std::string> mValueStrings;  // an array of value strings
	std::string mCustomUnitString;  // a text string display for parameters using custom unit types
	dfx::LockFreeAtomic<bool> mChanged {true};  // indicates if the value has changed
	dfx::LockFreeAtomic<bool> mTouched {false};  // indicates if the value has been newly set
	Attribute mAttributes = 0;  // a bit-mask of various parameter attributes

#ifdef TARGET_API_AUDIOUNIT
	// CoreFoundation string version of the parameter's name
	dfx::UniqueCFType<CFStringRef> mCFName;
	// array of CoreFoundation-style versions of the indexed value strings
	std::vector<dfx::UniqueCFType<CFStringRef>> mValueCFStrings;
#endif
};
// end of DfxParam






//-----------------------------------------------------------------------------
// this is a very simple class for preset stuff
// most of the fancy stuff is in the DfxParameter class
class DfxPreset
{
public:
	explicit DfxPreset(size_t inNumParameters);

	void setvalue(dfx::ParameterID inParameterIndex, DfxParam::Value inValue);
	DfxParam::Value getvalue(dfx::ParameterID inParameterIndex) const;
	void setname(std::string_view inText);
	std::string getname() const
	{
		return mName;
	}
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef getcfname() const noexcept
	{
		return mCFName.get();
	}
#endif

private:
	std::string mName;
	std::vector<DfxParam::Value> mValues;
#ifdef TARGET_API_AUDIOUNIT
	dfx::UniqueCFType<CFStringRef> mCFName;
#endif
};
