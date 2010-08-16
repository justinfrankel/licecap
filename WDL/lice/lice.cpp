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
#ifdef _WIN32
  m_dc = CreateCompatibleDC(NULL);
  m_bitmap = 0;
  m_oldbitmap = 0;
#else
  m_dc=0;
#endif
  m_bits=0;
  m_width=m_height=0;

  resize(w,h);
}


LICE_SysBitmap::~LICE_SysBitmap()
{
#ifdef _WIN32
  if(m_bitmap) DeleteObject(m_bitmap);
  DeleteDC(m_dc);
#else
  if (m_dc)
    WDL_GDP_DeleteContext(m_dc);
#endif
}

bool LICE_SysBitmap::resize(int w, int h)
{
  if (m_width==w && m_height == h) return false;

  m_width=w;
  m_height=h;

  if (!w || !h) return false;

#ifdef _WIN32
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
  pbmInfo.bmiHeader.biHeight = isFlipped()?m_height:-m_height;
  pbmInfo.bmiHeader.biPlanes = 1;
  pbmInfo.bmiHeader.biBitCount = sizeof(LICE_pixel)*8;
  pbmInfo.bmiHeader.biCompression = BI_RGB;
  m_bitmap = CreateDIBSection( NULL, &pbmInfo, DIB_RGB_COLORS, (void **)&m_bits, NULL, 0);

  m_oldbitmap=SelectObject(m_dc, m_bitmap);
#else
  if (m_dc) WDL_GDP_DeleteContext(m_dc);
  m_dc=WDL_GDP_CreateMemContext(0,w,h);
  m_bits=(LICE_pixel*)SWELL_GetCtxFrameBuffer(m_dc);
#endif

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

template<class COMBFUNC> class _LICE_Template_Blit
{
  public:
    static void gradBlit(LICE_pixel_chan *dest, int w, int h, 
                         int ir, int ig, int ib, int ia,
                         int drdx, int dgdx, int dbdx, int dadx,
                         int drdy, int dgdy, int dbdy, int dady,
                         int dest_span)
    {
      while (h--)
      {
        int r=ir,g=ig,b=ib,a=ia;
        ir+=drdy; ig+=dgdy; ib+=dbdy; ia+=dady;
        LICE_pixel_chan *pout=dest;
        int n=w;
        while (n--)
        {
          int ia=a/65536;
          COMBFUNC::doPix(pout,r/65536,g/65536,b/65536,ia,ia);          
          pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
          r+=drdx; g+=dgdx; b+=dbdx; a+=dadx;
        }
        dest+=dest_span;
      }
    }

    static void deltaBlit(LICE_pixel_chan *dest, LICE_pixel_chan *src, int w, int h, 
                          double srcx, double srcy, double dsdx, double dtdx, double dsdy, double dtdy,
                          double src_left, double src_top, double src_right, double src_bottom,
                          int src_span, int dest_span, float alpha, int filtermode)
    {
      int ia=(int)(alpha*256.0);
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

              int r,g,b,a;
              __LICE_BilinearFilter(&r,&g,&b,&a,pin,pin+src_span,thisx-curx,yfrac);

              COMBFUNC::doPix(pout,r,g,b,a,ia);
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

              LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);

              COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],ia);
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



    static void scaleBlit(LICE_pixel_chan *dest, LICE_pixel_chan *src, int w, int h, 
                          double srcx, double srcy, double dx, double dy, int srcw, int srch,     
                          int src_span, int dest_span, float alpha, int filtermode)
    {
      int ia=(int)(alpha*256.0);
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

                int r,g,b,a;
                __LICE_BilinearFilter(&r,&g,&b,&a,pin,pin+src_span,curx-offs,yfrac);

                COMBFUNC::doPix(pout,r,g,b,a,ia);
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

                COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],ia);
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


    static void blit(LICE_pixel_chan *dest, LICE_pixel_chan *src, int w, int h, int src_span, int dest_span, float alpha)
    {
      int ia=(int)(alpha*256.0);
      while (h-->0)
      {
        int n=w;
        LICE_pixel_chan *pin=src;
        LICE_pixel_chan *pout=dest;
        while (n--)
        {

          COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],ia);

          pin += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
          pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
        }
        dest+=dest_span;
        src += src_span;
      }
    }
};


