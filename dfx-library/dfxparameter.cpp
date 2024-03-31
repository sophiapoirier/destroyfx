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


#include "dfxparameter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <concepts>
#include <iterator>
#include <limits>
#include <numbers>
#include <unordered_set>
#include <utility>

#include "dfxmath.h"



//-----------------------------------------------------------------------------
// interpret fractional numbers as integral
constexpr int64_t Float2Int(double const& inValue)
{
	constexpr auto padding = DfxParam::kIntegerPadding;
	return static_cast<int64_t>(inValue + ((inValue < 0.) ? -padding : padding));
}

//-----------------------------------------------------------------------------
// interpret fractional numbers as booleans
constexpr bool Float2Boolean(double const& inValue)
{
	return !dfx::math::IsZero(inValue);
}

//-----------------------------------------------------------------------------
// interpret integral numbers as booleans
constexpr bool Int2Boolean(int64_t const& inValue)
{
	return (inValue != 0);
}

//-----------------------------------------------------------------------------
// TODO C++26: constexpr
template <std::floating_point T>
auto sqrt_safe(T inValue)
{
	return std::sqrt(std::max(inValue, T(0)));
}

//-----------------------------------------------------------------------------
// TODO C++26: constexpr
template <std::floating_point T>
auto pow_safe(T inBase, T inExponent)
{
	return std::pow(std::max(inBase, T(0)), inExponent);
}

//-----------------------------------------------------------------------------
// TODO C++26: constexpr
template <std::floating_point T>
auto log_safe(T inValue)
{
	return std::log(std::max(inValue, std::numeric_limits<T>::min()));
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
static auto CreateCFStringWithStringView(std::string_view inText) noexcept
{
	return dfx::MakeUniqueCFType(CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<UInt8 const*>(inText.data()), 
														 inText.length(), DfxParam::kDefaultCStringEncoding, false));
}
#endif



#pragma mark -
#pragma mark init
#pragma mark -

//-----------------------------------------------------------------------------
void DfxParam::init(std::vector<std::string_view> const& inNames, 
					Value inInitialValue, Value inDefaultValue, 
					Value inMinValue, Value inMaxValue, 
					Unit inUnit, Curve inCurve)
{
	assert(mName.empty());  // shortcut test ensuring init is only called once

	auto const valueType = inInitialValue.gettype();
	assert(valueType == inDefaultValue.gettype());
	assert(valueType == inMinValue.gettype());
	assert(valueType == inMaxValue.gettype());

	// accept all of the incoming init values
	initNames(inNames);
	mValue = inInitialValue;
	mDefaultValue = inDefaultValue;
	mMinValue = inMinValue;
	mMaxValue = inMaxValue;
	mCurve = inCurve;
	mUnit = inUnit;
	if ((valueType == Value::Type::Boolean) || (mUnit == Unit::List))
	{
		SetEnforceValueLimits(true);  // make sure not to go out of any array bounds
	}
	mChanged = true;


	// do some checks to make sure that the min and max are not swapped 
	// and that the default value is between the min and max
	switch (valueType)
	{
		case Value::Type::Float:
			assert(!std::isnan(inInitialValue.get_f()));
			assert(!std::isinf(inInitialValue.get_f()));
			assert(!std::isnan(inDefaultValue.get_f()));
			assert(!std::isinf(inDefaultValue.get_f()));
			assert(!std::isnan(inMinValue.get_f()));
			assert(!std::isinf(inMinValue.get_f()));
			assert(!std::isnan(inMaxValue.get_f()));
			assert(!std::isinf(inMaxValue.get_f()));
			if (mMinValue.get_f() > mMaxValue.get_f())
			{
				std::swap(mMinValue, mMaxValue);
			}
			if ((mDefaultValue.get_f() > mMaxValue.get_f()) || (mDefaultValue.get_f() < mMinValue.get_f()))
			{
				mDefaultValue = ((mMaxValue.get_f() - mMinValue.get_f()) * 0.5) + mMinValue.get_f();
			}
			mValue.store(limit_f(inInitialValue.get_f()));
			break;
		case Value::Type::Int:
			if (mMinValue.get_i() > mMaxValue.get_i())
			{
				std::swap(mMinValue, mMaxValue);
			}
			if ((mDefaultValue.get_i() > mMaxValue.get_i()) || (mDefaultValue.get_i() < mMinValue.get_i()))
			{
				mDefaultValue = ((mMaxValue.get_i() - mMinValue.get_i()) / 2) + mMinValue.get_i();
			}
			mValue.store(limit_i(inInitialValue.get_i()));
			break;
		case Value::Type::Boolean:
			mMinValue = false;
			mMaxValue = true;
			break;
		default:
			std::unreachable();
	}
}


