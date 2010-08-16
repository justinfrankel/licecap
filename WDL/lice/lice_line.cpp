#include "lice.h"
#include "lice_combine.h"
#include "lice_extended.h"
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

inline static void LICE_VertLineFAST(LICE_IBitmap* dest, int x, int y1, int y2, LICE_pixel color)
{
  if (y1 > y2) SWAP(y1, y2);
  int span = dest->getRowSpan();
  LICE_pixel* px = dest->getBits()+y1*span+x;

  int n = y2-y1+1;
  while (n--)
  {
    *px = color;
    px += span;
  }
}

inline static void LICE_HorizLineFAST(LICE_IBitmap* dest, int y, int x1, int x2, LICE_pixel color)
{
  if (x1 > x2) SWAP(x1, x2);
  int span = dest->getRowSpan();
  LICE_pixel* px = dest->getBits()+y*span+x1;

  int n = x2-x1+1;
  while (n--)
  {
    *px = color;
    ++px;
  }
}

inline static void LICE_DiagLineFAST(LICE_IBitmap* dest, int x1, int y1, int x2, int y2, LICE_pixel color, bool aa)
{
  int span = dest->getRowSpan();
  int n = abs(x2-x1);
  LICE_pixel* px = dest->getBits()+y1*span+x1;
  int xstep = (x2 > x1 ? 1 : -1);
  int ystep = (y2 > y1 ? span : -span);
  int step = xstep+ystep;

  if (aa)
  {
    LICE_pixel color75 = ((color>>1)&0x7f7f7f7f)+((color>>2)&0x3f3f3f3f);
    LICE_pixel color25 = (color>>2)&0x3f3f3f3f;
    while (n--)
    {
      _LICE_CombinePixelsThreeQuarterMix2::doPixFAST(px, color75);    
      _LICE_CombinePixelsQuarterMix2::doPixFAST(px+xstep, color25);
      _LICE_CombinePixelsQuarterMix2::doPixFAST(px+ystep, color25);
      px += step;
    }
    _LICE_CombinePixelsThreeQuarterMix2::doPixFAST(px, color75);  
  }
  else
  {
    ++n;
    while (n--)
    {
      *px = color;
      px += step;
    }
  }
}

inline static void LICE_DottedVertLineFAST(LICE_IBitmap* dest, int x, int y1, int y2, LICE_pixel color)
{
  if (y1 > y2) SWAP(y1, y2);
  int span = dest->getRowSpan();
  LICE_pixel* px = dest->getBits()+y1*span+x;

  int h = dest->getHeight();
  int n = (y2-y1+1)/2;
  while (n--)
  {
    *px = color;
    px += 2*span;
  }
}

