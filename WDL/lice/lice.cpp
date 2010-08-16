/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice.cpp (LICE core processing)
  See lice.h for license and other information
*/


#include "lice.h"
#include <math.h>


bool LICE_MemBitmap::resize(int w, int h)
{
  if (w!=m_width||h!=m_height)
  {
    int sz=(m_width=w)*(m_height=h)*sizeof(LICE_pixel);

    if (sz<=0) { free(m_fb); m_fb=0; }
    else if (!m_fb) m_fb=(LICE_pixel*)malloc(sz);
    else 
    {
      void *op=m_fb;
      if (!(m_fb=(LICE_pixel*)realloc(m_fb,sz)))
      {
        free(op);
        m_fb=(LICE_pixel*)malloc(sz);
      }
    }

    return true;
  }
  return false;
}



LICE_SysBitmap::LICE_SysBitmap(int w, int h)
{
  m_dc = CreateCompatibleDC(NULL);
  m_bitmap = 0;
  m_oldbitmap = 0;
  m_bits=0;
  m_width=m_height=0;

  resize(w,h);
}


LICE_SysBitmap::~LICE_SysBitmap()
{
  if(m_bitmap) DeleteObject(m_bitmap);
  DeleteDC(m_dc);
}

bool LICE_SysBitmap::resize(int w, int h)
{
  if (m_width==w && m_height == h) return false;

  m_width=w;
  m_height=h;

  if (!w || !h) return false;

  if (m_oldbitmap) 
  {
    SelectObject(m_dc,m_oldbitmap);
    m_oldbitmap=0;
  }
  if (m_bitmap) DeleteObject(m_bitmap);
  m_bits=0;

  BITMAPINFO pbmInfo = {0,};
  pbmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pbmInfo.bmiHeader.biWidth = m_width;
  pbmInfo.bmiHeader.biHeight = -m_height;
  pbmInfo.bmiHeader.biPlanes = 1;
  pbmInfo.bmiHeader.biBitCount = sizeof(LICE_pixel)*8;
  pbmInfo.bmiHeader.biCompression = BI_RGB;
  m_bitmap = CreateDIBSection( NULL, &pbmInfo, DIB_RGB_COLORS, (void **)&m_bits, NULL, 0);

  m_oldbitmap=SelectObject(m_dc, m_bitmap);

  return true;
}



void LICE_Copy(LICE_IBitmap *dest, LICE_IBitmap *src) // resizes dest
{
  if (src&&dest)
  {
    dest->resize(src->getWidth(),src->getHeight());
    LICE_Blit(dest,src,NULL,0,0,1.0,LICE_BLIT_MODE_COPY);
  }
}

static inline void BilinearFilter(float *r, float *g, float *b, float *a, LICE_pixel_chan *pin, LICE_pixel_chan *pinnext, double xfrac, double yfrac)
{
  double f1=(1.0-xfrac)*(1.0-yfrac);
  double f2=xfrac*(1.0-yfrac);
  double f3=(1.0-xfrac)*yfrac;
  double f4=xfrac*yfrac;
  *r=(float) (pin[LICE_PIXEL_R]*f1 + pin[4+LICE_PIXEL_R]*f2 + pinnext[LICE_PIXEL_R]*f3 + pinnext[4+LICE_PIXEL_R]*f4);
  *g=(float) (pin[LICE_PIXEL_G]*f1 + pin[4+LICE_PIXEL_G]*f2 + pinnext[LICE_PIXEL_G]*f3 + pinnext[4+LICE_PIXEL_G]*f4);
  *b=(float) (pin[LICE_PIXEL_B]*f1 + pin[4+LICE_PIXEL_B]*f2 + pinnext[LICE_PIXEL_B]*f3 + pinnext[4+LICE_PIXEL_B]*f4);
  *a=(float) (pin[LICE_PIXEL_A]*f1 + pin[4+LICE_PIXEL_A]*f2 + pinnext[LICE_PIXEL_A]*f3 + pinnext[4+LICE_PIXEL_A]*f4);
}

