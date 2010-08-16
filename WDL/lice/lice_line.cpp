#include "lice.h"
#include "lice_combine.h"
#include "lice_extended.h"
#include <math.h>
#include <stdio.h>

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

static int OffscreenFTest(float x, float y, float w, float h)
{
  int e = eOK;
  if (x < 0.0f) e |= eXLo; 
  else if (x >= w) e |= eXHi; 
  if (y < 0.0f) e |= eYLo; 
  else if (y >= h) e |= eYHi; 
  return e;
}

static bool ClipFLine(float* x1, float* y1, float* x2, float*y2, int w, int h)
{
  float tx1 = *x1, ty1 = *y1, tx2 = *x2, ty2 = *y2;
  float tw = (float) w, th = (float) h;
  
  int e1 = OffscreenTest(tx1, ty1, tw, th); 
  int e2 = OffscreenTest(tx2, ty2, tw, th);
  
  bool accept = false, done = false;
  do
  {
    if (!(e1|e2)) 
    {
      accept = done = true;
    }
    else
    if (e1&e2) 
    {
      done = true;	// Line is entirely offscreen.
    }
    else 
    { 
      float x, y;
      int eOut = (e1 ? e1 : e2);
      if (eOut&eYHi) 
      {
        x = tx1+(tx2-tx1)*(th-ty1)/(ty2-ty1);
        y = th-1.0f;
      }
      else if (eOut&eYLo) 
      {
        x = tx1+(tx2-tx1)*ty1/(ty1-ty2);
        y = 0.0f;
      }
      else if (eOut&eXHi) 
      {
        y = ty1+(ty2-ty1)*(tw-tx1)/(tx2-tx1);
        x = tw-1.0f;
      }
      else
      {
        y = ty1+(ty2-ty1)*tx1/(tx1-tx2);
        x = 0.0f;
      }

      if (eOut == e1) 
      { 
        tx1 = x; 
        ty1 = y;
        e1 = OffscreenTest(tx1, ty1, tw, th);
      }
      else 
      {
        tx2 = x;
        ty2 = y;
        e2 = OffscreenTest(tx2, ty2, tw, th);
      }
    }
  }
  while (!done);

  *x1 = tx1;
  *y1 = ty1;
  *x2 = tx2;
  *y2 = ty2;
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
    int span = pDest->getRowSpan();
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);

    int nx = abs(xi2-xi1);
    int ny = abs(yi2-yi1);
    int i, n, pxStep, pxStep2;
    double err, dErr;
    if (ny > nx) 
    {  
      n = ny;
      pxStep = (yi1 < yi2 ? span : -span);
      pxStep2 = (xi1 < xi2 ? 1 : -1);
      dErr = (double)nx/(double)ny;
    }
    else 
    {
      n = nx;
      pxStep = (xi1 < xi2 ? 1 : -1);
      pxStep2 = (yi1 < yi2 ? span : -span);
      dErr = (double)ny/(double)nx;
    }
    LICE_pixel* pPx = pDest->getBits()+yi1*span+xi1;   
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
        if (iErr) COMBFUNC::doPix((LICE_pixel_chan*) (pPx+pxStep2), r, g, b, a, aw*iErr/256);
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

  static void LICE_FLineImpl(LICE_IBitmap* pDest, float x1, float y1, float x2, float y2, LICE_pixel color, float alpha, bool aa)
  {    
    int span = pDest->getRowSpan();
    int r = LICE_GETR(color), g = LICE_GETG(color), b = LICE_GETB(color), a = LICE_GETA(color);

    float nx = fabs(x2-x1);
    float ny = fabs(y2-y1);
    int xstep = (x1 < x2 ? 1 : -1);
    int ystep = (y1 < y2 ? span : -span);
    bool steep = (ny > nx);

    int i, n, pxStep, pxStep2;
    double err, dErr;

    if (steep) 
    {  
      n = (int)ny;
      pxStep = ystep;
      pxStep2 = xstep;
      err = x1-floor(x1);    
      if (xstep < 0 && err > 0.0f) err = 1.0f-err;
      dErr = nx/ny;
    }
    else 
    {
      n = (int)nx;
      pxStep = xstep;
      pxStep2 = ystep;
      err = y1-floor(y1);
      if (ystep < 0 && err > 0.0f) err = 1.0f-err;
      dErr = ny/nx;
    }

    LICE_pixel* pPx = pDest->getBits()+(int)y1*span+(int)x1;   
    int aw = (int)(256.0f*alpha);

    if (aa) 
    {
      for (i = 0; i <= n; ++i, pPx += pxStep)   // <= scary
      {
        int iErr = __LICE_TOINT(256.0*err);
        COMBFUNC::doPix((LICE_pixel_chan*) pPx, r, g, b, a, aw*(256-iErr)/256);
        if (iErr) COMBFUNC::doPix((LICE_pixel_chan*) (pPx+pxStep2), r, g, b, a, aw*iErr/256);
        err += dErr;
        if (err > 1.0)
        {
          pPx += pxStep2;
          err -= 1.0;
        }
      }
    }
    else 
    {
      err = 0.0;
      for (i = 0; i <= n; ++i, pPx += pxStep)  // <= scary
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
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_DiagLine(pDest, x1, y1, x2, y2, color, alpha, aa)
        if (aa) 
        {
          __LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
        }
        else 
        {
          __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
        }
#undef __LICE__ACTION
      }
    }
    else 
    {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_LineImpl(pDest, x1, y1, x2, y2, color, alpha, aa)	
      if (aa) 
      {
        __LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
      }
      else 
      {
        __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
      }
#undef __LICE__ACTION
		}
	}
}

