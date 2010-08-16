#include "lice.h"
#include "lice_combine.h"
#include <math.h>

#define _PI 3.141592653589793238f
#define _QUADRANGLE (0.5f*_PI)
#define _CIRCLE (2.0f*_PI)
#define _SQRT_2 (sqrt(2.0f))

#ifndef __min
#define __min(x,y) ((x)<(y)?(x):(y))
#endif

template <class T> inline void _SWAP(T& a, T& b) { T tmp = a; a = b; b = tmp; }

typedef void (*DRAWFUNC)(LICE_IBitmap*, int, int, int, int, LICE_pixel, float, float);

template <class COMBFUNC> class LICE_QuadrantHandler
{
private:

	// Weights to map local coord space to screen coord space, eg: yx = weight of local y in screen x.
	// Only two of these are nonzero for a given instance.
	int m_xx, m_yx, m_xy, m_yy;
    
	inline void LocalToScreen(float cx, float cy, float xLoc, float yLoc, int& rxScr, int& ryScr)
	{
		rxScr = int(cx + m_xx*xLoc + m_yx*yLoc);
		ryScr = int(cy + m_xy*xLoc + m_yy*yLoc);
	}

	inline void ScreenToLocal(float cx, float cy, int xScr, int yScr, float& rxLoc, float& ryLoc)
	{
		if (m_xx) {
			rxLoc = (xScr-cx)/m_xx;
			ryLoc = (yScr-cy)/m_yy;
		}
		else {
			rxLoc = (yScr-cy)/m_xy;
			ryLoc = (xScr-cx)/m_yx;
		}
	}

	// DRAWFUNC list.
	static inline void Pix_AA(LICE_IBitmap* dest, int x1, int y1, int x2, int y2, LICE_pixel color, float alpha, float w)
	{
		LICE_pixel* px1 = dest->getBits() + y1 * dest->getRowSpan() + x1;
		COMBFUNC::doPix((LICE_pixel_chan*)px1,LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),LICE_GETA(color),(int)(alpha*(1.0f-w)*256.0f));
		LICE_pixel* px2 = dest->getBits() + y2 * dest->getRowSpan() + x2;
		COMBFUNC::doPix((LICE_pixel_chan*)px2,LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),LICE_GETA(color),(int)(alpha*w*256.0f));
	}
	static inline void Pix_noAA(LICE_IBitmap* dest, int x1, int y1, int x2, int y2, LICE_pixel color, float alpha, float w)
	{
		LICE_pixel* px1 = dest->getBits() + y1 * dest->getRowSpan() + x1;
		COMBFUNC::doPix((LICE_pixel_chan*)px1,LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),LICE_GETA(color),(int)(alpha*256.0f));
	}
	static inline void Pix_AA_Safe(LICE_IBitmap* dest, int x1, int y1, int x2, int y2, LICE_pixel color, float alpha, float w)
	{
		if (x1 >= 0 && x1 < dest->getWidth() && y1 >= 0 && y1 < dest->getHeight()) {
			LICE_pixel* px1 = dest->getBits() + y1 * dest->getRowSpan() + x1;
			COMBFUNC::doPix((LICE_pixel_chan*)px1,LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),LICE_GETA(color),(int)(alpha*(1.0f-w)*256.0f));
		}
		if (x2 >= 0 && x2 < dest->getWidth() && y2 >= 0 && y2 < dest->getHeight()) {
			LICE_pixel* px2 = dest->getBits() + y2 * dest->getRowSpan() + x2;
			COMBFUNC::doPix((LICE_pixel_chan*)px2,LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),LICE_GETA(color),(int)(alpha*w*256.0f));
		}
	}
	static inline void Pix_noAA_Safe(LICE_IBitmap* dest, int x1, int y1, int x2, int y2, LICE_pixel color, float alpha, float w)
	{
		if (x1 >= 0 && x1 < dest->getWidth() && y1 >= 0 && y1 < dest->getHeight()) {
			LICE_pixel* px1 = dest->getBits() + y1 * dest->getRowSpan() + x1;
			COMBFUNC::doPix((LICE_pixel_chan*)px1,LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),LICE_GETA(color),(int)(alpha*256.0f));
		}
	}

