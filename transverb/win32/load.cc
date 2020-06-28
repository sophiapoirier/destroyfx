#include <windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <winbase.h>

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

  

  return 0;
}
