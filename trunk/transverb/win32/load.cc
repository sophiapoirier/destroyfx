#include <windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <winbase.h>
#include <cstdint>
#include <inttypes.h>
#include <string>

#include "pluginterfaces/vst2.x/aeffect.h"

using uint32 = uint32_t;
using uint8 = uint8_t;
using string = std::string;

static void Error(const char *msg) {
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError();

  FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &lpMsgBuf,
      0, NULL);

  // Display the error message and exit the process

  lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
				    (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)msg) + 40) * sizeof(TCHAR));
  StringCchPrintf((LPTSTR)lpDisplayBuf,
		  LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		  TEXT("%s failed with error %d: %s"),
		  msg, dw, lpMsgBuf);


  printf("Failed:\n%s\n", (LPCTSTR)lpDisplayBuf);

  //   MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
}

using VstInt32 = int32_t;
using VstInt16 = int16_t;
using VstIntPtr = intptr_t;  // XXX??

typedef	VstIntPtr __cdecl (*audioMasterCallback) (
    AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);

typedef __cdecl AEffect* (*VSTPluginMainType)(audioMasterCallback);

VstIntPtr __cdecl Callback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
  printf("== Callback ==\n"
	 "Opcode %d\n"
	 "Index %d\n"
	 "Value %" PRIi64 "\n"
	 "Ptr %p\n"
	 "Opt %.2f\n",
	 opcode, index, (int64_t)value, ptr, opt);

  switch (opcode) {
  case 0:
    // Automate.
    return 0;
  case 1:
    // Version
    return 2400;
  case 2:
    // Unique ID
    return 0xBEEF;
  case 3:
    // Idle
    return 0;

  default:
    printf(" ! Unknown opcode %d\n", opcode);
    return 0;
  }
}


int main(int argc, char **argv) {
  if (argc != 2) {
    printf("./load something.dll\n");
    return 1;
  }

  const char *dllfile = argv[1];

  // A means ANSI (as opposed to W for Wide Unicode UTF-16)
  HMODULE lib = LoadLibraryExA(dllfile,
			       // must be null
			       nullptr,
			       // flags, 0 looks to be "normal"
			       0);

  if (lib == nullptr) {
    Error("LoadLibraryExA failed (null)");
    return 1;
  }

  printf("Loaded.\n");

  FARPROC proc = GetProcAddress(lib, "VSTPluginMain");
  if (proc == nullptr) {
    Error("GetProcAddress failed on VSTPluginMain (null)\n");
    return 2;
  }

  printf("Got VSTPluginMain.\n");

  VSTPluginMainType vst_plugin_main = (VSTPluginMainType)proc;

  AEffect *aeffect = vst_plugin_main(Callback);

  printf("Got AEffect.\n");

  if (aeffect->magic != CCONST('V', 's', 't', 'P')) {
    printf("Wrong aeffect magic: %8x\n", aeffect->magic);
    return 3;
  }

  auto ch = [uid = aeffect->uniqueID](int b) -> char {
      uint8 c = 255 & (uid >> (b * 8));
      if (c < 32 || c > 127) {
	return '?';
      } else {
	return c;
      }
    };


  uint32 flags = aeffect->flags;
  string flagstring;
  if (flags & effFlagsHasEditor) flagstring += "has editor, ";
  if (flags & effFlagsCanReplacing) flagstring += "replacing, ";
  if (flags & effFlagsProgramChunks) flagstring += "chunks, ";
  if (flags & effFlagsIsSynth) flagstring += "synth, ";
  if (flags & effFlagsNoSoundInStop) flagstring += "no sound in stop, ";
  if (flags & effFlagsCanDoubleReplacing) flagstring += "double replacing, ";
  if (flags & (1 << 1)) flagstring += "(deprecated) has clip, ";
  if (flags & (1 << 2)) flagstring += "(deprecated) has vu, ";
  if (flags & (1 << 3)) flagstring += "(deprecated) can mono, ";
  if (flags & (1 << 10)) flagstring += "(deprecated) async, ";
  if (flags & (1 << 11)) flagstring += "(deprecated) buffer, ";


  printf("numPrograms %d\n"
	 "numParams %d\n"
	 "numInputs %d\n"
	 "numOutputs %d\n"
	 "flags %8x (%s)\n"
	 "resvd1 %" PRIiPTR "\n"
	 "resvd2 %" PRIiPTR "\n"
	 "initialDelay %d\n"
	 "uniqueID %d (%c%c%c%c)\n"
	 "version %d\n",
	 aeffect->numPrograms,
	 aeffect->numParams,
	 aeffect->numInputs,
	 aeffect->numOutputs,
	 aeffect->flags, flagstring.c_str(),
	 aeffect->resvd1,
	 aeffect->resvd2,
	 aeffect->initialDelay,
	 aeffect->uniqueID, ch(3), ch(2), ch(1), ch(0),
	 aeffect->version);

  return 0;
}
