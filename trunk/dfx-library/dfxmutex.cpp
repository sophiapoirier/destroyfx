/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our mutually exclusive shit.
------------------------------------------------------------------------*/

#include "dfxmutex.h"



//------------------------------------------------------------------------
#ifdef __MACH__

void dfx::SpinLock::lock()
{
	os_unfair_lock_lock(&mLock);
}

bool dfx::SpinLock::try_lock()
{
	return os_unfair_lock_trylock(&mLock);
}

void dfx::SpinLock::unlock()
{
	os_unfair_lock_unlock(&mLock);
}



//------------------------------------------------------------------------
#else

void dfx::SpinLock::lock()
{
	while (!try_lock());
}

bool dfx::SpinLock::try_lock()
{
	return !mFlag.test_and_set(std::memory_order_acquire);
}

void dfx::SpinLock::unlock()
{
	mFlag.clear(std::memory_order_release);
}



#endif  // platform
