#include "lice.h"
#include "lice_combine.h"
#include <math.h>

template <class T> inline void SWAP(T& a, T& b) { T tmp = a; a = b; b = tmp; }




enum { eOK = 0, eXLo = 1, eXHi = 2, eYLo = 4, eYHi = 8 };

static int OffscreenTest(float x, float y, int nX, int nY)
{
  int e = eOK;
  if (x < 0.0f) e |= eXLo; 
  else if (x + 0.5f >= (float) nX) e |= eXHi; 
  if (y < 0.0f) e |= eYLo; 
  else if (y + 0.5f >= (float) nY) e |= eYHi; 
  return e;
}

// Cohen-Sutherland.  Returns false if the line is entirely offscreen.
static bool ClipLine(float& x1, float& y1, float& x2, float& y2, int nX, int nY)
{
  int e1 = OffscreenTest(x1, y1, nX, nY); 
  int e2 = OffscreenTest(x2, y2, nX, nY);
  
  bool accept = false, done = false;
  do		// Always hit a trivial case eventually.
  {
    if (!(e1 | e2)) {
      accept = done = true;
    }
    else 
      if (e1 & e2) {
        done = true;	// Line is entirely offscreen.
      }
      else { 
        float x, y;
        int eOut = e1 ? e1 : e2;
        if (eOut & eYHi) {
          x = x1 + (x2 - x1) * ((float) nY - y1) / (y2 - y1);
          y = (float) (nY - 1);
        }
        else 
          if (eOut & eYLo) {
            x = x1 + (x2 - x1) * -y1 / (y2 - y1);
            y = 0.0f;
          }
          else 
            if (eOut & eXHi) {
              y = y1 + (y2 - y1) * ((float) nX - x1) / (x2 - x1);
              x = (float) (nX - 1);
            }
            else {
              y = y1 + (y2 - y1) * -x1 / (x2 - x1);
              x = 0.0f;
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
  return accept;
}


template <class COMBFUNC> class __LICE_LineClass
{
    static inline void PixPlot(LICE_pixel* pix, int rowSpan, int x, int y, LICE_pixel color, float alpha, float weight)
    {
	    LICE_pixel* px = pix + y * rowSpan + x;
	    alpha *= weight;
	    COMBFUNC::doPix((LICE_pixel_chan*)px,LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),LICE_GETA(color),(int)(alpha*256.0f));
    }

  public:


    static void LICE_Line_noAA(LICE_IBitmap *dest, float x1, float y1, float x2, float y2, LICE_pixel color, float alpha)
    {
	    LICE_pixel* pix = dest->getBits();
	    int rowSpan = dest->getRowSpan();

	    if (int(x1) == int(x2) && int(y1) == int(y2)) {
		    PixPlot(pix, rowSpan, int(x1 + 0.5), int(y1 + 0.5), color, 1.0f, 1.0f);
		    return;
	    }

	    // Bresenham.
	    bool steep = (fabs(y2 - y1) > fabs(x2 - x1));
	    if (steep) {
		    SWAP(x1, y1);
		    SWAP(x2, y2);
	    }

	    if (x1 > x2) {
		    SWAP(x1, x2);
		    SWAP(y1, y2);
	    }

	    float dx = x2 - x1;
	    float dy = (float)fabs(y2 - y1);
	    float err = 0.0, dErr = dy / dx;

	    int yIncr;
	    if (y1 < y2) yIncr = 1;
	    else yIncr = -1;

	    if (steep) {
		    for (int xi = int(x1 + 0.5), yi = int(y1 + 0.5); xi <= int(x2 + 0.5); ++xi) {
			    PixPlot(pix, rowSpan, yi, xi, color, alpha, 1.0f);
			    err += dErr;
			    if (err >= 0.5f) {
				    yi += yIncr;
				    err -= 1.0f;
			    }
		    }
	    }
	    else {
		    for (int xi = int(x1 + 0.5), yi = int(y1 + 0.5); xi <= int(x2 + 0.5); ++xi) {
			    PixPlot(pix, rowSpan, xi, yi, color, alpha, 1.0f);
			    err += dErr;
			    if (err >= 0.5f) {
				    yi += yIncr;
				    err -= 1.0f;
			    }
		    }
	    }	
    }

    static void LICE_Line_AA(LICE_IBitmap* dest, float x1, float y1, float x2, float y2, LICE_pixel color, float alpha)
    {
	    LICE_pixel* pix = dest->getBits();
	    int rowSpan = dest->getWidth();
	    int xi, yi, nX = dest->getWidth(), nY = dest->getHeight();

	    if (int(x1) == int(x2) && int(y1) == int(y2)) {
		    xi = int(x1);
		    yi = int(y1);
		    float wXLo = x1 - (float)floor(x1), wXHi = 1.0f - wXLo;
		    float wYLo = y1 - (float)floor(y1), wYHi = 1.0f - wYLo;
        float weight = (wXLo + wYLo) / 4.0f;
		    PixPlot(pix, rowSpan, xi, yi, color, 1.0f, weight);
		    if (xi < nX - 1) {
			    PixPlot(pix, rowSpan, xi + 1, yi, color, 1.0f, weight);
		    }
		    if (yi < nY - 1) {
			    PixPlot(pix, rowSpan, xi, yi + 1, color, 1.0f, weight);
		    }
		    if (xi < nX - 1 && yi < nY - 1) {
			    PixPlot(pix, rowSpan, xi + 1, yi + 1, color, 1.0f, weight);
		    }
		    return;
	    }

	    // Wu.
	    bool steep = (fabs(y2 - y1) > fabs(x2 - x1));
	    if (steep) {
		    SWAP(x1, y1);
		    SWAP(x2, y2);
	    }

	    if (x2 < x1) {
		    SWAP(x1, x2);
		    SWAP(y1, y2);
	    }

      float dx = x2 - x1;
      float dy = y2 - y1;
      float m = dy / dx;

	    float xend = (float)floor(x1 + 0.5f);
      float yend = y1 + m * (xend - x1);
      float xgap = 1.0f - (x1 + 0.5f - xend);
	    int xpxl1 = int(xend);
      int ypxl1 = int(yend);

      float vweight = yend - (float)floor(yend);
	    if (steep) {
		    PixPlot(pix, rowSpan, ypxl1, xpxl1, color, alpha, xgap * (1.0f - vweight));
		    if (xpxl1 < nY - 1) {
			    PixPlot(pix, rowSpan, ypxl1 + 1, xpxl1, color, alpha, xgap * vweight);
		    }
	    }
	    else {
		    PixPlot(pix, rowSpan, xpxl1, ypxl1, color, alpha, xgap * (1.0f - vweight));
		    if (ypxl1 < nY - 1) {
			    PixPlot(pix, rowSpan, xpxl1, ypxl1 + 1, color, alpha, xgap * vweight);
		    }
	    }
	    float intery = yend + m;

	    xend = (float)floor(x2 + 0.5f);
	    yend = y2 + m * (xend - x2);
      xgap = x2 + 0.5f - xend;
      int xpxl2 = int(xend);
      int ypxl2 = int(yend);

      float vweight2 = yend - (float)floor(yend);
	    if (steep) {
		    PixPlot(pix, rowSpan, ypxl2, xpxl2, color, alpha, xgap * (1.0f - vweight2));
		    if (ypxl2 < nX - 1) {
			    PixPlot(pix, rowSpan, ypxl2 + 1, xpxl2, color, alpha, xgap * vweight2);
		    }
			if (xpxl1 < nY - 1) {
				for (xi = xpxl1 + 1; xi < xpxl2 - 1; ++xi) {
					float weight = intery - (float)floor(intery);
					PixPlot(pix, rowSpan, int(intery), xi, color, alpha, 1.0f - weight);
					PixPlot(pix, rowSpan, int(intery) + 1, xi, color, alpha, weight);
					intery += m;
				}
				float weight = intery - (float)floor(intery);
				PixPlot(pix, rowSpan, int(intery), xi, color, alpha, 1.0f - weight);
				if (int(intery) < nX - 1) {
				    PixPlot(pix, rowSpan, int(intery) + 1, xi, color, alpha, weight);
				}
			}
	    }
	    else {
		    PixPlot(pix, rowSpan, xpxl2, ypxl2, color, alpha, xgap * (1.0f - vweight2));
		    if (ypxl2 < nY - 1) {
			    PixPlot(pix, rowSpan, xpxl2, ypxl2 + 1, color, alpha, xgap * vweight2);
		    }
			if (xpxl1 < nX - 1) {
				for (xi = xpxl1 + 1; xi < xpxl2 - 1; ++xi) {
					float weight = intery - (float)floor(intery);
					PixPlot(pix, rowSpan, xi, int(intery), color, alpha, 1.0f - weight);
					PixPlot(pix, rowSpan, xi, int(intery) + 1, color, alpha, weight);
					intery += m;
				}
				float weight = intery - (float)floor(intery);
				PixPlot(pix, rowSpan, xi, int(intery), color, alpha, 1.0f - weight);
				if (int(intery) < nY - 1) {
				    PixPlot(pix, rowSpan, xi, int(intery) + 1, color, alpha, weight);
				}
			}
	    }
    }
};

void LICE_Line(LICE_IBitmap *dest, float x1, float y1, float x2, float y2, LICE_pixel color, float alpha, int mode, bool aa)
{
	int w = dest->getWidth(), h = dest->getHeight();
	if (ClipLine(x1, y1, x2, y2, w, h)) {
		if (aa) {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_Line_AA(dest, x1, y1, x2, y2, color, alpha)
			__LICE_ACTIONBYMODE(mode, alpha);
#undef __LICE__ACTION
		}
		else {
#define __LICE__ACTION(COMBFUNC) __LICE_LineClass<COMBFUNC>::LICE_Line_noAA(dest, x1, y1, x2, y2, color, alpha)
			__LICE_ACTIONBYMODE_NOALPHACHANGE(mode, color, alpha);
#undef __LICE__ACTION
		}
	}
}