#ifndef _LICE_BEZIER_
#define _LICE_BEZIER_

#include "lice.h"
#include <math.h>

// Returns Bezier x, y for a given t in [0,1].
template <class T>
void LICE_Bezier(T ctrl_x1, T ctrl_x2, T ctrl_x3, 
    T ctrl_y1, T ctrl_y2, T ctrl_y3, T t, T* pX, T* pY)
{
  T a = (1.0 - t) * (1.0 - t);
  T b = 2.0 * (1.0 - t) * t;
  T c = t * t;
  *pX = a * ctrl_x1 + b * ctrl_x2 + c * ctrl_x3;
  *pY = a * ctrl_y1 + b * ctrl_y2 + c * ctrl_y3;
}

// Returns Bezier y for a given x in [x1, x3] (for rasterizing).
template <class T>
T LICE_Bezier_GetY(T ctrl_x1, T ctrl_x2, T ctrl_x3, T ctrl_y1, T ctrl_y2, T ctrl_y3, T x)
{
  T a = ctrl_x1 - 2.0 * ctrl_x2 + ctrl_x3;
  T b = 2.0 * (ctrl_x2 - ctrl_x1);
  T c = ctrl_x1 - x;
  if (a == 0.0 && b == 0.0) {
    return ctrl_y2;
  }

  T t;
  if (a == 0.0) {
    t = -c / b;
  }
  else {
    t = (-b + sqrt(b * b - 4.0 * a * c)) / (2.0 * a);
  }

  a = (1.0 - t) * (1.0 - t);
  b = 2.0 * (1.0 - t) * t;
  c = t * t;
  return a * ctrl_y1 + b * ctrl_y2 + c * ctrl_y3;
}

// Basic quadratic nurbs.  Given a set of n (x,y) pairs, 
// populate pDest with the unit-spaced nurbs curve.
// pDest must be passed in with size (int) (*(pX + n) - *pX).
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
  for (/* */; xi < xm2; xi += 1.0, yi += m, ++pDest) {
    *pDest = (T) yi;
  }

  for (int i = 2; i < n; ++i, ++pX, ++pY) {
    x1 = x2;
    x2 = (double) *pX;
    y1 = y2;
    y2 = (double) *pY;

    xm1 = xm2;
    xm2 = 0.5 * (x1 + x2);
    ym1 = ym2;
    ym2 = 0.5 * (y1 + y2);

    for (/* */; xi < xm2; xi += 1.0, ++pDest) {
      *pDest = (T) LICE_Bezier_GetY(xm1, x1, xm2, ym1, y1, ym2, xi);
    }
  }

  m = (y2 - y1) / (x2 - x1);
  yi = ym2;
  for (/* */; xi <= x2; xi += 1.0, yi += m, ++pDest) {
    *pDest = (T) yi;
  }
}

#endif