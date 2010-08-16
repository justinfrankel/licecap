#ifndef _LICE_COMBINE_H_
#define _LICE_COMBINE_H_

#define __LICE_BOUND(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))


#define LICE_PIXEL_HALF(x) (((x)>>1)&0x7F7F7F7F)
#define LICE_PIXEL_QUARTER(x) (((x)>>2)&0x3F3F3F3F)
#define LICE_PIXEL_EIGHTH(x) (((x)>>3)&0x1F1F1F1F)



#ifdef _MSC_VER
static inline int __LICE_TOINT(double x) // don't use this _everywhere_ since it doesnt round the same as (int)
{
  int tmp;
  __asm
  {
    fld x
    fistp tmp
  };
  return tmp;
}
#else
#define __LICE_TOINT(x) ((int)(x))
#endif

static inline void __LICE_BilinearFilter(int *r, int *g, int *b, int *a, LICE_pixel_chan *pin, LICE_pixel_chan *pinnext, double xfrac, double yfrac)
{
  double f4=xfrac*yfrac;
  double f1=1.0-yfrac-xfrac+f4; // (1.0-xfrac)*(1.0-yfrac);
  double f3=yfrac-f4; // (1.0-xfrac)*yfrac;
  double f2=xfrac-f4; // xfrac*(1.0-yfrac);
  *r=__LICE_TOINT(pin[LICE_PIXEL_R]*f1 + pin[4+LICE_PIXEL_R]*f2 + pinnext[LICE_PIXEL_R]*f3 + pinnext[4+LICE_PIXEL_R]*f4);
  *g=__LICE_TOINT(pin[LICE_PIXEL_G]*f1 + pin[4+LICE_PIXEL_G]*f2 + pinnext[LICE_PIXEL_G]*f3 + pinnext[4+LICE_PIXEL_G]*f4);
  *b=__LICE_TOINT(pin[LICE_PIXEL_B]*f1 + pin[4+LICE_PIXEL_B]*f2 + pinnext[LICE_PIXEL_B]*f3 + pinnext[4+LICE_PIXEL_B]*f4);
  *a=__LICE_TOINT(pin[LICE_PIXEL_A]*f1 + pin[4+LICE_PIXEL_A]*f2 + pinnext[LICE_PIXEL_A]*f3 + pinnext[4+LICE_PIXEL_A]*f4);
}
static inline void __LICE_LinearFilter(int *r, int *g, int *b, int *a, LICE_pixel_chan *pin, LICE_pixel_chan *pinnext, double frac)
{
  double f=1.0-frac;
  *r=__LICE_TOINT(pin[LICE_PIXEL_R]*f + pinnext[LICE_PIXEL_R]*frac);
  *g=__LICE_TOINT(pin[LICE_PIXEL_G]*f + pinnext[LICE_PIXEL_G]*frac);
  *b=__LICE_TOINT(pin[LICE_PIXEL_B]*f + pinnext[LICE_PIXEL_B]*frac);
  *a=__LICE_TOINT(pin[LICE_PIXEL_A]*f + pinnext[LICE_PIXEL_A]*frac);
}


static void inline _LICE_MakePixel(LICE_pixel_chan *out, int r, int g, int b, int a)
{
  if (r&~0xff) out[LICE_PIXEL_R]=r<0?0:255; else out[LICE_PIXEL_R] = (LICE_pixel_chan) (r);
  if (g&~0xff) out[LICE_PIXEL_G]=g<0?0:255; else out[LICE_PIXEL_G] = (LICE_pixel_chan) (g);
  if (b&~0xff) out[LICE_PIXEL_B]=b<0?0:255; else out[LICE_PIXEL_B] = (LICE_pixel_chan) (b);
  if (a&~0xff) out[LICE_PIXEL_A]=a<0?0:255; else out[LICE_PIXEL_A] = (LICE_pixel_chan) (a);
}

// Optimization when a=255 and alpha=1.0f, useful for doing a big vector drawn fill or something.
// This could be called _LICE_PutPixel but that would probably be confusing.
class _LICE_CombinePixelsClobber
{
public:
  static inline void doPix(LICE_pixel_chan *dest, int r, int g, int b, int a, int alpha)    // alpha is ignored.
  {
    _LICE_MakePixel(dest, r, g, b, a);
  }
};

class _LICE_CombinePixelsCopy 
{
public:
  static inline void doPix(LICE_pixel_chan *dest, int r, int g, int b, int a, int alpha)
  {
    int a2=(256-alpha);

    // we could check alpha=0 here, but the caller should (since alpha is usually used for static alphas)
    _LICE_MakePixel(dest,
      (dest[LICE_PIXEL_R]*a2+r*alpha)/256,
      (dest[LICE_PIXEL_G]*a2+g*alpha)/256,
      (dest[LICE_PIXEL_B]*a2+b*alpha)/256,
      (dest[LICE_PIXEL_A]*a2+a*alpha)/256);
  }
};

