
#include "geometerview.hpp"

const CColor coldwave = {75, 151, 71, 0};
const CColor cnewwave = {240, 255, 160, 0};
const CColor cpointoutside = {0, 0, 0, 0};
const CColor cpointinside = {220, 100, 200, 0};
const CColor cbackground = {20, 50, 20, 0};

void GeometerView::draw(CDrawContext * ctx) {


  if (!(offc && inputs && outputs)) return; /* not ready yet */

  offc->setFrameColor(coldwave);

  offc->setFillColor(cbackground);
  offc->fillRect(CRect(-1,-1,gwidth,gheight));

  int start = apts<gwidth?(gwidth-apts)>>1:0;

  offc->moveTo (CPoint(start, gheight * ((- inputs[0])+1.0f) * 0.5f));

  for(int u = 0; u < apts; u ++) {
    offc->lineTo (CPoint(start + u, gheight * (((-inputs[u])+1.0f) * 0.5f)));
  }

  offc->setFrameColor(cnewwave);
  offc->moveTo (CPoint(start, gheight * ((- outputs[0])+1.0f) * 0.5f));

  for(int v = 0; v < apts; v ++) {
    offc->lineTo (CPoint(start + v, gheight * (((-outputs[v])+1.0f) * 0.5f)));
  }

#if 1
  offc->setFillColor(cpointoutside);
  for(int w = 0; w < numpts; w ++) {
    int yy = (int)(gheight * (((-pointsy[w])+1.0f)*0.5f));
    offc->fillRect(CRect(start + pointsx[w]-2, yy - 2,
			 start + pointsx[w]+2, yy + 2));
  }

  offc->setFrameColor(cpointinside);
  for(int t = 0; t < numpts; t ++) {
    offc->drawPoint(CPoint(start + pointsx[t], 
			   (gheight * (((-pointsy[t])+1.0f)*0.5f))), cpointinside);
  }
#endif

  offc->copyFrom (ctx, sz, CPoint(0, 0));

  setDirty(false);
}


void GeometerView::update(CDrawContext * ctx) {
  /* ??? */

  CView::update(ctx);
}

/* XXX use memcpy where applicable. */  
/* XXX don't bother running processw unless 
   the input data have changed. */
void GeometerView::reflect() {
  /* when idle, copy points out of Geometer */

  if (!inputs || !geom || !geom->in0) return; /* too soon */

#if 1
  for(int j=0; j < samples; j++) {
    inputs[j] = geom->in0[j];
  }
#else

  for(int j=0; j < samples; j++) {
    inputs[j] = sin((j * 10 * 3.14159) / samples);
  }

#endif

  geom->cs->grab();
  apts = geom->framesize;
  if (apts > samples) apts = samples;

  int npts = geom->processw(inputs, outputs, apts,
			    pointsx, pointsy, samples - 1,
			    tmpx, tmpy);
  geom->cs->release();

  numpts = npts;

  setDirty();
  
}

void GeometerView::init() {
  inputs = (float*)malloc(sizeof (float) * (samples + 3));
  pointsx = (int*)malloc(sizeof (int) * (samples + 3));
  pointsy = (float*)malloc(sizeof (float) * (samples + 3));
  tmpx = (int*)malloc(sizeof (int) * (samples + 3));
  tmpy = (float*)malloc(sizeof (float) * (samples + 3));
  outputs = (float*)malloc(sizeof (float) * (samples + 3));
  offc = new COffscreenContext (getParent (), gwidth, gheight, kBlackCColor);
  
  reflect();
}