public:

	LICE_QuadrantHandler(int xx, int yx, int xy, int yy) : m_xx(xx), m_yx(yx), m_xy(xy), m_yy(yy) {}
	
	void Draw(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, bool aa,
		float aMin = 0.0f, float aMax = _QUADRANGLE)
	{
		float xMin, xMax, yMin, yMax;	// Local inclusive bounds.
		ScreenToLocal(cx, cy, 0, 0, xMin, yMin);
		ScreenToLocal(cx, cy, (dest->getWidth()-1), (dest->getHeight()-1), xMax, yMax);
		if (xMin > xMax) _SWAP(xMin, xMax);
		if (yMin > yMax) _SWAP(yMin, yMax);

		// Local space is (+,+) quadrant.
		float xLo = 0.0f, xHi = r, yLo = 0.0f, yHi = r;
		float r2 = r*r;
		if (aMin > 0.0f) {
			xLo = (float) (r * sin(aMin));
			yHi = (float) (r * cos(aMin));
		}
		if (aMax < _QUADRANGLE) {
			xHi = (float) (r * sin(aMax));
			yLo = (float) (r * cos(aMax));
		}

		if (xLo > xMax || xHi < xMin || yLo > yMax || yHi < yMin) {
			return;	// Trivial rejection.
		}

		DRAWFUNC drawFunc = 0;
		if (xLo > xMin && xLo < xMax && xHi > xMin && xHi < xMax && 
			yLo > yMin && yLo < yMax && yHi > yMin && yHi < yMax) {
			// Trivial acceptance (1-pixel buffer).
			if (aa) drawFunc = &LICE_QuadrantHandler<COMBFUNC>::Pix_AA;
			else drawFunc = &LICE_QuadrantHandler<COMBFUNC>::Pix_noAA;
		}
		else {
			// The quadrant will be clipped.
			if (xLo < xMin) {
				xLo = xMin;
				yHi = (float)sqrt(r2-xMin*xMin);
			}
			if (xHi > xMax) {
				xHi = xMax;
				yLo = (float)sqrt(r2-xMax*xMax);
			}
			if (yLo < yMin) {
				yLo = yMin; 
				xHi = (float)sqrt(r2-yMin*yMin);
			}
			if (yHi > yMax) {
				yHi = yMax;
				xLo = (float)sqrt(r2-yMax*yMax);
			}
			if (xLo > xHi || yLo > yHi) {
				return;	// Clipped out.
			}
			if (aa) drawFunc = &LICE_QuadrantHandler<COMBFUNC>::Pix_AA_Safe;
			else drawFunc = &LICE_QuadrantHandler<COMBFUNC>::Pix_noAA_Safe;
		}

		float oct = r/(float)_SQRT_2;
		int xi1, yi1, xi2, yi2;
		float x, y, z, w, wPrev = 0.0f;
		float xMid = __min(xHi-1.0f, oct);
		for (x = xLo, y = yHi; x <= xMid; x += 1.0f) {
			z = (float)sqrt(r2-x*x);
			w = (float)ceil(z)-z;
			if (w < wPrev) {
				y -= 1.0f;
			}
			wPrev = w;
			LocalToScreen(cx, cy, x, y, xi1, yi1);
			LocalToScreen(cx, cy, x, y-1.0f, xi2, yi2);
			drawFunc(dest, xi1, yi1, xi2, yi2, color, alpha, w);
		}
		wPrev = 0.0f;
		float yMid = __min(yHi-1.0f, oct);
		for (y = yLo, x = xHi; y <= yMid; y += 1.0f) {
			z = (float)sqrt(r2-y*y);
			w = (float)ceil(z)-z;
			if (w < wPrev) {
				x -= 1.0f;
			}
			wPrev = w;
			LocalToScreen(cx, cy, x, y, xi1, yi1);
			LocalToScreen(cx, cy, x-1.0f, y, xi2, yi2);
			drawFunc(dest, xi1, yi1, xi2, yi2, color, alpha, w);
		}
	}
};