void LICE_FLine(LICE_IBitmap* dest, float x1, float y1, float x2, float y2, LICE_pixel color, float alpha, int mode, bool aa)
{
  if (!dest) return;

  int w = dest->getWidth();
  int h = dest->getHeight();
  if (dest->isFlipped()) 
  {
    y1 = (float)(h-1)-y1;
    y2 = (float)(h-1)-y2;
  }

  if (ClipFLine(&x1, &y1, &x2, &y2, w, h))
  {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_FLineImpl(dest, x1, y1, x2, y2, color, alpha, aa)
    if (aa) 
    {
      __LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);    
    }
    else 
    {
      __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
    }
#undef __LICE__ACTION
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

// MAX_BEZ_RECURSE of N probably means less than 2^N points, 
// because sharp curves will resolve much faster than flat areas
#define MAX_BEZ_RECURSE 12
// BEZ_PT_TOL2 = square of minimum distance between pixels (lower = smoother line = more calculations)
#define BEZ_PT_TOL2 (1.0f*1.0f)

  static void DrawClippedPt(LICE_IBitmap* dest, int x, int y, int w, int h, 
    LICE_pixel color, float alpha)
  {
    if (x >= 0 && y >= 0 && x < w && y < h)
    {
      LICE_pixel* px = dest->getBits()+y*dest->getRowSpan()+x;
      COMBFUNC::doPix((LICE_pixel_chan*)px, LICE_GETR(color), LICE_GETG(color), LICE_GETB(color), LICE_GETA(color), (int)(alpha*255.0f));
    }
  }
  
  static void DrawClippedFPt(LICE_IBitmap* dest, float x, float y, int w, int h, 
    LICE_pixel color, float alpha)
  {
    int xi = (int)x;
    int yi = (int)y;
    float wx = x-floor(x);
    float wy = y-floor(y);
    float iwx = 1.0f-wx;
    float iwy = 1.0f-wy;
    DrawClippedPt(dest, xi, yi, w, h, color, alpha*iwx*iwy);
    DrawClippedPt(dest, xi+1, yi, w, h, color, alpha*wx*iwy);
    DrawClippedPt(dest, xi, yi+1, w, h, color, alpha*iwx*wy);
    DrawClippedPt(dest, xi+1, yi+1, w, h, color, alpha*wx*wy);
  }

  static void DrawClippedLine(LICE_IBitmap* dest, float x1, float y1, float x2, float y2, int w, int h,
    LICE_pixel color, float alpha, bool aa)
  {
    if (ClipFLine(&x1, &y1, &x2, &y2, w, h))
    {
      __LICE_LineClass<COMBFUNC>::LICE_FLineImpl(dest, x1, y1, x2, y2, color, alpha, aa);
    }
  }

  static void DrawCBezRecurse(int recurseidx, LICE_IBitmap* dest, 
    double ax, double bx, double cx, double dx, double ay, double by, double cy, double dy, 
    LICE_pixel color, float alpha, bool aa, 
    double tlo, float xlo, float ylo, double thi, float xhi, float yhi, int w, int h, float maxsegmentpx2)
  {
    /*
    static int _maxrecurse = 0;
    if (recurseidx > _maxrecurse)
    {
      _maxrecurse = recurseidx;
      char buf[64];
      sprintf(buf, "%d", _maxrecurse);
      OutputDebugString(buf);
    }
    */

    if (!recurseidx)
    {
      if (maxsegmentpx2 <= 0.0f)
      {
        if (aa)
        {
          DrawClippedFPt(dest, xlo, ylo, w, h, color, alpha);
          DrawClippedFPt(dest, xhi, yhi, w, h, color, alpha);
        }
        else
        {
          DrawClippedPt(dest, (int)xlo, (int)ylo, w, h, color, alpha);
          DrawClippedPt(dest, (int)xhi, (int)yhi, w, h, color, alpha);
        }
      }
    }
    else if (recurseidx >= MAX_BEZ_RECURSE)  
    { 
      DrawClippedLine(dest, xlo, ylo, xhi, yhi, w, h, color, alpha, aa);
      return;
    }

    double tmid = 0.5*(tlo+thi);
    float xmid, ymid;
    EVAL_CBEZXY(xmid, ymid, ax, bx, cx, dx, ay, by, cy, dy, tmid);

    float dxlo = xmid-xlo;
    float dylo = ymid-ylo;
    float dxhi = xhi-xmid;
    float dyhi = yhi-ymid;
    float d2lo = dxlo*dxlo+dylo*dylo;
    float d2hi = dxhi*dxhi+dyhi*dyhi;

    bool recurselo, recursehi;

    if (maxsegmentpx2 <= 0.0f)
    {
      if (aa) DrawClippedFPt(dest, xmid, ymid, w, h, color, alpha);
      else DrawClippedPt(dest, (int)xmid, (int)ymid, w, h, color, alpha);       
      recurselo = (d2lo >= BEZ_PT_TOL2);
      recursehi = (d2hi >= BEZ_PT_TOL2);
    }
    else
    {
      recurselo = (d2lo >= maxsegmentpx2);
      if (!recurselo) DrawClippedLine(dest, xlo, ylo, xmid, ymid, w, h, color, alpha, aa);
      recursehi = (d2hi >= maxsegmentpx2);
      if (!recursehi) DrawClippedLine(dest, xmid, ymid, xhi, yhi, w, h, color, alpha, aa);
    }

    if (recurselo)
    {
      DrawCBezRecurse(recurseidx+1, dest, ax, bx, cx, dx, ay, by, cy, dy,
        color, alpha, aa, tlo, xlo, ylo, tmid, xmid, ymid, w, h, maxsegmentpx2);
    }
    if (recursehi) 
    {
      DrawCBezRecurse(recurseidx+1, dest, ax, bx, cx, dx, ay, by, cy, dy,
        color, alpha, aa, tmid, xmid, ymid, thi, xhi, yhi, w, h, maxsegmentpx2);
    }
  }

  static void DrawQBezRecurse(int recurseidx, LICE_IBitmap* dest,
    float xstart, float xctl, float xend, float ystart, float yctl, float yend, 
    LICE_pixel color, float alpha, bool aa,
    double tlo, float xlo, float ylo, double thi, float xhi, float yhi, int w, int h, float maxsegmentpx2)
  {
    if (!recurseidx)
    {
      if (aa)
      {
        DrawClippedFPt(dest, xlo, ylo, w, h, color, alpha);
        DrawClippedFPt(dest, xhi, yhi, w, h, color, alpha);
      }
      else
      {
        DrawClippedPt(dest, (int)xlo, (int)ylo, w, h, color, alpha);
        DrawClippedPt(dest, (int)xhi, (int)yhi, w, h, color, alpha);
      }
    }
    else if (recurseidx >= MAX_BEZ_RECURSE)  
    { 
      DrawClippedLine(dest, xlo, ylo, xhi, yhi, w, h, color, alpha, aa);
      return;
    }

    double tmid = 0.5*(tlo+thi);
    float xmid, ymid;
    LICE_Bezier(xstart, xctl, xend, ystart, yctl, yend, tmid, &xmid, &ymid);

    float dxlo = xmid-xlo;
    float dylo = ymid-ylo;
    float dxhi = xhi-xmid;
    float dyhi = yhi-ymid;
    float d2lo = dxlo*dxlo+dylo*dylo;
    float d2hi = dxhi*dxhi+dyhi*dyhi;

    bool recurselo, recursehi;

    if (maxsegmentpx2 <= 0.0f)
    {
      if (aa) DrawClippedFPt(dest, xmid, ymid, w, h, color, alpha);
      else DrawClippedFPt(dest, (int)xmid, (int)ymid, w, h, color, alpha);
      recurselo = (d2lo >= BEZ_PT_TOL2);
      recursehi = (d2hi >= BEZ_PT_TOL2);
    }
    else
    {
      recurselo = (d2lo >= maxsegmentpx2);
      if (!recurselo) DrawClippedLine(dest, xlo, ylo, xmid, ymid, w, h, color, alpha, aa);
      recursehi = (d2hi >= maxsegmentpx2);
      if (!recursehi) DrawClippedLine(dest, xmid, ymid, xhi, yhi, w, h, color, alpha, aa);
    }

    if (recurselo) 
    {
      DrawQBezRecurse(recurseidx+1, dest, xstart, xctl, xend, ystart, yctl, yend, color, alpha, aa, 
        tlo, xlo, ylo, tmid, xmid, ymid, w, h, maxsegmentpx2);
    }
    if (recursehi)
    {
      DrawQBezRecurse(recurseidx+1, dest, xstart, xctl, xend, ystart, yctl, yend, color, alpha, aa, 
        tmid, xmid, ymid, thi, xhi, yhi, w, h, maxsegmentpx2);
    }
  }

};