class _LICE_CombinePixelsCopySourceAlpha
{
public:
  static inline void doPix(LICE_pixel_chan *dest, int r, int g, int b, int a, int alpha)
  {
    if (a)
    {
      alpha = (alpha*a)/256;

      int a2=(255-alpha);

      _LICE_MakePixel(dest,
        (dest[LICE_PIXEL_R]*a2+r*alpha)/256,
        (dest[LICE_PIXEL_G]*a2+g*alpha)/256,
        (dest[LICE_PIXEL_B]*a2+b*alpha)/256,
        (dest[LICE_PIXEL_A]*a2+a*alpha)/256);  
    }
  }
};
class _LICE_CombinePixelsAdd
{
public:
  static inline void doPix(LICE_pixel_chan *dest, int r, int g, int b, int a, int alpha)
  { 
    // we could check alpha=0 here, but the caller should (since alpha is usually used for static alphas)

    _LICE_MakePixel(dest,
      dest[LICE_PIXEL_R]+(r*alpha)/256,
      dest[LICE_PIXEL_G]+(g*alpha)/256,
      dest[LICE_PIXEL_B]+(b*alpha)/256,
      dest[LICE_PIXEL_A]+(a*alpha)/256);

  }
};
class _LICE_CombinePixelsAddSourceAlpha
{
public:
  static inline void doPix(LICE_pixel_chan *dest, int r, int g, int b, int a, int alpha)
  { 
    if (a)
    {
      alpha=(alpha*a)/256;
      _LICE_MakePixel(dest,
        dest[LICE_PIXEL_R]+(r*alpha)/256,
        dest[LICE_PIXEL_G]+(g*alpha)/256,
        dest[LICE_PIXEL_B]+(b*alpha)/256,
        dest[LICE_PIXEL_A]+(a*alpha)/256);
    }
  }
};


class _LICE_CombinePixelsColorDodge
{
public:
  static inline void doPix(LICE_pixel_chan *dest, int r, int g, int b, int a, int alpha)
  { 
      int src_r = 255-r*alpha/256;
      int src_g = 255-g*alpha/256;
      int src_b = 255-b*alpha/256;
      int src_a = 255-a*alpha/256;

      _LICE_MakePixel(dest,
        src_r > 1 ? 256*dest[LICE_PIXEL_R] / src_r : 256*dest[LICE_PIXEL_R],
        src_g > 1 ? 256*dest[LICE_PIXEL_G] / src_g : 256*dest[LICE_PIXEL_G],
        src_b > 1 ? 256*dest[LICE_PIXEL_B] / src_b : 256*dest[LICE_PIXEL_B],
        src_a > 1 ? 256*dest[LICE_PIXEL_A] / src_a : 256*dest[LICE_PIXEL_A]);
  }
};

class _LICE_CombinePixelsColorDodgeSourceAlpha
{
public:
  static inline void doPix(LICE_pixel_chan *dest, int r, int g, int b, int a, int alpha)
  { 
      alpha=(alpha*a)/256;
      int src_r = 255-r*alpha/256;
      int src_g = 255-g*alpha/256;
      int src_b = 255-b*alpha/256;
      int src_a = 255-a*alpha/256;

      _LICE_MakePixel(dest,
        src_r > 1 ? 256*dest[LICE_PIXEL_R] / src_r : 256*dest[LICE_PIXEL_R],
        src_g > 1 ? 256*dest[LICE_PIXEL_G] / src_g : 256*dest[LICE_PIXEL_G],
        src_b > 1 ? 256*dest[LICE_PIXEL_B] / src_b : 256*dest[LICE_PIXEL_B],
        src_a > 1 ? 256*dest[LICE_PIXEL_A] / src_a : 256*dest[LICE_PIXEL_A]);
  }
};


//#define __LICE__ACTION(comb) templateclass<comb>::function(parameters)
//__LICE_ACTIONBYMODE(mode,alpha);
//#undef __LICE__ACTION


#define __LICE_ACTIONBYMODE(mode,alpha) \
     switch ((mode)&LICE_BLIT_MODE_MASK) { \
      case LICE_BLIT_MODE_COPY: \
        if ((alpha)>0.0) {  \
          if ((mode)&LICE_BLIT_USE_ALPHA) __LICE__ACTION(_LICE_CombinePixelsCopySourceAlpha); \
          else __LICE__ACTION(_LICE_CombinePixelsCopy); \
        } \
      break;  \
      case LICE_BLIT_MODE_ADD:  \
        if ((alpha)!=0.0) { \
          if ((mode)&LICE_BLIT_USE_ALPHA) __LICE__ACTION(_LICE_CombinePixelsAddSourceAlpha);  \
          else __LICE__ACTION(_LICE_CombinePixelsAdd);  \
        } \
      break;  \
      case LICE_BLIT_MODE_DODGE: \
        if ((alpha)!=0.0) { \
          if ((mode)&LICE_BLIT_USE_ALPHA) __LICE__ACTION(_LICE_CombinePixelsColorDodgeSourceAlpha); \
          else __LICE__ACTION(_LICE_CombinePixelsColorDodge); \
        } \
      break;  \
    }

// used by GradRect, etc
#define __LICE_ACTIONBYMODE_NOALPHA(mode) \
     switch ((mode)&LICE_BLIT_MODE_MASK) { \
      case LICE_BLIT_MODE_COPY: __LICE__ACTION(_LICE_CombinePixelsCopy); break;  \
      case LICE_BLIT_MODE_ADD: __LICE__ACTION(_LICE_CombinePixelsAdd); break;  \
    }

// Vector drawing with a single color can be optimized for copy mode if the color has alpha = 255.
#define __LICE_ACTIONBYMODE_NOALPHACHANGE(mode,color,alpha) \
    if (LICE_GETA(color)==255 && (alpha)==1.0f) __LICE__ACTION(_LICE_CombinePixelsClobber); \
    else __LICE_ACTIONBYMODE(mode,alpha);
        

#endif