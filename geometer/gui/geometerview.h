
#include "vstgui.h"
#include "vstcontrols.h"
#include "geometer.hpp"

/* this class is for the display of the wave at the top of the plugin.
   It reads the current 'in' buffer for the plugin, runs it through
   geometer with its current settings, and then draws it up there
   whenever the plugin is idle.
*/

#ifdef WIN32
/* turn off warnings about default but no cases in switch, etc. */
   #pragma warning( disable : 4065 57 4200 4244 )
   #include <windows.h>
#endif

class GeometerView : public CView {

private:
  CRect sz;
  int gwidth;
  int gheight;
  int gx;
  int gy;

  int samples;
  int numpts;

  /* active points */
  int apts;

  float * inputs;
  int * pointsx;
  float * pointsy;
  float * outputs;

  /* passed to processw */
  int * tmpx;
  float * tmpy;

#if TARGET_PLUGIN_USES_DSPCORE
  GeometerDSP * geom;
#else
  Geometer * geom;
#endif

  COffscreenContext * offc;

public:

  GeometerView(const CRect & size, Geometer * listener) : CView(size) {
    sz = size;
  #if TARGET_PLUGIN_USES_DSPCORE
    geom = (GeometerDSP*) (listener->getplugincore(0));
  #else
    geom = listener;
  #endif
    gwidth = size.right - size.left;
    gheight = size.bottom - size.top;
    gx = size.left;
    gy = size.top;
    samples = gwidth;
    inputs = outputs = pointsy = tmpy = NULL;
	pointsx = tmpx = NULL;
    numpts = 0;
	apts = 0;
    offc = NULL;
  }

  void init ();

  ~GeometerView() {
    if (inputs != NULL)
      free (inputs);
    if (pointsx != NULL)
      free (pointsx);
    if (pointsy != NULL)
      free (pointsy);
    if (tmpx != NULL)
      free (tmpx);
    if (tmpy != NULL)
      free (tmpy);
    if (outputs != NULL)
      free (outputs);
    if (offc != NULL)
      delete offc;
  }

  virtual void draw (CDrawContext *pContext);

  virtual void update (CDrawContext *pContext);

  void reflect();

};