//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with float value type
void DfxParam::init_f(std::vector<std::string_view> const& inNames, 
					  double inInitialValue, double inDefaultValue, 
					  double inMinValue, double inMaxValue, 
					  Unit inUnit, Curve inCurve)
{
	init(inNames, inInitialValue, inDefaultValue, inMinValue, inMaxValue, inUnit, inCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with int value type
void DfxParam::init_i(std::vector<std::string_view> const& inNames, 
					  int64_t inInitialValue, int64_t inDefaultValue, 
					  int64_t inMinValue, int64_t inMaxValue, 
					  Unit inUnit, Curve inCurve)
{
	init(inNames, inInitialValue, inDefaultValue, inMinValue, inMaxValue, inUnit, inCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with boolean value type
void DfxParam::init_b(std::vector<std::string_view> const& inNames, 
					  bool inInitialValue, bool inDefaultValue, Unit inUnit)
{
	constexpr Value min(false);
	constexpr Value max(true);
	init(inNames, inInitialValue, inDefaultValue, min, max, inUnit, Curve::Linear);
}

//-----------------------------------------------------------------------------
void DfxParam::initNames(std::vector<std::string_view> const& inNames)
{
	auto const stringLength = [](auto const& string)
	{
		return string.length();
	};

	assert(!inNames.empty());
	assert(std::ranges::all_of(inNames, [](auto const& name){ return !name.empty(); }));
	{
		std::vector<size_t> lengths;
		std::ranges::transform(inNames, std::back_inserter(lengths), stringLength);
		assert(std::unordered_set<size_t>(lengths.cbegin(), lengths.cend()).size() == lengths.size());
	}

	// sort the names by length (ascending)
	mShortNames.assign(inNames.cbegin(), inNames.cend());
	std::ranges::sort(mShortNames, [stringLength](auto const& a, auto const& b)
	{
		return stringLength(a) < stringLength(b);
	});

	mName.assign(mShortNames.back(), 0, dfx::kParameterNameMaxLength - 1);
	assert(mName == mShortNames.back());  // if not, your parameter name is too long!
#ifdef TARGET_API_AUDIOUNIT
	mCFName = CreateCFStringWithStringView(mShortNames.back());
#endif
	// done pulling out the full name, so remove it now from the list of short names
	mShortNames.pop_back();
}

//-----------------------------------------------------------------------------
// set a value string's text contents
void DfxParam::setusevaluestrings(bool inMode)
{
	assert(inMode ? mValueStrings.empty() : true);

	mValueStrings.clear();
#ifdef TARGET_API_AUDIOUNIT
	mValueCFStrings.clear();
#endif

	// if we're using value strings, initialize
	if (inMode)
	{
		// determine how many items there are in the array from the parameter value range
		auto const numAllocatedValueStrings = getmax_i() - getmin_i() + 1;
		mValueStrings.resize(numAllocatedValueStrings);

	#ifdef TARGET_API_AUDIOUNIT
		mValueCFStrings.resize(numAllocatedValueStrings);
	#endif

		SetEnforceValueLimits(true);  // make sure not to go out of any array bounds
	}
}

//-----------------------------------------------------------------------------
// set a value string's text contents
bool DfxParam::setvaluestring(int64_t inIndex, std::string_view inText)
{
	assert(!inText.empty());

	if (!ValueStringIndexIsValid(inIndex))
	{
		return false;
	}

	// the actual index of the array is the incoming index 
	// minus the parameter's minimum value
	auto const arrayIndex = inIndex - getmin_i();
	mValueStrings.at(arrayIndex).assign(inText, 0, dfx::kParameterValueStringMaxLength - 1);

#ifdef TARGET_API_AUDIOUNIT
	mValueCFStrings.at(arrayIndex) = CreateCFStringWithStringView(inText);
#endif

	return true;
}

//-----------------------------------------------------------------------------
// get a copy of the contents of a specific value string
std::optional<std::string> DfxParam::getvaluestring(int64_t inIndex) const
{
	if (!ValueStringIndexIsValid(inIndex))
	{
		return {};
	}
	return mValueStrings.at(inIndex - getmin_i());
}

//-----------------------------------------------------------------------------
// safety check for an index into the value strings array
bool DfxParam::ValueStringIndexIsValid(int64_t inIndex) const
{
	if (!getusevaluestrings())
	{
		return false;
	}
	if ((inIndex < getmin_i()) || (inIndex > getmax_i()))
	{
		return false;
	}
	return true;
}



#pragma mark -
#pragma mark gettingvalues
#pragma mark -

//-----------------------------------------------------------------------------
// extract the value of a Value as float type
// (perform type conversion if float is not contained in the Value)
double DfxParam::derive_f(Value inValue) noexcept
{
	switch (inValue.gettype())
	{
		case Value::Type::Float:
			assert(!std::isnan(inValue.get_f()));
			assert(!std::isinf(inValue.get_f()));
			return inValue.get_f();
		case Value::Type::Int:
			return static_cast<double>(inValue.get_i());
		case Value::Type::Boolean:
			return inValue.get_b() ? 1. : 0.;
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
// extract the value of a Value as int type
// (perform type conversion if int is not contained in the Value)
int64_t DfxParam::derive_i(Value inValue)
{
	switch (inValue.gettype())
	{
		case Value::Type::Float:
			assert(!std::isnan(inValue.get_f()));
			assert(!std::isinf(inValue.get_f()));
			return Float2Int(inValue.get_f());
		case Value::Type::Int:
			return inValue.get_i();
		case Value::Type::Boolean:
			return inValue.get_b() ? 1 : 0;
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
// extract the value of a Value as boolean type
// (perform type conversion if boolean is not contained in the Value)
bool DfxParam::derive_b(Value inValue) noexcept
{
	switch (inValue.gettype())
	{
		case Value::Type::Float:
			assert(!std::isnan(inValue.get_f()));
			assert(!std::isinf(inValue.get_f()));
			return Float2Boolean(inValue.get_f());
		case Value::Type::Int:
			return Int2Boolean(inValue.get_i());
		case Value::Type::Boolean:
			return inValue.get_b();
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
// take a real parameter value and contract it to a generic 0 to 1 float value
// this takes into account the parameter curve
double DfxParam::contract(double inLiteralValue) const
{
	return contract(inLiteralValue, getmin_f(), getmax_f(), mCurve, mCurveSpec);
}

//-----------------------------------------------------------------------------
// take a real parameter value and contract it to a generic 0 to 1 float value
// this takes into account the parameter curve
double DfxParam::contract(double inLiteralValue, double inMinValue, double inMaxValue, Curve inCurveType, double inCurveSpec)
{
	auto const valueRange = inMaxValue - inMinValue;
	constexpr double oneDivThree = 1. / 3.;
	constexpr auto logTwo = std::numbers::ln2_v<double>;

	switch (inCurveType)
	{
		case Curve::Linear:
			return (inLiteralValue - inMinValue) / valueRange;
		case Curve::Stepped:
			// XXX is this a good way to do this?
			return (inLiteralValue - inMinValue) / valueRange;
		case Curve::SquareRoot:
			return (sqrt_safe(inLiteralValue) * valueRange) + inMinValue;
		case Curve::Squared:
			return sqrt_safe((inLiteralValue - inMinValue) / valueRange);
		case Curve::Cubed:
			return pow_safe((inLiteralValue - inMinValue) / valueRange, oneDivThree);
		case Curve::Pow:
			return pow_safe((inLiteralValue - inMinValue) / valueRange, 1.0 / inCurveSpec);
		case Curve::Exp:
			return log_safe(1.0 - inMinValue + inLiteralValue) / log_safe(1.0 - inMinValue + inMaxValue);
		case Curve::Log:
			return (log_safe(inLiteralValue / inMinValue) / logTwo) / (log_safe(inMaxValue / inMinValue) / logTwo);
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
// get the parameter's current value scaled into a generic 0...1 float value
double DfxParam::get_gen() const
{
	return contract(get_f());
}



#pragma mark -
#pragma mark setting values
#pragma mark -

//-----------------------------------------------------------------------------
// apply a value of float type to the current value
// (perform type conversion if float is not the parameter's "native" type)
bool DfxParam::accept_f(double inValue)
{
	auto const coercedValue = coerce_f(inValue);
	return (mValue.exchange(coercedValue) != coercedValue);
}

//-----------------------------------------------------------------------------
// apply a value of int type to the current value
// (perform type conversion if int is not the parameter's "native" type)
bool DfxParam::accept_i(int64_t inValue) noexcept
{
	auto const coercedValue = coerce_i(inValue);
	return (mValue.exchange(coercedValue) != coercedValue);
}

//-----------------------------------------------------------------------------
// apply a value of boolean type to the current value
// (perform type conversion if boolean is not the parameter's "native" type)
bool DfxParam::accept_b(bool inValue) noexcept
{
	auto const coercedValue = coerce_b(inValue);
	return (mValue.exchange(coercedValue) != coercedValue);
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxParam::coerce_f(double inValue) const
{
	switch (getvaluetype())
	{
		case Value::Type::Float:
			return inValue;
		case Value::Type::Int:
			return Float2Int(inValue);
		case Value::Type::Boolean:
			return Float2Boolean(inValue);
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxParam::coerce_i(int64_t inValue) const noexcept
{
	switch (getvaluetype())
	{
		case Value::Type::Float:
			return static_cast<double>(inValue);
		case Value::Type::Int:
			return inValue;
		case Value::Type::Boolean:
			return Int2Boolean(inValue);
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxParam::coerce_b(bool inValue) const noexcept
{
	switch (getvaluetype())
	{
		case Value::Type::Float:
			return (inValue ? 1. : 0.);
		case Value::Type::Int:
			return int64_t(inValue ? 1 : 0);
		case Value::Type::Boolean:
			return inValue;
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
// take a generic 0 to 1 float value and expand it to a real parameter value
// this takes into account the parameter curve
double DfxParam::expand(double inGenValue) const
{
	return expand(inGenValue, getmin_f(), getmax_f(), mCurve, mCurveSpec);
}

//-----------------------------------------------------------------------------
// take a generic 0 to 1 float value and expand it to a real parameter value
// this takes into account the parameter curve
double DfxParam::expand(double inGenValue, double inMinValue, double inMaxValue, Curve inCurveType, double inCurveSpec)
{
	auto const valueRange = inMaxValue - inMinValue;
	constexpr auto logTwoInv = 1. / std::numbers::ln2_v<double>;

	switch (inCurveType)
	{
		case Curve::Linear:
			return (inGenValue * valueRange) + inMinValue;
		case Curve::Stepped:
			// XXX is this a good way to do this?
			return static_cast<double>(Float2Int((inGenValue * valueRange) + inMinValue));
		case Curve::SquareRoot:
			return (sqrt_safe(inGenValue) * valueRange) + inMinValue;
		case Curve::Squared:
			return (inGenValue*inGenValue * valueRange) + inMinValue;
		case Curve::Cubed:
			return (inGenValue*inGenValue*inGenValue * valueRange) + inMinValue;
		case Curve::Pow:
			return (pow_safe(inGenValue, inCurveSpec) * valueRange) + inMinValue;
		case Curve::Exp:
			return std::exp(log_safe(valueRange + 1.0) * inGenValue) + inMinValue - 1.0;
		case Curve::Log:
			return inMinValue * std::pow(2.0, inGenValue * log_safe(inMaxValue / inMinValue) * logTwoInv);
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
// set the parameter's current value using a Value
void DfxParam::set(Value inValue)
{
	assert(inValue.gettype() == getvaluetype());

	switch (inValue.gettype())
	{
		case Value::Type::Float:
			assert(!std::isnan(inValue.get_f()));
			assert(!std::isinf(inValue.get_f()));
			inValue = limit_f(inValue.get_f());
			break;
		case Value::Type::Int:
			inValue = limit_i(inValue.get_i());
			break;
		case Value::Type::Boolean:
			break;
		default:
			std::unreachable();
	}

	if (mValue.exchange(inValue) != inValue)
	{
		setchanged(true);
	}
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using a float type value
void DfxParam::set_f(double inValue)
{
	auto const changed = accept_f(limit_f(inValue));
	if (changed)
	{
		setchanged(true);
	}
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using an int type value
void DfxParam::set_i(int64_t inValue)
{
	auto const changed = accept_i(limit_i(inValue));
	if (changed)
	{
		setchanged(true);
	}
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using a boolean type value
void DfxParam::set_b(bool inValue)
{
	auto const changed = accept_b(inValue);
	if (changed)
	{
		setchanged(true);
	}
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the parameter's current value with a generic 0...1 float value
void DfxParam::set_gen(double inGenValue)
{
	set_f(expand(inGenValue));
}

//-----------------------------------------------------------------------------
void DfxParam::setquietly_f(double inValue)
{
	accept_f(inValue);
}

//-----------------------------------------------------------------------------
void DfxParam::setquietly_i(int64_t inValue)
{
	accept_i(inValue);
}

//-----------------------------------------------------------------------------
void DfxParam::setquietly_b(bool inValue)
{
	accept_b(inValue);
}



#pragma mark -
#pragma mark handling values
#pragma mark -

//-----------------------------------------------------------------------------
void DfxParam::SetEnforceValueLimits(bool inMode)
{
	assert(!(!inMode && (getvaluetype() == Value::Type::Boolean)));

	auto const oldMode = std::exchange(mEnforceValueLimits, inMode);
	if (inMode && !oldMode)
	{
		set(mValue);  // this will trigger value limiting
	}
}

//-----------------------------------------------------------------------------
double DfxParam::limit_f(double inValue) const
{
	if (!mEnforceValueLimits)
	{
		return inValue;
	}
	return std::clamp(inValue, getmin_f(), getmax_f());
}

//-----------------------------------------------------------------------------
int64_t DfxParam::limit_i(int64_t inValue) const noexcept
{
	if (!mEnforceValueLimits)
	{
		return inValue;
	}
	return std::clamp(inValue, getmin_i(), getmax_i());
}



#pragma mark -
#pragma mark state
#pragma mark -

//-----------------------------------------------------------------------------
// set the property indicating whether the parameter value has changed
bool DfxParam::setchanged(bool inChanged) noexcept
{
	return mChanged.exchange(inChanged);
}



#pragma mark -
#pragma mark info
#pragma mark -

//-----------------------------------------------------------------------------
std::string DfxParam::getname(size_t inMaxLength) const
{
	if (mName.length() <= inMaxLength)
	{
		return mName;
	}
	if (mShortNames.empty())
	{
		return {mName, 0, inMaxLength};
	}

	// we want the nearest length match that is equal to or less than the requested maximum
	auto const bestMatch = std::ranges::min_element(mShortNames, [inMaxLength](auto const& a, auto const& b)
	{
		auto const reduce = [inMaxLength](auto const& string)
		{
			return (string.length() <= inMaxLength) ? (inMaxLength - string.length()) : std::numeric_limits<size_t>::max();
		};
		return reduce(a) < reduce(b);
	});
	assert(bestMatch != mShortNames.cend());
	// but if nothing was short enough to fully qualify, truncate the shortest of what we have
	if (bestMatch->length() > inMaxLength)
	{
		auto const shortest = std::ranges::min_element(mShortNames, [](auto const& a, auto const& b)
		{
			return a.length() < b.length();
		});
		assert(shortest != mShortNames.cend());
		return {*shortest, 0, inMaxLength};
	}
	return *bestMatch;
}

//-----------------------------------------------------------------------------
// get a text string of the unit type
std::string DfxParam::getunitstring() const
{
	switch (mUnit)
	{
		case Unit::Generic:
			return "";
		case Unit::Percent:
			return "%";
		case Unit::LinearGain:
			return "";
		case Unit::Decibles:
			return "dB";
		case Unit::DryWetMix:
			return "";
		case Unit::Hz:
			return "Hz";
		case Unit::Seconds:
			return "seconds";
		case Unit::MS:
			return "ms";
		case Unit::Samples:
			return "samples";
		case Unit::Scalar:
			return "x";
		case Unit::Divisor:
			return "";
		case Unit::Exponent:
			return "exponent";
		case Unit::Semitones:
			return "semitones";
		case Unit::Octaves:
			return "octaves";
		case Unit::Cents:
			return "cents";
		case Unit::Notes:
			return "";
		case Unit::Pan:
			return "";
		case Unit::BPM:
			return "bpm";
		case Unit::Beats:
			return "per beat";
		case Unit::List:
			return "";
		case Unit::Custom:
			return mCustomUnitString;
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
// set the text for a custom unit type display
void DfxParam::setcustomunitstring(std::string_view inText)
{
	assert(!inText.empty());
	mCustomUnitString.assign(inText, 0, dfx::kParameterUnitStringMaxLength - 1);
}

//-----------------------------------------------------------------------------
DfxParam::Value::Type DfxParam::Value::gettype() const noexcept
{
	if (std::holds_alternative<double>(*this))
	{
		return Value::Type::Float;
	}
	if (std::holds_alternative<int64_t>(*this))
	{
		return Value::Type::Int;
	}
	if (std::holds_alternative<bool>(*this))
	{
		return Value::Type::Boolean;
	}
	std::unreachable();
}






#pragma mark -
#pragma mark DfxPreset
#pragma mark -

//-----------------------------------------------------------------------------
DfxPreset::DfxPreset(size_t inNumParameters)
:	mValues(inNumParameters)
{
}

//-----------------------------------------------------------------------------
void DfxPreset::setvalue(dfx::ParameterID inParameterIndex, DfxParam::Value inValue)
{
	if (inParameterIndex < mValues.size())
	{
		mValues[inParameterIndex] = inValue;
	}
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxPreset::getvalue(dfx::ParameterID inParameterIndex) const
{
	if (inParameterIndex < mValues.size())
	{
		return mValues[inParameterIndex];
	}
	return {};
}

//-----------------------------------------------------------------------------
void DfxPreset::setname(std::string_view inText)
{
	assert(!inText.empty());

	mName.assign(inText, 0, dfx::kPresetNameMaxLength - 1);

#ifdef TARGET_API_AUDIOUNIT
	mCFName = CreateCFStringWithStringView(inText);
#endif
}
