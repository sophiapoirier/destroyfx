
#include <d3d8.h>
#include <d3dx8math.h>

struct PANELVERTEX
{
    FLOAT x, y, z;
    DWORD color;
    FLOAT u, v;
};

#define D3DFVF_PANELVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct Graphic {

  public:
  LPDIRECT3DVERTEXBUFFER8 g_pVertices;
  LPDIRECT3DTEXTURE8      g_pTexture;

  LPDIRECT3DDEVICE8 device;

  Graphic(LPDIRECT3DDEVICE8 dev, const char * path) {

    device = dev;

    /* initialize a single rectangle. */
    float PanelWidth = 150.0f;
    float PanelHeight = 150.0f;

    dev->CreateVertexBuffer(4 * sizeof(PANELVERTEX), D3DUSAGE_WRITEONLY,
				     D3DFVF_PANELVERTEX, D3DPOOL_MANAGED, &g_pVertices);

    PANELVERTEX* pVertices = NULL;
    g_pVertices->Lock(0, 4 * sizeof(PANELVERTEX), (BYTE**)&pVertices, 0);

    // Set all the colors to white
    /* apparently changing these will change the alpha of the texture. */
    pVertices[0].color = pVertices[1].color = pVertices[2].color = pVertices[3].color = 0x77ff77ff;

    /* Set positions and texture coordinate. */
    pVertices[0].x = pVertices[3].x = -PanelWidth / 2.0;
    pVertices[1].x = pVertices[2].x = PanelWidth / 2.0;

    pVertices[2].y = pVertices[3].y = - PanelHeight / 2.0;
    pVertices[0].y = pVertices[1].y = PanelHeight / 2.0;


    pVertices[0].z = pVertices[1].z = pVertices[2].z = pVertices[3].z = 1.0f;

    pVertices[1].u = pVertices[2].u = 1.0f;
    pVertices[0].u = pVertices[3].u = 0.0f;

    pVertices[0].v = pVertices[1].v = 0.0f;
    pVertices[2].v = pVertices[3].v = 1.0f;

    g_pVertices->Unlock();


    /* FILTER_NONE could also be FILTER_POINT */
    HRESULT hr = 
      D3DXCreateTextureFromFileEx(dev, 
				  /*
				    "c:\\temp\\alpha.png", 
				  */
				  path,
				  0, 0, 0, 0,
				  D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, 

				  D3DX_FILTER_NONE,
				  D3DX_FILTER_NONE, 

				  /*
				    D3DX_DEFAULT,
				    D3DX_DEFAULT,
				  */
				  0, NULL, NULL, &g_pTexture);
#if 0
    switch (hr) {
    case D3DERR_NOTAVAILABLE:
      MessageBox(0, "Not avail.", "", MB_HELP);
      break;
    case D3DERR_OUTOFVIDEOMEMORY:
      MessageBox(0, "Out of vid mem.", "", MB_HELP);
      break;
    case D3DERR_INVALIDCALL:
      MessageBox(0, "Inv call.", "", MB_HELP);
      break;
    case D3DXERR_INVALIDDATA:
      MessageBox(0, "Inv data.", "", MB_HELP);
      break;
    case E_OUTOFMEMORY:
      MessageBox(0, "Out of mem.", "", MB_HELP);
      break;
    case D3D_OK:
      MessageBox(0, "OK.", "", MB_HELP);
      break;
    default:
      MessageBox(0, "Some other.", "", MB_HELP);
      break;
    }
#endif


  }

  void drawat(int x, int y) {
    D3DXMATRIX Position;
    D3DXMatrixTranslation(&Position, x, y, 0.0f);

    device->SetTransform(D3DTS_WORLD, &Position);

    device->SetTexture(0, g_pTexture);
    device->SetVertexShader(D3DFVF_PANELVERTEX);
    device->SetStreamSource(0, g_pVertices, sizeof(PANELVERTEX));
    device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);

  }


};
