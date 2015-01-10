#ifndef _LICE_BEZIER_
#define _LICE_BEZIER_

#include "lice.h"
#include <math.h>

// Returns quadratic bezier x, y for a given t in [0,1].
template <class T>
void LICE_Bezier(T ctrl_x1, T ctrl_x2, T ctrl_x3, 
  T ctrl_y1, T ctrl_y2, T ctrl_y3, double t, T* pX, T* pY)
{
  double it = 1.0 - t;
  double a = it * it;
  double b = 2.0 * it * t;
  double c = t * t;
  *pX = (T) (a * (double) ctrl_x1 + b * (double) ctrl_x2 + c * (double) ctrl_x3);
  *pY = (T) (a * (double) ctrl_y1 + b * (double) ctrl_y2 + c * (double) ctrl_y3);
}

template <class T>
void LICE_CBezier_GetCoeffs(T ctrl_x1, T ctrl_x2, T ctrl_x3, T ctrl_x4,
  T ctrl_y1, T ctrl_y2, T ctrl_y3, T ctrl_y4, 
  double* pAX, double* pBX, double* pCX, 
  double* pAY, double* pBY, double* pCY)
{
  double cx = *pCX = 3.0 * (double) (ctrl_x2 - ctrl_x1);
  double bx = *pBX = 3.0 * (double) (ctrl_x3 - ctrl_x2) - cx;
  *pAX = (double) (ctrl_x4 - ctrl_x1) - cx - bx;
  double cy = *pCY = 3.0 * (double) (ctrl_y2 - ctrl_y1);
  double by = *pBY = 3.0 * (double) (ctrl_y3 - ctrl_y2) - cy;
  *pAY = (double) (ctrl_y4 - ctrl_y1) - cy - by;
}

// Returns cubic bezier x, y for a given t in [0,1].
template <class T>
void LICE_CBezier(T ctrl_x1, T ctrl_x2, T ctrl_x3, T ctrl_x4,
  T ctrl_y1, T ctrl_y2, T ctrl_y3, T ctrl_y4, double t, T* pX, T* pY)
{
  double ax, bx, cx, ay, by, cy;
  LICE_CBezier_GetCoeffs(ctrl_x1, ctrl_x2, ctrl_x3, ctrl_x4,
    ctrl_y1, ctrl_y2, ctrl_y3, ctrl_y4, 
    &ax, &bx, &cx, &ay, &by, &cy);

  double t2 = t * t;
  double t3 = t * t2;
  *pX = (T) (ax * t3 + bx * t2 + cx * t) + ctrl_x1;
  *pY = (T) (ay * t3 + by * t2 + cy * t) + ctrl_y1;
}

// Returns quadratic bezier y for a given x in [x1, x3] (for rasterizing).
// ctrl_x1 < ctrl_x3 required.
template <class T>
T LICE_Bezier_GetY(T ctrl_x1, T ctrl_x2, T ctrl_x3, T ctrl_y1, T ctrl_y2, T ctrl_y3, T x, double* pt=0)
{
  if (x <= ctrl_x1) 
  {
    if (pt) *pt = 0.0;
    return ctrl_y1;
  }
  if (x >= ctrl_x3)
  {
    if (pt) *pt = 1.0;
    return ctrl_y3;
  }

  double a = (double) ctrl_x1 - (double) (2 * ctrl_x2) + (double) ctrl_x3;
  if (a == 0.0)   // linear
  { 
    T y = ctrl_y1;
    double t = 0.0;
    if (ctrl_x1 != ctrl_x3) 
    {
      t = (x-ctrl_x1)/(ctrl_x3-ctrl_x1);
      y += t*(ctrl_y3-ctrl_y1);      
    }
    if (pt) *pt = t;
    return y;
  }

  double b = (double) (2 * (ctrl_x2 - ctrl_x1));
  double c = (double) (ctrl_x1 - x);
  double t = (-b + sqrt(b * b - 4.0 * a * c)) / (2.0 * a);
  double it = 1.0 - t;

  a = it * it;
  b = 2.0 * it * t;
  c = t * t;

  if (pt) *pt = t;
  return (T) (a * (double) ctrl_y1 + b * (double) ctrl_y2 + c * (double) ctrl_y3);
}