template <class COMBFUNC> class __LICE_LineClass
{
public:

  static void LICE_VertLine(LICE_IBitmap* dest, int x, int y1, int y2, LICE_pixel color, float alpha)
  {
    if (y1 > y2) SWAP(y1, y2);
    int span = dest->getRowSpan();
    LICE_pixel* px = dest->getBits()+y1*span+x;
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);
    int aw = (int)(256.0f*alpha);

    int y;
    for (y = y1; y <= y2; ++y, px += span) {
      COMBFUNC::doPix((LICE_pixel_chan*) px, r, g, b, a, aw);
    }
  }

  static void LICE_HorizLine(LICE_IBitmap* dest, int y, int x1, int x2, LICE_pixel color, float alpha)
  {
    if (x1 > x2) SWAP(x1, x2);
    int span = dest->getRowSpan();
    LICE_pixel* px = dest->getBits()+y*span+x1;
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);
    int aw = (int)(256.0f*alpha);

    int x;
    for (x = x1; x <= x2; ++x, ++px) {
      COMBFUNC::doPix((LICE_pixel_chan*) px, r, g, b, a, aw);
    }
  }

  static void LICE_DashedLine(LICE_IBitmap* dest, int x1, int y1, int x2, int y2, int pxon, int pxoff, LICE_pixel color, float alpha)
  {
    if (x1 > x2) SWAP(x1, x2);
    if (y1 > y2) SWAP(y1, y2);
    int span = dest->getRowSpan();
    LICE_pixel* px = dest->getBits()+y1*span+x1;
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);
    int aw = (int)(256.0f*alpha);

    if (x1 == x2)
    {
      int i, y;
      for (y = y1; y < y2-pxon; y += pxon+pxoff)
      {
        for (i = 0; i < pxon; ++i, px += span) COMBFUNC::doPix((LICE_pixel_chan*) px, r, g, b, a, aw);
        px += pxoff*span;
      }
      for (i = 0; i < min(pxon, y2-y); px += span) COMBFUNC::doPix((LICE_pixel_chan*) px, r, g, b, a, aw);
    }
    else if (y1 == y2)
    {
      int i, x;
      for (x = x1; x < x2-pxon; x += pxon+pxoff)
      {
        for (i = 0; i < pxon; ++i, ++px) COMBFUNC::doPix((LICE_pixel_chan*) px, r, g, b, a, aw);
        px += pxoff;
      }
      for (i = 0; i < min(pxon, x2-x); ++px) COMBFUNC::doPix((LICE_pixel_chan*) px, r, g, b, a, aw);
    }
  }

  static void LICE_DiagLine(LICE_IBitmap* dest, int x1, int y1, int x2, int y2, LICE_pixel color, float alpha, bool aa)
  {
    int span = dest->getRowSpan();
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);
    int n = abs(x2-x1);
    LICE_pixel* px = dest->getBits()+y1*span+x1;
    int xstep = (x2 > x1 ? 1 : -1);
    int ystep = (y2 > y1 ? span : -span);
    int step = xstep+ystep;
    int aw = (int)(256.0f*alpha);

    if (aa) 
    {
      int iw = aw*3/4;
      int dw = aw/4;
      for (int i = 0; i < n; ++i, px += step)
      {
        COMBFUNC::doPix((LICE_pixel_chan*) px, r, g, b, a, iw);       
        COMBFUNC::doPix((LICE_pixel_chan*) (px+xstep), r, g, b, a, dw); 
        COMBFUNC::doPix((LICE_pixel_chan*) (px+ystep), r, g, b, a, dw);
      }
      COMBFUNC::doPix((LICE_pixel_chan*) px, r, g, b, a, iw);
    }
    else
    {
      for (int i = 0; i <= n; ++i, px += step) 
      {
        COMBFUNC::doPix((LICE_pixel_chan*) px, r, g, b, a, aw);
      }
    }
  }

  static void LICE_LineImpl(LICE_IBitmap* pDest, int xi1, int yi1, int xi2, int yi2, LICE_pixel color, float alpha, bool aa)
  {
    int w = pDest->getRowSpan();
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);

    int nx = abs(xi2-xi1);
    int ny = abs(yi2-yi1);
    int i, n, pxStep, pxStep2;
    double err, dErr;
    if (ny > nx) 
    {  
      n = ny;
      pxStep = (yi1 < yi2 ? w : -w);
      pxStep2 = (xi1 < xi2 ? 1 : -1);
      dErr = (double)nx/(double)ny;
    }
    else 
    {
      n = nx;
      pxStep = (xi1 < xi2 ? 1 : -1);
      pxStep2 = (yi1 < yi2 ? w : -w);
      dErr = (double)ny/(double)nx;
    }
    LICE_pixel* pPx = pDest->getBits()+yi1*w+xi1;   
    int aw = (int)(256.0f*alpha);

    if (aa) 
    {
      COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw);
      err = dErr;
      pPx += pxStep;
      for (i = 1; i < n; ++i, pPx += pxStep)
      {
        int iErr = __LICE_TOINT(256.0*err);
        COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw*(256-iErr)/256);
        COMBFUNC::doPix((LICE_pixel_chan*) (pPx+pxStep2), r, g, b, a, aw*iErr/256);
        err += dErr;
        if (err > 1.0)
        {
          pPx += pxStep2;
          err -= 1.0;
        }
      }
      int iErr = __LICE_TOINT(256.0*err);
      COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw*(256-iErr)/256);
    }
    else 
    {
      err = 0.0;
      for (i = 0; i <= n; ++i, pPx += pxStep) 
      {
        COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw);
        err += dErr;
        if (err > 0.5) 
        {
          pPx += pxStep2;
          err -= 1.0;
        }
      }
    }
  }

};