template<class COMBFUNC> class _LICE_Template_Blit
{
  public:

    static void deltaBlit(LICE_pixel_chan *dest, LICE_pixel_chan *src, int w, int h, 
                          double srcx, double srcy, double dsdx, double dtdx, double dsdy, double dtdy,
                          double src_left, double src_top, double src_right, double src_bottom,
                          int src_span, int dest_span, float alpha, bool sourceAlpha, int filtermode)
    {
      if (sourceAlpha)
      {
        alpha *= 1.0f/255.0f;
        if (filtermode == LICE_BLIT_FILTER_BILINEAR)
        {
          while (h--)
          {
            double thisx=srcx;
            double thisy=srcy;
            LICE_pixel_chan *pout=dest;
            int n=w;
            while (n--)
            {
              if (thisy >= src_top && thisy < src_bottom-1 && thisx >= src_left && thisx < src_right-1)
              {
                int cury = (int) thisy;
                int curx = (int) thisx;
                double yfrac=thisy-cury;

                LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);

                float r,g,b,a;
                BilinearFilter(&r,&g,&b,&a,pin,pin+src_span,thisx-curx,yfrac);

                COMBFUNC::doPixSourceAlpha(pout,r,g,b,a,alpha);
              }

              pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
              thisx+=dsdx;
              thisy+=dtdx;
            }
            dest+=dest_span;
            srcx+=dsdy;
            srcy+=dtdy;
          }
        }
        else
        {
          while (h--)
          {
            double thisx=srcx;
            double thisy=srcy;
            LICE_pixel_chan *pout=dest;
            int n=w;
            while (n--)
            {
              if (thisy >= src_top && thisy < src_bottom && thisx >= src_left && thisx < src_right)
              {
                int cury = (int) thisy;
                int curx = (int) thisx;
                double yfrac=thisy-cury;

                LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);

                COMBFUNC::doPixSourceAlpha(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],alpha);
              }

              pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
              thisx+=dsdx;
              thisy+=dtdx;
            }
            dest+=dest_span;
            srcx+=dsdy;
            srcy+=dtdy;
          }
        }
      }
      else
      {
        if (filtermode == LICE_BLIT_FILTER_BILINEAR)
        {
          while (h--)
          {
            double thisx=srcx;
            double thisy=srcy;
            LICE_pixel_chan *pout=dest;
            int n=w;
            while (n--)
            {
              if (thisy >= src_top && thisy < src_bottom-1 && thisx >= src_left && thisx < src_right-1)
              {
                int cury = (int) thisy;
                int curx = (int) thisx;
                double yfrac=thisy-cury;

                LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);

                float r,g,b,a;
                BilinearFilter(&r,&g,&b,&a,pin,pin+src_span,thisx-curx,yfrac);

                COMBFUNC::doPix(pout,r,g,b,a,alpha);
              }

              pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
              thisx+=dsdx;
              thisy+=dtdx;
            }
            dest+=dest_span;
            srcx+=dsdy;
            srcy+=dtdy;
          }
        }
        else
        {
          while (h--)
          {
            double thisx=srcx;
            double thisy=srcy;
            LICE_pixel_chan *pout=dest;
            int n=w;
            while (n--)
            {
              if (thisy >= src_top && thisy < src_bottom && thisx >= src_left && thisx < src_right)
              {
                int cury = (int) thisy;
                int curx = (int) thisx;
                double yfrac=thisy-cury;

                LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);

                COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],alpha);
              }

              pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
              thisx+=dsdx;
              thisy+=dtdx;
            }
            dest+=dest_span;
            srcx+=dsdy;
            srcy+=dtdy;
          }
        }
      }
    }



    static void scaleBlit(LICE_pixel_chan *dest, LICE_pixel_chan *src, int w, int h, 
                          double srcx, double srcy, double dx, double dy, int srcw, int srch,     
                          int src_span, int dest_span, float alpha, bool sourceAlpha, int filtermode)
    {
      if (sourceAlpha)
      {
        alpha *= 1.0f/255.0f;
        if (filtermode == LICE_BLIT_FILTER_BILINEAR)
        {
          while (h--)
          {
            int cury = (int) srcy;
            if (cury >= 0 && cury < srch-1)
            {
              double yfrac=srcy-cury;
              double curx=srcx;
              LICE_pixel_chan *inptr=src + cury * src_span;
              LICE_pixel_chan *pout=dest;
              int n=w;
              while (n--)
              {
                int offs=(int) curx;
                if (offs>=0 && offs<srcw-1)
                {
                  LICE_pixel_chan *pin = inptr + offs*sizeof(LICE_pixel);

                  float r,g,b,a;
                  BilinearFilter(&r,&g,&b,&a,pin,pin+src_span,curx-offs,yfrac);

                  COMBFUNC::doPixSourceAlpha(pout,r,g,b,a,alpha);
                }

                pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
                curx+=dx;
              }
            }
            dest+=dest_span;
            srcy+=dy;
          }
        }
        else
        {
          while (h--)
          {
            int cury = (int) srcy;
            if (cury >= 0 && cury < srch)
            {
              double curx=srcx;
              LICE_pixel_chan *inptr=src + cury * src_span;
              LICE_pixel_chan *pout=dest;
              int n=w;
              while (n--)
              {
                int offs=(int) curx;
                if (offs>=0 && offs<srcw)
                {
                  LICE_pixel_chan *pin = inptr + offs*sizeof(LICE_pixel);

                  COMBFUNC::doPixSourceAlpha(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],alpha);
                }

                pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
                curx+=dx;
              }
            }
            dest+=dest_span;
            srcy+=dy;
          }
        }
      }
      else
      {
        if (filtermode == LICE_BLIT_FILTER_BILINEAR)
        {
          while (h--)
          {
            int cury = (int) srcy;
            if (cury >= 0 && cury < srch-1)
            {
              double yfrac=srcy-cury;
              double curx=srcx;
              LICE_pixel_chan *inptr=src + cury * src_span;
              LICE_pixel_chan *pout=dest;
              int n=w;
              while (n--)
              {
                int offs=(int) curx;
                if (offs>=0 && offs<srcw-1)
                {
                  LICE_pixel_chan *pin = inptr + offs*sizeof(LICE_pixel);

                  float r,g,b,a;
                  BilinearFilter(&r,&g,&b,&a,pin,pin+src_span,curx-offs,yfrac);

                  COMBFUNC::doPix(pout,r,g,b,a,alpha);
                }

                pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
                curx+=dx;
              }
            }
            dest+=dest_span;
            srcy+=dy;
          }
        }
        else
        {
          while (h--)
          {
            int cury = (int) srcy;
            if (cury >= 0 && cury < srch)
            {
              double curx=srcx;
              LICE_pixel_chan *inptr=src + cury * src_span;
              LICE_pixel_chan *pout=dest;
              int n=w;
              while (n--)
              {
                int offs=(int) curx;
                if (offs>=0 && offs<srcw)
                {
                  LICE_pixel_chan *pin = inptr + offs*sizeof(LICE_pixel);

                  COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],alpha);
                }

                pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
                curx+=dx;
              }
            }
            dest+=dest_span;
            srcy+=dy;
          }
        }
      }
    }


    static void blit(LICE_pixel_chan *dest, LICE_pixel_chan *src, int w, int h, int src_span, int dest_span, float alpha, bool sourceAlpha)
    {
      if (sourceAlpha)
      {
        alpha *= 1.0f/255.0f;
        while (h-->0)
        {
          int n=w;
          LICE_pixel_chan *pin=src;
          LICE_pixel_chan *pout=dest;
          while (n--)
          {

            COMBFUNC::doPixSourceAlpha(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],alpha);

            pin += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
            pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
          }
          dest+=dest_span;
          src += src_span;
        }
      }
      else
      {
        while (h-->0)
        {
          int n=w;
          LICE_pixel_chan *pin=src;
          LICE_pixel_chan *pout=dest;
          while (n--)
          {

            COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],alpha);

            pin += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
            pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
          }
          dest+=dest_span;
          src += src_span;
        }
      }
    }
};


