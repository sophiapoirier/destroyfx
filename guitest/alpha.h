/*--------------------------------

  alpha.h

  Headerfile of alpha.cpp

  -------------------------------*/

#ifndef _ALPHA_H_
#define _ALPHA_H_

#include <ddraw.h>

/*
typedef LPDIRECTDRAWSURFACE LPDIRECTDRAWSURFACE7;
typedef DDSURFACEDESC DDSURFACEDESC2;
*/

//
// Define macros that identify certain display modes.
//
#define RGBMODE_555		0
#define RGBMODE_565		1
#define RGBMODE_16      2
#define RGBMODE_24		3
#define RGBMODE_32		4
#define RGBMODE_NONE	5

//
// Function prototypes.
//
DWORD GetRGBMode( LPDIRECTDRAWSURFACE7 lpDDS );

int BltAlpha( LPDIRECTDRAWSURFACE7 lpDDSDest, LPDIRECTDRAWSURFACE7 lpDDSSource, 
			 int iDestX, int iDestY, LPRECT lprcSource, int iAlpha, DWORD dwMode );

int BltAlphaMMX( LPDIRECTDRAWSURFACE7 lpDDSDest, LPDIRECTDRAWSURFACE7 lpDDSSource, 
			 int iDestX, int iDestY, LPRECT lprcSource, int iAlpha, DWORD dwMode );

int BltAlphaFast( LPDIRECTDRAWSURFACE7 lpDDSDest, LPDIRECTDRAWSURFACE7 lpDDSSource, 
			 int iDestX, int iDestY, LPRECT lprcSource, DWORD dwMode );

int BltAlphaFastMMX( LPDIRECTDRAWSURFACE7 lpDDSDest, LPDIRECTDRAWSURFACE7 lpDDSSource, 
			 int iDestX, int iDestY, LPRECT lprcSource, DWORD dwMode );

#endif // _ALPHA_H_



