/*--------------------------------------------------------------------------------------

  Module: alpha.hpp

  Author: Matthias Hofmann

  Implements routines related to alpha blending.

----------------------------------------------------------------------------------------*/

#include "alpha.h"


/*
 * Description:		Determines the exact RGB mode used by a surface.
 *
 * Parameters:		lpDD7 - An IDirectDrawSurface7 interface.
 *
 * Return value:	One of the following predefined values:
 *					RGBMODE_555	 - 16 bit mode ( 555 )
 *					RBGMODE_565	 - 16 bit mode ( 565 )
 *					RGBMODE_16	 - 16 bit mode ( unknown )
 *					RGBMODE_24   - 24 bit mode
 *					RGBMODE_32   - 32 bit mode
 *					RGBMODE_NONE - Palletized mode.
 *
 */
DWORD GetRGBMode( LPDIRECTDRAWSURFACE7 lpDDS )
{
	DDSURFACEDESC2	ddsd;


	//
	// Initialize the surface description structure.
	//
	memset( &ddsd, 0, sizeof ddsd );
	ddsd.dwSize = sizeof ddsd;

	// Get the description of the surface.
	lpDDS->GetSurfaceDesc( &ddsd );

	// If we are in 32 bit mode ...
	if ( ddsd.ddpfPixelFormat.dwRGBBitCount == 32 )
	{
		// ... inform the caller.
		return RGBMODE_32;
	}

	// If we are in 24 bit mode ...
	if ( ddsd.ddpfPixelFormat.dwRGBBitCount == 24 )
	{
		// ... inform the caller.
		return RGBMODE_24;
	}

	// If we are in 16 bit mode ...
	if ( ddsd.ddpfPixelFormat.dwRGBBitCount == 16 )
	{
		// 
		// ... determine the exact mode.
		//

		// If we are in 565 mode ...
		if ( ddsd.ddpfPixelFormat.dwRBitMask == ( 31 << 11 ) &&
			 ddsd.ddpfPixelFormat.dwGBitMask == ( 63 << 5 ) &&
			 ddsd.ddpfPixelFormat.dwBBitMask == 31 )
		{
			// ... inform the caller.
			return RGBMODE_565;
		}

		// If we are in 555 mode ...
		if ( ddsd.ddpfPixelFormat.dwRBitMask == ( 31 << 10 ) &&
			 ddsd.ddpfPixelFormat.dwGBitMask == ( 31 << 5 ) &&
			 ddsd.ddpfPixelFormat.dwBBitMask == 31 )
		{
			// ... inform the caller.
			return RGBMODE_555;
		}

		// We got an unknown 16 bit mode.
		return RGBMODE_16;
	}

	// Any other mode must be a palletized one.
	return RGBMODE_NONE;
}

/*
 * Description:		Performs a blit operation while allowing for a variable alpha value.
 *					The function uses black as a color key for the blit operation.
 *
 * Parameters:		lpDDSDest   - The destination surface of the blit.
 *
 *					lpDDSSource - The source surface of the blit.
 *
 *					iDestX      - The horizontal coordinate to blit to on 
 *								  the destination surface. 
 *
 *					iDestY      - The vertical coordinate to blit to on the
 *								  destination surface.
 *
 *					lprcSource  - The address of a RECT structure that defines 
 *								  the upper-left and lower-right corners of the 
 *								  rectangle to blit from on the source surface. 
 *
 *					iAlpha      - A value in the range from 0 to 256 that 
 *								  determines the opacity of the source.
 *
 *					dwMode		- One of the following predefined values:
 *								  RGBMODE_555	- 16 bit mode ( 555 )
 *								  RBGMODE_565	- 16 bit mode ( 565 )
 *								  RGBMODE_16	- 16 bit mode ( unknown )
 *								  RGBMODE_24    - 24 bit mode
 *								  RGBMODE_32    - 32 bit mode
 *
 * Return value:	The functions returns 0 to indicate success or -1 if the call fails.
 *
 */
