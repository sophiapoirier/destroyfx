
#ifndef __DFX_GUITESTEDITOR_H
#define __DFX_GUITESTEDITOR_H

#include <audioeffectx.h>
#include <aeffeditor.hpp>
#include <windows.h>
// #include <ddraw.h>

#include <commctrl.h>
#include <d3d8.h>
#include <d3dx8math.h>

#include "graphic.h"

struct Where {
  int x;
  int y;
  int dx;
  int dy; 
};

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

  static LONG WINAPI WindowProc (HWND hwnd, UINT message, WPARAM wParam, 
				 LPARAM lParam);

  HRESULT InitD3D(HWND hWnd);

  void Render2D ();

  void redraw();

  HWND window;



 public:
  void setValue(void* fader, int value);

  LPDIRECT3D8             pD3D;  //     = NULL; // Used to create the D3DDevice
  LPDIRECT3DDEVICE8       pd3dDevice;// = NULL; // Our rendering device
  
  /* XXX */

  Graphic * ggg;

  Where * www;

};

#endif
