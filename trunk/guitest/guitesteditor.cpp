
#include "guitesteditor.h"

#define WINDOWCLASSNAME "GuiTestClass"

// #include <control.h>


#define EDIT_HEIGHT 900
#define EDIT_WIDTH 900


#define THINGS 100

extern HINSTANCE instance;
int useCount = 0;

GuitestEditor::GuitestEditor (AudioEffect *effect) : AEffEditor (effect) {
  effect->setEditor (this);
  pD3D = 0;
  pd3dDevice = 0;
}

GuitestEditor::~GuitestEditor () {
  if( pd3dDevice != NULL) 
    pd3dDevice->Release();

  if( pD3D != NULL)
    pD3D->Release();
}


long GuitestEditor::getRect (ERect **erect) {
  static ERect r = {0, 0, EDIT_HEIGHT, EDIT_WIDTH};
  *erect = &r;
  return true;
}


long GuitestEditor::open (void *ptr) {

  // Remember the parent window
  systemWindow = ptr;

  www = (Where *) malloc(THINGS * sizeof (Where));

  for(int i = 0; i < THINGS; i ++) {

    www[i].x = rand () % EDIT_WIDTH;
    www[i].y = rand () % EDIT_HEIGHT;

    www[i].dy = (rand () & 3) - 2;
    www[i].dx = (rand () & 3) - 2;

  }

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

  window = hwnd;

  /* store this pointer with window so that callback can
     dispatch to appropriate class instance. */
  SetWindowLong (hwnd, GWL_USERDATA, (long)this);

  if (FAILED (InitD3D (hwnd))) return false;
    

  /* set up orthographic projection and no wolrd/view transform.
     This gives us a flat view of the coordinate space
     from -EDIT_WIDTH/2 to EDIT_WIDTH/2, etc. */
  D3DXMATRIX Ortho2D;	
  D3DXMATRIX Identity;
	
  D3DXMatrixOrthoLH(&Ortho2D, EDIT_WIDTH, EDIT_HEIGHT, 0.0f, 1.0f);
  D3DXMatrixIdentity(&Identity);

  pd3dDevice->SetTransform(D3DTS_PROJECTION, &Ortho2D);
  pd3dDevice->SetTransform(D3DTS_WORLD, &Identity);
  pd3dDevice->SetTransform(D3DTS_VIEW, &Identity);

  pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

  ggg = new Graphic(pd3dDevice, "dfximage");

  /* turn on alpha blending */
  // Turn off culling, so we see the front and back of primitives
  // DXTEST( lpD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE) );


  pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  TRUE);
  pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);  

  return true;
}

void GuitestEditor::close () {
  /* FIXME destroy dd surfaces, etc. */

  useCount--;
  if (useCount == 0) {
    UnregisterClass (WINDOWCLASSNAME, instance);
  }
}


void GuitestEditor::idle () {
  redraw();

  AEffEditor::idle ();
}


void GuitestEditor::update() {
#if 0
  SendMessage (volumeFader, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) effect->getParameter (1));
#endif
  AEffEditor::update ();
}


void GuitestEditor::setValue(void* fader, int value) {
#if 0
  if (fader == delayFader)
    effect->setParameterAutomated (0, (float)value / 100.f);
#endif
}

/* XXX ? */
void GuitestEditor::postUpdate() {
  AEffEditor::postUpdate ();
}

void GuitestEditor::redraw() {

    if( NULL == pd3dDevice )
        return;

    // Clear the backbuffer to a green color
    pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, 
			 D3DCOLOR_XRGB(0,50,0), 1.0f, 0 );

    // Begin the scene
    pd3dDevice->BeginScene();

    // Rendering of scene objects can happen here

    Render2D(); 

    // End the scene
    pd3dDevice->EndScene();

    // Present the backbuffer contents to the display
    pd3dDevice->Present( NULL, NULL, NULL, NULL );
}

void GuitestEditor::Render2D () {

#if 0
  ggg->drawat(1, 1);
  ggg->drawat(50, 50);

#else
  for(int i = 0; i < THINGS; i ++) {
    ggg->drawat(www[i].x - EDIT_WIDTH / 2,
		www[i].y - EDIT_HEIGHT / 2);

    www[i].x += www[i].dx; www[i].y += www[i].dy;

    if (www[i].x > (EDIT_WIDTH - 160)) { www[i].x = (EDIT_WIDTH - 160); www[i].dx = -www[i].dx; }
    if (www[i].y > (EDIT_HEIGHT - 160)) { www[i].y = (EDIT_WIDTH - 160); www[i].dy = -www[i].dy; }
    if (www[i].x <= 0) { www[i].x = 0; www[i].dx = -www[i].dx; }
    if (www[i].y <= 0) { www[i].y = 0; www[i].dy = -www[i].dy; }
  }
#endif

}

LONG WINAPI GuitestEditor::WindowProc (HWND hwnd, UINT message, WPARAM wParam, 
				       LPARAM lParam) {
  GuitestEditor * editor = 
    (GuitestEditor *)GetWindowLong (hwnd, GWL_USERDATA);

  switch (message) {
  case WM_PAINT: {
    editor->redraw();
    ValidateRect( hwnd, NULL );
  }
  default:
    return DefWindowProc (hwnd, message, wParam, lParam);
  }
  return true;
}

//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT GuitestEditor:: InitD3D( HWND hWnd )
{
    // Create the D3D object, which is needed to create the D3DDevice.
    if( NULL == ( pD3D = Direct3DCreate8( D3D_SDK_VERSION ) ) )
      return E_FAIL;

    // Get the current desktop display mode
    D3DDISPLAYMODE d3ddm;
    DXTEST( pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ) );

    // Set up the structure used to create the D3DDevice. Most parameters are
    // zeroed out. We set Windowed to TRUE, since we want to do D3D in a
    // window, and then set the SwapEffect to "discard", which is the most
    // efficient method of presenting the back buffer to the display.  And 
    // we request a back buffer format that matches the current desktop display 
    // format.
    D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.BackBufferWidth = 0;
    d3dpp.BackBufferHeight = 0;


    // Create the Direct3D device. Here we are using the default adapter (most
    // systems only have one, unless they have multiple graphics hardware cards
    // installed) and requesting the HAL (which is saying we want the hardware
    // device rather than a software one). Software vertex processing is 
    // specified since we know it will work on all cards. On cards that support 
    // hardware vertex processing, though, we would see a big performance gain 
    // by specifying hardware vertex processing.
    if( FAILED( pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &pd3dDevice ) ) ) {
        return E_FAIL;
    }

    // Device state would normally be set here

    return S_OK;
}