static void inline FloatsToPixel(LICE_pixel_chan *out, float r, float g, float b, float a)
{
  if (r<0.5) out[LICE_PIXEL_R]=0; else if (r>=254.5) out[LICE_PIXEL_R]=255; else out[LICE_PIXEL_R] = (LICE_pixel_chan) (r+0.5);
  if (g<0.5) out[LICE_PIXEL_G]=0; else if (g>=254.5) out[LICE_PIXEL_G]=255; else out[LICE_PIXEL_G] = (LICE_pixel_chan) (g+0.5);
  if (b<0.5) out[LICE_PIXEL_B]=0; else if (b>=254.5) out[LICE_PIXEL_B]=255; else out[LICE_PIXEL_B] = (LICE_pixel_chan) (b+0.5);
  if (a<0.5) out[LICE_PIXEL_A]=0; else if (a>=254.5) out[LICE_PIXEL_A]=255; else out[LICE_PIXEL_A] = (LICE_pixel_chan) (a+0.5);
}

class _LICE_CombinePixelsCopy
{
public:
  static inline void doPix(LICE_pixel_chan *dest, float r, float g, float b, float a, float alpha)
  {
    float ialpha=1.0f-alpha;
    FloatsToPixel(dest,
      (float)dest[LICE_PIXEL_R]*ialpha+r*alpha,
      (float)dest[LICE_PIXEL_G]*ialpha+g*alpha,
      (float)dest[LICE_PIXEL_B]*ialpha+b*alpha,
      (float)dest[LICE_PIXEL_A]*ialpha+a*alpha);
  }
  static inline void doPixSourceAlpha(LICE_pixel_chan *dest, float r, float g, float b, float a, float alpha)
  {
    alpha *= a;
    float ialpha=1.0f-alpha;
    FloatsToPixel(dest,
      (float)dest[LICE_PIXEL_R]*ialpha+r*alpha,
      (float)dest[LICE_PIXEL_G]*ialpha+g*alpha,
      (float)dest[LICE_PIXEL_B]*ialpha+b*alpha,
      (float)dest[LICE_PIXEL_A]*ialpha+a*alpha);
  }
};
class _LICE_CombinePixelsAdd
{
public:
  static inline void doPix(LICE_pixel_chan *dest, float r, float g, float b, float a, float alpha)
  { 
    FloatsToPixel(dest,
      (float)dest[LICE_PIXEL_R]+r*alpha,
      (float)dest[LICE_PIXEL_G]+g*alpha,
      (float)dest[LICE_PIXEL_B]+b*alpha,
      (float)dest[LICE_PIXEL_A]+a*alpha);
  }
  static inline void doPixSourceAlpha(LICE_pixel_chan *dest, float r, float g, float b, float a, float alpha)
  { 
    alpha *= a;
    FloatsToPixel(dest,
      (float)dest[LICE_PIXEL_R]+r*alpha,
      (float)dest[LICE_PIXEL_G]+g*alpha,
      (float)dest[LICE_PIXEL_B]+b*alpha,
      (float)dest[LICE_PIXEL_A]+a*alpha);
  }
};


