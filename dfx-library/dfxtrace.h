/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2020-2025  Tom Murphy 7

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
------------------------------------------------------------------------*/

#ifndef DFX_TRACE_H
#define DFX_TRACE_H

// Feel free to ignore this! Tom is using it for rudimentary debugging
// of plugins in live hosts where there aren't other good options for
// feedback about what's happening. It pops up a message box if ENABLE_TRACE
// is defined to 1.
//
// XXX remove or improve

// Debugging facilities.

#if !ENABLE_TRACE
  // Trace does nothing.
  #define TRACE(m) do { } while(0)

#else

  #if WIN32
    #include <windows.h>

    #define TRACE(m) do {				  \
      (void)MessageBoxA(nullptr, ("" m ""), __FILE__, 0x0);	  \
    } while (0)

  #else

    #error TRACE unimplemented on this platform.

  #endif

#endif


#endif  // DFX_TRACE_H