template <class COMBFUNC> class LICE_QuadrantChooser
{
public:
  static void dq(int quadrant, LICE_IBitmap* dest, float cx, float cy, float r,
		LICE_pixel color, float alpha, bool aa, float aMin = 0.0f, float aMax = _QUADRANGLE)
  {

    static LICE_QuadrantHandler<COMBFUNC> _q0(1, 0, 0, -1);
    static LICE_QuadrantHandler<COMBFUNC> _q1(0, 1, 1, 0);
    static LICE_QuadrantHandler<COMBFUNC> _q2(-1, 0, 0, 1);
    static LICE_QuadrantHandler<COMBFUNC> _q3(0, -1, -1, 0);
    static LICE_QuadrantHandler<COMBFUNC> *_q[4] = { &_q0, &_q1, &_q2, &_q3 };

	  _q[quadrant%4]->Draw(dest, cx, cy, r, color, alpha, aa, aMin, aMax);
  }
};

void LICE_Quadrant(int quadrant, LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, int mode, bool aa,
	float minAngle = 0.0f, float maxAngle = _QUADRANGLE)
{
#define __LICE__ACTION(COMBFUNC) LICE_QuadrantChooser<COMBFUNC>::dq(quadrant, dest, cx, cy, r, color, alpha, aa, minAngle, maxAngle)
  if (aa)
  {
  	__LICE_ACTIONBYMODE_NOSRCALPHA(mode, alpha);
  }
  else
  {
    __LICE_ACTIONBYMODE_CONSTANTALPHA(mode,alpha);
  }
#undef __LICE__ACTION
}

void ModAngle(float& rA)
{
	while (rA < 0.0f) rA += _CIRCLE;
	while (rA > _CIRCLE) rA -= _CIRCLE;
}

void LICE_Arc(LICE_IBitmap* dest, float cx, float cy, float r, float minAngle, float maxAngle, 
	LICE_pixel color, float alpha, int mode, bool aa)
{
  if (dest && dest->isFlipped())
  {
    cy=dest->getHeight()-1-cy;
    minAngle=_PI-minAngle;
    maxAngle=_PI-maxAngle;
  }
  
	ModAngle(minAngle);
	ModAngle(maxAngle);

	int startQ = int(minAngle / _QUADRANGLE);
	int endQ = int(ceil(maxAngle / _QUADRANGLE))-1;
	float aLo = minAngle - startQ * _QUADRANGLE;
	float aHi = maxAngle - endQ * _QUADRANGLE;
	while (endQ < startQ) endQ += 4;

	if (startQ == endQ) {
		LICE_Quadrant(startQ, dest, cx, cy, r, color, alpha, mode, aa, aLo, aHi);
		return;
	}
	
	LICE_Quadrant(startQ, dest, cx, cy, r, color, alpha, mode, aa, aLo, _QUADRANGLE);
	for (int i = startQ+1; i < endQ; ++i) {
		LICE_Quadrant(i, dest, cx, cy, r, color, alpha, mode, aa);
	}
	LICE_Quadrant(endQ, dest, cx, cy, r, color, alpha, mode, aa, 0.0f, aHi);
}

#define A(x) ((LICE_pixel_chan)((x)*255.0+0.5))