void LICE_Blit(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, RECT *srcrect, float alpha, int mode)
{
  if (!dest || !src) return;

  RECT sr={0,0,src->getWidth(),src->getHeight()};
  if (srcrect) 
  {
    sr=*srcrect;    
    if (sr.left < 0) sr.left=0;
    if (sr.top < 0) sr.top=0;
    if (sr.right > src->getWidth()) sr.right=src->getWidth();
    if (sr.bottom > src->getHeight()) sr.bottom = src->getHeight();
  }

  // clip to output
  if (dstx < 0) { sr.left -= dstx; dstx=0; }
  if (dsty < 0) { sr.top -= dsty; dsty=0; }  
  if (dstx+sr.right-sr.left > dest->getWidth()) sr.right = sr.left + (dest->getWidth()-dstx);
  if (dsty+sr.bottom-sr.top > dest->getHeight()) sr.bottom = sr.top + (dest->getHeight()-dsty);

  // ignore blits that are 0
  if (sr.right <= sr.left || sr.bottom <= sr.top) return;

  int dest_span=dest->getRowSpan()*sizeof(LICE_pixel);
  int src_span=src->getRowSpan()*sizeof(LICE_pixel);
  LICE_pixel_chan *psrc = (LICE_pixel_chan *)src->getBits();
  LICE_pixel_chan *pdest = (LICE_pixel_chan *)dest->getBits();
  if (!psrc || !pdest) return;

  psrc += sr.left*sizeof(LICE_pixel) + sr.top*src_span;
  pdest += dstx*sizeof(LICE_pixel) + dsty*dest_span;

  int i=sr.bottom-sr.top;
  int cpsize=sr.right-sr.left;

  // todo: alpha channel use, alpha paramter use
  switch (mode&LICE_BLIT_MODE_MASK)
  {
    case LICE_BLIT_MODE_COPY:
      if (alpha>0.0)
      {
        if (alpha<1.0||(mode&LICE_BLIT_USE_ALPHA))
          _LICE_Template_Blit<_LICE_CombinePixelsCopy>::blit(pdest,psrc,cpsize,i,src_span,dest_span,alpha,!!(mode&LICE_BLIT_USE_ALPHA));
        else
        {
          while (i-->0)
          {
            memcpy(pdest,psrc,cpsize*sizeof(LICE_pixel));
            pdest+=dest_span;
            psrc += src_span;
          }

        }
      }
    break;
    case LICE_BLIT_MODE_ADD:
      _LICE_Template_Blit<_LICE_CombinePixelsAdd>::blit(pdest,psrc,cpsize,i,src_span,dest_span,alpha,!!(mode&LICE_BLIT_USE_ALPHA));
    break;
  }
}