// Special case for x = y = [0,1]
template <class T>
void LICE_Bezier_Norm(T ctrl_x2, T ctrl_y2, double t, T* pX, T* pY)
{
  double b = 2.0 * (1.0 - t) * t;
  double c = t * t;
  *pX = (T) (b * (double) ctrl_x2 + c);
  *pY = (T) (b * (double) ctrl_y2 + c);
}

// special case for x = y = [0,1].
template <class T>
T LICE_Bezier_GetY_Norm(T ctrl_x2, T ctrl_y2, T x)
{
  if (x < (T) 0.0) {
    return (T) 0.0;
  }
  if (x >= (T) 1.0) {
    return (T) 1.0;
  }
  if (ctrl_x2 == (T) 0.5) { // linear
    return x;
  }

  double b = (double) (2 * ctrl_x2);
  double a = 1.0 - b;
  double c = (double) -x;
  double t = (-b + sqrt(b * b - 4.0 * a * c)) / (2.0 * a);

  b = 2.0 * (1.0 - t) * t;
  c = t * t;
  return (T) (b * (double) ctrl_y2 + c);
}

// Finds the cardinal bezier control points surrounding x2.  
// Cubic bezier over (x1,x1b,x2a,x2), (y1,y1b,y2a,y2) or
// quadratic bezier over (x1,x1b,mid(x1b,x2a)), (y1,y1b,mid(y1b,y2a))
// will smoothly interpolate between (x1,y1) and (x2,y2) while preserving all existing values.
// The lower alpha is, the more tame the bezier curve will be (0.25 = subtle).
template <class T>
void LICE_Bezier_FindCardinalCtlPts(double alpha, T x1, T x2, T x3, T y1, T y2, T y3, 
  T* ctrl_x2a, T* ctrl_x2b, T* ctrl_y2a, T* ctrl_y2b)
{
  double dxa = alpha * (double) (x2 - x1);
  double dxb = alpha * (double) (x3 - x2);
  if (ctrl_x2a) *ctrl_x2a = x2 - (T) dxa;
  if (ctrl_x2b) *ctrl_x2b = x2 + (T) dxb;

  if (x1 == x3) 
  {
    if (ctrl_y2a) *ctrl_y2a = y2;
    if (ctrl_y2b) *ctrl_y2b = y2;
  }
  else 
  {
    double m = (double) (y3 - y1) / (double) (x3 - x1);
    if (ctrl_y2a) *ctrl_y2a = y2 - (T) (m * dxa);
    if (ctrl_y2b) *ctrl_y2b = y2 + (T) (m * dxb);
  }
}

// Basic quadratic nurbs.  Given a set of n (x,y) pairs, 
// populate pDest with the unit-spaced nurbs curve.
// pDest must be passed in with size (int) (*(pX+n-1) - *pX).
// pX must be monotonically increasing and no duplicates.
template <class T>
inline void LICE_QNurbs(T* pDest, T* pX, T* pY, int n)
{
  double x1 = (double) *pX++, x2 = (double) *pX++;
  double y1 = (double) *pY++, y2 = (double) *pY++;
  double xm1, xm2 = 0.5 * (x1 + x2);
  double ym1, ym2 = 0.5 * (y1 + y2);

  double m = (y2 - y1) / (x2 - x1);
  double xi = x1, yi = y1;
  for (/* */; xi < xm2; xi += 1.0, yi += m, ++pDest) 
  {
    *pDest = (T) yi;
  }

  for (int i = 2; i < n; ++i, ++pX, ++pY) 
  {
    x1 = x2;
    x2 = (double) *pX;
    y1 = y2;
    y2 = (double) *pY;

    xm1 = xm2;
    xm2 = 0.5 * (x1 + x2);
    ym1 = ym2;
    ym2 = 0.5 * (y1 + y2);

    if (ym1 == ym2) 
    {
      for (/* */; xi < xm2; xi += 1.0, ++pDest) 
      {
        *pDest = (T) y1;
      }
    }
    else 
    {    
      for (/* */; xi < xm2; xi += 1.0, ++pDest) 
      {
        *pDest = (T) LICE_Bezier_GetY(xm1, x1, xm2, ym1, y1, ym2, xi);
      }
    }
  }

  m = (y2 - y1) / (x2 - x1);
  yi = ym2;
  for (/* */; xi < x2; xi += 1.0, yi += m, ++pDest) 
  {
    *pDest = (T) yi;
  }
}