// quadratic bezier
// maxsegmentpx<=0 means recurse (best), maxsegmentpx>0 means draw the curve piecewise in segments with length < maxsegmentpx pixels
void LICE_DrawQBezier(LICE_IBitmap* dest, float xstart, float ystart, float xctl, float yctl, float xend, float yend, 
  LICE_pixel color, float alpha, int mode, bool aa, float maxsegmentpx)
{
  if (!dest) return;

  int w = dest->getWidth();
  int h = dest->getHeight();
    
  if (xstart > xend) 
  {
    SWAP(xstart, xend);
    SWAP(ystart, yend);
  }
      
  float xlo = xstart, xhi = xend;
  float ylo = ystart, yhi = yend;
  double tlo = 0.0, thi = 1.0;

  if (xlo < 0) 
  {
    xlo = 0;
    ylo = LICE_Bezier_GetY(xstart, xctl, xend, ystart, yctl, yend, xlo, &tlo);
  }
  if (xhi >= w)
  {
    xhi = w-1;
    yhi = LICE_Bezier_GetY(xstart, xctl, xend, ystart, yctl, yend, xhi, &thi);
  }
  if (xlo > xhi) return;

  float maxsegmentpx2 = (maxsegmentpx <= 1.0f ? 0.0f : maxsegmentpx*maxsegmentpx);

#define __LICE__ACTION(COMBFUNC) _LICE_CurveDrawer<COMBFUNC>::DrawQBezRecurse(0, dest, xstart, xctl, xend, ystart, yctl, yend, color, alpha, aa, tlo, xlo, ylo, thi, xhi, yhi, w, h, maxsegmentpx2);
  if (aa)
  {
    __LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
  }
  else
  {
    __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
  }
#undef __LICE__ACTION
}