#include "lice_combine.h"


void LICE_GradRect(LICE_IBitmap *dest, int dstx, int dsty, int dstw, int dsth, 
                      float ir, float ig, float ib, float ia,
                      float drdx, float dgdx, float dbdx, float dadx,
                      float drdy, float dgdy, float dbdy, float dady,
                      int mode)
{
  if (!dest) return;

  ir*=255.0; ig*=255.0; ib*=255.0; ia*=256.0;
  drdx*=255.0; dgdx*=255.0; dbdx*=255.0; dadx*=256.0;
  drdy*=255.0; dgdy*=255.0; dbdy*=255.0; dady*=256.0;
  // dont scale alpha

  // clip to output
  if (dstx < 0) { ir-=dstx*drdx; ig-=dstx*dgdx; ib-=dstx*dbdx; ia-=dstx*dadx; dstw+=dstx; dstx=0; }
  if (dsty < 0) 
  {
    ir -= dsty*drdy; ig-=dsty*dgdy; ib -= dsty*dbdy; ia -= dsty*dady;
    dsth += dsty; 
    dsty=0; 
  }  
  if (dstx+dstw > dest->getWidth()) dstw =(dest->getWidth()-dstx);
  if (dsty+dsth > dest->getHeight()) dsth = (dest->getHeight()-dsty);

  if (dstw<1 || dsth<1) return;

  int dest_span=dest->getRowSpan()*sizeof(LICE_pixel);
  LICE_pixel_chan *pdest = (LICE_pixel_chan *)dest->getBits();
  if (!pdest) return;
 
  if (dest->isFlipped())
  {
    pdest += (dest->getHeight()-dsty - 1)*dest_span;
    dest_span=-dest_span;
  }
  else
  {
    pdest += dsty*dest_span;
  }
  pdest+=dstx*sizeof(LICE_pixel);
#define TOFIX(a) ((int)((a)*65536.0))

#define __LICE__ACTION(comb) _LICE_Template_Blit<comb>::gradBlit(pdest,dstw,dsth,TOFIX(ir),TOFIX(ig),TOFIX(ib),TOFIX(ia),TOFIX(drdx),TOFIX(dgdx),TOFIX(dbdx),TOFIX(dadx),TOFIX(drdy),TOFIX(dgdy),TOFIX(dbdy),TOFIX(dady),dest_span)
    __LICE_ACTIONBYMODE_NOALPHA(mode);
#undef __LICE__ACTION

#undef TOFIX
}


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

  if (src->isFlipped())
  {
    psrc += (src->getHeight()-sr.top - 1)*src_span;
    src_span=-src_span;
  }
  else psrc += sr.top*src_span;
  psrc += sr.left*sizeof(LICE_pixel);

  if (dest->isFlipped())
  {
    pdest += (dest->getHeight()-dsty - 1)*dest_span;
    dest_span=-dest_span;
  }
  else pdest += dsty*dest_span;
  pdest+=dstx*sizeof(LICE_pixel);

  int i=sr.bottom-sr.top;
  int cpsize=sr.right-sr.left;

  if ((mode&LICE_BLIT_MODE_MASK) >= LICE_BLIT_MODE_CHANCOPY && (mode&LICE_BLIT_MODE_MASK) < LICE_BLIT_MODE_CHANCOPY+0x10)
  {
    while (i-->0)
    {
      LICE_pixel_chan *o=pdest+((mode>>2)&3);
      LICE_pixel_chan *in=psrc+(mode&3);
      int a=cpsize;
      while (a--)
      {
        *o=*in;
        o+=sizeof(LICE_pixel);
        in+=sizeof(LICE_pixel);
      }
      pdest+=dest_span;
      psrc += src_span;
    }
  }
  // special fast case for copy with no source alpha and alpha=1.0
  else if ((mode&LICE_BLIT_MODE_MASK)==LICE_BLIT_MODE_COPY && !(mode&LICE_BLIT_USE_ALPHA) && alpha==1.0)
  {
    while (i-->0)
    {
      memcpy(pdest,psrc,cpsize*sizeof(LICE_pixel));
      pdest+=dest_span;
      psrc += src_span;
    }
  }
  else 
  {

#define __LICE__ACTION(comb) _LICE_Template_Blit<comb>::blit(pdest,psrc,cpsize,i,src_span,dest_span,alpha)
    __LICE_ACTIONBYMODE(mode,alpha);
#undef __LICE__ACTION

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


  if (src->isFlipped())
  {
    psrc += (src->getHeight()-1)*src_span;
    src_span=-src_span;
  }

  if (dest->isFlipped())
  {
    pdest += (dest->getHeight()-dsty - 1)*dest_span;
    dest_span=-dest_span;
  }
  else pdest += dsty*dest_span;
  pdest+=dstx*sizeof(LICE_pixel);

#define __LICE__ACTION(comb) _LICE_Template_Blit<comb>::scaleBlit(pdest,psrc,dstw,dsth,srcx,srcy,xadvance,yadvance,src->getWidth(),src->getHeight(),src_span,dest_span,alpha,mode&LICE_BLIT_FILTER_MASK)
    __LICE_ACTIONBYMODE(mode,alpha);
#undef __LICE__ACTION

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

  if (src->isFlipped())
  {
    psrc += (src->getHeight()-1)*src_span;
    src_span=-src_span;
  }

  if (dest->isFlipped())
  {
    pdest += (dest->getHeight()-dsty - 1)*dest_span;
    dest_span=-dest_span;
  }
  else pdest += dsty*dest_span;
  pdest+=dstx*sizeof(LICE_pixel);

#define __LICE__ACTION(comb) _LICE_Template_Blit<comb>::deltaBlit(pdest,psrc,dstw,dsth,srcx,srcy,dsdx,dtdx,dsdy,dtdy,src_left,src_top,src_right,src_bottom,src_span,dest_span,alpha,mode&LICE_BLIT_FILTER_MASK)
    __LICE_ACTIONBYMODE(mode,alpha);
#undef __LICE__ACTION
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

  if (src->isFlipped())
  {
    psrc += (src->getHeight()-1)*src_span;
    src_span=-src_span;
  }

  if (dest->isFlipped())
  {
    pdest += (dest->getHeight()-dsty - 1)*dest_span;
    dest_span=-dest_span;
  }
  else pdest += dsty*dest_span;
  pdest+=dstx*sizeof(LICE_pixel);

#define __LICE__ACTION(comb) _LICE_Template_Blit<comb>::deltaBlit(pdest,psrc,dstw,dsth,srcx,srcy,dsdx,dtdx,dsdy,dtdy,src_left,src_top,src_right,src_bottom,src_span,dest_span,alpha,mode&LICE_BLIT_FILTER_MASK)
    __LICE_ACTIONBYMODE(mode,alpha);
#undef __LICE__ACTION
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

  if (dest->isFlipped())
  {
    p+=(dest->getHeight() - y - h)*sp;
  }
  else p+=sp*y;

  p += x;
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

LICE_pixel LICE_GetPixel(LICE_IBitmap *bm, int x, int y)
{
  LICE_pixel *px;
  if (!bm || !(px=bm->getBits()) || x < 0 || y < 0 || x >= bm->getWidth() || y>= bm->getHeight()) return 0;
	return px[y * bm->getRowSpan() + x];
}

void LICE_PutPixel(LICE_IBitmap *bm, int x, int y, LICE_pixel color, float alpha, int mode)
{
  LICE_pixel *px;
  if (!bm || !(px=bm->getBits()) || x < 0 || y < 0 || x >= bm->getWidth() || y>= bm->getHeight()) return;

  px+=x+y*bm->getRowSpan();

#define __LICE__ACTION(comb) comb::doPix((LICE_pixel_chan *)px, LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),LICE_GETA(color), (int) (alpha * 256.0f))
  __LICE_ACTIONBYMODE(mode,alpha);
#undef __LICE__ACTION
}