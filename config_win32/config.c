#include <windows.h>

#include "resource.h" 

#include <commctrl.h>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow) {

  InitCommonControls ();

  MessageBox(NULL, "THIS IS DFX CONFIG APP !!!!", "Warning", MB_OK);

  return 0;

}