int BltAlpha( LPDIRECTDRAWSURFACE7 lpDDSDest, LPDIRECTDRAWSURFACE7 lpDDSSource, 
			 int iDestX, int iDestY, LPRECT lprcSource, int iAlpha, DWORD dwMode )
{
	DDSURFACEDESC2	ddsdSource;
	DDSURFACEDESC2	ddsdTarget;
	RECT			rcDest;
	DWORD			dwTargetPad;
	DWORD			dwSourcePad;
	DWORD			dwTargetTemp;
	DWORD			dwSourceTemp;
	DWORD			dwSrcRed, dwSrcGreen, dwSrcBlue;
	DWORD			dwTgtRed, dwTgtGreen, dwTgtBlue;
	DWORD			dwRed, dwGreen, dwBlue;
	DWORD			dwAdd64;
	DWORD			dwAlphaOver4;
	BYTE*			lpbTarget;
	BYTE*			lpbSource;
	int				iWidth;
	int				iHeight;
	bool			gOddWidth;
	int				iRet = 0;
	int				i;


	//
	// Enforce the lower limit for the alpha value.
	//
	if ( iAlpha < 0 )
		iAlpha = 0;

	//
	// Enforce the upper limit for the alpha value.
	//
	if ( iAlpha > 256 )
		iAlpha = 256;

	//
	// Determine the dimensions of the source surface.
	//
	if ( lprcSource )
	{
		//
		// Get the width and height from the passed rectangle.
		//
		iWidth = lprcSource->right - lprcSource->left;
		iHeight = lprcSource->bottom - lprcSource->top; 
	}
	else
	{
		//
		// Get the with and height from the surface description.
		//
		memset( &ddsdSource, 0, sizeof ddsdSource );
		ddsdSource.dwSize = sizeof ddsdSource;
		ddsdSource.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		lpDDSSource->GetSurfaceDesc( &ddsdSource );

		//
		// Remember the dimensions.
		//
		iWidth = ddsdSource.dwWidth;
		iHeight = ddsdSource.dwHeight;
	}

	//
	// Calculate the rectangle to be locked in the target.
	//
	rcDest.left = iDestX;
	rcDest.top = iDestY;
	rcDest.right = iDestX + iWidth;
	rcDest.bottom = iDestY + iHeight;

	//
	// Lock down the destination surface.
	//
	memset( &ddsdTarget, 0, sizeof ddsdTarget );
	ddsdTarget.dwSize = sizeof ddsdTarget;
	lpDDSDest->Lock( &rcDest, &ddsdTarget, DDLOCK_WAIT, NULL );  

	//
	// Lock down the source surface.
	//
	memset( &ddsdSource, 0, sizeof ddsdSource );
	ddsdSource.dwSize = sizeof ddsdSource;
	lpDDSSource->Lock( lprcSource, &ddsdSource, DDLOCK_WAIT, NULL );

	//
	// Perform the blit operation.
	//
	switch ( dwMode )
	{
	/* 16 bit mode ( 555 ). This algorithm 
	   can process two pixels at once. */
	case RGBMODE_555:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		// If the width is odd ...
		if ( iWidth & 0x01 )
		{
			// ... set the flag ...
			gOddWidth = true;

			// ... and calculate the width.
			iWidth = ( iWidth - 1 ) / 2;
		}
		// If the width is even ...
		else
		{
			// ... clear the flag ...
			gOddWidth = false;

			// ... and calculate the width.
			iWidth /= 2;
		}

		//
		// Calculate certain auxiliary values.
		//
		dwAdd64 = 64 | ( 64 << 16 );
		dwAlphaOver4 = ( iAlpha / 4 ) | ( ( iAlpha / 4 ) << 16 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend two pixels at once.
			//
			while ( i-- > 0 )
			{
				// Read in two source pixels.
				dwSourceTemp = *( ( DWORD* ) lpbSource );

				// If the two source pixels are not both black ...
				if ( dwSourceTemp != 0 )
				{
					// ... read in two target pixels.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = ( dwTargetTemp >> 10 ) & 0x001f001f;
					dwSrcRed = ( dwSourceTemp >> 10 ) & 0x001f001f;

					// Extract the green channels.
					dwTgtGreen = ( dwTargetTemp >> 5 ) & 0x001f001f;
					dwSrcGreen = ( dwSourceTemp >> 5 ) & 0x001f001f;

					// Extract the blue channel.
					dwTgtBlue = dwTargetTemp & 0x001f001f;
					dwSrcBlue = dwSourceTemp & 0x001f001f;

					// Calculate the alpha-blended red channel.
					dwRed = ( ( ( ( iAlpha * ( dwSrcRed + dwAdd64 - dwTgtRed ) ) >> 8 ) 
						+ dwTgtRed - dwAlphaOver4 ) & 0x001f001f ) << 10;

					// Calculate the alpha-blended green channel.
					dwGreen = ( ( ( ( iAlpha * ( dwSrcGreen + dwAdd64 - dwTgtGreen ) ) >> 8 ) 
						+ dwTgtGreen - dwAlphaOver4 ) & 0x001f001f ) << 5; 

					// Calculate the alpha-blended blue channel.
					dwBlue = ( ( ( iAlpha * ( dwSrcBlue + dwAdd64 - dwTgtBlue ) ) >> 8 ) 
						+ dwTgtBlue - dwAlphaOver4 ) & 0x001f001f;

					// If the first source pixel is black ...
					if ( ( dwSourceTemp >> 16 ) == 0 )
					{
						// ... leave the corresponding target pixel unchanged.
						*( ( DWORD* ) lpbTarget ) = ( ( dwRed | dwGreen | dwBlue ) & 0xffff ) | 
							( dwTargetTemp & 0xffff0000 );
					}
					// If the second source pixel is black ...
					else if ( ( dwSourceTemp & 0xffff ) == 0 )
					{
						// ... leave the corresponding target pixel unchanged.
						*( ( DWORD* ) lpbTarget ) = ( ( dwRed | dwGreen | dwBlue ) & 0xffff0000 ) | 
							( dwTargetTemp & 0xffff ); 
					}
					// If none of the pixels is black ...
					else
					{
						// ... write both of the new pixels to the target.
						*( ( DWORD* ) lpbTarget ) = ( dwRed | dwGreen | dwBlue );
					}
				}

				//
				// Proceed to the next two pixels.
				//
				lpbTarget += 4;
				lpbSource += 4;
			} 

			//
			// Handle an odd width.
			//
			if ( gOddWidth )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = ( dwTargetTemp >> 10 ) & 0x1f;
					dwSrcRed = ( dwSourceTemp >> 10 ) & 0x1f;

					// Extract the green channels.
					dwTgtGreen = ( dwTargetTemp >> 5 ) & 0x1f;
					dwSrcGreen = ( dwSourceTemp >> 5 ) & 0x1f;

					// Extract the blue channels.
					dwTgtBlue = dwTargetTemp & 0x1f;
					dwSrcBlue = dwSourceTemp & 0x1f;

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( iAlpha * ( dwSrcRed - dwTgtRed ) >> 8 ) + dwTgtRed ) << 10 |
						  ( ( iAlpha * ( dwSrcGreen - dwTgtGreen ) >> 8 ) + dwTgtGreen ) << 5 |
						  ( ( iAlpha * ( dwSrcBlue - dwTgtBlue ) >> 8 ) + dwTgtBlue ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}
			 
			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		} 
		while ( --iHeight > 0 );

		break;

	/* 16 bit mode ( 565 ). This algorithm 
	   can process two pixels at once. */
	case RGBMODE_565:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		// If the width is odd ...
		if ( iWidth & 0x01 )
		{
			// ... set the flag ...
			gOddWidth = true;

			// ... and calculate the width.
			iWidth = ( iWidth - 1 ) / 2;
		}
		// If the width is even ...
		else
		{
			// ... clear the flag ...
			gOddWidth = false;

			// ... and calculate the width.
			iWidth /= 2;
		}

		//
		// Calculate certain auxiliary values.
		//
		dwAdd64 = 64 | ( 64 << 16 );
		dwAlphaOver4 = ( iAlpha / 4 ) | ( ( iAlpha / 4 ) << 16 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend two pixels at once.
			//
			while ( i-- > 0 )
			{
				// Read in two source pixels.
				dwSourceTemp = *( ( DWORD* ) lpbSource );

				// If the two source pixels are not both black ...
				if ( dwSourceTemp != 0 )
				{
					// ... read in two target pixels.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = ( dwTargetTemp >> 11 ) & 0x001f001f;
					dwSrcRed = ( dwSourceTemp >> 11 ) & 0x001f001f;

					// Extract the green channels.
					dwTgtGreen = ( dwTargetTemp >> 5 ) & 0x003f003f;
					dwSrcGreen = ( dwSourceTemp >> 5 ) & 0x003f003f;

					// Extract the blue channel.
					dwTgtBlue = dwTargetTemp & 0x001f001f;
					dwSrcBlue = dwSourceTemp & 0x001f001f;

					// Calculate the alpha-blended red channel.
					dwRed = ( ( ( ( iAlpha * ( dwSrcRed + dwAdd64 - dwTgtRed ) ) >> 8 ) 
						+ dwTgtRed - dwAlphaOver4 ) & 0x001f001f ) << 11;

					// Calculate the alpha-blended green channel.
					dwGreen = ( ( ( ( iAlpha * ( dwSrcGreen + dwAdd64 - dwTgtGreen ) ) >> 8 ) 
						+ dwTgtGreen - dwAlphaOver4 ) & 0x003f003f ) << 5; 

					// Calculate the alpha-blended blue channel.
					dwBlue = ( ( ( iAlpha * ( dwSrcBlue + dwAdd64 - dwTgtBlue ) ) >> 8 ) 
						+ dwTgtBlue - dwAlphaOver4 ) & 0x001f001f;

					// If the first source pixel is black ...
					if ( ( dwSourceTemp >> 16 ) == 0 )
					{
						// ... leave the corresponding target pixel unchanged.
						*( ( DWORD* ) lpbTarget ) = ( ( dwRed | dwGreen | dwBlue ) & 0xffff ) | 
							( dwTargetTemp & 0xffff0000 );
					}
					// If the second source pixel is black ...
					else if ( ( dwSourceTemp & 0xffff ) == 0 )
					{
						// ... leave the corresponding target pixel unchanged.
						*( ( DWORD* ) lpbTarget ) = ( ( dwRed | dwGreen | dwBlue ) & 0xffff0000 ) | 
							( dwTargetTemp & 0xffff ); 
					}
					// If none of the pixels is black ...
					else
					{
						// ... write both of the new pixels to the target.
						*( ( DWORD* ) lpbTarget ) = ( dwRed | dwGreen | dwBlue );
					}
				}

				//
				// Proceed to the next two pixels.
				//
				lpbTarget += 4;
				lpbSource += 4;
			} 

			//
			// Handle an odd width.
			//
			if ( gOddWidth )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = ( dwTargetTemp >> 11 ) & 0x1f;
					dwSrcRed = ( dwSourceTemp >> 11 ) & 0x1f;

					// Extract the green channels.
					dwTgtGreen = ( dwTargetTemp >> 5 ) & 0x3f;
					dwSrcGreen = ( dwSourceTemp >> 5 ) & 0x3f;

					// Extract the blue channels.
					dwTgtBlue = dwTargetTemp & 0x1f;
					dwSrcBlue = dwSourceTemp & 0x1f;

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( iAlpha * ( dwSrcRed - dwTgtRed ) >> 8 ) + dwTgtRed ) << 11 |
						  ( ( iAlpha * ( dwSrcGreen - dwTgtGreen ) >> 8 ) + dwTgtGreen ) << 5 |
						  ( ( iAlpha * ( dwSrcBlue - dwTgtBlue ) >> 8 ) + dwTgtBlue ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}
			 
			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		} 
		while ( --iHeight > 0 );

		break;

	/* 16 bit mode ( unknown ). This algorithm 
	   processes only one pixel at a time. */
	case RGBMODE_16:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend the pixels in the current row.
			//
			while ( i-- > 0 )
			{
				// Read in the next source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If the source pixel is not black ...
				if ( dwSourceTemp != 0 )
				{
					// ... read in the next target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = dwTargetTemp & ddsdTarget.ddpfPixelFormat.dwRBitMask;
					dwSrcRed = dwSourceTemp & ddsdSource.ddpfPixelFormat.dwRBitMask;

					// Extract the green channels.
					dwTgtGreen = dwTargetTemp & ddsdTarget.ddpfPixelFormat.dwGBitMask;
					dwSrcGreen = dwSourceTemp & ddsdSource.ddpfPixelFormat.dwGBitMask;

					// Extract the blue channel.
					dwTgtBlue = dwTargetTemp & ddsdTarget.ddpfPixelFormat.dwBBitMask;
					dwSrcBlue = dwSourceTemp & ddsdSource.ddpfPixelFormat.dwBBitMask;

					// Calculate the alpha-blended red channel.
					dwRed = ( ( ( iAlpha * ( dwSrcRed - dwTgtRed ) ) >> 8 ) + 
						dwTgtRed ) & ddsdTarget.ddpfPixelFormat.dwRBitMask;

					// Calculate the alpha-blended green channel.
					dwGreen = ( ( ( iAlpha * ( dwSrcGreen - dwTgtGreen ) ) >> 8 ) + 
						dwTgtGreen ) & ddsdTarget.ddpfPixelFormat.dwGBitMask;

					// Calculate the alpha-blended blue channel.
					dwBlue = ( ( ( iAlpha * ( dwSrcBlue - dwTgtBlue ) ) >> 8 ) + 
						dwTgtBlue ) & ddsdTarget.ddpfPixelFormat.dwBBitMask;

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) ( dwRed | dwGreen | dwBlue );
				}

				//
				// Proceed to the next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			} 
			 
			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		} 
		while ( --iHeight > 0 );

		break;

	/* 24 bit mode. */
	case RGBMODE_24:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 3 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 3 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend the pixels in the current row.
			//
			while ( i-- > 0 )
			{
				// Read in the next source pixel.
				dwSourceTemp = *( ( DWORD* ) lpbSource );

				// If the source pixel is not black ...
				if ( ( dwSourceTemp & 0xffffff ) != 0 )
				{
					// ... read in the next target pixel.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = ( dwTargetTemp >> 16 ) & 0xff;
					dwSrcRed = ( dwSourceTemp >> 16 ) & 0xff;

					// Extract the green channels.
					dwTgtGreen = ( dwTargetTemp >> 8 ) & 0xff;
					dwSrcGreen = ( dwSourceTemp >> 8 ) & 0xff;

					// Extract the blue channel.
					dwTgtBlue = dwTargetTemp & 0xff;
					dwSrcBlue = dwSourceTemp & 0xff;
							
					// Calculate the destination pixel.
					dwTargetTemp = 
						( ( ( iAlpha * ( dwSrcRed - dwTgtRed ) >> 8 ) + dwTgtRed ) << 16 |
						  ( ( iAlpha * ( dwSrcGreen - dwTgtGreen ) >> 8 ) + dwTgtGreen ) << 8 |
						  ( ( iAlpha * ( dwSrcBlue - dwTgtBlue ) >> 8 ) + dwTgtBlue ) );

					//
					// Write the destination pixel.
					//
					*( ( WORD* ) lpbTarget ) = ( WORD ) ( dwTargetTemp & 0xffff );
					lpbTarget += 2;
					*lpbTarget = ( BYTE ) ( dwTargetTemp >> 16 );
					lpbTarget++;
				}
				// If the source pixel is our color key ...
				else
				{
					// ... advance the target pointer.
					lpbTarget += 3;
				}

				// Proceed to the next source pixel.
				lpbSource += 3;
			} 
			 
			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		} 
		while ( --iHeight > 0 );

		break;

	/* 32 bit mode. */
	case RGBMODE_32:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 4 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 4 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend the pixels in the current row.
			//
			while ( i-- > 0 )
			{
				// Read in the next source pixel.
				dwSourceTemp = *( ( DWORD* ) lpbSource );

				// If the source pixel is not black ...
				if ( ( dwSourceTemp & 0xffffff ) != 0 )
				{
					// ... read in the next target pixel.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = dwTargetTemp & 0xff0000;
					dwSrcRed = dwSourceTemp & 0xff0000;

					// Extract the green channels.
					dwTgtGreen = dwTargetTemp & 0xff00;
					dwSrcGreen = dwSourceTemp & 0xff00;

					// Extract the blue channel.
					dwTgtBlue = dwTargetTemp & 0xff;
					dwSrcBlue = dwSourceTemp & 0xff;
							
					// Calculate the destination pixel.
					dwTargetTemp = 
						( ( ( ( iAlpha * ( dwSrcRed - dwTgtRed ) >> 8 ) + dwTgtRed ) & 0xff0000 ) |
						  ( ( ( iAlpha * ( dwSrcGreen - dwTgtGreen ) >> 8 ) + dwTgtGreen ) & 0xff00 ) |
						    ( ( iAlpha * ( dwSrcBlue - dwTgtBlue ) >> 8 ) + dwTgtBlue ) );

					// Write the destination pixel.
					*( ( DWORD* ) lpbTarget ) = dwTargetTemp;
				}

				//
				// Proceed to the next pixel.
				//
				lpbTarget += 4;
				lpbSource += 4;
			} 
			 
			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		} 
		while ( --iHeight > 0 );

		break;

	/* Invalid mode. */
	default:
		iRet = -1;
	}

	// Unlock the target surface.
	lpDDSDest->Unlock( &rcDest );

	// Unlock the source surface.
	lpDDSSource->Unlock( lprcSource );

	// Return the result.
	return iRet;
}

/*
 * Description:		Performs a blit operation while allowing for a variable alpha value,
 *					making use of MMX technology. The function uses black as a color key 
 *					for the blit operation.
 *
 * Parameters:		lpDDSDest   - The destination surface of the blit.
 *
 *					lpDDSSource - The source surface of the blit.
 *
 *					iDestX      - The horizontal coordinate to blit to on 
 *								  the destination surface. 
 *
 *					iDestY      - The vertical coordinate to blit to on the
 *								  destination surface.
 *
 *					lprcSource  - The address of a RECT structure that defines 
 *								  the upper-left and lower-right corners of the 
 *								  rectangle to blit from on the source surface. 
 *
 *					iAlpha      - A value in the range from 0 to 256 that 
 *								  determines the opacity of the source.
 *
 *					dwMode		- One of the following predefined values:
 *								  RGBMODE_555	- 16 bit mode ( 555 )
 *								  RBGMODE_565	- 16 bit mode ( 565 )
 *								  RGBMODE_16	- 16 bit mode ( unknown )
 *								  RGBMODE_24    - 24 bit mode
 *								  RGBMODE_32    - 32 bit mode
 *
 * Return value:	The functions returns 0 to indicate success or -1 if the call fails.
 *
 */
