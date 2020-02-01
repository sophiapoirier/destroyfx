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

#pragma once


#ifdef __MACH__
	#include <os/lock.h>
#else
	#include <atomic>
#endif



namespace dfx
{

// A typical mutex can in some cases block during try_lock, making it unsafe to use at all
// in realtime contexts.  A lightweight spinlock, while having some behavioral disadvantages,
// can meet the try_lock performance requirements for such use cases.
class SpinLock
{
public:
	SpinLock() noexcept = default;
	~SpinLock() noexcept = default;
	SpinLock(SpinLock const&) = delete;
	SpinLock(SpinLock&&) = delete;
	SpinLock& operator=(SpinLock const&) = delete;
	SpinLock& operator=(SpinLock&&) = delete;

	// interface matches that of std::mutex to allow usage with STL scoped lock guards
	void lock();
	bool try_lock();
	void unlock();

private:
#ifdef __MACH__
	os_unfair_lock mLock = OS_UNFAIR_LOCK_INIT;  // the optimal realtime synchronization primitive on Apple platforms
#else
	std::atomic_flag mFlag = ATOMIC_FLAG_INIT;
#endif
};

}  // dfx
