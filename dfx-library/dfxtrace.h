#ifndef _DFX_TRACE_H
#define _DFX_TRACE_H

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


#endif  // _DFX_TRACE_H
