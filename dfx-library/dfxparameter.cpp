/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2021  Sophia Poirier

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
This is our class for doing all kinds of fancy plugin parameter stuff.
------------------------------------------------------------------------*/


#include "dfxparameter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <unordered_set>



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
void DfxParam::init(std::vector<std::string_view> const& inNames, ValueType inType, 
					Value inInitialValue, Value inDefaultValue, 
					Value inMinValue, Value inMaxValue, 
					Unit inUnit, Curve inCurve)
{
	assert(mName.empty());  // shortcut test ensuring init is only called once

	// accept all of the incoming init values
	initNames(inNames);
	mValueType = inType;
	mValue = mOldValue = inInitialValue;
	mDefaultValue = inDefaultValue;
	mMinValue = inMinValue;
	mMaxValue = inMaxValue;
	mCurve = inCurve;
	mUnit = inUnit;
	if (mUnit == Unit::List)
	{
		SetEnforceValueLimits(true);  // make sure not to go out of any array bounds
	}
	mChanged = true;


	// do some checks to make sure that the min and max are not swapped 
	// and that the default value is between the min and max
	switch (inType)
	{
		case ValueType::Float:
			if (mMinValue.f > mMaxValue.f)
			{
				std::swap(mMinValue.f, mMaxValue.f);
			}
			if ((mDefaultValue.f > mMaxValue.f) || (mDefaultValue.f < mMinValue.f))
			{
				mDefaultValue.f = ((mMaxValue.f - mMinValue.f) * 0.5) + mMinValue.f;
			}
			break;
		case ValueType::Int:
			if (mMinValue.i > mMaxValue.i)
			{
				std::swap(mMinValue.i, mMaxValue.i);
			}
			if ((mDefaultValue.i > mMaxValue.i) || (mDefaultValue.i < mMinValue.i))
			{
				mDefaultValue.i = ((mMaxValue.i - mMinValue.i) / 2) + mMinValue.i;
			}
			break;
		case ValueType::Boolean:
			mMinValue.b = false;
			mMaxValue.b = true;
			if ((mDefaultValue.b > mMaxValue.b) || (mDefaultValue.b < mMinValue.b))
			{
				mDefaultValue.b = true;
			}
			break;
		default:
			assert(false);
			break;
	}

	// now squeeze the current value within range, if necessary/desired
	limit();
}