void LICE_Line(LICE_IBitmap *pDest, int x1, int y1, int x2, int y2, LICE_pixel color, float alpha, int mode, bool aa)
{
  if (!pDest) return;

#ifndef DISABLE_LICE_EXTENSIONS
  if (pDest->Extended(LICE_EXT_SUPPORTS_ID, (void*) LICE_EXT_LINE_ACCEL))
  {
    LICE_Ext_Line_acceldata data(x1, y1, x2, y2, color, alpha, mode, aa);
    if (pDest->Extended(LICE_EXT_LINE_ACCEL, &data)) return;
  }
#endif

	int w = pDest->getWidth();
  int h = pDest->getHeight();
  if (pDest->isFlipped()) 
  {
    y1 = h-1-y1;
    y2 = h-1-y2;
  }

	if (ClipLine(&x1, &y1, &x2, &y2, w, h)) {

    if (x1 == x2) 
    {
      if ((mode&LICE_BLIT_MODE_MASK) == LICE_BLIT_MODE_COPY && alpha == 1.0f)
      {
        LICE_VertLineFAST(pDest, x1, y1, y2, color);        
      }
      else
      {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_VertLine(pDest, x1, y1, y2, color, alpha)
        __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
#undef __LICE__ACTION
      }
    }
    else if (y1 == y2)
    {
      if ((mode&LICE_BLIT_MODE_MASK) == LICE_BLIT_MODE_COPY && alpha == 1.0f)
      {
        LICE_HorizLineFAST(pDest, y1, x1, x2, color);        
      }
      else
      {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_HorizLine(pDest, y1, x1, x2, color, alpha)
        __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
#undef __LICE__ACTION
      }
    }
    else if (abs(x2-x1) == abs(y2-y1)) 
    {
      if ((mode&LICE_BLIT_MODE_MASK) == LICE_BLIT_MODE_COPY && alpha == 1.0f)
      {
        LICE_DiagLineFAST(pDest, x1, y1, x2, y2, color, aa);        
      }
      else
      {
        if (aa) 
        {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_DiagLine(pDest, x1, y1, x2, y2, color, alpha, true)
        __LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
#undef __LICE__ACTION
        }
        else 
        {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_DiagLine(pDest, x1, y1, x2, y2, color, alpha, false)
        __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
#undef __LICE__ACTION
        }
      }
    }
    else 
    {
      if (aa) 
      {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_LineImpl(pDest, x1, y1, x2, y2, color, alpha, true)
			__LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
#undef __LICE__ACTION
      }
      else 
      {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_LineImpl(pDest, x1, y1, x2, y2, color, alpha, false)
			__LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
#undef __LICE__ACTION

      }
		}
	}
}

void LICE_DashedLine(LICE_IBitmap* dest, int x1, int y1, int x2, int y2, int pxon, int pxoff, LICE_pixel color, float alpha, int mode, bool aa) 
{  
  if (!dest) return;

#ifndef DISABLE_LICE_EXTENSIONS
  if (dest->Extended(LICE_EXT_SUPPORTS_ID, (void*) LICE_EXT_DASHEDLINE_ACCEL))
  {
    LICE_Ext_DashedLine_acceldata data(x1, y1, x2, y2, pxon, pxoff, color, alpha, mode, aa);
    if (dest->Extended(LICE_EXT_DASHEDLINE_ACCEL, &data)) return;
  }
#endif

	int w = dest->getWidth();
  int h = dest->getHeight();
	if (ClipLine(&x1, &y1, &x2, &y2, w, h)) 
  {
    if (pxon == 1 && pxoff == 1 && x1 == x2 && (mode&LICE_BLIT_MODE_MASK) == LICE_BLIT_MODE_COPY && alpha == 1.0f)
    {
      LICE_DottedVertLineFAST(dest, x1, y1, y2, color);        
    }
    else
    {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_DashedLine(dest, x1, y1, x2, y2, pxon, pxoff, color, alpha);
      __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);   
#undef __LICE__ACTION
    }
  }
}

bool LICE_ClipLine(int* pX1, int* pY1, int* pX2, int* pY2, int xLo, int yLo, int xHi, int yHi)
{
    int x1 = *pX1-xLo;
    int y1 = *pY1-yLo;
    int x2 = *pX2-xLo;
    int y2 = *pY2-yLo;
    bool onscreen = ClipLine(&x1, &y1, &x2, &y2, xHi-xLo, yHi-yLo);
    *pX1 = x1+xLo;
    *pY1 = y1+yLo;
    *pX2 = x2+xLo;
    *pY2 = y2+yLo;
    return onscreen;
}