void LICE_ScaledBlit(LICE_IBitmap *dest, LICE_IBitmap *src, 
                     int dstx, int dsty, int dstw, int dsth, 
                     float srcx, float srcy, float srcw, float srch, 
                     float alpha, int mode)
{
  if (!dest || !src || !dstw || !dsth) return;

  if (dstw<0)
  {
    dstw=-dstw;
    dstx-=dstw;
    srcx+=srcw;  
    srcw=-srcw;
  }
  if (dsth<0)
  {
    dsth=-dsth;
    dsty-=dsth;
    srcy+=srch;
    srch=-srch;
  }

  double xadvance = srcw / dstw;
  double yadvance = srch / dsth;

  if (dstx < 0) { srcx -= (float) (dstx*xadvance); dstw+=dstx; dstx=0; }
  if (dsty < 0) { srcy -= (float) (dsty*yadvance); dsth+=dsty; dsty=0; }  
  if (dstx+dstw > dest->getWidth()) dstw=dest->getWidth()-dstx;
  if (dsty+dsth > dest->getHeight()) dsth=dest->getHeight()-dsty;

  if (dstw<1 || dsth<1) return;


  int dest_span=dest->getRowSpan()*sizeof(LICE_pixel);
  int src_span=src->getRowSpan()*sizeof(LICE_pixel);

  LICE_pixel_chan *psrc = (LICE_pixel_chan *)src->getBits();
  LICE_pixel_chan *pdest = (LICE_pixel_chan *)dest->getBits();
  if (!psrc || !pdest) return;

  pdest += dstx*sizeof(LICE_pixel) + dsty*dest_span;


  switch (mode&LICE_BLIT_MODE_MASK)
  {
    case LICE_BLIT_MODE_COPY:
      if (alpha>0.0)
      {
        _LICE_Template_Blit<_LICE_CombinePixelsCopy>::scaleBlit(pdest,psrc,dstw,dsth,srcx,srcy,xadvance,yadvance,src->getWidth(),src->getHeight(),src_span,dest_span,alpha,!!(mode&LICE_BLIT_USE_ALPHA),mode&LICE_BLIT_FILTER_MASK);
      }
    break;
    case LICE_BLIT_MODE_ADD:
      _LICE_Template_Blit<_LICE_CombinePixelsAdd>::scaleBlit(pdest,psrc,dstw,dsth,srcx,srcy,xadvance,yadvance,src->getWidth(),src->getHeight(),src_span,dest_span,alpha,!!(mode&LICE_BLIT_USE_ALPHA),mode&LICE_BLIT_FILTER_MASK);
    break;
  }
}

