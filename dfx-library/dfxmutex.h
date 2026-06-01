/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2026  Sophia Poirier

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
This is our mutually exclusive shit.
------------------------------------------------------------------------*/

#pragma once


#include <atomic>

#include "dfx-base.h"



namespace dfx
{

// A typical mutex can block when unlocking if another thread is waiting to acquire the lock, 
// because the kernel must wake up the waiting thread, making it unsafe to use in realtime contexts, 
// even if the realtime thread avoids waiting when acquiring the lock (e.g. using try_lock).  
// A lightweight spinlock, while having the behavioral disadvantages of busy waiting to lock, 
// can meet the wait-free / try_lock performance requirements for such use cases.
// It is best used with very brief locking periods so that the non-realtime locking context
// does not burn cycles excessively while busy waiting.
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
	void lock() noexcept;
	bool try_lock() noexcept DFX_RT_ATTR;
	void unlock() noexcept DFX_RT_ATTR;

private:
	std::atomic_flag mFlag = ATOMIC_FLAG_INIT;
};

}  // dfx