#include "lice_bezier.h"

template <class COMBFUNC> class _LICE_CurveDrawer
{
public:

  static void DrawClippedPt(LICE_IBitmap* dest, int x, int y, int clip[], 
    int r, int g, int b, int a, int alpha, bool doclip)
  {
    if (doclip && (x < clip[0] || y < clip[1] || x >= clip[2] || y >= clip[3])) return;
    LICE_pixel* px = dest->getBits()+y*dest->getRowSpan()+x;
    COMBFUNC::doPix((LICE_pixel_chan*)px, r, g, b, a, alpha);
  }

// MAX_BEZ_RECURSE of N probably means less than 2^N points, 
// because sharp curves will resolve much faster than flat areas
#define MAX_BEZ_RECURSE 12
// lower BEZ_TOL = more px = more calculations = thicker line
#define BEZ_TOL 1.5f

  static void DrawCBezRecurseAA(int recurseidx, LICE_IBitmap* dest, 
    double ax, double bx, double cx, double dx, double ay, double by, double cy, double dy, 
    int r, int g, int b, int a, int ai, float alpha, 
    double tlo, float xlo, float ylo, double thi, float xhi, float yhi, int clip[], bool doclip)
  {
    if (recurseidx >= MAX_BEZ_RECURSE) {  // the line is ugly, this case is worth avoiding
      int xilo = xlo, xihi = xhi, yilo = ylo, yihi = yhi;
      if (!doclip || ClipLine(&xilo, &yilo, &xihi, &yihi, dest->getWidth(), dest->getHeight())) {        
        __LICE_LineClass<COMBFUNC>::LICE_LineImpl(dest, xilo, yilo, xihi, yihi, LICE_RGBA(r, g, b, a), alpha, true);
      }
      return;
    }

    double tmid = 0.5*(tlo+thi);
    float xmid, ymid;
    EVAL_CBEZXY(xmid, ymid, ax, bx, cx, dx, ay, by, cy, dy, tmid);

    int xi = xmid, yi = ymid;
    float dxlo = fabs(xmid-xlo), dylo = fabs(ymid-ylo);
    float dxhi = fabs(xhi-xmid), dyhi = fabs(yhi-ymid);

    float wx = xmid-floor(xmid), wy = ymid-floor(ymid);
    float iwx = 1.0f-wx, iwy = 1.0f-wy;
    DrawClippedPt(dest, xi, yi, clip, r, g, b, a, (int)(alpha*(iwx*iwy)*255.0f), doclip);
    DrawClippedPt(dest, xi+1, yi, clip, r, g, b, a, (int)(alpha*(wx*iwy)*255.0f), doclip);
    DrawClippedPt(dest, xi, yi+1, clip, r, g, b, a, (int)(alpha*(iwx*wy)*255.0f), doclip);
    DrawClippedPt(dest, xi+1, yi+1, clip, r, g, b, a, (int)(alpha*(wx*wy)*255.0f), doclip);

    if (dxlo > BEZ_TOL || dylo > BEZ_TOL) {
      DrawCBezRecurseAA(recurseidx+1, dest, ax, bx, cx, dx, ay, by, cy, dy,
        r, g, b, a, ai, alpha, tlo, xlo, ylo, tmid, xmid, ymid, clip, doclip);
    }
  
    if (dxhi > BEZ_TOL || dyhi > BEZ_TOL) {
      DrawCBezRecurseAA(recurseidx+1, dest, ax, bx, cx, dx, ay, by, cy, dy,
        r, g, b, a, ai, alpha, tmid, xmid, ymid, thi, xhi, yhi, clip, doclip);
    }
  }

};

// quadratic bezier
void LICE_DrawBezier(LICE_IBitmap* dest, float xstart, float ystart, float xctl, float yctl, float xend, float yend, 
  LICE_pixel color, float alpha, int mode, bool aa)
{
  // todo like cbez

}

