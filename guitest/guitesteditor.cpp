
#include "guitesteditor.h"

#define WINDOWCLASSNAME "GuiTestClass"

// #include <control.h>
#include <commctrl.h>

#define EDIT_HEIGHT 500
#define EDIT_WIDTH 500

LPDIRECTDRAW dd;



extern HINSTANCE instance;
int useCount = 0;
HWND CreateFader (HWND parent, char* title, 
		  int x, int y, int w, int h, 
		  int min, int max);

LONG WINAPI WindowProc (HWND hwnd, UINT message, 
			WPARAM wParam, LPARAM lParam);

GuitestEditor::GuitestEditor (AudioEffect *effect) : AEffEditor (effect) {
  effect->setEditor (this);
}

GuitestEditor::~GuitestEditor () {
}

long GuitestEditor::getRect (ERect **erect) {
  static ERect r = {0, 0, EDIT_HEIGHT, EDIT_WIDTH};
  *erect = &r;
  return true;
}


long GuitestEditor::open (void *ptr) {

  // Remember the parent window
  systemWindow = ptr;
  // Create window class, if we are called the first time
  useCount++;
  if (useCount == 1) {
    WNDCLASS windowClass;
    windowClass.style = 0;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = instance;
    windowClass.hIcon = 0;
    windowClass.hCursor = 0;
    windowClass.hbrBackground = GetSysColorBrush (COLOR_BTNFACE);
    windowClass.lpszMenuName = 0;
    windowClass.lpszClassName = WINDOWCLASSNAME;
    RegisterClass (&windowClass);
  }
  // Create our base window
  HWND hwnd = CreateWindowEx (0, WINDOWCLASSNAME, "Window",
			      WS_CHILD | WS_VISIBLE,
			      0, 0, EDIT_HEIGHT, EDIT_WIDTH,
			      (HWND)systemWindow, NULL, instance, NULL);

  /* store this pointer with window so that callback can
     dispatch to appropriate class instance. */
  SetWindowLong (hwnd, GWL_USERDATA, (long)this);

  HRESULT ddrval;
                
  ddrval = DirectDrawCreate( NULL, &dd, NULL );

  if( ddrval != DD_OK ) return(false);

  ddrval = dd->SetCooperativeLevel( hwnd, DDSCL_NORMAL );

  if( ddrval != DD_OK ) {

    dd->Release();

    return(false);

  }

  if (!CreatePrimarySurface()) return false;

  return true;
}

bool GuitestEditor::CreatePrimarySurface() {
  DDSURFACEDESC ddsd;
  DDSCAPS ddscaps;
  HRESULT ddrval;

  // Create the primary surface with 1 back buffer
  memset( &ddsd, 0, sizeof(ddsd) );
  ddsd.dwSize = sizeof( ddsd );

  ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
  ddsd.dwBackBufferCount = 1;

  ddrval = dd->CreateSurface( &ddsd, &primary, NULL );
  if( ddrval != DD_OK ) {
    dd->Release();
    return(false);
  }

  // Get the pointer to the back buffer
  ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
  ddrval = primary->GetAttachedSurface(&ddscaps, &back);
  if( ddrval != DD_OK ) {
    primary->Release();
    dd->Release();
    return(false);
  }

  return true;
}

IDirectDrawSurface * GuitestEditor::DDLoadBitmap(IDirectDraw *pdd, LPCSTR file) {
  HBITMAP hbm;
  BITMAP bm;
  IDirectDrawSurface *pdds;
                
  // LoadImage has some added functionality in Windows 95 that allows
  // you to load a bitmap from a file on disk.
  hbm = (HBITMAP)LoadImage(NULL, file, IMAGE_BITMAP, 0, 0,
			   LR_LOADFROMFILE | LR_CREATEDIBSECTION);
                
  if (hbm == NULL)
    return NULL;
                
  GetObject(hbm, sizeof(bm), &bm); // get size of bitmap
                
  /*
  * create a DirectDrawSurface for this bitmap
  * source to function CreateOffScreenSurface() follows immediately
  */
                
  pdds = CreateOffScreenSurface(pdd, bm.bmWidth, bm.bmHeight);
  if (pdds) {
    DDCopyBitmap(pdds, hbm, bm.bmWidth, bm.bmHeight);
  }
                
  DeleteObject(hbm);
                
  return pdds;
}

IDirectDrawSurface * CreateOffScreenSurface(IDirectDraw *pdd, int dx, int dy) {
  DDSURFACEDESC ddsd;
  IDirectDrawSurface *pdds;
                
  // create a DirectDrawSurface for this bitmap
  ZeroMemory(&ddsd, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT |DDSD_WIDTH;
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
  ddsd.dwWidth = dx;
  ddsd.dwHeight = dy;
                
  if (pdd->CreateSurface(&ddsd, &pdds, NULL) != DD_OK) return 0;
  else return pdds;

}

HRESULT DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int dx, int dy) {
  HDC hdcImage;
  HDC hdc;
  HRESULT hr;
  HBITMAP hbmOld;
                
  //
  // select bitmap into a memoryDC so we can use it.
  //
  hdcImage = CreateCompatibleDC(NULL);
  hbmOld = (HBITMAP)SelectObject(hdcImage, hbm);
                
  if ((hr = pdds->GetDC(&hdc)) == DD_OK)
    {
      BitBlt(hdc, 0, 0, dx, dy, hdcImage, 0, 0, SRCCOPY);
      pdds->ReleaseDC(hdc);
    }
                
  SelectObject(hdcImage, hbmOld);
  DeleteDC(hdcImage);
                
  return hr;
}

void GuitestEditor::close () {
  useCount--;
  if (useCount == 0) {
    UnregisterClass (WINDOWCLASSNAME, instance);
  }
}


void GuitestEditor::idle () {
  AEffEditor::idle ();
}


void GuitestEditor::update() {
#if 0
  SendMessage (delayFader, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) effect->getParameter (0));
  SendMessage (feedbackFader, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) effect->getParameter
	       (1));
  SendMessage (volumeFader, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) effect->getParameter (1));
#endif
  AEffEditor::update ();
}


void GuitestEditor::setValue(void* fader, int value) {
#if 0
  if (fader == delayFader)
    effect->setParameterAutomated (0, (float)value / 100.f);
  else if (fader == feedbackFader)
    effect->setParameterAutomated (1, (float)value / 100.f);
  else if (fader == volumeFader)
    effect->setParameterAutomated (1, (float)value / 100.f);
#endif
}

/* XXX ? */
void GuitestEditor::postUpdate() {
  AEffEditor::postUpdate ();
}


LONG WINAPI WindowProc (HWND hwnd, UINT message, WPARAM wParam, 
			LPARAM lParam) {
#if 0
  switch (message) {
    case WM_VSCROLL: {

	int newValue = SendMessage ((HWND)lParam, TBM_GETPOS, 0, 0);
	GuitestEditor* editor = 
	  (GuitestEditor*)GetWindowLong (hwnd, GWL_USERDATA);
	if (editor)
	  editor->setValue ((void*)lParam, newValue);
      }
      break;
    }
#endif
  return DefWindowProc (hwnd, message, wParam, lParam);
}

