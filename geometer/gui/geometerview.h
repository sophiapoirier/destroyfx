
#include "vstgui.h"
#include "vstcontrols.h"
#include "geometer.hpp"

/* this class is for the display of the wave at the top of the plugin. */



class GeometerView : public CView {

private:
  CRect sz;
  int gwidth;
  int gheight;
  int gx;
  int gy;

  int samples;
  int numpts;

  float * inputs;
  int * pointsx;
  float * pointsy;
  float * outputs;

  bool dirty;

  Geometer * geom;

  COffscreenContext * offc;

public:

  GeometerView(const CRect & size, Geometer * listener) : CView(size) {
    sz = size;
    geom = listener;
    gwidth = size.right - size.left;
    gheight = size.bottom - size.top;
    gx = size.left;
    gy = size.top;
    samples = gwidth;
    inputs = outputs = pointsy = 0;
    pointsx = 0;
    numpts = 0;
    offc = 0;
  }

  void init ();

  ~GeometerView() {
    free (inputs);
    free (pointsx);
    free (pointsy);
    free (outputs);
    delete offc;
  }

  virtual void draw (CDrawContext *pContext);

  virtual void update (CDrawContext *pContext);

  void reflect();

};