void LICE_DeltaBlit(LICE_IBitmap *dest, LICE_IBitmap *src, 
                    int dstx, int dsty, int dstw, int dsth, 
                    float srcx, float srcy, float srcw, float srch, 
                    double dsdx, double dtdx, double dsdy, double dtdy,
                    bool cliptosourcerect, float alpha, int mode)
{
  if (!dest || !src || !dstw || !dsth) return;

  double src_top=0.0,src_left=0.0,src_right=src->getWidth(),src_bottom=src->getHeight();

  if (cliptosourcerect)
  {
    if (srcx > src_left) src_left=srcx;
    if (srcy > src_top) src_top=srcy;
    if (srcx+srcw < src_right) src_right=srcx+srcw;
    if (srcy+srch < src_bottom) src_bottom=srcy+srch;
  }

  if (dstw<0)
  {
    dstw=-dstw;
    dstx-=dstw;
    srcx+=srcw;  
    srcw=-srcw;
  }
  if (dsth<0)
  {
    dsth=-dsth;
    dsty-=dsth;
    srcy+=srch;
    srch=-srch;
  }


  if (dstx < 0) 
  { 
    srcx -= (float) (dstx*dsdx); 
    srcy -= (float) (dstx*dtdx);
    dstw+=dstx; 
    dstx=0; 
  }
  if (dsty < 0) 
  { 
    srcy -= (float) (dsty*dtdy);
    srcx -= (float) (dsty*dsdy);
    dsth+=dsty; 
    dsty=0; 
  }  
  if (dstx+dstw > dest->getWidth()) dstw=dest->getWidth()-dstx;
  if (dsty+dsth > dest->getHeight()) dsth=dest->getHeight()-dsty;

  if (dstw<1 || dsth<1) return;


  int dest_span=dest->getRowSpan()*sizeof(LICE_pixel);
  int src_span=src->getRowSpan()*sizeof(LICE_pixel);

  LICE_pixel_chan *psrc = (LICE_pixel_chan *)src->getBits();
  LICE_pixel_chan *pdest = (LICE_pixel_chan *)dest->getBits();
  if (!psrc || !pdest) return;

  pdest += dstx*sizeof(LICE_pixel) + dsty*dest_span;

  switch (mode&LICE_BLIT_MODE_MASK)
  {
    case LICE_BLIT_MODE_COPY:
      if (alpha>0.0)
      {
        _LICE_Template_Blit<_LICE_CombinePixelsCopy>::deltaBlit(pdest,psrc,dstw,dsth,srcx,srcy,dsdx,dtdx,dsdy,dtdy,src_left,src_top,src_right,src_bottom,src_span,dest_span,alpha,!!(mode&LICE_BLIT_USE_ALPHA),mode&LICE_BLIT_FILTER_MASK);
      }
    break;
    case LICE_BLIT_MODE_ADD:
      _LICE_Template_Blit<_LICE_CombinePixelsAdd>::deltaBlit(pdest,psrc,dstw,dsth,srcx,srcy,dsdx,dtdx,dsdy,dtdy,src_left,src_top,src_right,src_bottom,src_span,dest_span,alpha,!!(mode&LICE_BLIT_USE_ALPHA),mode&LICE_BLIT_FILTER_MASK);
    break;
  }
}
                      