//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with float variable type
void DfxParam::init_f(std::vector<std::string_view> const& inNames, 
					  double inInitialValue, double inDefaultValue, 
					  double inMinValue, double inMaxValue, 
					  Unit inUnit, Curve inCurve)
{
	Value val {}, def {}, min {}, max {};
	val.f = inInitialValue;
	def.f = inDefaultValue;
	min.f = inMinValue;
	max.f = inMaxValue;
	init(inNames, ValueType::Float, val, def, min, max, inUnit, inCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with int variable type
void DfxParam::init_i(std::vector<std::string_view> const& inNames, 
					  int64_t inInitialValue, int64_t inDefaultValue, 
					  int64_t inMinValue, int64_t inMaxValue, 
					  Unit inUnit, Curve inCurve)
{
	Value val {}, def {}, min {}, max {};
	val.i = inInitialValue;
	def.i = inDefaultValue;
	min.i = inMinValue;
	max.i = inMaxValue;
	init(inNames, ValueType::Int, val, def, min, max, inUnit, inCurve);
}
//-----------------------------------------------------------------------------
// convenience wrapper of init() for initializing with boolean variable type
void DfxParam::init_b(std::vector<std::string_view> const& inNames, 
					  bool inInitialValue, bool inDefaultValue, Unit inUnit)
{
	Value val {}, def {}, min {}, max {};
	val.b = inInitialValue;
	def.b = inDefaultValue;
	min.b = false;
	max.b = true;
	init(inNames, ValueType::Boolean, val, def, min, max, inUnit, Curve::Linear);
}

//-----------------------------------------------------------------------------
void DfxParam::initNames(std::vector<std::string_view> const& inNames)
{
	auto const stringLength = [](auto const& string)
	{
		return string.length();
	};

	assert(!inNames.empty());
	assert(std::all_of(inNames.cbegin(), inNames.cend(), [](auto const& name){ return !name.empty(); }));
	{
		std::vector<size_t> lengths;
		std::transform(inNames.cbegin(), inNames.cend(), std::back_inserter(lengths), stringLength);
		assert(std::unordered_set<size_t>(lengths.cbegin(), lengths.cend()).size() == lengths.size());
	}

	// sort the names by length (ascending)
	mShortNames.assign(inNames.cbegin(), inNames.cend());
	std::sort(mShortNames.begin(), mShortNames.end(), [stringLength](auto const& a, auto const& b)
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
// interpret fractional numbers as booleans
static bool Float2Boolean(double inValue)
{
	return (inValue != 0.0);
}

//-----------------------------------------------------------------------------
// figure out the value of a Value as float type value
// (perform type conversion if float is not the parameter's "native" type)
double DfxParam::derive_f(Value inValue) const noexcept
{
	switch (mValueType)
	{
		case ValueType::Float:
			return inValue.f;
		case ValueType::Int:
			return static_cast<double>(inValue.i);
		case ValueType::Boolean:
			return (inValue.b != 0) ? 1.0 : 0.0;
	}
	assert(false);
	return 0.0;
}

//-----------------------------------------------------------------------------
// figure out the value of a Value as int type value
// (perform type conversion if int is not the parameter's "native" type)
int64_t DfxParam::derive_i(Value inValue) const
{
	switch (mValueType)
	{
		case ValueType::Float:
			return static_cast<int64_t>(inValue.f + ((inValue.f < 0.0) ? -kIntegerPadding : kIntegerPadding));
		case ValueType::Int:
			return inValue.i;
		case ValueType::Boolean:
			return (inValue.b != 0) ? 1 : 0;
	}
	assert(false);
	return 0;
}

//-----------------------------------------------------------------------------
// figure out the value of a Value as boolean type value
// (perform type conversion if boolean is not the parameter's "native" type)
bool DfxParam::derive_b(Value inValue) const noexcept
{
	switch (mValueType)
	{
		case ValueType::Float:
			return Float2Boolean(inValue.f);
		case ValueType::Int:
			return (inValue.i != 0);
		case ValueType::Boolean:
			return (inValue.b != 0);
	}
	assert(false);
	return false;
}

//-----------------------------------------------------------------------------
// take a real parameter value and contract it to a generic 0.0 to 1.0 float value
// this takes into account the parameter curve
double DfxParam::contract(double inLiteralValue) const
{
	return contract(inLiteralValue, getmin_f(), getmax_f(), mCurve, mCurveSpec);
}

//-----------------------------------------------------------------------------
// take a real parameter value and contract it to a generic 0.0 to 1.0 float value
// this takes into account the parameter curve
double DfxParam::contract(double inLiteralValue, double inMinValue, double inMaxValue, Curve inCurveType, double inCurveSpec)
{
	auto const valueRange = inMaxValue - inMinValue;
	static constexpr double oneDivThree = 1.0 / 3.0;
	static double const logTwo = std::log(2.0);

	switch (inCurveType)
	{
		case Curve::Linear:
			return (inLiteralValue - inMinValue) / valueRange;
		case Curve::Stepped:
			// XXX is this a good way to do this?
			return (inLiteralValue - inMinValue) / valueRange;
		case Curve::SquareRoot:
			return (std::sqrt(std::max(inLiteralValue, 0.0)) * valueRange) + inMinValue;
		case Curve::Squared:
			return std::sqrt(std::max((inLiteralValue - inMinValue) / valueRange, 0.0));
		case Curve::Cubed:
			return std::pow(std::max((inLiteralValue - inMinValue) / valueRange, 0.0), oneDivThree);
		case Curve::Pow:
			return std::pow(std::max((inLiteralValue - inMinValue) / valueRange, 0.0), 1.0 / inCurveSpec);
		case Curve::Exp:
			return std::log(1.0 - inMinValue + inLiteralValue) / std::log(1.0 - inMinValue + inMaxValue);
		case Curve::Log:
			return (std::log(inLiteralValue / inMinValue) / logTwo) / (std::log(inMaxValue / inMinValue) / logTwo);
	}
	assert(false);
	return inLiteralValue;
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
// set a Value with a value of a float type
// (perform type conversion if float is not the parameter's "native" type)
bool DfxParam::accept_f(double inValue, Value& ioValue) const
{
	switch (mValueType)
	{
		case ValueType::Float:
			ioValue.f = inValue;
			return true;  // XXX do this smarter?
		case ValueType::Int:
		{
			auto const entryValue = ioValue.i;
			if (inValue < 0.0)
			{
				ioValue.i = static_cast<int64_t>(inValue - kIntegerPadding);
			}
			else
			{
				ioValue.i = static_cast<int64_t>(inValue + kIntegerPadding);
			}
			return (ioValue.i != entryValue);
		}
		case ValueType::Boolean:
		{
			auto const entryValue = ioValue.b;
			ioValue.b = Float2Boolean(inValue) ? 1 : 0;
			return (ioValue.b != entryValue);
		}
	}
	assert(false);
	return false;
}

//-----------------------------------------------------------------------------
// set a Value with a value of a int type
// (perform type conversion if int is not the parameter's "native" type)
bool DfxParam::accept_i(int64_t inValue, Value& ioValue) const noexcept
{
	switch (mValueType)
	{
		case ValueType::Float:
			ioValue.f = static_cast<double>(inValue);
			return true;  // XXX do this smarter?
		case ValueType::Int:
		{
			auto const entryValue = ioValue.i;
			ioValue.i = inValue;
			return (ioValue.i != entryValue);
		}
		case ValueType::Boolean:
		{
			auto const entryValue = ioValue.b;
			ioValue.b = (inValue == 0) ? 0 : 1;
			return (ioValue.b != entryValue);
		}
	}
	assert(false);
	return false;
}

//-----------------------------------------------------------------------------
// set a Value with a value of a boolean type
// (perform type conversion if boolean is not the parameter's "native" type)
bool DfxParam::accept_b(bool inValue, Value& ioValue) const noexcept
{
	switch (mValueType)
	{
		case ValueType::Float:
			ioValue.f = (inValue ? 1.0 : 0.0);
			return true;  // XXX do this smarter?
		case ValueType::Int:
		{
			auto const entryValue = ioValue.i;
			ioValue.i = (inValue ? 1 : 0);
			return (ioValue.i != entryValue);
		}
		case ValueType::Boolean:
		{
			auto const entryValue = ioValue.b;
			ioValue.b = (inValue ? 1 : 0);
			return (ioValue.b != entryValue);
		}
	}
	assert(false);
	return false;
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxParam::pack_f(double inValue) const
{
	Value resultValue {};
	accept_f(inValue, resultValue);
	return resultValue;
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxParam::pack_i(int64_t inValue) const noexcept
{
	Value resultValue {};
	accept_i(inValue, resultValue);
	return resultValue;
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxParam::pack_b(bool inValue) const noexcept
{
	Value resultValue {};
	accept_b(inValue, resultValue);
	return resultValue;
}

//-----------------------------------------------------------------------------
// take a generic 0.0 to 1.0 float value and expand it to a real parameter value
// this takes into account the parameter curve
double DfxParam::expand(double inGenValue) const
{
	return expand(inGenValue, getmin_f(), getmax_f(), mCurve, mCurveSpec);
}

//-----------------------------------------------------------------------------
// take a generic 0.0 to 1.0 float value and expand it to a real parameter value
// this takes into account the parameter curve
double DfxParam::expand(double inGenValue, double inMinValue, double inMaxValue, Curve inCurveType, double inCurveSpec)
{
	auto const valueRange = inMaxValue - inMinValue;
	static double const logTwoInv = 1.0 / std::log(2.0);

	switch (inCurveType)
	{
		case Curve::Linear:
			return (inGenValue * valueRange) + inMinValue;
		case Curve::Stepped:
		{
			double tempval = (inGenValue * valueRange) + inMinValue;
			tempval += (tempval < 0.0) ? -DfxParam::kIntegerPadding : DfxParam::kIntegerPadding;
			// XXX is this a good way to do this?
			return static_cast<double>(static_cast<int64_t>(tempval));
		}
		case Curve::SquareRoot:
			return (std::sqrt(std::max(inGenValue, 0.0)) * valueRange) + inMinValue;
		case Curve::Squared:
			return (inGenValue*inGenValue * valueRange) + inMinValue;
		case Curve::Cubed:
			return (inGenValue*inGenValue*inGenValue * valueRange) + inMinValue;
		case Curve::Pow:
			return (std::pow(std::max(inGenValue, 0.0), inCurveSpec) * valueRange) + inMinValue;
		case Curve::Exp:
			return std::exp(std::log(valueRange + 1.0) * inGenValue) + inMinValue - 1.0;
		case Curve::Log:
			return inMinValue * std::pow(2.0, inGenValue * std::log(inMaxValue / inMinValue) * logTwoInv);
	}
	assert(false);
	return inGenValue;
}

//-----------------------------------------------------------------------------
// set the parameter's current value using a Value
void DfxParam::set(Value inNewValue)
{
	mValue = inNewValue;
	limit();
	setchanged(true);  // XXX do this smarter?
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using a float type value
void DfxParam::set_f(double inNewValue)
{
	auto const changed1 = accept_f(inNewValue, mValue);
	auto const changed2 = limit();
	if (changed1 || changed2)
	{
		setchanged(true);
	}
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using an int type value
void DfxParam::set_i(int64_t inNewValue)
{
	auto const changed1 = accept_i(inNewValue, mValue);
	auto const changed2 = limit();
	if (changed1 || changed2)
	{
		setchanged(true);
	}
	settouched(true);
}

//-----------------------------------------------------------------------------
// set the current parameter value using a boolean type value
void DfxParam::set_b(bool inNewValue)
{
	auto const changed1 = accept_b(inNewValue, mValue);
	auto const changed2 = limit();
	if (changed1 || changed2)
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
void DfxParam::setquietly_f(double inNewValue)
{
	accept_f(inNewValue, mValue);
}

//-----------------------------------------------------------------------------
void DfxParam::setquietly_i(int64_t inNewValue)
{
	accept_i(inNewValue, mValue);
}

//-----------------------------------------------------------------------------
void DfxParam::setquietly_b(bool inNewValue)
{
	accept_b(inNewValue, mValue);
}



#pragma mark -
#pragma mark handling values
#pragma mark -

//-----------------------------------------------------------------------------
void DfxParam::SetEnforceValueLimits(bool inMode)
{
	mEnforceValueLimits = inMode;
	if (inMode)
	{
		limit();
	}
}

//-----------------------------------------------------------------------------
// randomize the current parameter value
// this takes into account the parameter curve
DfxParam::Value DfxParam::randomize(dfx::math::RandomEngine& inEngine, dfx::SpinLock& inEngineLock)
{
	mChanged = true;  // XXX do this smarter?
	settouched(true);

	std::lock_guard const guard(inEngineLock);

	switch (mValueType)
	{
		case ValueType::Float:
			set_gen(inEngine.next<double>());
			break;
		case ValueType::Int:
			mValue.i = inEngine.next(mMinValue.i, mMaxValue.i);
			break;
		case ValueType::Boolean:
			// but we don't really need to worry about the curve for boolean values
			mValue.b = inEngine.next<bool>();
			break;
		default:
			assert(false);
			break;
	}

	// just in case this ever expedites things
	return mValue;
}

//-----------------------------------------------------------------------------
// limits the current value to be within the parameter's min/max range
// returns true if the value was altered, false if not
bool DfxParam::limit()
{
	if (!mEnforceValueLimits)
	{
		return false;
	}

	switch (mValueType)
	{
		case ValueType::Float:
		{
			auto const entryValue = mValue.f;
			mValue.f = std::clamp(mValue.f, mMinValue.f, mMaxValue.f);
			if (mValue.f == entryValue)
			{
				return false;
			}
			break;
		}
		case ValueType::Int:
		{
			auto const entryValue = mValue.i;
			mValue.i = std::clamp(mValue.i, mMinValue.i, mMaxValue.i);
			if (mValue.i == entryValue)
			{
				return false;
			}
			break;
		}
		case ValueType::Boolean:
		{
			auto const entryValue = mValue.b;
			mValue.b = std::clamp(mValue.b, mMinValue.b, mMaxValue.b);
			if (mValue.b == entryValue)
			{
				return false;
			}
			break;
		}
		default:
			assert(false);
			return false;
	}

	// if we reach this point, then the value was changed, so return true
	mChanged = true;
	settouched(true);
	return true;
}



#pragma mark -
#pragma mark state
#pragma mark -

//-----------------------------------------------------------------------------
// set the property indicating whether the parameter value has changed
bool DfxParam::setchanged(bool inChanged) noexcept
{
	// XXX this is when we stuff the current value away as the old value (?)
	if (!inChanged)
	{
		mOldValue = mValue;
	}

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
	auto const bestMatch = std::min_element(mShortNames.cbegin(), mShortNames.cend(), [inMaxLength](auto const& a, auto const& b)
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
		auto const shortest = std::min_element(mShortNames.cbegin(), mShortNames.cend(), [](auto const& a, auto const& b)
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
			return "";
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
	assert(false);
	return "";
}

//-----------------------------------------------------------------------------
// set the text for a custom unit type display
void DfxParam::setcustomunitstring(std::string_view inText)
{
	assert(!inText.empty());
	mCustomUnitString.assign(inText, 0, dfx::kParameterUnitStringMaxLength - 1);
}






#pragma mark -
#pragma mark DfxPreset
#pragma mark -

//-----------------------------------------------------------------------------
DfxPreset::DfxPreset(long inNumParameters)
{
	constexpr DfxParam::Value emptyValue {};
	mValues.assign(inNumParameters, emptyValue);
}

//-----------------------------------------------------------------------------
void DfxPreset::setvalue(long inParameterIndex, DfxParam::Value inNewValue)
{
	if ((inParameterIndex >= 0) && (inParameterIndex < static_cast<long>(mValues.size())))
	{
		mValues[inParameterIndex] = inNewValue;
	}
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxPreset::getvalue(long inParameterIndex) const
{
	if ((inParameterIndex >= 0) && (inParameterIndex < static_cast<long>(mValues.size())))
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
