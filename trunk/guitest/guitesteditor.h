
#ifndef __DFX_GUITESTEDITOR_H
#define __DFX_GUITESTEDITOR_H

#include <audioeffectx.h>
#include <aeffeditor.hpp>
#include <windows.h>
#include <ddraw.h>

struct GuitestEditor : public AEffEditor {
  friend class AudioEffect;
  
  virtual void update();
  virtual void postUpdate();

  GuitestEditor(AudioEffect *effect);
  virtual ~GuitestEditor();
  virtual long getRect(ERect **rect);
  virtual long open(void *ptr);
  virtual void close();
  virtual void idle();

  private:
  
  LPDIRECTDRAWSURFACE7 primary; // DirectDraw primary surface
  
  LPDIRECTDRAWSURFACE7 back; // DirectDraw back surface

  bool CreatePrimarySurface();

  static IDirectDrawSurface7 * DDLoadBitmap(IDirectDraw7 *pdd, LPCSTR szBitmap);
  static IDirectDrawSurface7 * CreateOffScreenSurface(IDirectDraw7 *pdd, int dx, int dy);
  static HRESULT DDCopyBitmap(IDirectDrawSurface7 *pdds, HBITMAP hbm, int dx, int dy);

  static LONG WINAPI WindowProc (HWND hwnd, UINT message, WPARAM wParam, 
				 LPARAM lParam);

  void redraw();

  HWND window;
  IDirectDrawSurface7 * bg, * guit;

  int guitx, guity, guitdy, guitdx;

  public:
  void setValue(void* fader, int value);

};

#endif
