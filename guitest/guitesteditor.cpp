
#include "guitesteditor.h"

#define WINDOWCLASSNAME "GuiTestClass"

// #include <control.h>
#include <commctrl.h>

#define EDIT_HEIGHT 500
#define EDIT_WIDTH 500



extern HINSTANCE hInstance;
int useCount = 0;
HWND CreateFader (HWND parent, char* title, int x, int y, int w, int h, int min, int max);
LONG WINAPI WindowProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


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
#if 0
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
      windowClass.hInstance = hInstance;
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
			      (HWND)systemWindow, NULL, hInstance, NULL);
  SetWindowLong (hwnd, GWL_USERDATA, (long)this);
  // Create three fader controls
  delayFader = CreateFader (hwnd, "Delay", 10, 10, 35, 300, 0, 100);
  feedbackFader = CreateFader (hwnd, "Feedback", 50, 10, 64, 300, 0, 100);

  volumeFader = CreateFader (hwnd, "Volume", 300, 12, 32, 350, 0, 100);
#endif
  return true;
}


void GuitestEditor::close () {
#if 0
  useCount--;
  if (useCount == 0) {
      UnregisterClass (WINDOWCLASSNAME, hInstance);
    }
#endif
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
void GuitestEditor::postUpdate() {}


HWND CreateFader (HWND parent, char* title, 
		  int x, int y, int w, int h, 
		  int min, int max) {
  HWND hwndTrack = CreateWindowEx (0, TRACKBAR_CLASS, title,
				   WS_CHILD | WS_VISIBLE |
				   TBS_NOTICKS | TBS_ENABLESELRANGE | TBS_VERT,
				   x, y, w, h, parent, NULL, hInstance, NULL);
  SendMessage (hwndTrack, TBM_SETRANGE, (WPARAM ) TRUE,
	       (LPARAM) MAKELONG (min, max));
  SendMessage (hwndTrack, TBM_SETPAGESIZE, 0, (LPARAM) 4);
  SendMessage (hwndTrack, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) min);
  return hwndTrack;
}


LONG WINAPI WindowProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_VSCROLL: {
#if 0
	int newValue = SendMessage ((HWND)lParam, TBM_GETPOS, 0, 0);
	GuitestEditor* editor = 
	  (GuitestEditor*)GetWindowLong (hwnd, GWL_USERDATA);
	if (editor)
	  editor->setValue ((void*)lParam, newValue);
#endif
      }
      break;
    }
  return DefWindowProc (hwnd, message, wParam, lParam);
}