int BltAlphaMMX( LPDIRECTDRAWSURFACE7 lpDDSDest, LPDIRECTDRAWSURFACE7 lpDDSSource, 
			 int iDestX, int iDestY, LPRECT lprcSource, int iAlpha, DWORD dwMode )
{
	DDSURFACEDESC2	ddsdSource;
	DDSURFACEDESC2	ddsdTarget;
	RECT			rcDest;
	DWORD			dwTargetPad;
	DWORD			dwSourcePad;
	DWORD			dwTargetTemp;
	DWORD			dwSourceTemp;
	DWORD			dwSrcRed, dwSrcGreen, dwSrcBlue;
	DWORD			dwTgtRed, dwTgtGreen, dwTgtBlue;
	DWORD			dwRed, dwGreen, dwBlue;
	BYTE*			lpbTarget;
	BYTE*			lpbSource;
	__int64			i64MaskRed;
	__int64			i64MaskGreen;
	__int64			i64MaskBlue;
	__int64			i64Alpha;
	__int64			i64RdShift = 0;
	__int64			i64GrShift = 0;
	__int64			i64BlShift = 0;
	__int64			i64Mask;
	int				iWidth;
	int				iHeight;
	int				iRemainder;
	bool			gOddWidth;
	int				iRet = 0;
	int				i;


	//
	// Enforce the lower limit for the alpha value.
	//
	if ( iAlpha < 0 )
		iAlpha = 0;

	//
	// Enforce the upper limit for the alpha value.
	//
	if ( iAlpha > 256 )
		iAlpha = 256;

	//
	// Determine the dimensions of the source surface.
	//
	if ( lprcSource )
	{
		//
		// Get the width and height from the passed rectangle.
		//
		iWidth = lprcSource->right - lprcSource->left;
		iHeight = lprcSource->bottom - lprcSource->top; 
	}
	else
	{
		//
		// Get the with and height from the surface description.
		//
		memset( &ddsdSource, 0, sizeof ddsdSource );
		ddsdSource.dwSize = sizeof ddsdSource;
		ddsdSource.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		lpDDSSource->GetSurfaceDesc( &ddsdSource );

		//
		// Remember the dimensions.
		//
		iWidth = ddsdSource.dwWidth;
		iHeight = ddsdSource.dwHeight;
	}

	//
	// Calculate the rectangle to be locked in the target.
	//
	rcDest.left = iDestX;
	rcDest.top = iDestY;
	rcDest.right = iDestX + iWidth;
	rcDest.bottom = iDestY + iHeight;

	//
	// Lock down the destination surface.
	//
	memset( &ddsdTarget, 0, sizeof ddsdTarget );
	ddsdTarget.dwSize = sizeof ddsdTarget;
	lpDDSDest->Lock( &rcDest, &ddsdTarget, DDLOCK_WAIT, NULL );  

	//
	// Lock down the source surface.
	//
	memset( &ddsdSource, 0, sizeof ddsdSource );
	ddsdSource.dwSize = sizeof ddsdSource;
	lpDDSSource->Lock( lprcSource, &ddsdSource, DDLOCK_WAIT, NULL );

	switch ( dwMode )
	{
	/* 16 bit mode ( 555 ). This algorithm 
	   can process four pixels at once. */
	case RGBMODE_555:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		//
		// We process four pixels at once, so the 
		// width must be a multiple of four.
		//
		iRemainder = ( iWidth & 0x03 ); 
		iWidth = ( iWidth & ~0x03 ) / 4;

		//
		// Set the bit masks for red, green and blue.
		//
		i64MaskRed = 0x7c007c007c007c00;
		i64MaskGreen =0x03e003e003e003e0;
		i64MaskBlue = 0x001f001f001f001f;

		//
		// Compose the quadruple alpha value.
		//
		i64Alpha = iAlpha;
		i64Alpha |= ( i64Alpha << 16 ) | ( i64Alpha << 32 ) | ( i64Alpha << 48 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;
	
			// 
			// Alpha-blend four pixels at once.
			//
			__asm
			{
				//
				// Initialize the counter and skip 
				// if the latter is equal to zero.
				//
				push		ecx		
				mov			ecx, i	
				cmp			ecx, 0	
				jz			skip555	

				//
				// Load the frame buffer pointers into the registers.
				//
				push		edi		
				push		esi
				mov			edi, lpbTarget		
				mov			esi, lpbSource

do_blend555:
				//
				// Skip these four pixels if they are all black.
				//
				cmp			dword ptr [esi], 0
				jnz			not_black555
				cmp			dword ptr [esi + 4], 0
				jnz			not_black555
				jmp			next555

not_black555:
				//
				// Alpha blend four target and source pixels.
				//

				/* The mmx registers will basically be used in the following way:

					mm0:	red target value
					mm1:	red source value
					mm2:	green target value
					mm3:	green source value
					mm4:	blue target value
					mm5:	blue source value
					mm6:	original target pixel
					mm7:	original source pixel

				/* Note: Two lines together are assumed to pair 
				         in the processor´s U- and V-pipes. */

				movq		mm6, [edi]				// Load the original target pixel.
				nop

				movq		mm7, [esi]				// Load the original source pixel.
				movq		mm0, mm6				// Load the register for the red target.

				pand		mm0, i64MaskRed			// Extract the red target channel.
				movq		mm1, mm7				// Load the register for the red source.	

				pand		mm1, i64MaskRed			// Extract the red source channel.
				psrlw		mm0, 10					// Shift down the red target channel.

				movq		mm2, mm6				// Load the register for the green target.
				psrlw		mm1, 10					// Shift down the red source channel.

				movq		mm3, mm7				// Load the register for the green source.
				psubw		mm1, mm0				// Calculate red source minus red target.

				pmullw		mm1, i64Alpha			// Multiply the red result with alpha.
				nop

				pand		mm2, i64MaskGreen		// Extract the green target channel.
				nop

				pand		mm3, i64MaskGreen		// Extract the green source channel.
				psraw		mm1, 8					// Divide the red result by 256.

				psrlw		mm2, 5					// Shift down the green target channel.
				paddw		mm1, mm0				// Add the red target to the red result.

				psllw		mm1, 10					// Shift up the red source again.
				movq		mm4, mm6				// Load the register for the blue target.

				psrlw		mm3, 5					// Shift down the green source channel.
				movq		mm5, mm7				// Load the register for the blue source.

				pand		mm4, i64MaskBlue		// Extract the blue target channel.
				psubw		mm3, mm2				// Calculate green source minus green target.

				pand		mm5, i64MaskBlue		// Extract the blue source channel.
				pmullw		mm3, i64Alpha			// Multiply the green result with alpha.

				psubw		mm5, mm4				// Calculate blue source minus blue target.
				pxor		mm0, mm0				// Create black as the color key.

				pmullw		mm5, i64Alpha			// Multiply the blue result with alpha.
				psraw		mm3, 8					// Divide the green result by 256.

				paddw		mm3, mm2				// Add the green target to the green result.
				pcmpeqw		mm0, mm7				// Create a color key mask.

				psraw		mm5, 8					// Divide the blue result by 256.
				psllw		mm3, 5					// Shift up the green source again.
				
				paddw		mm5, mm4				// Add the blue target to the blue result.
				por			mm1, mm3				// Combine the new red and green values.

				pand		mm6, mm0				// Keep old target where the color key applies.
				por			mm1, mm5				// Combine new blue value with the others.

				pandn		mm0, mm1				// Keep new target where no color key applies.
				por			mm6, mm0				// Assemble new target value.

				movq		[edi], mm6				// Write back new target value.

next555:
				//
				// Advance to the next four pixels.
				//
				add			edi, 8
				add			esi, 8
				
				//
				// Loop again or break.
				//
				dec			ecx
				jnz			do_blend555

				//
				// Write back the frame buffer pointers and clean up.
				//
				mov			lpbTarget, edi	
				mov			lpbSource, esi
				pop			esi		
				pop			edi
				emms

skip555:
				pop			ecx
			}

			//
			// Alpha blend any remaining pixels.
			//
			for ( i = 0; i < iRemainder; i++ )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = ( dwTargetTemp >> 10 ) & 0x1f;
					dwSrcRed = ( dwSourceTemp >> 10 ) & 0x1f;

					// Extract the green channels.
					dwTgtGreen = ( dwTargetTemp >> 5 ) & 0x1f;
					dwSrcGreen = ( dwSourceTemp >> 5 ) & 0x1f;

					// Extract the blue channels.
					dwTgtBlue = dwTargetTemp & 0x1f;
					dwSrcBlue = dwSourceTemp & 0x1f;

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( iAlpha * ( dwSrcRed - dwTgtRed ) >> 8 ) + dwTgtRed ) << 10 |
						  ( ( iAlpha * ( dwSrcGreen - dwTgtGreen ) >> 8 ) + dwTgtGreen ) << 5 |
						  ( ( iAlpha * ( dwSrcBlue - dwTgtBlue ) >> 8 ) + dwTgtBlue ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while ( --iHeight > 0 );

		break;
		
	/* 16 bit mode ( 565 ). This algorithm 
	   can process four pixels at once. */
	case RGBMODE_565:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		//
		// We process four pixels at once, so the 
		// width must be a multiple of four.
		//
		iRemainder = ( iWidth & 0x03 ); 
		iWidth = ( iWidth & ~0x03 ) / 4;

		//
		// Set the bit masks for red, green and blue.
		//
		i64MaskRed = 0xf800f800f800f800;
		i64MaskGreen =0x07e007e007e007e0;
		i64MaskBlue = 0x001f001f001f001f;

		//
		// Compose the quadruple alpha value.
		//
		i64Alpha = iAlpha;
		i64Alpha |= ( i64Alpha << 16 ) | ( i64Alpha << 32 ) | ( i64Alpha << 48 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;
	
			// 
			// Alpha-blend four pixels at once.
			//
			__asm
			{
				//
				// Initialize the counter and skip 
				// if the latter is equal to zero.
				//
				push		ecx		
				mov			ecx, i	
				cmp			ecx, 0	
				jz			skip565	

				//
				// Load the frame buffer pointers into the registers.
				//
				push		edi		
				push		esi
				mov			edi, lpbTarget		
				mov			esi, lpbSource	

do_blend565:
				//
				// Skip these four pixels if they are all black.
				//
				cmp			dword ptr [esi], 0
				jnz			not_black565
				cmp			dword ptr [esi + 4], 0
				jnz			not_black565
				jmp			next565

not_black565:
				//
				// Alpha blend four target and source pixels.
				//

				/* The mmx registers will basically be used in the following way:

					mm0:	red target value
					mm1:	red source value
					mm2:	green target value
					mm3:	green source value
					mm4:	blue target value
					mm5:	blue source value
					mm6:	original target pixel
					mm7:	original source pixel

				/* Note: Two lines together are assumed to pair 
				         in the processor´s U- and V-pipes. */

				movq		mm6, [edi]				// Load the original target pixel.
				nop

				movq		mm7, [esi]				// Load the original source pixel.
				movq		mm0, mm6				// Load the register for the red target.

				pand		mm0, i64MaskRed			// Extract the red target channel.
				movq		mm1, mm7				// Load the register for the red source.	

				pand		mm1, i64MaskRed			// Extract the red source channel.
				psrlw		mm0, 11					// Shift down the red target channel.

				movq		mm2, mm6				// Load the register for the green target.
				psrlw		mm1, 11					// Shift down the red source channel.

				movq		mm3, mm7				// Load the register for the green source.
				psubw		mm1, mm0				// Calculate red source minus red target.

				pmullw		mm1, i64Alpha			// Multiply the red result with alpha.
				nop

				pand		mm2, i64MaskGreen		// Extract the green target channel.
				nop

				pand		mm3, i64MaskGreen		// Extract the green source channel.
				psraw		mm1, 8					// Divide the red result by 256.

				psrlw		mm2, 5					// Shift down the green target channel.
				paddw		mm1, mm0				// Add the red target to the red result.

				psllw		mm1, 11					// Shift up the red source again.
				movq		mm4, mm6				// Load the register for the blue target.

				psrlw		mm3, 5					// Shift down the green source channel.
				movq		mm5, mm7				// Load the register for the blue source.

				pand		mm4, i64MaskBlue		// Extract the blue target channel.
				psubw		mm3, mm2				// Calculate green source minus green target.

				pand		mm5, i64MaskBlue		// Extract the blue source channel.
				pmullw		mm3, i64Alpha			// Multiply the green result with alpha.

				psubw		mm5, mm4				// Calculate blue source minus blue target.
				pxor		mm0, mm0				// Create black as the color key.

				pmullw		mm5, i64Alpha			// Multiply the blue result with alpha.
				psraw		mm3, 8					// Divide the green result by 256.

				paddw		mm3, mm2				// Add the green target to the green result.
				pcmpeqw		mm0, mm7				// Create a color key mask.

				psraw		mm5, 8					// Divide the blue result by 256.
				psllw		mm3, 5					// Shift up the green source again.
				
				paddw		mm5, mm4				// Add the blue target to the blue result.
				por			mm1, mm3				// Combine the new red and green values.
									
				pand		mm6, mm0				// Keep old target where the color key applies.
				por			mm1, mm5				// Combine new blue value with the others.

				pandn		mm0, mm1				// Keep new target where no color key applies.
				por			mm6, mm0				// Assemble new target value.

				movq		[edi], mm6				// Write back new target value.

next565:
				//
				// Advance to the next four pixels.
				//
				add			edi, 8
				add			esi, 8
				
				//
				// Loop again or break.
				//
				dec			ecx
				jnz			do_blend565

				//
				// Write back the frame buffer pointers and clean up.
				//
				mov			lpbTarget, edi	
				mov			lpbSource, esi
				pop			esi		
				pop			edi
				emms

skip565:
				pop			ecx
			}

			//
			// Alpha blend any remaining pixels.
			//
			for ( i = 0; i < iRemainder; i++ )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = ( dwTargetTemp >> 11 ) & 0x1f;
					dwSrcRed = ( dwSourceTemp >> 11 ) & 0x1f;

					// Extract the green channels.
					dwTgtGreen = ( dwTargetTemp >> 5 ) & 0x3f;
					dwSrcGreen = ( dwSourceTemp >> 5 ) & 0x3f;

					// Extract the blue channels.
					dwTgtBlue = dwTargetTemp & 0x1f;
					dwSrcBlue = dwSourceTemp & 0x1f;

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( iAlpha * ( dwSrcRed - dwTgtRed ) >> 8 ) + dwTgtRed ) << 11 |
						  ( ( iAlpha * ( dwSrcGreen - dwTgtGreen ) >> 8 ) + dwTgtGreen ) << 5 |
						  ( ( iAlpha * ( dwSrcBlue - dwTgtBlue ) >> 8 ) + dwTgtBlue ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while ( --iHeight > 0 );

		break;

	/* 16 bit mode ( unknown ). This algorithm 
	   can process four pixels at once. */
	case RGBMODE_16:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		//
		// We process four pixels at once, so the 
		// width must be a multiple of four.
		//
		iRemainder = ( iWidth & 0x03 ); 
		iWidth = ( iWidth & ~0x03 ) / 4;

		//
		// Determine the distance in bits of each bit mask from the right.
		//
		while ( ( ( ddsdTarget.ddpfPixelFormat.dwRBitMask >> i64RdShift ) & 0x01 ) == 0 )
			i64RdShift++;
		
		while ( ( ( ddsdTarget.ddpfPixelFormat.dwGBitMask >> i64GrShift ) & 0x01 ) == 0 )
			i64GrShift++;
	
		while ( ( ( ddsdTarget.ddpfPixelFormat.dwBBitMask >> i64BlShift ) & 0x01 ) == 0 )
			i64BlShift++;
	
		//
		// Compose the bit masks for each color channel.
		//
		i64MaskRed = ddsdTarget.ddpfPixelFormat.dwRBitMask;
		i64MaskRed |= ( i64MaskRed << 16 ) | ( i64MaskRed << 32 ) | ( i64MaskRed << 48 );
		i64MaskGreen = ddsdTarget.ddpfPixelFormat.dwGBitMask;
		i64MaskGreen |= ( i64MaskGreen << 16 ) | ( i64MaskGreen << 32 ) | ( i64MaskGreen << 48 );
		i64MaskBlue = ddsdTarget.ddpfPixelFormat.dwBBitMask;
		i64MaskBlue |= ( i64MaskBlue << 16 ) | ( i64MaskBlue << 32 ) | ( i64MaskBlue << 48 );

		//
		// Compose the quadruple alpha value.
		//
		i64Alpha = iAlpha;
		i64Alpha |= ( i64Alpha << 16 ) | ( i64Alpha << 32 ) | ( i64Alpha << 48 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend four pixels at once.
			//
			__asm
			{
				//
				// Initialize the counter and skip 
				// if the latter is equal to zero.
				//
				push		ecx		
				mov			ecx, i	
				cmp			ecx, 0	
				jz			skip16

				//
				// Load the frame buffer pointers into the registers.
				//
				push		edi		
				push		esi
				mov			edi, lpbTarget		
				mov			esi, lpbSource	

do_blend16:
				//
				// Skip these four pixels if they are all black.
				//
				cmp			dword ptr [esi], 0
				jnz			not_black16
				cmp			dword ptr [esi + 4], 0
				jnz			not_black16
				jmp			next16

not_black16:
				//
				// Alpha blend four target and source pixels.
				//

				/* The mmx registers will basically be used in the following way:

					mm0:	red target value
					mm1:	red source value
					mm2:	green target value
					mm3:	green source value
					mm4:	blue target value
					mm5:	blue source value
					mm6:	original target pixel
					mm7:	original source pixel

				/* Note: Two lines together are assumed to pair 
				         in the processor´s U- and V-pipes. */

				movq		mm6, [edi]				// Load the original target pixel.
				nop

				movq		mm7, [esi]				// Load the original source pixel.
				movq		mm0, mm6				// Load the register for the red target.

				pand		mm0, i64MaskRed			// Extract the red target channel.
				movq		mm1, mm7				// Load the register for the red source.
				 
				pand		mm1, i64MaskRed			// Extract the red source channel.
				nop

				psrlw		mm0, i64RdShift			// Shift down the red target channel.
				movq		mm2, mm6				// Load the register for the green target.

				psrlw		mm1, i64RdShift			// Shift down the red source channel.
				movq		mm3, mm7				// Load the register for the green source.

				pand		mm2, i64MaskGreen		// Extract the green target channel.
				psubw		mm1, mm0				// Calculate red source minus red target.

				pmullw		mm1, i64Alpha			// Multiply the red result with alpha.
				movq		mm5, mm7				// Load the register for the blue source.

				pand		mm3, i64MaskGreen		// Extract the green source channel.
				movq		mm4, mm6				// Load the register for the blue target.

				psraw		mm1, 8					// Divide the red result by 256.
				nop
				
				paddw		mm1, mm0				// Add the red target to the red result.
				nop

				psllw		mm1, i64RdShift			// Shift up the red source again.
				pxor		mm0, mm0				// Create black as the color key.

				psrlw		mm2, i64GrShift			// Shift down the green target channel.
				pcmpeqw		mm0, mm7				// Create a color key mask.

				psrlw		mm3, i64GrShift			// Shift down the green source channel.
				pand		mm6, mm0				// Keep old target where the color key applies.

				pand		mm4, i64MaskBlue		// Extract the blue target channel.
				psubw		mm3, mm2				// Calculate green source minus green target.

				pmullw		mm3, i64Alpha			// Multiply the green result with alpha.
				nop

				psrlw		mm4, i64BlShift			// Shift down the blue source channel.
				nop

				pand		mm5, i64MaskBlue		// Extract the blue source channel.
				psraw		mm3, 8					// Divide the green result by 256.

				paddw		mm3, mm2				// Add the green target to the green result.
				nop

				psllw		mm3, i64GrShift			// Shift up the green source again.
				por			mm1, mm3				// Combine the new red and green values.
				 
				psrlw		mm5, i64BlShift			// Divide the blue result by 256.
				nop

				psubw		mm5, mm4				// Add the blue target to the blue result.
				nop

				pmullw		mm5, i64Alpha			// Multiply the blue result with alpha.
				nop

				nop
				nop

				psraw		mm5, 8					// Divide the blue result by 256.
				nop

				paddw		mm5, mm4				// Add the blue target to the blue result.
				nop

				psllw		mm5, i64BlShift			// Shift up the blue source again.
				nop

				por			mm1, mm5				// Combine new blue value with the others.
				nop

				pandn		mm0, mm1				// Keep new target where no color key applies.
				nop

				por			mm6, mm0				// Assemble new target value.
				nop

				movq		[edi], mm6				// Write back new target value.
				
next16:
				//
				// Advance to the next two pixels.
				//
				add			edi, 8
				add			esi, 8
				
				//
				// Loop again or break.
				//
				dec			ecx
				jnz			do_blend16

				//
				// Write back the frame buffer pointers and clean up.
				//
				mov			lpbTarget, edi	
				mov			lpbSource, esi
				pop			esi		
				pop			edi
				emms

skip16:
				pop			ecx
			}

			//
			// Alpha blend any remaining pixels.
			//
			for ( i = 0; i < iRemainder; i++ )
			{
				// Read in the next source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If the source pixel is not black ...
				if ( dwSourceTemp != 0 )
				{
					// ... read in the next target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = dwTargetTemp & ddsdTarget.ddpfPixelFormat.dwRBitMask;
					dwSrcRed = dwSourceTemp & ddsdSource.ddpfPixelFormat.dwRBitMask;

					// Extract the green channels.
					dwTgtGreen = dwTargetTemp & ddsdTarget.ddpfPixelFormat.dwGBitMask;
					dwSrcGreen = dwSourceTemp & ddsdSource.ddpfPixelFormat.dwGBitMask;

					// Extract the blue channel.
					dwTgtBlue = dwTargetTemp & ddsdTarget.ddpfPixelFormat.dwBBitMask;
					dwSrcBlue = dwSourceTemp & ddsdSource.ddpfPixelFormat.dwBBitMask;

					// Calculate the alpha-blended red channel.
					dwRed = ( ( ( iAlpha * ( dwSrcRed - dwTgtRed ) ) >> 8 ) + 
						dwTgtRed ) & ddsdTarget.ddpfPixelFormat.dwRBitMask;

					// Calculate the alpha-blended green channel.
					dwGreen = ( ( ( iAlpha * ( dwSrcGreen - dwTgtGreen ) ) >> 8 ) + 
						dwTgtGreen ) & ddsdTarget.ddpfPixelFormat.dwGBitMask;

					// Calculate the alpha-blended blue channel.
					dwBlue = ( ( ( iAlpha * ( dwSrcBlue - dwTgtBlue ) ) >> 8 ) + 
						dwTgtBlue ) & ddsdTarget.ddpfPixelFormat.dwBBitMask;

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) ( dwRed | dwGreen | dwBlue );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while ( --iHeight > 0 );

		break;

	/* 24 bit mode. */
	case RGBMODE_24:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 3 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 3 );

		//
		// Compose the triple alpha value.
		//
		i64Alpha = iAlpha;
		i64Alpha |= ( i64Alpha << 16 ) | ( i64Alpha << 32 );

		// Create a general purpose mask.
		i64Mask = 0x0000000000ffffff;

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			//
			// Alpha blend the pixels in the current row.
			//
			__asm
			{
				// Reset the width counter.
				push		ecx		
				mov			ecx, iWidth	

				//
				// Load the frame buffer pointers into the registers.
				//
				push		edi		
				push		esi
				mov			edi, lpbTarget		
				mov			esi, lpbSource
							
				// Load the mask into an mmx register.
				movq		mm3, i64Mask

				// Load the alpha value into an mmx register.
				movq		mm5, i64Alpha

				// Clear an mmx register to facilitate unpacking.
				pxor		mm6, mm6

				push eax
				
do_blend24:
				//
				// Skip this pixel if it is black.
				//
				mov			eax, [esi]
				test		eax, 00ffffffh	// Do not 'and' so that the high order byte is kept.
				jnz			not_black24
				jmp			next24

not_black24:
				//
				// Get a target and a source pixel.
				//

				/* The mmx registers will basically be used in the following way:

					mm0:	target value
					mm1:	source value
					mm2:	working register
					mm3:	mask ( 0x00ffffff )
					mm4:	working register
					mm5:	alpha value
					mm6:	zero for unpacking
					mm7:	original target

				/* Note: Two lines together are assumed to pair 
				         in the processor´s U- and V-pipes. */

				movd		mm0, [edi]				// Load the target pixel.
				movq		mm4, mm3				// Reload the mask ( 0x00ffffff ).
					  
				movd		mm1, eax				// Load the source pixel.				
				movq		mm7, mm0				// Save the target pixel.
				
				punpcklbw	mm0, mm6				// Unpack the target pixel.
				punpcklbw	mm1, mm6				// Unpack the source pixel.

				movq		mm2, mm0				// Save the unpacked target values.
				nop

				pmullw		mm0, mm5				// Multiply the target with the alpha value.
				nop

				pmullw		mm1, mm5				// Multiply the source with the alpha value.
				nop

				psrlw		mm0, 8					// Divide the target by 256.
				nop

				psrlw		mm1, 8					// Divide the source by 256.
				nop

				psubw		mm1, mm0				// Calculate the source minus target.
				nop

				paddw		mm2, mm1				// Add former target value to the result.
				nop

				packuswb	mm2, mm2				// Pack the new target.
				nop

				pand		mm2, mm4				// Mask of unwanted bytes.
				nop

				pandn		mm4, mm7				// Get the high order byte we must keep.
				nop

				por			mm2, mm4				// Assemble the value to write back.
				nop

				movd		[edi], mm2				// Write back the new value.

next24:
				//
				// Advance to the next pixel.
				//
				add			edi, 3
				add			esi, 3

				//
				// Loop again or break.
				//
				dec			ecx
				jnz			do_blend24

				//
				// Write back the frame buffer pointers and clean up.
				//
				mov			lpbTarget, edi	
				mov			lpbSource, esi
				pop			eax
				pop			esi		
				pop			edi
				emms

				pop			ecx
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while ( --iHeight > 0 );

		break;

	/* 32 bit mode. This algorithm can
	   process two pixels at once. */
	case RGBMODE_32:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 4 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 4 );

		// If the width is odd ...
		if ( iWidth & 0x01 )
		{
			// ... set the flag ...
			gOddWidth = true;

			// ... and calculate the width.
			iWidth = ( iWidth - 1 ) / 2;
		}
		// If the width is even ...
		else
		{
			// ... clear the flag ...
			gOddWidth = false;

			// ... and calculate the width.
			iWidth /= 2;
		}

		//
		// Compose the triple alpha value.
		//
		i64Alpha = iAlpha;
		i64Alpha |= ( i64Alpha << 16 ) | ( i64Alpha << 32 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			//
			// Alpha blend two pixels at once.
			//
			__asm
			{
				//
				// Initialize the counter and skip 
				// if the latter is equal to zero.
				//
				push		ecx		
				mov			ecx, i	
				cmp			ecx, 0	
				jz			skip32

				//
				// Load the frame buffer pointers into the registers.
				//
				push		edi		
				push		esi
				mov			edi, lpbTarget		
				mov			esi, lpbSource

				// Load	the alpha value into an mmx register.
				movq		mm5, i64Alpha

				push		eax

do_blend32:
				//
				// Skip these two pixels if they are both black.
				//
				mov			eax, [esi]
				test		eax, 00ffffffh
				jnz			not_black32
				mov			eax, [esi + 4]
				test		eax, 00ffffffh
				jnz			not_black32
				jmp			next32

not_black32:
				//
				// Alpha blend two target and two source pixels.
				//

				/* The mmx registers will basically be used in the following way:

					mm0:	target pixel one
					mm1:	source pixel one
					mm2:	target pixel two
					mm3:	source pixel two
					mm4:	working register
					mm5:	alpha value
					mm6:	original target
					mm7:	original source

				/* Note: Two lines together are assumed to pair 
				         in the processor´s U- and V-pipes. */

				movq		mm6, [edi]				// Load the target pixels.
				pxor		mm4, mm4				// Clear mm4 so we can unpack easily.

				movq		mm7, [esi]				// Load the source pixels.
				movq		mm0, mm6				// Create copy one of the target.

				movq		mm2, mm6				// Create copy two of the target.
				punpcklbw	mm0, mm4				// Unpack the first target copy.

				movq		mm1, mm7				// Create copy one of the source.
				psrlq		mm2, 32					// Move the high order dword of target two. 

				punpcklbw	mm2, mm4				// Unpack the second target copy. 
				movq		mm3, mm7				// Create copy two of the source.

				psrlq		mm3, 32					// Move the high order dword of source two.
				punpcklbw	mm1, mm4				// Unpack the first source copy.

				punpcklbw	mm3, mm4				// Unpack the second source copy.
				pslld		mm7, 8					// Shift away original source highest bytes.

				movq		mm4, mm0				// Save target one.
				pmullw		mm0, mm5				// Multiply target one with alpha.

				pmullw		mm1, mm5				// Multiply source one with alpha.
				psrld		mm7, 8					// Complete high order byte clearance.	

				psrlw		mm1, 8					// Divide source one by 256.
				nop

				psrlw		mm0, 8					// Divide target one by 256.
				nop

				psubw		mm1, mm0				// Calculate source one minus target one.
				nop
				
				paddw		mm1, mm4				// Add the former target one to the result.
				nop

				movq		mm4, mm2				// Save target two.
				pmullw		mm2, mm5				// Multiply target two with alpha.
				
				pmullw		mm3, mm5				// Multiply source two with alpha.
				nop
				
				psrlw		mm2, 8					// Divide target two by 256.
				nop

				psrlw		mm3, 8					// Divide source two by 256.
				nop

				psubw		mm3, mm2				// Calculate source two minus source one.
				nop

				paddw		mm3, mm4				// Add the former target two to the result.
				pxor		mm4, mm4				// Clear mm4 so we can pack easily.

				packuswb	mm1, mm4				// Pack the new target one.
				packuswb	mm3, mm4				// Pack the new target two.

				psllq		mm3, 32					// Shift up the new target two.
				pcmpeqd		mm4, mm7				// Create a color key mask.

				por			mm1, mm3				// Combine the new targets.
				pand		mm6, mm4				// Keep old target where color key applies.

				pandn		mm4, mm1				// Clear new target where color key applies.
				nop

				por			mm6, mm4				// Assemble the new target value.
				nop

				movq		[edi], mm6				// Write back the new target value.

next32:
				//
				// Advance to the next pixel.
				//
				add			edi, 8
				add			esi, 8

				//
				// Loop again or break.
				//
				dec			ecx
				jnz			do_blend32

				//
				// Write back the frame buffer pointers and clean up.
				//
				mov			lpbTarget, edi	
				mov			lpbSource, esi
				pop			eax
				pop			esi		
				pop			edi
				emms

skip32:
				pop			ecx
			}

			//
			// Handle an odd width.
			//
			if ( gOddWidth )
			{
				// Read in the next source pixel.
				dwSourceTemp = *( ( DWORD* ) lpbSource );

				// If the source pixel is not black ...
				if ( ( dwSourceTemp & 0xffffff ) != 0 )
				{
					// ... read in the next target pixel.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// Extract the red channels.
					dwTgtRed = dwTargetTemp & 0xff0000;
					dwSrcRed = dwSourceTemp & 0xff0000;

					// Extract the green channels.
					dwTgtGreen = dwTargetTemp & 0xff00;
					dwSrcGreen = dwSourceTemp & 0xff00;

					// Extract the blue channel.
					dwTgtBlue = dwTargetTemp & 0xff;
					dwSrcBlue = dwSourceTemp & 0xff;
							
					// Calculate the destination pixel.
					dwTargetTemp = 
						( ( ( ( iAlpha * ( dwSrcRed - dwTgtRed ) >> 8 ) + dwTgtRed ) & 0xff0000 ) |
						  ( ( ( iAlpha * ( dwSrcGreen - dwTgtGreen ) >> 8 ) + dwTgtGreen ) & 0xff00 ) |
						    ( ( iAlpha * ( dwSrcBlue - dwTgtBlue ) >> 8 ) + dwTgtBlue ) );

					// Write the destination pixel.
					*( ( DWORD* ) lpbTarget ) = dwTargetTemp;
				}

				//
				// Proceed to the next pixel.
				//
				lpbTarget += 4;
				lpbSource += 4;	
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while ( --iHeight > 0 );

		break;

	/* Invalid mode. */
	default:
		iRet = -1;
	}

	// Unlock the target surface.
	lpDDSDest->Unlock( &rcDest );

	// Unlock the source surface.
	lpDDSSource->Unlock( lprcSource );

	// Return the result.
	return iRet;
}

/*
 * Description:		Performs a blit operation while using a fixed opacity of 50 percent
 *					for alpha blending the source and the target surface with each other.
 *					The function uses black as a color key for the blit operation.
 *
 * Parameters:		lpDDSDest   - The destination surface of the blit.
 *
 *					lpDDSSource - The source surface of the blit.
 *
 *					iDestX      - The horizontal coordinate to blit to on 
 *								  the destination surface. 
 *
 *					iDestY      - The vertical coordinate to blit to on the
 *								  destination surface.
 *
 *					lprcSource  - The address of a RECT structure that defines 
 *								  the upper-left and lower-right corners of the 
 *								  rectangle to blit from on the source surface. 
 *
 *					dwMode		- One of the following predefined values:
 *								  RGBMODE_555	- 16 bit mode ( 555 )
 *								  RBGMODE_565	- 16 bit mode ( 565 )
 *								  RGBMODE_16	- 16 bit mode ( unknown )
 *								  RGBMODE_24    - 24 bit mode
 *								  RGBMODE_32    - 32 bit mode
 *
 * Return value:	The functions returns 0 to indicate success or -1 if the call fails.
 *
 */
int BltAlphaFast( LPDIRECTDRAWSURFACE7 lpDDSDest, LPDIRECTDRAWSURFACE7 lpDDSSource, 
			 int iDestX, int iDestY, LPRECT lprcSource, DWORD dwMode )
{
	DDSURFACEDESC2	ddsdSource;
	DDSURFACEDESC2	ddsdTarget;
	RECT			rcDest;
	DWORD			dwTargetPad;
	DWORD			dwSourcePad;
	DWORD			dwTargetTemp;
	DWORD			dwSourceTemp;
	WORD			wMask;
	DWORD			dwDoubleMask;
	BYTE*			lpbTarget;
	BYTE*			lpbSource;
	int				iWidth;
	int				iHeight;
	bool			gOddWidth;
	int				iRet = 0;
	int				i;


	//
	// Determine the dimensions of the source surface.
	//
	if ( lprcSource )
	{
		//
		// Get the width and height from the passed rectangle.
		//
		iWidth = lprcSource->right - lprcSource->left;
		iHeight = lprcSource->bottom - lprcSource->top; 
	}
	else
	{
		//
		// Get the with and height from the surface description.
		//
		memset( &ddsdSource, 0, sizeof ddsdSource );
		ddsdSource.dwSize = sizeof ddsdSource;
		ddsdSource.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		lpDDSSource->GetSurfaceDesc( &ddsdSource );

		//
		// Remember the dimensions.
		//
		iWidth = ddsdSource.dwWidth;
		iHeight = ddsdSource.dwHeight;
	}

	//
	// Calculate the rectangle to be locked in the target.
	//
	rcDest.left = iDestX;
	rcDest.top = iDestY;
	rcDest.right = iDestX + iWidth;
	rcDest.bottom = iDestY + iHeight;

	//
	// Lock down the destination surface.
	//
	memset( &ddsdTarget, 0, sizeof ddsdTarget );
	ddsdTarget.dwSize = sizeof ddsdTarget;
	lpDDSDest->Lock( &rcDest, &ddsdTarget, DDLOCK_WAIT, NULL );  

	//
	// Lock down the source surface.
	//
	memset( &ddsdSource, 0, sizeof ddsdSource );
	ddsdSource.dwSize = sizeof ddsdSource;
	lpDDSSource->Lock( lprcSource, &ddsdSource, DDLOCK_WAIT, NULL );

	//
	// Perform the blit operation.
	//
	switch ( dwMode )
	{
	/* 16 bit mode ( 555 ). This algorithm 
	   can process two pixels at once. */
	case RGBMODE_555:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		// If the width is odd ...
		if ( iWidth & 0x01 )
		{
			// ... set the flag ...
			gOddWidth = true;

			// ... and calculate the width.
			iWidth = ( iWidth - 1 ) / 2;
		}
		// If the width is even ...
		else
		{
			// ... clear the flag ...
			gOddWidth = false;

			// ... and calculate the width.
			iWidth /= 2;
		}

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend two pixels at once.
			//
			while ( i-- > 0 )
			{
				// Read in two source pixels.
				dwSourceTemp = *( ( DWORD* ) lpbSource );

				// If the two source pixels are not both black ...
				if ( dwSourceTemp != 0 )
				{
					// ... read in two target pixels.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// If the first source is black ...
					if ( ( dwSourceTemp >> 16 ) == 0 )
					{
						// ... make sure the first target pixel won´t change.
						dwSourceTemp |= dwTargetTemp & 0xffff0000;
					}

					// If the second source is black ...
					if ( ( dwSourceTemp & 0xffff ) == 0 )
					{
						// ... make sure the second target pixel won´t change.
						dwSourceTemp |= dwTargetTemp & 0xffff;
					}

					// Calculate the destination pixels.
					dwTargetTemp = ( ( dwTargetTemp & 0x7bde7bde ) >> 1 ) + 
						( ( dwSourceTemp & 0x7bde7bde ) >> 1 );

					// Write the destination pixels.
					*( ( DWORD* ) lpbTarget ) = dwTargetTemp;
				}

				//
				// Proceed to the next two pixels.
				//
				lpbTarget += 4;
				lpbSource += 4;
			}

			//
			// Handle an odd width.
			//
			if ( gOddWidth )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( dwTargetTemp & 0x7bde ) >> 1 ) + 
						  ( ( dwSourceTemp & 0x7bde ) >> 1 ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		} 
		while ( --iHeight > 0 );

		break;

	/* 16 bit mode ( 565 ). This algorithm 
	   can process two pixels at once. */
	case RGBMODE_565:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		// If the width is odd ...
		if ( iWidth & 0x01 )
		{
			// ... set the flag ...
			gOddWidth = true;

			// ... and calculate the width.
			iWidth = ( iWidth - 1 ) / 2;
		}
		// If the width is even ...
		else
		{
			// ... clear the flag ...
			gOddWidth = false;

			// ... and calculate the width.
			iWidth /= 2;
		}

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend two pixels at once.
			//
			while ( i-- > 0 )
			{
				// Read in two source pixels.
				dwSourceTemp = *( ( DWORD* ) lpbSource );

				// If the two source pixels are not both black ...
				if ( dwSourceTemp != 0 )
				{
					// ... read in two target pixels.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// If the first source is black ...
					if ( ( dwSourceTemp >> 16 ) == 0 )
					{
						// ... make sure the first target pixel won´t change.
						dwSourceTemp |= dwTargetTemp & 0xffff0000;
					}

					// If the second source is black ...
					if ( ( dwSourceTemp & 0xffff ) == 0 )
					{
						// ... make sure the second target pixel won´t change.
						dwSourceTemp |= dwTargetTemp & 0xffff;
					}

					// Calculate the destination pixels.
					dwTargetTemp = ( ( dwTargetTemp & 0xf7def7de ) >> 1 ) + 
						( ( dwSourceTemp & 0xf7def7de ) >> 1 );

					// Write the destination pixels.
					*( ( DWORD* ) lpbTarget ) = dwTargetTemp;
				}

				//
				// Proceed to the next two pixels.
				//
				lpbTarget += 4;
				lpbSource += 4;
			}

			//
			// Handle an odd width.
			//
			if ( gOddWidth )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( dwTargetTemp & 0xf7de ) >> 1 ) + 
						  ( ( dwSourceTemp & 0xf7de ) >> 1 ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		} 
		while ( --iHeight > 0 );

		break;

	/* 16 bit mode ( unknown ). This algorithm 
	   can process two pixels at once. */
	case RGBMODE_16:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		// If the width is odd ...
		if ( iWidth & 0x01 )
		{
			// ... set the flag ...
			gOddWidth = true;

			// ... and calculate the width.
			iWidth = ( iWidth - 1 ) / 2;
		}
		// If the width is even ...
		else
		{
			// ... clear the flag ...
			gOddWidth = false;

			// ... and calculate the width.
			iWidth /= 2;
		}

		// Create the bit mask used to clear the lowest bit of each color channel´s mask.
		wMask = ( WORD ) ( ( ddsdTarget.ddpfPixelFormat.dwRBitMask & 
						   ( ddsdTarget.ddpfPixelFormat.dwRBitMask << 1 ) ) | 
						   ( ddsdTarget.ddpfPixelFormat.dwGBitMask & 
						   ( ddsdTarget.ddpfPixelFormat.dwGBitMask << 1 ) ) | 
						   ( ddsdTarget.ddpfPixelFormat.dwBBitMask & 
						   ( ddsdTarget.ddpfPixelFormat.dwBBitMask << 1 ) ) );

		// Create a double bit mask.
		dwDoubleMask = wMask | ( wMask << 16 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend two pixels at once.
			//
			while ( i-- > 0 )
			{
				// Read in two source pixels.
				dwSourceTemp = *( ( DWORD* ) lpbSource );

				// If the two source pixels are not both black ...
				if ( dwSourceTemp != 0 )
				{
					// ... read in two target pixels.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// If the first source is black ...
					if ( ( dwSourceTemp >> 16 ) == 0 )
					{
						// ... make sure the first target pixel won´t change.
						dwSourceTemp |= dwTargetTemp & 0xffff0000;
					}

					// If the second source is black ...
					if ( ( dwSourceTemp & 0xffff ) == 0 )
					{
						// ... make sure the second target pixel won´t change.
						dwSourceTemp |= dwTargetTemp & 0xffff;
					}

					// Calculate the destination pixels.
					dwTargetTemp = ( ( dwTargetTemp & dwDoubleMask ) >> 1 ) + 
						( ( dwSourceTemp & dwDoubleMask ) >> 1 );

					// Write the destination pixels.
					*( ( DWORD* ) lpbTarget ) = dwTargetTemp;
				}

				//
				// Proceed to the next two pixels.
				//
				lpbTarget += 4;
				lpbSource += 4;
			}

			//
			// Handle an odd width.
			//
			if ( gOddWidth )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( dwTargetTemp & wMask ) >> 1 ) + 
						  ( ( dwSourceTemp & wMask ) >> 1 ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		} 
		while ( --iHeight > 0 );

		break;

	/* 24 bit mode. */
	case RGBMODE_24:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 3 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 3 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend the pixels in the current row.
			//
			while ( i-- > 0 )
			{
				// Read in the next source pixel.
				dwSourceTemp = *( ( DWORD* ) lpbSource );	

				// If the source pixel is not black ...
				if ( ( dwSourceTemp & 0x00ffffff ) != 0 )
				{
					// ... read in the next target pixel.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// Calculate the destination pixel.
					dwTargetTemp = ( ( dwTargetTemp & 0xfefefe ) >> 1 ) + 
								   ( ( dwSourceTemp & 0xfefefe ) >> 1 ); 

					//
					// Write the destination pixel.
					//
					*( ( WORD* ) lpbTarget ) = ( WORD ) dwTargetTemp;
					lpbTarget += 2;
					*lpbTarget = ( BYTE ) ( dwTargetTemp >> 16 );
					lpbTarget++;
				}
				// If the source pixel is our color key ...
				else
				{
					// ... advance the target pointer.
					lpbTarget += 3;
				}

				// Proceed to the next source pixel.
				lpbSource += 3;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while  ( --iHeight > 0 );

		break;

	/* 32 bit mode. */
	case RGBMODE_32:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 4 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 4 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend the pixels in the current row.
			//
			while ( i-- > 0 )
			{
				// Read in the next source pixel.
				dwSourceTemp = *( ( DWORD* ) lpbSource );	

				// If the source pixel is not black ...
				if ( ( dwSourceTemp & 0xffffff ) != 0 )
				{
					// ... read in the next target pixel.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// Calculate the destination pixel.
					dwTargetTemp = ( ( dwTargetTemp & 0xfefefe ) >> 1 ) + 
								   ( ( dwSourceTemp & 0xfefefe ) >> 1 ); 

					// Write the destination pixel.
					*( ( DWORD* ) lpbTarget ) = dwTargetTemp;
				}

				//
				// Proceed to the next pixel.
				//
				lpbTarget += 4;
				lpbSource += 4;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while  ( --iHeight > 0 );

		break;

	/* Invalid mode. */
	default:
		iRet = -1;
	}

	// Unlock the target surface.
	lpDDSDest->Unlock( &rcDest );

	// Unlock the source surface.
	lpDDSSource->Unlock( lprcSource );

	// Return the result.
	return iRet;
}

/*
 * Description:		Performs a blit operation while using a fixed opacity of 50 percent
 *					for alpha blending the source and the target surface with each other,
 *					making use of MMX technology. The function uses black as a color key 
 *					for the blit operation.
 *
 * Parameters:		lpDDSDest   - The destination surface of the blit.
 *
 *					lpDDSSource - The source surface of the blit.
 *
 *					iDestX      - The horizontal coordinate to blit to on 
 *								  the destination surface. 
 *
 *					iDestY      - The vertical coordinate to blit to on the
 *								  destination surface.
 *
 *					lprcSource  - The address of a RECT structure that defines 
 *								  the upper-left and lower-right corners of the 
 *								  rectangle to blit from on the source surface. 
 *
 *					dwMode		- One of the following predefined values:
 *								  RGBMODE_555	- 16 bit mode ( 555 )
 *								  RBGMODE_565	- 16 bit mode ( 565 )
 *								  RGBMODE_16	- 16 bit mode ( unknown )
 *								  RGBMODE_24    - 24 bit mode
 *								  RGBMODE_32    - 32 bit mode
 *
 * Return value:	The functions returns 0 to indicate success or -1 if the call fails.
 *
 */
int BltAlphaFastMMX( LPDIRECTDRAWSURFACE7 lpDDSDest, LPDIRECTDRAWSURFACE7 lpDDSSource, 
			 int iDestX, int iDestY, LPRECT lprcSource, DWORD dwMode )
{
	DDSURFACEDESC2	ddsdSource;
	DDSURFACEDESC2	ddsdTarget;
	RECT			rcDest;
	DWORD			dwTargetPad;
	DWORD			dwSourcePad;
	DWORD			dwTargetTemp;
	DWORD			dwSourceTemp;
	WORD			wMask;
	BYTE*			lpbTarget;
	BYTE*			lpbSource;
	__int64			i64Mask;
	int				iWidth;
	int				iHeight;
	int				iRemainder;
	bool			gOddWidth;
	int				iRet = 0;
	int				i;


	//
	// Determine the dimensions of the source surface.
	//
	if ( lprcSource )
	{
		//
		// Get the width and height from the passed rectangle.
		//
		iWidth = lprcSource->right - lprcSource->left;
		iHeight = lprcSource->bottom - lprcSource->top; 
	}
	else
	{
		//
		// Get the with and height from the surface description.
		//
		memset( &ddsdSource, 0, sizeof ddsdSource );
		ddsdSource.dwSize = sizeof ddsdSource;
		ddsdSource.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		lpDDSSource->GetSurfaceDesc( &ddsdSource );

		//
		// Remember the dimensions.
		//
		iWidth = ddsdSource.dwWidth;
		iHeight = ddsdSource.dwHeight;
	}

	//
	// Calculate the rectangle to be locked in the target.
	//
	rcDest.left = iDestX;
	rcDest.top = iDestY;
	rcDest.right = iDestX + iWidth;
	rcDest.bottom = iDestY + iHeight;

	//
	// Lock down the destination surface.
	//
	memset( &ddsdTarget, 0, sizeof ddsdTarget );
	ddsdTarget.dwSize = sizeof ddsdTarget;
	lpDDSDest->Lock( &rcDest, &ddsdTarget, DDLOCK_WAIT, NULL );  

	//
	// Lock down the source surface.
	//
	memset( &ddsdSource, 0, sizeof ddsdSource );
	ddsdSource.dwSize = sizeof ddsdSource;
	lpDDSSource->Lock( lprcSource, &ddsdSource, DDLOCK_WAIT, NULL );

	//
	// Perform the blit operation.
	//
	switch ( dwMode )
	{
	/* 16 bit mode ( 555 ). This algorithm
	   can process four pixels at once. */
	case RGBMODE_555:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		//
		// We process four pixels at once, so the 
		// width must be a multiple of four.
		//
		iRemainder = ( iWidth & 0x03 ); 
		iWidth = ( iWidth & ~0x03 ) / 4;

		// Set an auxiliary bit mask.
		i64Mask = 0x7bde7bde7bde7bde;

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend four pixels at once.
			//
			__asm
			{
				//
				// Initialize the counter and skip 
				// if the latter is equal to zero.
				//
				push		ecx		
				mov			ecx, i	
				cmp			ecx, 0	
				jz			skip555
				
				//
				// Load the frame buffer pointers into the registers.
				//
				push		edi		
				push		esi
				mov			edi, lpbTarget		
				mov			esi, lpbSource

				// Create black as the color key. 
				pxor		mm4, mm4

				// Load the auxiliary mask into an mmx register.
				movq		mm5, i64Mask

do_blend555:
				//
				// Skip these four pixels if they are all black.
				//
				cmp			dword ptr [esi], 0
				jnz			not_black555
				cmp			dword ptr [esi + 4], 0
				jnz			not_black555
				jmp			next555

not_black555:
				//
				// Alpha blend four target and source pixels.
				//

				/* The mmx registers will basically be used in the following way:

					mm0:	target working register
					mm1:	source working register
					mm2:	not used
					mm3:	not used
					mm4:	black as the color key
					mm5:	auxiliary mask
					mm6:	original target pixel
					mm7:	original source pixel

				/* Note: Two lines together are assumed to pair 
				         in the processor´s U- and V-pipes. */

				movq		mm6, [edi]				// Load the original target pixel.
				nop

				movq		mm7, [esi]				// Load the original source pixel.
				movq		mm0, mm6				// Copy the target pixel.

				movq		mm1, mm7				// Copy the source pixel.
				pand		mm0, mm5				// Clear each bottom-most target bit.

				pand		mm1, mm5				// Clear each bottom-most source bit.
				psrlq		mm0, 1					// Divide the target by 2.

				pcmpeqw		mm7, mm4				// Create a color key mask.
				psrlq		mm1, 1					// Divide the source by 2.

				paddw		mm0, mm1				// Add the target and the source.
				pand		mm6, mm7				// Keep old target where color key applies.

				pandn		mm7, mm0				// Keep new target where no color key applies.
				nop

				por			mm7, mm6				// Combine source and new target.
				nop

				movq		[edi], mm7				// Write back the new target.

next555:
				//
				// Advance to the next four pixels.
				//
				add			edi, 8
				add			esi, 8
				
				//
				// Loop again or break.
				//
				dec			ecx
				jnz			do_blend555

				//
				// Write back the frame buffer pointers and clean up.
				//
				mov			lpbTarget, edi	
				mov			lpbSource, esi
				pop			esi		
				pop			edi
				emms

skip555:
				pop			ecx
			}

			//
			// Alpha blend any remaining pixels.
			//
			for ( i = 0; i < iRemainder; i++ )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( dwTargetTemp & 0x7bde ) >> 1 ) + 
						  ( ( dwSourceTemp & 0x7bde ) >> 1 ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while ( --iHeight > 0 ); 

		break;

	/* 16 bit mode ( 565 ). This algorithm 
	   can process four pixels at once. */
	case RGBMODE_565:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		//
		// We process four pixels at once, so the 
		// width must be a multiple of four.
		//
		iRemainder = ( iWidth & 0x03 ); 
		iWidth = ( iWidth & ~0x03 ) / 4;

		// Set an auxiliary bit mask.
		i64Mask = 0xf7def7def7def7de;

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend four pixels at once.
			//
			__asm
			{
				//
				// Initialize the counter and skip 
				// if the latter is equal to zero.
				//
				push		ecx		
				mov			ecx, i	
				cmp			ecx, 0	
				jz			skip565
				
				//
				// Load the frame buffer pointers into the registers.
				//
				push		edi		
				push		esi
				mov			edi, lpbTarget		
				mov			esi, lpbSource

				// Create black as the color key. 
				pxor		mm4, mm4

				// Load the auxiliary mask into an mmx register.
				movq		mm5, i64Mask

do_blend565:
				//
				// Skip these four pixels if they are all black.
				//
				cmp			dword ptr [esi], 0
				jnz			not_black565
				cmp			dword ptr [esi + 4], 0
				jnz			not_black565
				jmp			next565

not_black565:
				//
				// Alpha blend four target and source pixels.
				//

				/* The mmx registers will basically be used in the following way:

					mm0:	target working register
					mm1:	source working register
					mm2:	not used
					mm3:	not used
					mm4:	black as the color key
					mm5:	auxiliary mask
					mm6:	original target pixel
					mm7:	original source pixel

				/* Note: Two lines together are assumed to pair 
				         in the processor´s U- and V-pipes. */

				movq		mm6, [edi]				// Load the original target pixel.
				nop

				movq		mm7, [esi]				// Load the original source pixel.
				movq		mm0, mm6				// Copy the target pixel.

				movq		mm1, mm7				// Copy the source pixel.
				pand		mm0, mm5				// Clear each bottom-most target bit.

				pand		mm1, mm5				// Clear each bottom-most source bit.
				psrlq		mm0, 1					// Divide the target by 2.

				pcmpeqw		mm7, mm4				// Create a color key mask.
				psrlq		mm1, 1					// Divide the source by 2.

				paddw		mm0, mm1				// Add the target and the source.
				pand		mm6, mm7				// Keep old target where color key applies.

				pandn		mm7, mm0				// Keep new target where no color key applies.
				nop

				por			mm7, mm6				// Combine source and new target.
				nop

				movq		[edi], mm7				// Write back the new target.

next565:
				//
				// Advance to the next four pixels.
				//
				add			edi, 8
				add			esi, 8
				
				//
				// Loop again or break.
				//
				dec			ecx
				jnz			do_blend565

				//
				// Write back the frame buffer pointers and clean up.
				//
				mov			lpbTarget, edi	
				mov			lpbSource, esi
				pop			esi		
				pop			edi
				emms

skip565:
				pop			ecx
			}

			//
			// Alpha blend any remaining pixels.
			//
			for ( i = 0; i < iRemainder; i++ )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( dwTargetTemp & 0xf7de ) >> 1 ) + 
						  ( ( dwSourceTemp & 0xf7de ) >> 1 ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while ( --iHeight > 0 ); 

		break; 

	/* 16 bit mode ( unknown ). This algorithm
	   can process four pixels at once. */
	case RGBMODE_16:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 2 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 2 );

		//
		// We process four pixels at once, so the 
		// width must be a multiple of four.
		//
		iRemainder = ( iWidth & 0x03 ); 
		iWidth = ( iWidth & ~0x03 ) / 4;

		// Create the bit mask used to clear the lowest bit of each color channel´s mask.
		wMask = ( WORD ) ( ( ddsdTarget.ddpfPixelFormat.dwRBitMask & 
						   ( ddsdTarget.ddpfPixelFormat.dwRBitMask << 1 ) ) | 
						   ( ddsdTarget.ddpfPixelFormat.dwGBitMask & 
						   ( ddsdTarget.ddpfPixelFormat.dwGBitMask << 1 ) ) | 
						   ( ddsdTarget.ddpfPixelFormat.dwBBitMask & 
						   ( ddsdTarget.ddpfPixelFormat.dwBBitMask << 1 ) ) );

		// Create a quadruple bit mask.
		i64Mask = wMask;
		i64Mask |= ( i64Mask << 16 ) | ( i64Mask << 32 ) | ( i64Mask << 48 );

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			// Reset the width.
			i = iWidth;

			// 
			// Alpha-blend four pixels at once.
			//
			__asm
			{
				//
				// Initialize the counter and skip 
				// if the latter is equal to zero.
				//
				push		ecx		
				mov			ecx, i	
				cmp			ecx, 0	
				jz			skip16
				
				//
				// Load the frame buffer pointers into the registers.
				//
				push		edi		
				push		esi
				mov			edi, lpbTarget		
				mov			esi, lpbSource

				// Create black as the color key. 
				pxor		mm4, mm4

				// Load the auxiliary mask into an mmx register.
				movq		mm5, i64Mask

do_blend16:
				//
				// Skip these four pixels if they are all black.
				//
				cmp			dword ptr [esi], 0
				jnz			not_black16
				cmp			dword ptr [esi + 4], 0
				jnz			not_black16
				jmp			next16

not_black16:
				//
				// Alpha blend four target and source pixels.
				//

				/* The mmx registers will basically be used in the following way:

					mm0:	target working register
					mm1:	source working register
					mm2:	not used
					mm3:	not used
					mm4:	black as the color key
					mm5:	auxiliary mask
					mm6:	original target pixel
					mm7:	original source pixel

				/* Note: Two lines together are assumed to pair 
				         in the processor´s U- and V-pipes. */

				movq		mm6, [edi]				// Load the original target pixel.
				nop

				movq		mm7, [esi]				// Load the original source pixel.
				movq		mm0, mm6				// Copy the target pixel.

				movq		mm1, mm7				// Copy the source pixel.
				pand		mm0, mm5				// Clear each bottom-most target bit.

				pand		mm1, mm5				// Clear each bottom-most source bit.
				psrlq		mm0, 1					// Divide the target by 2.

				pcmpeqw		mm7, mm4				// Create a color key mask.
				psrlq		mm1, 1					// Divide the source by 2.

				paddw		mm0, mm1				// Add the target and the source.
				pand		mm6, mm7				// Keep old target where color key applies.

				pandn		mm7, mm0				// Keep new target where no color key applies.
				nop

				por			mm7, mm6				// Combine source and new target.
				nop

				movq		[edi], mm7				// Write back the new target.

next16:
				//
				// Advance to the next four pixels.
				//
				add			edi, 8
				add			esi, 8
				
				//
				// Loop again or break.
				//
				dec			ecx
				jnz			do_blend16

				//
				// Write back the frame buffer pointers and clean up.
				//
				mov			lpbTarget, edi	
				mov			lpbSource, esi
				pop			esi		
				pop			edi
				emms

skip16:
				pop			ecx
			}

			//
			// Alpha blend any remaining pixels.
			//
			for ( i = 0; i < iRemainder; i++ )
			{
				// Read in one source pixel.
				dwSourceTemp = *( ( WORD* ) lpbSource );

				// If this is not the color key ...
				if ( dwSourceTemp != 0 )
				{
					//
					// ... apply the alpha blend to it.
					//

					// Read in one target pixel.
					dwTargetTemp = *( ( WORD* ) lpbTarget );

					// Write the destination pixel.
					*( ( WORD* ) lpbTarget ) = ( WORD ) 
						( ( ( dwTargetTemp & wMask ) >> 1 ) + 
						  ( ( dwSourceTemp & wMask ) >> 1 ) );
				}

				// 
				// Proceed to next pixel.
				//
				lpbTarget += 2;
				lpbSource += 2;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while ( --iHeight > 0 ); 

		break;

	/* 24 bit mode. */
	case RGBMODE_24:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 3 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 3 );

		// Create a general purpose mask.
		i64Mask = 0xffffffffff000000;       
		__asm movq	mm2, i64Mask

		// Create a mask that lets us clear each channel´s lowest bit.
		i64Mask = 0x0000000000fefefe;
		__asm movq	mm3, i64Mask;

		// Get the address of the target.
		__asm mov	edi, ddsdTarget.lpSurface;

		// Get the address of the source.
		__asm mov	esi, ddsdSource.lpSurface;


		//
		// Do the alpha blend.
		//
		__asm
		{
			//
			// Save registers.
			//
			push	edx
			push	ecx
			push	edi
			push	esi

			// Initialize the height counter.
			mov			edx, iHeight

new_line24:
			// Reset the width counter. 
			mov			ecx, iWidth	

do_blend24:
			//
			// Skip this pixel if it is black.
			//
			test		dword ptr [esi], 00ffffffh
			jz			next24
			
			//
			// Alpha blend a target and a source pixel.
			//

			/* The mmx registers will basically be used in the following way:

				mm0:	target value
				mm1:	source value
				mm2:	mask one
				mm3:	mask two
				mm4:	not used
				mm5:	not used
				mm6:	not used
				mm7:	original target

			/* Note: Two lines together are assumed to pair 
				     in the processor´s U- and V-pipes. */

			movd		mm0, [edi]				// Load the target pixel.
			movq		mm1, mm3				// Reload the shift mask.

			movq		mm7, mm0				// Save the target pixel.
			pand		mm0, mm1				// Clear each target channel´s lowest bit.

			pand		mm1, [esi]				// Clear each source channel´s lowest bit.
			psrlw		mm0, 1					// Divide the target by 2.

			pand		mm7, mm2				// Get the high order byte we must keep.
			psrlw		mm1, 1					// Divide the source by 2.
			
			paddw		mm0, mm1				// Add the target and the source.
			nop

			por			mm0, mm7				// Assemble the value to write back.
			nop

			movd		[edi], mm0				// Write back the new value.
			nop

next24:
			//
			// Advance to next pixel.
			//
			add			edi, 3	
			add			esi, 3		

			//
			// Loop again or break.
			//
			dec			ecx
			jnz			do_blend24

			// MMX done.
			emms

			//
			// Proceed to the next line.
			//
			add			edi, dwTargetPad;
			add			esi, dwSourcePad;

			dec			edx
			jnz			new_line24

			//
			// Restore registers.
			//
			pop		esi
			pop		edi
			pop		ecx
			pop		edx
		}

		break;

	/* 32 bit mode. This algorithm can
	   process two pixels at once. */
	case RGBMODE_32:
		//
		// Determine the padding bytes for the target and the source.
		//
		dwTargetPad = ddsdTarget.lPitch - ( iWidth * 4 );
		dwSourcePad = ddsdSource.lPitch - ( iWidth * 4 );

		// If the width is odd ...
		if ( iWidth & 0x01 )
		{
			// ... set the flag ...
			gOddWidth = true;

			// ... and calculate the width.
			iWidth = ( iWidth - 1 ) / 2;
		}
		// If the width is even ...
		else
		{
			// ... clear the flag ...
			gOddWidth = false;

			// ... and calculate the width.
			iWidth /= 2;
		}

		// Create an auxiliary mask.
		i64Mask = 0x00ffffff00ffffff;
		__asm movq	mm2, i64Mask

		// Create a mask that allows us to clear each channel´s highest bit.
		i64Mask = 0x007f7f7f007f7f7f;
		__asm movq	mm3, i64Mask

		// The mask must initially be present in two registers.
		__asm movq	mm1, mm3

		// Get the address of the target.
		lpbTarget = ( BYTE* ) ddsdTarget.lpSurface;

		// Get the address of the source.
		lpbSource = ( BYTE* ) ddsdSource.lpSurface;

		do
		{
			//
			// Alpha blend two pixels at once.
			//
			__asm
			{
				//
				// Initialize the counter and skip 
				// if the latter is equal to zero.
				//
				push		ecx		
				mov			ecx, iWidth	
				cmp			ecx, 0	
				jz			skip32

				//
				// Load the frame buffer pointers into the registers.
				//
				push		edi		
				push		esi
				mov			edi, lpbTarget		
				mov			esi, lpbSource

do_blend32:
				//
				// Skip these two pixels if they are both black.
				//
				test		dword ptr [esi], 00ffffffh
				jnz			not_black32
				test		dword ptr [esi + 4], 00ffffffh
				jnz			not_black32
				jmp			next32

not_black32:
				//
				// Alpha blend two target and two source pixels.
				//

				/* The mmx registers will basically be used in the following way:

					mm0:	target pixels
					mm1:	source pixels
					mm2:	mask one
					mm3:	mask two
					mm4:	color key mask
					mm5:	not used
					mm6:	not used
					mm7:	working register

				/* Note: Two lines together are assumed to pair 
				         in the processor´s U- and V-pipes. */

				movq		mm0, [edi]				// Load the target pixels.
				pxor		mm4, mm4				// Create black as the color key.

				pand		mm1, [esi]				// Mask off unwanted bytes.
				movq		mm7, mm0				// Save the original target.

				pcmpeqd		mm4, mm1				// Create a color key mask.
				psrld		mm0, 1					// Divide the target by 2.

				pand		mm0, mm3				// Clear each source channel´s highest bit.
				psrld		mm1, 1					// Divide the source by 2.

				pand		mm1, mm3				// Clear each target channel´s highest bit.
				pand		mm7, mm4				// Keep old target where color key applies.
				
				paddd		mm0, mm1				// Add the source and the target.	
				movq		mm1, mm2				// Reload mask one.

				pandn		mm4, mm0				// Keep new target where no color key applies.
				nop

				por			mm7, mm4				// Assemble the value to write back.
				nop

				movq		[edi], mm7				// Write back the new target value.
				nop

next32:
				//
				// Advance to the next pixel.
				//
				add			edi, 8
				add			esi, 8

				//
				// Loop again or break.
				//
				dec			ecx
				jnz			do_blend32

				//
				// Write back the frame buffer pointers and clean up.
				//
				mov			lpbTarget, edi	
				mov			lpbSource, esi
				pop			esi		
				pop			edi
				emms

skip32:
				pop			ecx
			}

			//
			// Handle an odd width.
			//
			if ( gOddWidth )
			{
				// Read in the next source pixel.
				dwSourceTemp = *( ( DWORD* ) lpbSource );	

				// If the source pixel is not black ...
				if ( ( dwSourceTemp & 0xffffff ) != 0 )
				{
					// ... read in the next target pixel.
					dwTargetTemp = *( ( DWORD* ) lpbTarget );

					// Calculate the destination pixel.
					dwTargetTemp = ( ( dwTargetTemp & 0xfefefe ) >> 1 ) + 
								   ( ( dwSourceTemp & 0xfefefe ) >> 1 ); 

					// Write the destination pixel.
					*( ( DWORD* ) lpbTarget ) = dwTargetTemp;
				}

				//
				// Proceed to the next pixel.
				//
				lpbTarget += 4;
				lpbSource += 4;
			}

			//
			// Proceed to the next line.
			//
			lpbTarget += dwTargetPad;
			lpbSource += dwSourcePad;
		}
		while ( --iHeight > 0 );

		break;

	/* Invalid mode. */
	default:
		iRet = -1;
	}

	// Unlock the target surface.
	lpDDSDest->Unlock( &rcDest );

	// Unlock the source surface.
	lpDDSSource->Unlock( lprcSource );

	return iRet;
}



