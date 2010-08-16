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
public:

  static void LICE_VertLine(LICE_IBitmap* pDest, int xi, int yi1, int yi2, LICE_pixel color, float alpha)
  {
    int w = pDest->getRowSpan();
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);
    int n = abs(yi2 - yi1);

    LICE_pixel* pPx = pDest->getBits() + yi1*w + xi;
    int pxStep = (yi1 < yi2 ? w : -w);
    int aw = (int) (255.0f * alpha);
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
    int aw = (int) (255.0f * alpha);
    for (int i = 0; i <= n; ++i, pPx += pxStep) {
      COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw);
    }
  }
     
  static void LICE_DiagLine(LICE_IBitmap* pDest, int xi1, int yi1, int xi2, int yi2, LICE_pixel color, float alpha, bool aa)
  {
    int w = pDest->getRowSpan();
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);
    int n = abs(xi2 - xi1);
    LICE_pixel* pPx = pDest->getBits() + yi1*w + xi1;
    int xStep = (xi2 > xi1 ? 1 : -1);
    int yStep = (yi2 > yi1 ? w : -w);
    int step = xStep + yStep;
    int aw = (int) (255.0f * alpha);

    if (aa) {
      int iw = aw * 3 / 4;
      int dw = aw / 4;
      for (int i = 0; i < n; ++i, pPx += step) {
        COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, iw);       
        COMBFUNC::doPix((LICE_pixel_chan*) (pPx+xStep), r, g, b, a, dw); 
        COMBFUNC::doPix((LICE_pixel_chan*) (pPx+yStep), r, g, b, a, dw);
      }
      COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, iw);
    }
    else {
      for (int i = 0; i <= n; ++i, pPx += step) {
        COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw);
      }
    }
  }

  static void LICE_LineImpl(LICE_IBitmap* pDest, int xi1, int yi1, int xi2, int yi2, LICE_pixel color, float alpha, bool aa)
  {
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
    int aw = (int) (255.0f * alpha);

    if (aa) {
      for (int i = 0; i < n; ++i, pPx += pxStep) {
        int iErr = __LICE_TOINT(255.0 * err);
        COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw * (255 - iErr) / 256);
        COMBFUNC::doPix((LICE_pixel_chan*) (pPx + pxStep2), r, g, b, a, aw * iErr / 256);
        err += dErr;
        if (err > 1.0) {
          pPx += pxStep2;
          err -= 1.0;
        }
      }
      int iErr = __LICE_TOINT(255.0 * err);
      COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw * (255 - iErr) / 256);
    }
    else {
      for (int i = 0; i <= n; ++i, pPx += pxStep) {
        COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw);
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
      __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
#undef __LICE__ACTION
    }
    else
    if (yi1 == yi2) {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_HorizLine(pDest, yi1, xi1, xi2, color, alpha)
      __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
#undef __LICE__ACTION
    }
    else
    if (abs(xi2-xi1) == abs(yi2-yi1)) {
      if (aa) {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_DiagLine(pDest, xi1, yi1, xi2, yi2, color, alpha, aa)
      __LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
#undef __LICE__ACTION
      }
      else {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_DiagLine(pDest, xi1, yi1, xi2, yi2, color, alpha, aa)
      __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
#undef __LICE__ACTION
      }
    }
    else {
      if (aa) {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_LineImpl(pDest, xi1, yi1, xi2, yi2, color, alpha, aa)
			__LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
#undef __LICE__ACTION
      }
      else {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_LineImpl(pDest, xi1, yi1, xi2, yi2, color, alpha, aa)
			__LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
#undef __LICE__ACTION

      }
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

#include "lice_bezier.h"

#define BEZ_RES 4.0f

// quadratic bezier
void LICE_DrawBezier(LICE_IBitmap* dest, float xstart, float ystart, float xctl, float yctl, float xend, float yend, 
  LICE_pixel color, float alpha, int mode, bool aa)
{
  float dx = xctl-xstart, dy = yctl-ystart;
  int i, n = (int) sqrt(dx*dx+dy*dy);
  dx = xend-xctl, dy = yend-yctl;
  n += (int) sqrt(dx*dx+dy*dy);
  n /= 2;
  float xprev = xstart, yprev = ystart, x, y;
  for (i = 1; i < n; ++i) {
    LICE_Bezier(xstart, xctl, xend, ystart, yctl, yend, (double)i/(double)n, &x, &y);
    if (fabs(xprev-x) > 1.0f || fabs(yprev-y) > 1.0f) {
      LICE_Line(dest, xprev, yprev, x, y, color, alpha, mode, aa);
      xprev = x;
      yprev = y;
    }
  }
  LICE_Line(dest, xprev, yprev, xend, yend, color, alpha, mode, aa);
}

// cubic bezier
void LICE_DrawCBezier(LICE_IBitmap* dest, float xstart, float ystart, float xctl1, float yctl1,
  float xctl2, float yctl2, float xend, float yend, LICE_pixel color, float alpha, int mode, bool aa)
{

#if 0
  // rasterizing costs a binary search per x-pixel, a significant CPU hit over the quick & dirty bez drawing below
  if (xend < xstart) {
    SWAP(xstart, xend);
    SWAP(ystart, yend);
    SWAP(xctl1, xctl2);
    SWAP(yctl1, yctl2);
  }
  float x, xprev = xstart, yprev = ystart;
  for (x = xstart; x < xend; x += 2.0f) {
    float y = LICE_CBezier_GetY(xstart, xctl1, xctl2, xend, ystart, yctl1, yctl2, yend, x);
    LICE_Line(dest, xprev, yprev, x, y, color, alpha, mode, aa);
    xprev = x;
    yprev = y;
  }
  LICE_Line(dest, xprev, yprev, xend, yend, color, alpha, mode, aa);
#endif

  float dx = xctl1-xstart, dy = yctl1-ystart;
  int i, n = (int) sqrt(dx*dx+dy*dy);
  dx = xctl2-xctl1, dy = yctl2-yctl1;
  n += (int) sqrt(dx*dx+dy*dy);
  dx = xend-xctl2, dy = yend-yctl2;
  n += (int) sqrt(dx*dx+dy*dy);
  n /= 2;
  float xprev = xstart, yprev = ystart, x, y;
  for (i = 1; i < n; ++i) {
    LICE_CBezier(xstart, xctl1, xctl2, xend, ystart, yctl1, yctl2, yend, (double)i/(double)n, &x, &y);
    if (fabs(xprev-x) > 1.0f || fabs(yprev-y) > 1.0f) {
      LICE_Line(dest, xprev, yprev, x, y, color, alpha, mode, aa);
      xprev = x;
      yprev = y;
    }
  }
  LICE_Line(dest, xprev, yprev, xend, yend, color, alpha, mode, aa);
}

void LICE_FillTriangle(LICE_IBitmap *dest, int x1, int y1, int x2, int y2, int x3, int y3, LICE_pixel color, float alpha, int mode)
{
  if (!dest) return;
  char i0 = 0, i1 = 1, i2 = 2; 
  int coords[3][2]={{x1,y1},{x2,y2},{x3,y3}};
  if (coords[0][1] > coords[1][1]) {  i0 = 1; i1 = 0;  } 
  if (coords[i0][1] > coords[2][1]) { char c=i0; i0=i2; i2=c; }
  if (coords[i1][1] > coords[i2][1]) { char c=i1; i1=i2; i2=c; }

  int ypos=coords[i0][1];
  if (ypos > coords[i2][1]) return; // ack no pixels
  double edge1=coords[i0][0], edge2=edge1, de1,de2;

  int tmp=coords[i2][1]-ypos;
  if (tmp) de2 = (coords[i2][0]-edge2)/tmp;
  tmp=coords[i1][1]-ypos;
  if (tmp) de1=(coords[i1][0]-edge1)/tmp;

  bool swap=coords[i1][0] > coords[i2][0]; // if edge1 (ends sooner) is right of edge2, swap

  int clipheight=dest->getHeight();
  int clipwidth = dest->getWidth();

  while (ypos < coords[i2][1])
  {
    if (ypos == coords[i1][1])
    {
      tmp = coords[i2][1]-ypos;
      if (tmp) de1 = (coords[i2][0]-coords[i1][0])/tmp;
      edge1=coords[i1][0];
    }

    if (ypos>=0)
    {
      if (ypos >= clipheight) break;
      double a,b;
      if (swap) { a=edge2; b=edge1; } else { a=edge1; b=edge2; }
      int aa=(int) floor(a+0.5);
      int bb=(int) floor(b+0.5);
      if (aa<0)aa=0;
      if (bb>clipwidth) bb=clipwidth;
      if (aa<clipwidth && bb>=0)
      {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_HorizLine(dest, ypos, aa, bb, color, alpha)
      __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
#undef __LICE__ACTION
//        LICE_Line(dest,aa,ypos,bb,ypos,color,alpha,mode,false); // lice_line optimizes horz runs anyway
      }
    }

    edge1+=de1;
    edge2+=de2;
    ypos++;
  }
}


void LICE_DrawRect(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel color, float alpha, int mode)
{
  LICE_Line(dest, x, y, x+w, y, color, alpha, mode, false);
  LICE_Line(dest, x+w, y, x+w, y+h, color, alpha, mode, false);
  LICE_Line(dest, x+w, y+h, x, y+h, color, alpha, mode, false);
  LICE_Line(dest, x, y+h, x, y, color, alpha, mode, false);
}

void LICE_BorderedRect(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel bgcolor, LICE_pixel fgcolor, float alpha, int mode)
{
  // could be optimized
  LICE_FillRect(dest, x+1, y+1, w-1, h-1, bgcolor, alpha, mode);
  LICE_DrawRect(dest, x, y, w, h, fgcolor, alpha, mode);
}
