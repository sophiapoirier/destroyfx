
#include <d3d8.h>
#include <d3dx8math.h>
#include <d3dx8tex.h>
#include <dxerr8.h>
#include <stdio.h>

struct PANELVERTEX
{
    FLOAT x, y, z;
    DWORD color;
    FLOAT u, v;
};

#define D3DFVF_PANELVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define SURFACE_FORMAT D3DFMT_A8R8G8B8
#define FILTER_HOW D3DX_FILTER_NONE


#define DXTEST(c) do { int err; if ((err=(c)) != D3D_OK) \
                       MessageBox(0, #c, "ERROR", MB_OK); } while(0)

extern HINSTANCE instance;

struct Graphic {

  public:
LPDIRECT3DVERTEXBUFFER8 verts;
  LPDIRECT3DTEXTURE8      texture;

  LPDIRECT3DDEVICE8 device;

  int height;
  int width;

  /* the texture may have a larger size.
     (many video cards make you round up
     to the nearest power-of-two.) */
  unsigned int theight;
  unsigned int twidth;
  
  /* load and set height/width */
  void CreateSurfaceFromFile(LPDIRECT3DSURFACE8* ppSurface, 
			     const char * filename) {
    
    D3DXIMAGE_INFO srcInfo;

    /* I use alpha from file, no color key */
    D3DCOLOR colourKey = 0; // 0xFF000000
    
    LPDIRECT3DSURFACE8 pSurface;
    PALETTEENTRY palette[256]; // Optional
    
    // A quick hack to get the size of the image into srcInfo.
    DXTEST(device->CreateImageSurface(1, 1, 
				      SURFACE_FORMAT, &pSurface));
#if 0
    DXTEST(D3DXLoadSurfaceFromFile(pSurface, NULL, NULL, filename, 
				   NULL, FILTER_HOW, 0, &srcInfo));
#endif
    DXTEST(D3DXLoadSurfaceFromResource(pSurface,
				       NULL, NULL,
				       instance,
				       filename,
				       NULL, FILTER_HOW, 0, &srcInfo));

    pSurface->Release();

    height = srcInfo.Height;
    width = srcInfo.Width;

    // Create a surface to hold the entire file
    DXTEST(device->CreateImageSurface(width, height, 
				      SURFACE_FORMAT, ppSurface));
    pSurface = *ppSurface;
    
    // The default colour key is 0xFF000000 (opaque black). Magenta 
    // (0xFFFF00FF) is another common colour used for transparency.
#if 1
    DXTEST(D3DXLoadSurfaceFromResource(pSurface, 
				       palette, 
				       NULL, 
				       instance,
				       filename, 
				       NULL, FILTER_HOW,
				       colourKey, &srcInfo));
#endif
  }


  /* height and width have to be set */
  void CreateTextureFromSurface(LPDIRECT3DSURFACE8 surf,
				LPDIRECT3DTEXTURE8* ppTexture) {
    RECT SrcRect;
    SrcRect.top = 0;
    SrcRect.left = 0;
    SrcRect.right = width; // - 1;
    SrcRect.bottom = height; // - 1;

    unsigned int dummy_mip = 1;
    theight = height;
    twidth = width;
    D3DXCheckTextureRequirements(device,
				 &twidth,
				 &theight,
				 &dummy_mip,
				 0, 0, D3DPOOL_DEFAULT);

    D3DSURFACE_DESC surfDesc;
    DXTEST( surf->GetDesc(&surfDesc) );
    DXTEST( D3DXCreateTexture(device, width, height, 
			      1, 0, SURFACE_FORMAT, 
			      D3DPOOL_DEFAULT, ppTexture) );

    // Retrieve the surface image of the texture.
    LPDIRECT3DSURFACE8 pTexSurface;
    LPDIRECT3DTEXTURE8 pTexture = *ppTexture;
    DXTEST( pTexture->GetLevelDesc(0, &surfDesc) );
    DXTEST( pTexture->GetSurfaceLevel(0, &pTexSurface) );

    // Create a clean surface to clear the texture with.
    LPDIRECT3DSURFACE8 pCleanSurface;
    D3DLOCKED_RECT lockRect;
    DXTEST( device->CreateImageSurface
	    (surfDesc.Width, surfDesc.Height, 
	     surfDesc.Format, &pCleanSurface) );
    DXTEST( pCleanSurface->LockRect(&lockRect, NULL, 0) );
    memset((BYTE*)lockRect.pBits, 0, surfDesc.Height * lockRect.Pitch);
    DXTEST( pCleanSurface->UnlockRect() );

    /* clear */
    DXTEST( device->CopyRects(pCleanSurface, NULL, 0, 
			      pTexSurface, NULL) );
    pCleanSurface->Release();

    // Copy the image to the texture.
    POINT destPoint = { 0, 0 };
    DXTEST( device->CopyRects(surf, &SrcRect, 1, 
			      pTexSurface, &destPoint) );
    pTexSurface->Release();

  }

  Graphic(LPDIRECT3DDEVICE8 dev, const char * path) {

    device = dev;

    LPDIRECT3DSURFACE8 surf = 0;

    CreateSurfaceFromFile(&surf, path);

    /* initialize a single rectangle. */

#if 0
    char mess[256];
    sprintf(mess, "width: %d height: %d", width, height);
    MessageBox(0, mess, "HEY", MB_OK);
#endif

    CreateTextureFromSurface(surf, &texture);

    dev->CreateVertexBuffer(4 * sizeof(PANELVERTEX), D3DUSAGE_WRITEONLY,
			    D3DFVF_PANELVERTEX, D3DPOOL_MANAGED, &verts);

    PANELVERTEX* pVertices = NULL;
    verts->Lock(0, 4 * sizeof(PANELVERTEX), (BYTE**)&pVertices, 0);

    // Set all the colors to white
    /* apparently changing these will change the alpha of the texture. */
    pVertices[0].color = pVertices[1].color = 
      pVertices[2].color = pVertices[3].color = 0xe0ffffff;

    pVertices[0].x = pVertices[3].x = 0.0;
    pVertices[1].x = pVertices[2].x = (float)width;

    pVertices[2].y = pVertices[3].y = 0.0;
    pVertices[0].y = pVertices[1].y = (float)height;


    pVertices[0].z = pVertices[1].z = pVertices[2].z = pVertices[3].z = 1.0f;

    pVertices[1].u = pVertices[2].u = ((float)width + 1.0)/(float)twidth;
    pVertices[0].u = pVertices[3].u = 0.0f;

    pVertices[0].v = pVertices[1].v = 0.0f;
    pVertices[2].v = pVertices[3].v = ((float)height + 1.0)/(float)theight;

    verts->Unlock();

  }

  void drawat(int x, int y) {
    D3DXMATRIX Position;
    D3DXMatrixTranslation(&Position, x, y, 0.0f);

    device->SetTransform(D3DTS_WORLD, &Position);

    device->SetTexture(0, texture);
    device->SetVertexShader(D3DFVF_PANELVERTEX);
    device->SetStreamSource(0, verts, sizeof(PANELVERTEX));
    device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);

  }


};