static bool CachedCircle(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, int mode, bool aa)
{
  // fast draw for some small circles 
  if (r == 2.5f) {
    if (aa) {
      LICE_pixel_chan alphas[36] = {
        A(0.06), A(0.75), A(1.00), A(1.00), A(0.75), A(0.06),
        A(0.75), A(0.82), A(0.31), A(0.31), A(0.82), A(0.75),
        A(1.00), A(0.31), A(0.00), A(0.00), A(0.31), A(1.00),
        A(1.00), A(0.31), A(0.00), A(0.00), A(0.31), A(1.00),
        A(0.75), A(0.82), A(0.31), A(0.31), A(0.82), A(0.75),
        A(0.06), A(0.75), A(1.00), A(1.00), A(0.75), A(0.06)
      };
      LICE_DrawGlyph(dest, cx-r, cy-r, color, alphas, 6, 6, alpha, mode);
    }
    else {
      LICE_pixel_chan alphas[36] = {
        A(0.00), A(0.00), A(1.00), A(1.00), A(0.00), A(0.00),
        A(0.00), A(1.00), A(0.00), A(0.00), A(1.00), A(0.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(0.00), A(1.00), A(0.00), A(0.00), A(1.00), A(0.00),
        A(0.00), A(0.00), A(1.00), A(1.00), A(0.00), A(0.00)
      };
      LICE_DrawGlyph(dest, cx-r, cy-r, color, alphas, 6, 6, alpha, mode);    
    }
    return true;
  }
  else if (r == 3.0f) {
    if (aa) {
      LICE_pixel_chan alphas[49] = {
        A(0.00), A(0.56), A(1.00), A(1.00), A(1.00), A(0.56), A(0.00),
        A(0.56), A(1.00), A(0.38), A(0.25), A(0.38), A(1.00), A(0.56),
        A(1.00), A(0.44), A(0.00), A(0.00), A(0.00), A(0.44), A(1.00),
        A(1.00), A(0.19), A(0.00), A(0.00), A(0.00), A(0.19), A(1.00),
        A(1.00), A(0.44), A(0.00), A(0.00), A(0.00), A(0.44), A(1.00),
        A(0.56), A(1.00), A(0.38), A(0.25), A(0.38), A(1.00), A(0.56),
        A(0.00), A(0.56), A(1.00), A(1.00), A(1.00), A(0.56), A(0.00)
      };
      LICE_DrawGlyph(dest, cx-r, cy-r, color, alphas, 7, 7, alpha, mode);
    }
    else {
      LICE_pixel_chan alphas[49] = {
        A(0.00), A(0.00), A(1.00), A(1.00), A(1.00), A(0.00), A(0.00),
        A(0.00), A(1.00), A(0.00), A(0.00), A(0.00), A(1.00), A(0.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(0.00), A(1.00), A(0.00), A(0.00), A(0.00), A(1.00), A(0.00),
        A(0.00), A(0.00), A(1.00), A(1.00), A(1.00), A(0.00), A(0.00)
      };
      LICE_DrawGlyph(dest, cx-r, cy-r, color, alphas, 7, 7, alpha, mode);    
    }
    return true;
  }
  else if (r == 3.5f) {
    if (aa) {
      LICE_pixel_chan alphas[64] = {
        A(0.00), A(0.31), A(0.87), A(1.00), A(1.00), A(0.87), A(0.31), A(0.00),
        A(0.31), A(1.00), A(0.69), A(0.25), A(0.25), A(0.69), A(1.00), A(0.31),
        A(0.87), A(0.69), A(0.00), A(0.00), A(0.00), A(0.00), A(0.69), A(0.87),
        A(1.00), A(0.25), A(0.00), A(0.00), A(0.00), A(0.00), A(0.25), A(1.00),
        A(1.00), A(0.25), A(0.00), A(0.00), A(0.00), A(0.00), A(0.25), A(1.00),
        A(0.87), A(0.69), A(0.00), A(0.00), A(0.00), A(0.00), A(0.69), A(0.87),
        A(0.31), A(1.00), A(0.69), A(0.25), A(0.25), A(0.69), A(1.00), A(0.31),
        A(0.00), A(0.31), A(0.87), A(1.00), A(1.00), A(0.87), A(0.31), A(0.00)
      };
      LICE_DrawGlyph(dest, cx-r, cy-r, color, alphas, 8, 8, alpha, mode);
    }
    else {
      LICE_pixel_chan alphas[64] = {
        A(0.00), A(0.00), A(1.00), A(1.00), A(1.00), A(1.00), A(0.00), A(0.00),
        A(0.00), A(1.00), A(1.00), A(0.00), A(0.00), A(1.00), A(1.00), A(0.00),
        A(1.00), A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00), A(1.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(1.00), A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00), A(1.00),
        A(0.00), A(1.00), A(1.00), A(0.00), A(0.00), A(1.00), A(1.00), A(0.00),
        A(0.00), A(0.00), A(1.00), A(1.00), A(1.00), A(1.00), A(0.00), A(0.00)
      };
      LICE_DrawGlyph(dest, cx-r, cy-r, color, alphas, 8, 8, alpha, mode);
    }
    return true;
  }
  else if (r == 4.0f) {
    if (aa) {
      LICE_pixel_chan alphas[81] = {
        A(0.00), A(0.12), A(0.69), A(1.00), A(1.00), A(1.00), A(0.69), A(0.12), A(0.00),
        A(0.12), A(0.94), A(0.82), A(0.31), A(0.25), A(0.31), A(0.82), A(0.94), A(0.12),
        A(0.69), A(0.82), A(0.06), A(0.00), A(0.00), A(0.00), A(0.06), A(0.82), A(0.69),
        A(1.00), A(0.31), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.31), A(1.00),
        A(1.00), A(0.19), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.19), A(1.00),
        A(1.00), A(0.31), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.31), A(1.00),
        A(0.69), A(0.82), A(0.06), A(0.00), A(0.00), A(0.00), A(0.06), A(0.82), A(0.69),
        A(0.12), A(0.94), A(0.82), A(0.31), A(0.25), A(0.31), A(0.82), A(0.94), A(0.12),
        A(0.00), A(0.12), A(0.69), A(1.00), A(1.00), A(1.00), A(0.69), A(0.12), A(0.00)
      };
      LICE_DrawGlyph(dest, cx-r, cy-r, color, alphas, 9, 9, alpha, mode);
    }
    else {
      LICE_pixel_chan alphas[81] = {
        A(0.00), A(0.00), A(1.00), A(1.00), A(1.00), A(1.00), A(1.00), A(0.00), A(0.00),
        A(0.00), A(1.00), A(1.00), A(0.00), A(0.00), A(0.00), A(1.00), A(1.00), A(0.00),
        A(1.00), A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00), A(1.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00),
        A(1.00), A(1.00), A(0.00), A(0.00), A(0.00), A(0.00), A(0.00), A(1.00), A(1.00),
        A(0.00), A(1.00), A(1.00), A(0.00), A(0.00), A(0.00), A(1.00), A(1.00), A(0.00),
        A(0.00), A(0.00), A(1.00), A(1.00), A(1.00), A(1.00), A(1.00), A(0.00), A(0.00)
      };
      LICE_DrawGlyph(dest, cx-r, cy-r, color, alphas, 9, 9, alpha, mode);
    }
    return true;
  }

  return false;
}

