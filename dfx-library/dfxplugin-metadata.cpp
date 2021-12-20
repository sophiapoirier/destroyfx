/*------------------------------------------------------------------------
Copyright (C) 2021  Sophia Poirier

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
------------------------------------------------------------------------*/

#include <AudioToolbox/AUComponent.h>
#include <CoreFoundation/CFByteOrder.h>
#include <cstdlib>
#include <iostream>
#include <string>


static std::string FourCCToString(uint32_t fourCharCode)
{
	uint32_t const fourCharCode_native = CFSwapInt32HostToBig(fourCharCode);
	return std::string(reinterpret_cast<char const*>(&fourCharCode_native), sizeof(fourCharCode_native));
}

int main(int const argc, char const* const* const argv)
{
	if (argc != 2)
	{
		std::cerr << "ERR0R: single argument required" << std::endl;
		return EXIT_FAILURE;
	}

	std::string const arg(argv[1]);
	auto const output = [arg]() -> std::string
	{
		if (arg == "id")
		{
			return FourCCToString(PLUGIN_ID);
		}
		if (arg == "version")
		{
			return std::to_string(PLUGIN_VERSION_MAJOR) + "." + std::to_string(PLUGIN_VERSION_MINOR) + "." + std::to_string(PLUGIN_VERSION_BUGFIX);
		}
		if (arg == "type")
		{
#if TARGET_PLUGIN_IS_INSTRUMENT
			return FourCCToString(kAudioUnitType_MusicDevice);
#elif TARGET_PLUGIN_USES_MIDI
			return FourCCToString(kAudioUnitType_MusicEffect);
#else
			return FourCCToString(kAudioUnitType_Effect);
#endif
		}
		return {};
	}();
	if (output.empty())
	{
		std::cerr << "ERR0R: unrecognized argument: " << arg << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << output << std::endl;
	return EXIT_SUCCESS;
}