#define CBEZ_ITERS 8

#define EVAL_CBEZ(tx,a,b,c,d,t) \
{ \
  double _t2=t*t; \
  tx=(a*t*_t2+b*_t2+c*t+d); \
}

#define EVAL_CBEZXY(tx, ty, ax, bx, cx, dx, ay, by, cy, dy, t) \
{ \
  double _t2=t*t; \
  double _t3=t*_t2; \
  tx=ax*_t3+bx*_t2+cx*t+dx; \
  ty=ay*_t3+by*_t2+cy*t+dy; \
}

template <class T>
T LICE_CBezier_GetY(T ctrl_x1, T ctrl_x2, T ctrl_x3, T ctrl_x4,
  T ctrl_y1, T ctrl_y2, T ctrl_y3, T ctrl_y4, T x, 
  T* pNextX = 0, T* pdYdX = 0, double* ptLo = 0, double* ptHi = 0)
{
  if (x < ctrl_x1) 
  {
    if (pNextX) *pNextX = ctrl_x1;
    if (pdYdX) *pdYdX = (T) 0.0;
    return ctrl_y1;
  }
  if (x >= ctrl_x4) 
  {
    if (pNextX) *pNextX = ctrl_x4;
    if (pdYdX) *pdYdX = (T) 0.0;
    return ctrl_y4;
  }

  double ax, bx, cx, ay, by, cy;
  LICE_CBezier_GetCoeffs(ctrl_x1, ctrl_x2, ctrl_x3, ctrl_x4,
    ctrl_y1, ctrl_y2, ctrl_y3, ctrl_y4, 
    &ax, &bx, &cx, &ay, &by, &cy);

  double tx, t, tLo = 0.0, tHi = 1.0;
  double xLo, xHi, yLo, yHi;
  int i;
  for (i = 0; i < CBEZ_ITERS; ++i)
  {
    t = 0.5 * (tLo + tHi);
    EVAL_CBEZ(tx, ax, bx, cx, (double) ctrl_x1, t);
    if (tx < (double) x)
    {
      tLo = t;
      xLo = tx;
    }
    else if (tx > (double) x) 
    {
      tHi = t;
      xHi = tx;
    }
    else
    {
      tLo = t;
      xLo = tx;
      tHi = t + 1.0/pow(2.0,CBEZ_ITERS);
      if (tHi > 1.0) tHi = 1.0; // floating point error 
      EVAL_CBEZ(xHi, ax, bx, cx, (double) ctrl_x1, tHi);
      break;
    }
  }

  if (tLo == 0.0) EVAL_CBEZ(xLo, ax, bx, cx, (double) ctrl_x1, 0.0);
  if (tHi == 1.0) EVAL_CBEZ(xHi, ax, bx, cx, (double) ctrl_x1, 1.0);

  EVAL_CBEZ(yLo, ay, by, cy, (double) ctrl_y1, tLo);
  EVAL_CBEZ(yHi, ay, by, cy, (double) ctrl_y1, tHi);

  double dYdX = (xLo == xHi ? 0.0 : (yHi - yLo) / (xHi - xLo));
  double y = yLo + ((double) x - xLo) * dYdX;

  if (pNextX) *pNextX = (T) xHi;
  if (pdYdX) *pdYdX = (T) dYdX;

  if (ptLo) *ptLo = tLo;
  if (ptHi) *ptHi = tHi;

  return (T) y;
}

#endif

