
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
  
  LPDIRECTDRAWSURFACE primary; // DirectDraw primary surface
  
  LPDIRECTDRAWSURFACE back; // DirectDraw back surface

  bool CreatePrimarySurface();

  static IDirectDrawSurface * DDLoadBitmap(IDirectDraw *pdd, LPCSTR szBitmap);
  static IDirectDrawSurface * CreateOffScreenSurface(IDirectDraw *pdd, int dx, int dy);
  static HRESULT DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int dx, int dy);

  static LONG WINAPI WindowProc (HWND hwnd, UINT message, WPARAM wParam, 
				 LPARAM lParam);

  void redraw();

  HWND window;
  IDirectDrawSurface * bg, * guit;

  int guitx, guity, guitdy, guitdx;

  public:
  void setValue(void* fader, int value);

};

#endif