void LICE_Circle(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, int mode, bool aa)
{
  if (CachedCircle(dest, cx, cy, r, color, alpha, mode, aa)) {
    return;
  }
	LICE_Arc(dest, cx, cy, r, 0.0f, _CIRCLE, color, alpha, mode, aa);
}


void LICE_RoundRect(LICE_IBitmap *drawbm, float xpos, float ypos, float w, float h, int cornerradius,
                    LICE_pixel col, float alpha, int mode, bool aa)
{
  if (cornerradius>0)
  {
    float cr=cornerradius;
    if (cr > w*0.5) cr=w*0.5;
    if (cr > h*0.5) cr=h*0.5;
    cr=floor(cr);
    if (cr>=2)
    {
      LICE_Line(drawbm,xpos+cr,ypos,xpos+w-cr,ypos,col,alpha,mode,aa);
      LICE_Line(drawbm,xpos+cr,ypos+h,xpos+w-cr,ypos+h,col,alpha,mode,aa);
      LICE_Line(drawbm,xpos+w,ypos+cr,xpos+w,ypos+h-cr,col,alpha,mode,aa);
      LICE_Line(drawbm,xpos,ypos+cr,xpos,ypos+h-cr,col,alpha,mode,aa);


      LICE_Arc(drawbm,xpos+cr,ypos+cr,cr,-_PI*0.5,0,col,alpha,mode,aa);
      LICE_Arc(drawbm,xpos+w-cr,ypos+cr,cr,0,_PI*0.5,col,alpha,mode,aa);
      LICE_Arc(drawbm,xpos+w-cr,ypos+h-cr,cr,_PI*0.5,_PI,col,alpha,mode,aa);
      LICE_Arc(drawbm,xpos+cr,ypos+h-cr,cr,_PI,_PI*1.5,col,alpha,mode,aa);

      return;
    }
  }

  LICE_Line(drawbm,xpos,ypos,xpos+w,ypos,col,alpha,mode,aa);
  LICE_Line(drawbm,xpos,ypos+h,xpos+w,ypos+h,col,alpha,mode,aa);
  LICE_Line(drawbm,xpos+w,ypos,xpos+w,ypos+h,col,alpha,mode,aa);
  LICE_Line(drawbm,xpos,ypos,xpos,ypos+h,col,alpha,mode,aa);
}