// cubic bezier
// maxsegmentpx<=0 means recurse (best), maxsegmentpx>0 means draw the curve piecewise in segments with length < maxsegmentpx pixels
void LICE_DrawCBezier(LICE_IBitmap* dest, float xstart, float ystart, float xctl1, float yctl1,
  float xctl2, float yctl2, float xend, float yend, LICE_pixel color, float alpha, int mode, bool aa, float maxsegmentpx)
{ 
  if (!dest) return;

#ifndef DISABLE_LICE_EXTENSIONS
  if (dest->Extended(LICE_EXT_SUPPORTS_ID, (void*) LICE_EXT_DRAWCBEZIER_ACCEL))
  {
    LICE_Ext_DrawCBezier_acceldata data(xstart, ystart, xctl1, yctl1, xctl2, yctl2, xend, yend, color, alpha, mode, aa);
    if (dest->Extended(LICE_EXT_DRAWCBEZIER_ACCEL, &data)) return;
  }
#endif

  int w = dest->getWidth();
  int h = dest->getHeight();
    
  if (xstart > xend) 
  {
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

  if (xlo < 0) 
  {
    xlo = 0;
    ylo = LICE_CBezier_GetY(xstart, xctl1, xctl2, xend, ystart, yctl1, yctl2, yend, xlo, (float*)0, (float*)0, (double*)0, &tlo);
  }
  if (xhi >= w)
  {
    xhi = w-1;
    yhi = LICE_CBezier_GetY(xstart, xctl1, xctl2, xend, ystart, yctl1, yctl2, yend, xhi, (float*)0, (float*)0, &thi, (double*)0);
  }
  if (xlo > xhi) return;

  float maxsegmentpx2 = (maxsegmentpx <= 0.0f ? 0.0f : maxsegmentpx*maxsegmentpx);

#define __LICE__ACTION(COMBFUNC) _LICE_CurveDrawer<COMBFUNC>::DrawCBezRecurse(0, dest, ax, bx, cx, xstart, ay, by, cy, ystart, color, alpha, aa, tlo, xlo, ylo, thi, xhi, yhi, w, h, maxsegmentpx2);
  if (aa)
  {
    __LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
  }
  else
  {
    __LICE_ACTIONBYMODE_CONSTANTALPHA(mode, alpha);
  }
#undef __LICE__ACTION
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
