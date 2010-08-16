#include "lice.h"
#include "lice_combine.h"
#include <math.h>

template <class T> inline void SWAP(T& a, T& b) { T tmp = a; a = b; b = tmp; }

enum { eOK = 0, eXLo = 1, eXHi = 2, eYLo = 4, eYHi = 8 };

static int OffscreenTest(int x, int y, int nX, int nY)
{
  int e = eOK;
  if (x < 0) e |= eXLo; 
  else if (x >= nX) e |= eXHi; 
  if (y < 0) e |= eYLo; 
  else if (y >= nY) e |= eYHi; 
  return e;
}

// Cohen-Sutherland.  Returns false if the line is entirely offscreen.
static bool ClipLine(int* pX1, int* pY1, int* pX2, int* pY2, int nX, int nY)
{
  int x1 = *pX1, y1 = *pY1, x2 = *pX2, y2 = *pY2;
  int e1 = OffscreenTest(x1, y1, nX, nY); 
  int e2 = OffscreenTest(x2, y2, nX, nY);
  
  bool accept = false, done = false;
  do
  {
    if (!(e1 | e2)) {
      accept = done = true;
    }
    else
    if (e1 & e2) {
      done = true;	// Line is entirely offscreen.
    }
    else { 
      int x, y;
      int eOut = e1 ? e1 : e2;
      if (eOut & eYHi) {
        x = x1 + (int) ((double) (x2 - x1) * (double) (nY - y1) / (double) (y2 - y1));
        y = nY - 1;
      }
      else
      if (eOut & eYLo) {
        x = x1 + (int) ((double) (x2 - x1) * (double) -y1 / (double) (y2 - y1));
        y = 0;
      }
      else 
      if (eOut & eXHi) {
        y = y1 + (int) ((double) (y2 - y1) * (double) (nX - x1) / (double) (x2 - x1));
        x = nX - 1;
      }
      else {
        y = y1 + (int) ((double) (y2 - y1) * (double) -x1 / (double) (x2 - x1));
        x = 0;
      }

      if (eOut == e1) { 
        x1 = x; 
        y1 = y;
        e1 = OffscreenTest(x1, y1, nX, nY);
      }
      else {
        x2 = x;
        y2 = y;
        e2 = OffscreenTest(x2, y2, nX, nY);
      }
    }
  }
  while (!done);

  *pX1 = x1;
  *pY1 = y1;
  *pX2 = x2;
  *pY2 = y2;
  return accept;
}

template <class COMBFUNC> class __LICE_LineClass
{
private:

  static inline void DoPx(LICE_pixel* pPx, int r, int g, int b, int a, float alpha, double weight)
  {
    int aw = (int) (alpha*(float) weight*256.0f);  // Optimize with __LICE_TOINT() ?
    COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw);
  }

public:

  static void LICE_VertLine(LICE_IBitmap* pDest, int xi, int yi1, int yi2, LICE_pixel color, float alpha)
  {
    int w = pDest->getRowSpan();
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);
    int n = abs(yi2 - yi1);
    LICE_pixel* pPx = pDest->getBits() + yi1*w + xi;
    int pxStep = (yi1 < yi2 ? w : -w);
    int aw = (int) (alpha*256.0f);
    for (int i = 0; i <= n; ++i, pPx += pxStep) {
      COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw);
    }
  }

  static void LICE_HorizLine(LICE_IBitmap* pDest, int yi, int xi1, int xi2, LICE_pixel color, float alpha)
  {
    int w = pDest->getRowSpan();
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);
    int n = abs(xi2 - xi1);
    LICE_pixel* pPx = pDest->getBits() + yi*w + xi1;
    int pxStep = (xi1 < xi2 ? 1 : -1);
    int aw = (int) (alpha*256.0f);
    for (int i = 0; i <= n; ++i, pPx += pxStep) {
      COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw);
    }
  }
     
  static void LICE_LineImpl(LICE_IBitmap* pDest, int xi1, int yi1, int xi2, int yi2, LICE_pixel color, float alpha, bool aa)
  {
    // If (xi1 == xi2) or (yi1 == yi2) LICE_Line calls VertLine or HorizLine instead of getting here.
    int w = pDest->getRowSpan();
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);

    int nx = abs(xi2 - xi1);
    int ny = abs(yi2 - yi1);
    int n, pxStep, pxStep2;
    double err = 0.0, dErr;
    if (ny > nx) {  // Steep.
      n = ny;
      pxStep = (yi1 < yi2 ? w : -w);
      pxStep2 = (xi1 < xi2 ? 1 : -1);
      dErr = (double) nx / (double) ny;
    }
    else {
      n = nx;
      pxStep = (xi1 < xi2 ? 1 : -1);
      pxStep2 = (yi1 < yi2 ? w : -w);
      dErr = (double) ny / (double) nx;
    }
    LICE_pixel* pPx = pDest->getBits() + yi1*w + xi1;   

    if (aa) {
      for (int i = 0; i < n; ++i, pPx += pxStep) {
        DoPx(pPx, r, g, b, a, alpha, 1.0 - err);
        DoPx(pPx + pxStep2, r, g, b, a, alpha, err);
        err += dErr;
        if (err > 1.0) {
          pPx += pxStep2;
          err -= 1.0;
        }
      }
    }
    else {
      for (int i = 0; i <= n; ++i, pPx += pxStep) {
        DoPx(pPx, r, g, b, a, alpha, 1.0);
        err += dErr;
        if (err > 0.5) {
          pPx += pxStep2;
          err -= 1.0;
        }
      }
    }
  }

};

void LICE_Line(LICE_IBitmap *pDest, float x1, float y1, float x2, float y2, LICE_pixel color, float alpha, int mode, bool aa)
{
  if (!pDest) return;
  int xi1 = (int) x1, yi1 = (int) y1, xi2 = (int) x2, yi2 = (int) y2;
	int w = pDest->getWidth(), h = pDest->getHeight();
  if (pDest->isFlipped()) {
    yi1 = h - 1 - yi1;
    yi2 = h - 1 - yi2;
  }

	if (ClipLine(&xi1, &yi1, &xi2, &yi2, w, h)) {

    if (xi1 == xi2) {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_VertLine(pDest, xi1, yi1, yi2, color, alpha)
      __LICE_ACTIONBYMODE(mode, alpha);
#undef __LICE__ACTION
    }
    else
    if (yi1 == yi2) {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_HorizLine(pDest, yi1, xi1, xi2, color, alpha)
      __LICE_ACTIONBYMODE(mode, alpha);
#undef __LICE__ACTION
    }
    else {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_LineImpl(pDest, xi1, yi1, xi2, yi2, color, alpha, aa)
			__LICE_ACTIONBYMODE(mode, alpha);
#undef __LICE__ACTION
		}
	}
}

bool LICE_ClipLine(float* pX1, float* pY1, float* pX2, float* pY2, int xLo, int yLo, int xHi, int yHi)
{
    int x1 = (int) *pX1 - xLo;
    int y1 = (int) *pY1 - yLo;
    int x2 = (int) *pX2 - xLo;
    int y2 = (int) *pY2 - yLo;
    bool onscreen = ClipLine(&x1, &y1, &x2, &y2, xHi - xLo, yHi - yLo);
    *pX1 = (float) (x1 + xLo);
    *pY1 = (float) (y1 + yLo);
    *pX2 = (float) (x2 + xLo);
    *pY2 = (float) (y2 + yLo);
    return onscreen;
}