void LICE_RotatedBlit(LICE_IBitmap *dest, LICE_IBitmap *src, 
                      int dstx, int dsty, int dstw, int dsth, 
                      float srcx, float srcy, float srcw, float srch, 
                      float angle, 
                      bool cliptosourcerect, float alpha, int mode, float rotxcent, float rotycent)
{
  if (!dest || !src || !dstw || !dsth) return;

  double src_top=0.0,src_left=0.0,src_right=src->getWidth(),src_bottom=src->getHeight();

  if (cliptosourcerect)
  {
    if (srcx > src_left) src_left=srcx;
    if (srcy > src_top) src_top=srcy;
    if (srcx+srcw < src_right) src_right=srcx+srcw;
    if (srcy+srch < src_bottom) src_bottom=srcy+srch;
  }

  if (dstw<0)
  {
    dstw=-dstw;
    dstx-=dstw;
    srcx+=srcw;  
    srcw=-srcw;
  }
  if (dsth<0)
  {
    dsth=-dsth;
    dsty-=dsth;
    srcy+=srch;
    srch=-srch;
  }

  double cosa=cos(angle);
  double sina=sin(angle);

  double xsc=srcw / dstw;
  double ysc=srch / dsth;

  double dsdx = xsc * cosa;
  double dtdy = ysc * cosa;
  double dsdy = xsc * sina;
  double dtdx = ysc * -sina;

  srcx -= (float) (0.5 * (dstw*dsdx + dsth*dsdy - srcw) - rotxcent);
  srcy -= (float) (0.5 * (dsth*dtdy + dstw*dtdx - srch) - rotycent);

  if (dstx < 0) 
  { 
    srcx -= (float) (dstx*dsdx); 
    srcy -= (float) (dstx*dtdx);
    dstw+=dstx; 
    dstx=0; 
  }
  if (dsty < 0) 
  { 
    srcy -= (float) (dsty*dtdy);
    srcx -= (float) (dsty*dsdy);
    dsth+=dsty; 
    dsty=0; 
  }  
  if (dstx+dstw > dest->getWidth()) dstw=dest->getWidth()-dstx;
  if (dsty+dsth > dest->getHeight()) dsth=dest->getHeight()-dsty;

  if (dstw<1 || dsth<1) return;


  int dest_span=dest->getRowSpan()*sizeof(LICE_pixel);
  int src_span=src->getRowSpan()*sizeof(LICE_pixel);

  LICE_pixel_chan *psrc = (LICE_pixel_chan *)src->getBits();
  LICE_pixel_chan *pdest = (LICE_pixel_chan *)dest->getBits();
  if (!psrc || !pdest) return;

  pdest += dstx*sizeof(LICE_pixel) + dsty*dest_span;

  switch (mode&LICE_BLIT_MODE_MASK)
  {
    case LICE_BLIT_MODE_COPY:
      if (alpha>0.0)
      {
        _LICE_Template_Blit<_LICE_CombinePixelsCopy>::deltaBlit(pdest,psrc,dstw,dsth,srcx,srcy,dsdx,dtdx,dsdy,dtdy,src_left,src_top,src_right,src_bottom,src_span,dest_span,alpha,!!(mode&LICE_BLIT_USE_ALPHA),mode&LICE_BLIT_FILTER_MASK);
      }
    break;
    case LICE_BLIT_MODE_ADD:
      _LICE_Template_Blit<_LICE_CombinePixelsAdd>::deltaBlit(pdest,psrc,dstw,dsth,srcx,srcy,dsdx,dtdx,dsdy,dtdy,src_left,src_top,src_right,src_bottom,src_span,dest_span,alpha,!!(mode&LICE_BLIT_USE_ALPHA),mode&LICE_BLIT_FILTER_MASK);
    break;
  }

}

void LICE_Clear(LICE_IBitmap *dest, LICE_pixel color)
{
  if (!dest) return;
  LICE_pixel *p=dest->getBits();
  int h=dest->getHeight();
  int w=dest->getWidth();
  int sp=dest->getRowSpan();
  if (!p || w<1 || h<1 || sp<1) return;

  while (h-->0)
  {
    int n=w;
    while (n--) *p++ = color;
    p+=sp-w;
  }
}

void LICE_ClearRect(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel mask, LICE_pixel orbits)
{
  if (!dest) return;
  LICE_pixel *p=dest->getBits();

  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x+w>dest->getWidth()) w=dest->getWidth()-x;
  if (y+h>dest->getHeight()) h=dest->getHeight()-y;

  int sp=dest->getRowSpan();
  if (!p || w<1 || h<1 || sp<1) return;

  p += sp*y + x;
  while (h-->0)
  {
    int n=w;
    while (n--) 
    {
      *p = (*p&mask)|orbits;
      p++;
    }
    p+=sp-w;
  }
}


void LICE_SetAlphaFromColorMask(LICE_IBitmap *dest, LICE_pixel color)
{
  if (!dest) return;
  LICE_pixel *p=dest->getBits();
  int h=dest->getHeight();
  int w=dest->getWidth();
  int sp=dest->getRowSpan();
  if (!p || w<1 || h<1 || sp<1) return;

  while (h-->0)
  {
    int n=w;
    while (n--) 
    {
      if ((*p&0xffffff) == color) *p&=~0xffffff;
      else *p|=0xff000000;
      p++;
    }
    p+=sp-w;
  }
}