// cubic bezier
void LICE_DrawCBezier(LICE_IBitmap* dest, float xstart, float ystart, float xctl1, float yctl1,
  float xctl2, float yctl2, float xend, float yend, LICE_pixel color, float alpha, int mode, bool aa)
{ 
  if (!dest) return;

#ifndef DISABLE_LICE_EXTENSIONS
  if (dest->Extended(LICE_EXT_SUPPORTS_ID, (void*) LICE_EXT_DRAWCBEZIER_ACCEL))
  {
    LICE_Ext_DrawCBezier_acceldata data(xstart, ystart, xctl1, yctl1, xctl2, yctl2, xend, yend, color, alpha, mode, aa);
    if (dest->Extended(LICE_EXT_DRAWCBEZIER_ACCEL, &data)) return;
  }
#endif

  int w = dest->getWidth(), h = dest->getHeight();
    
  if (xstart > xend) {
    SWAP(xstart, xend);
    SWAP(xctl1, xctl2);
    SWAP(ystart, yend);
    SWAP(yctl1, yctl2);
  }
    
  double ax, bx, cx, ay, by, cy;
  LICE_CBezier_GetCoeffs(xstart, xctl1, xctl2, xend, ystart, yctl1, yctl2, yend, &ax, &bx, &cx, &ay, &by, &cy);

  float xlo = xstart, xhi = xend;
  float ylo = ystart, yhi = yend;
  double tlo = 0.0, thi = 1.0;
  if (xlo < 0) {
    xlo = 0;
    ylo = LICE_CBezier_GetY(xstart, xctl1, xctl2, xend, ystart, yctl1, yctl2, yend, xlo, (float*)0, (float*)0, (double*)0, &tlo);
  }
  if (xhi >= w) {
    xhi = w-1;
    yhi = LICE_CBezier_GetY(xstart, xctl1, xctl2, xend, ystart, yctl1, yctl2, yend, xhi, (float*)0, (float*)0, &thi, (double*)0);
  }
  if (xlo >= xhi) return;

  if (aa) {

    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);
    int ai = (int)(alpha*255.0f);

    int clip[4] = { 0, 0, w, h };

    // safety
    int xmin = 1;
    int xmax = w-1;
    int ymin = 32;
    int ymax = h-32;

    bool doclip = (xlo < xmin || xctl1 < xmin || xctl2 < xmin || xhi < xmin ||
        ylo < ymin || yctl1 < ymin || yctl2 < ymin || yhi < ymin ||
        xlo >= xmax || xctl1 >= xmax || xctl2 >= xmax || xhi >= xmax ||
        ylo >= ymax || yctl1 >= ymax || yctl2 >= ymax || yhi >= ymax);

#define __LICE__ACTION(COMBFUNC) _LICE_CurveDrawer<COMBFUNC>::DrawCBezRecurseAA(0, dest, ax, bx, cx, xstart, ay, by, cy, ystart, r, g, b, a, ai, alpha, tlo, xlo, ylo, thi, xhi, yhi, clip, doclip);
    __LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
#undef __LICE__ACTION

  }
  else {

    // quick, dirty, and ugly
    int i, n = abs((int)(xhi-xlo))+abs((int)(yhi-ylo));
    float xprev = xlo, yprev = ylo, x, y;
    for (i = 0; i < n; ++i) {
      double t = tlo+(double)i/(double)n*(thi-tlo);
      EVAL_CBEZXY(x, y, ax, bx, cx, xstart, ay, by, cy, ystart, t);
      float dx = x-xprev, dy = y-yprev, dz = dx*dx+dy*dy;
      if (dz >= 16.0f) {
        LICE_Line(dest, xprev, yprev, x, y, color, alpha, mode, false);
        xprev = x;
        yprev = y;
      }
    }
    LICE_Line(dest, xprev, yprev, xend, yend, color, alpha, mode, false);
  }
}

void LICE_FillTriangle(LICE_IBitmap *dest, int x1, int y1, int x2, int y2, int x3, int y3, LICE_pixel color, float alpha, int mode)
{
  if (!dest) return;
  char i0 = 0, i1 = 1, i2 = 2; 
  if (dest->isFlipped())
  {
    int h=dest->getHeight()-1;
    y1 = h-y1;
    y2 = h-y2;
    y3 = h-y3;
  }
  
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
  LICE_FillRect(dest, x+1, y+1, w-1, h-1, bgcolor, alpha, mode);
  LICE_DrawRect(dest, x, y, w, h, fgcolor, alpha, mode);
}
