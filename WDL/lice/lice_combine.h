#ifndef _LICE_COMBINE_H_
#define _LICE_COMBINE_H_


static void inline _LICE_MakePixel(LICE_pixel_chan *out, int r, int g, int b, int a)
{
  if (r<1) out[LICE_PIXEL_R]=0; else if (r>=255) out[LICE_PIXEL_R]=255; else out[LICE_PIXEL_R] = (LICE_pixel_chan) (r);
  if (g<1) out[LICE_PIXEL_G]=0; else if (g>=255) out[LICE_PIXEL_G]=255; else out[LICE_PIXEL_G] = (LICE_pixel_chan) (g);
  if (b<1) out[LICE_PIXEL_B]=0; else if (b>=255) out[LICE_PIXEL_B]=255; else out[LICE_PIXEL_B] = (LICE_pixel_chan) (b);
  if (a<1) out[LICE_PIXEL_A]=0; else if (a>=255) out[LICE_PIXEL_A]=255; else out[LICE_PIXEL_A] = (LICE_pixel_chan) (a);
}


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


#endif