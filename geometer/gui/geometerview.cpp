
#include "geometerview.hpp"

const CColor coldwave = {75, 151, 71, 0};
const CColor cnewwave = {190, 255, 130, 0};
const CColor cpointoutside = {0, 0, 0, 0};
const CColor cpointinside = {220, 151, 200, 0};
const CColor cbackground = {20, 50, 20, 0};

void GeometerView::draw(CDrawContext * ctx) {


  if (!(offc && inputs && outputs)) return; /* not ready yet */

  offc->setFrameColor(coldwave);

  offc->setFillColor(cbackground);
  offc->fillRect(CRect(-1,-1,gwidth,gheight));

  offc->moveTo (CPoint(0, gheight * ((- inputs[0])+1.0f) * 0.5f));

  for(int u = 0; u < samples; u ++) {
    offc->lineTo (CPoint(u, gheight * (((-inputs[u])+1.0f) * 0.5f)));
  }

  offc->setFrameColor(cnewwave);
  offc->moveTo (CPoint(0, gheight * ((- outputs[0])+1.0f) * 0.5f));

  for(int v = 0; v < samples; v ++) {
    offc->lineTo (CPoint(v, gheight * (((-outputs[v])+1.0f) * 0.5f)));
  }

#if 1
  offc->setFillColor(cpointoutside);
  for(int w = 0; w < numpts; w ++) {
    int yy = (int)(gheight * (((-pointsy[w])+1.0f)*0.5f));
    offc->fillRect(CRect(pointsx[w]-2, yy - 2,
			 pointsx[w]+2, yy + 2));
  }

  offc->setFrameColor(cpointinside);
  for(int t = 0; t < numpts; t ++) {
    offc->drawPoint(CPoint(pointsx[t], (gheight * (((-pointsy[t])+1.0f)*0.5f))), cpointinside);
  }
#endif

  offc->copyFrom (ctx, sz, CPoint(0, 0));

  //  CView::draw(ctx);

  setDirty(false);
}


void GeometerView::update(CDrawContext * ctx) {
  /* ??? */

  CView::update(ctx);
}

/* XXX use memcpy where applicable. */  
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
  int npts = geom->processw(inputs, outputs, samples);
  geom->cs->release();

  if (npts >= samples) npts = samples - 1;

  for(int k=0; k < npts; k++) {
	pointsx[k] = geom->pointx[k];
	pointsy[k] = geom->pointy[k];
  }

  numpts = npts;

  setDirty();
  
}

void GeometerView::init() {
  inputs = (float*)malloc(sizeof (float) * (samples + 3));
  pointsx = (int*)malloc(sizeof (int) * (samples + 3));
  pointsy = (float*)malloc(sizeof (float) * (samples + 3));
  outputs = (float*)malloc(sizeof (float) * (samples + 3));
  offc = new COffscreenContext (getParent (), gwidth, gheight, kBlackCColor);
  
  reflect();
}
