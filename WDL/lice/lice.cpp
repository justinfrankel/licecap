/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice.cpp (LICE core processing)
  See lice.h for license and other information
*/


#include "lice.h"
#include <math.h>


#include "lice_combine.h"


#define LICE_SCALEDBLIT_USE_FIXEDPOINT
#define LICE_DELTABLIT_USE_FIXEDPOINT

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
    static void solidBlit(LICE_pixel_chan *dest, int w, int h, 
                         int ir, int ig, int ib, int ia,
                         int dest_span)
    {
      while (h--)
      {
        LICE_pixel_chan *pout=dest;
        int n=w;
        while (n--)
        {
          COMBFUNC::doPix(pout,ir,ig,ib,ia,ia);          
          pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
        }
        dest+=dest_span;
      }
    }

    static void deltaBlit(LICE_pixel_chan *dest, LICE_pixel_chan *src, int w, int h, 
                          double srcx, double srcy, double dsdx, double dtdx, double dsdy, double dtdy,
                          double dsdxdy, double dtdxdy,
                          int src_left, int src_top, int src_right, int src_bottom,
                          int src_span, int dest_span, float alpha, int filtermode)
    {
      int ia=(int)(alpha*256.0);
#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
      int isrcx=(int)(srcx*65536.0);
      int isrcy=(int)(srcy*65536.0);
      int idsdx=(int)(dsdx*65536.0);
      int idtdx=(int)(dtdx*65536.0);
      int idsdy=(int)(dsdy*65536.0);
      int idtdy=(int)(dtdy*65536.0);
      int idsdxdy=(int)(dsdxdy*65536.0);
      int idtdxdy=(int)(dtdxdy*65536.0);
#endif

      if (filtermode == LICE_BLIT_FILTER_BILINEAR)
      {
        while (h--)
        {
#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
          int thisx=isrcx;
          int thisy=isrcy;
#else
          double thisx=srcx;
          double thisy=srcy;
#endif
          LICE_pixel_chan *pout=dest;
          int n=w;
          while (n--)
          {
#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
            int cury = thisy/65536;
            int curx = thisx/65536;
#else
            int cury = (int)(thisy);
            int curx = (int)(thisx);
#endif
            if (cury >= src_top && cury < src_bottom-1)
            {
              if (curx >= src_left && curx < src_right-1)
              {
                LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);
                int r,g,b,a;

#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
                __LICE_BilinearFilterI(&r,&g,&b,&a,pin,pin+src_span,thisx&65535,thisy&65535);
#else
                __LICE_BilinearFilter(&r,&g,&b,&a,pin,pin+src_span,thisx-curx,thisy-cury);
#endif

                COMBFUNC::doPix(pout,r,g,b,a,ia);
              }
              else if (curx==src_right-1)
              {

                LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);
                int r,g,b,a;

#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
                __LICE_LinearFilterI(&r,&g,&b,&a,pin,pin+src_span,thisy&65535);
#else
                __LICE_LinearFilter(&r,&g,&b,&a,pin,pin+src_span,thisy-cury);
#endif
                COMBFUNC::doPix(pout,r,g,b,a,ia);
              }
            }
            else if (cury==src_bottom-1)
            {
              if (curx>=src_left && curx<src_right-1)
              {
                LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);

                int r,g,b,a;

#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
                __LICE_LinearFilterI(&r,&g,&b,&a,pin,pin+sizeof(LICE_pixel)/sizeof(LICE_pixel_chan),thisx&65535);
#else
                __LICE_LinearFilter(&r,&g,&b,&a,pin,pin+sizeof(LICE_pixel)/sizeof(LICE_pixel_chan),thisx-curx);
#endif

                COMBFUNC::doPix(pout,r,g,b,a,ia);
              }
              else if (curx==src_right-1)
              {
                LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);
                COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],ia);
              }
            }

            pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
            thisx+=idsdx;
            thisy+=idtdx;
#else
            thisx+=dsdx;
            thisy+=dtdx;
#endif
          }
#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
          idsdx+=idsdxdy;
          idtdx+=idtdxdy;
          isrcx+=idsdy;
          isrcy+=idtdy;
#else
          dsdx+=dsdxdy;
          dtdx+=dtdxdy;
          srcx+=dsdy;
          srcy+=dtdy;
#endif
          dest+=dest_span;
        }
      }
      else
      {
        while (h--)
        {
#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
          int thisx=isrcx;
          int thisy=isrcy;
#else
          double thisx=srcx;
          double thisy=srcy;
#endif
          LICE_pixel_chan *pout=dest;
          int n=w;
          while (n--)
          {
#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
            int cury = thisy/65536;
            int curx = thisx/65536;
#else
            int cury = (int)(thisy);
            int curx = (int)(thisx);
#endif
            if (cury >= src_top && cury < src_bottom && curx >= src_left && curx < src_right)
            {

              LICE_pixel_chan *pin = src + cury * src_span + curx*sizeof(LICE_pixel);

              COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],ia);
            }

            pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
            thisx+=idsdx;
            thisy+=idtdx;
#else
            thisx+=dsdx;
            thisy+=dtdx;
#endif
          }
#ifdef LICE_DELTABLIT_USE_FIXEDPOINT
          idsdx+=idsdxdy;
          idtdx+=idtdxdy;
          isrcx+=idsdy;
          isrcy+=idtdy;
#else
          dsdx+=dsdxdy;
          dtdx+=dtdxdy;
          srcx+=dsdy;
          srcy+=dtdy;
#endif
          dest+=dest_span;
        }
      }
    }


    static void scaleBlit(LICE_pixel_chan *dest, LICE_pixel_chan *src, int w, int h, 
                          double srcx, double srcy, double dx, double dy, int clipleft, int cliptop, int clipright, int clipbottom,     
                          int src_span, int dest_span, float alpha, int filtermode)
    {
      int ia=(int)(alpha*256.0);
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
      int idx=(int)(dx*65536.0);
      int icurx=(int) (srcx*65536.0);
#endif
      if (filtermode == LICE_BLIT_FILTER_BILINEAR)
      {
        while (h--)
        {
          int cury = (int)(srcy);
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
          int yfrac=(int) ((srcy-cury)*65536.0);
          int curx=icurx;          
#else
          double yfrac=srcy-cury;
          double curx=srcx;
#endif
          LICE_pixel_chan *inptr=src + cury * src_span;
          LICE_pixel_chan *pout=dest;
          int n=w;
          if (cury >= cliptop && cury < clipbottom-1)
          {
            while (n--)
            {
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
              int offs=curx/65536;
#else
              int offs=(int)(curx);
#endif
              LICE_pixel_chan *pin = inptr + offs*sizeof(LICE_pixel);
              if (offs>=clipleft && offs<clipright-1)
              {
                int r,g,b,a;
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
                __LICE_BilinearFilterI(&r,&g,&b,&a,pin,pin+src_span,curx&0xffff,yfrac);
#else
                __LICE_BilinearFilter(&r,&g,&b,&a,pin,pin+src_span,curx-offs,yfrac);
#endif
                COMBFUNC::doPix(pout,r,g,b,a,ia);
              }
              else if (offs==clipright-1)
              {
                int r,g,b,a;
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
                __LICE_LinearFilterI(&r,&g,&b,&a,pin,pin+src_span,yfrac);
#else
                __LICE_LinearFilter(&r,&g,&b,&a,pin,pin+src_span,yfrac);
#endif
                COMBFUNC::doPix(pout,r,g,b,a,ia);
              }

              pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
              curx+=idx;
#else
              curx+=dx;
#endif
            }
          }
          else if (cury == clipbottom-1)
          {
            while (n--)
            {
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
              int offs=curx/65536;
#else
              int offs=(int)(curx);
#endif
              LICE_pixel_chan *pin = inptr + offs*sizeof(LICE_pixel);
              if (offs>=clipleft && offs<clipright-1)
              {
                int r,g,b,a;
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
                __LICE_LinearFilterI(&r,&g,&b,&a,pin,pin+sizeof(LICE_pixel)/sizeof(LICE_pixel_chan),curx&0xffff);
#else
                __LICE_LinearFilter(&r,&g,&b,&a,pin,pin+sizeof(LICE_pixel)/sizeof(LICE_pixel_chan),curx-offs);
#endif
                COMBFUNC::doPix(pout,r,g,b,a,ia);
              }
              else if (offs==clipright-1)
              {
                COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],ia);
              }

              pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
              curx+=idx;
#else
              curx+=dx;
#endif
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
          int cury = (int)(srcy);
          if (cury >= cliptop && cury < clipbottom)
          {
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
            int curx=icurx;
#else
            double curx=srcx;
#endif
            LICE_pixel_chan *inptr=src + cury * src_span;
            LICE_pixel_chan *pout=dest;
            int n=w;
            while (n--)
            {
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
              int offs=curx>>16;
#else
              int offs=(int)(curx);
#endif
              if (offs>=clipleft && offs<clipright)
              {
                LICE_pixel_chan *pin = inptr + offs*sizeof(LICE_pixel);

                COMBFUNC::doPix(pout,pin[LICE_PIXEL_R],pin[LICE_PIXEL_G],pin[LICE_PIXEL_B],pin[LICE_PIXEL_A],ia);
              }

              pout += sizeof(LICE_pixel)/sizeof(LICE_pixel_chan);
#ifdef LICE_SCALEDBLIT_USE_FIXEDPOINT
              curx+=idx;
#else
              curx+=dx;
#endif
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


void LICE_Blit(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, int srcx, int srcy, int srcw, int srch, float alpha, int mode)
{
  RECT r={srcx,srcy,srcx+srcw,srcy+srch};
  LICE_Blit(dest,src,dstx,dsty,&r,alpha,mode);
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

void LICE_Blur(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, int srcx, int srcy, int srcw, int srch) // src and dest can overlap, however it may look fudgy if they do
{
  if (!dest || !src) return;

  RECT sr={srcx,srcy,srcx+srcw,srcy+srch};
  if (sr.left < 0) sr.left=0;
  if (sr.top < 0) sr.top=0;
  if (sr.right > src->getWidth()) sr.right=src->getWidth();
  if (sr.bottom > src->getHeight()) sr.bottom = src->getHeight();

  // clip to output
  if (dstx < 0) { sr.left -= dstx; dstx=0; }
  if (dsty < 0) { sr.top -= dsty; dsty=0; }  
  if (dstx+sr.right-sr.left > dest->getWidth()) sr.right = sr.left + (dest->getWidth()-dstx);
  if (dsty+sr.bottom-sr.top > dest->getHeight()) sr.bottom = sr.top + (dest->getHeight()-dsty);

  // ignore blits that are smaller than 2x2
  if (sr.right <= sr.left+1 || sr.bottom <= sr.top+1) return;

  int dest_span=dest->getRowSpan();
  int src_span=src->getRowSpan();
  LICE_pixel *psrc = (LICE_pixel *)src->getBits();
  LICE_pixel *pdest = (LICE_pixel *)dest->getBits();
  if (!psrc || !pdest) return;

  if (src->isFlipped())
  {
    psrc += (src->getHeight()-sr.top - 1)*src_span;
    src_span=-src_span;
  }
  else psrc += sr.top*src_span;
  psrc += sr.left;

  if (dest->isFlipped())
  {
    pdest += (dest->getHeight()-dsty - 1)*dest_span;
    dest_span=-dest_span;
  }
  else pdest += dsty*dest_span;
  pdest+=dstx;

  LICE_pixel *tmpbuf=NULL;
  int w=sr.right-sr.left;  

  // buffer to save the last unprocessed lines for the cases where blurring from a bitmap to itself
  LICE_pixel turdbuf[2048];
  if (src==dest)
  {
    if (w <= sizeof(turdbuf)/sizeof(turdbuf[0])/2) tmpbuf=turdbuf;
    else tmpbuf=(LICE_pixel*)malloc(w*2*sizeof(LICE_pixel));
  }

  int i;
  for (i = sr.top; i < sr.bottom; i ++)
  {
    if (tmpbuf)
      memcpy(tmpbuf+((i&1)?w:0),psrc,w*sizeof(LICE_pixel));

    if (i==sr.top || i==sr.bottom-1)
    {
      LICE_pixel *psrc2=psrc+(i==sr.top ? src_span : -src_span);

      LICE_pixel lp;

      pdest[0] = LICE_PIXEL_HALF(lp=psrc[0]) + 
                 LICE_PIXEL_QUARTER(psrc[1]) + 
                 LICE_PIXEL_QUARTER(psrc2[0]);
      int x;
      for (x = 1; x < w-1; x ++)
      {
        LICE_pixel tp;
        pdest[x] = LICE_PIXEL_HALF(tp=psrc[x]) + 
                   LICE_PIXEL_QUARTER(psrc2[x]) + 
                   LICE_PIXEL_EIGHTH(psrc[x+1]) +
                   LICE_PIXEL_EIGHTH(lp);
        lp=tp;
      }
      pdest[x] = LICE_PIXEL_HALF(psrc[x]) + 
                   LICE_PIXEL_QUARTER(lp) + 
                   LICE_PIXEL_QUARTER(psrc2[x]);
    }
    else
    {
      LICE_pixel *psrc2=psrc-src_span;
      LICE_pixel *psrc3=psrc+src_span;
      if (tmpbuf)
        psrc2=tmpbuf + ((i&1) ? 0 : w);

      LICE_pixel lp;
      pdest[0] = LICE_PIXEL_HALF(lp=psrc[0]) + 
                 LICE_PIXEL_QUARTER(psrc[1]) +
                 LICE_PIXEL_EIGHTH(psrc2[0]) +
                 LICE_PIXEL_EIGHTH(psrc3[0]);
      int x;
      for (x = 1; x < w-1; x ++)
      {
        LICE_pixel tp;
        pdest[x] = LICE_PIXEL_HALF(tp=psrc[x]) +
                   LICE_PIXEL_EIGHTH(psrc[x+1]) +
                   LICE_PIXEL_EIGHTH(lp) +
                   LICE_PIXEL_EIGHTH(psrc2[x]) + 
                   LICE_PIXEL_EIGHTH(psrc3[x]);
        lp=tp;
      }
      pdest[x] = LICE_PIXEL_HALF(psrc[x]) + 
                 LICE_PIXEL_QUARTER(lp) + 
                 LICE_PIXEL_EIGHTH(psrc2[x]) +
                 LICE_PIXEL_EIGHTH(psrc3[x]);
    }
    pdest+=dest_span;
    psrc += src_span;
  }
  if (tmpbuf && tmpbuf != turdbuf)
    free(tmpbuf);
}

void LICE_ScaledBlit(LICE_IBitmap *dest, LICE_IBitmap *src, 
                     int dstx, int dsty, int dstw, int dsth, 
                     float srcx, float srcy, float srcw, float srch, 
                     float alpha, int mode)
{
  if (!dest || !src || !dstw || !dsth) return;

  // non-scaling optimized omde
  if (fabs(srcw-dstw)<0.001 && fabs(srch-dsth)<0.001)
  {
    // and if not bilinear filtering, or 
    // the source coordinates are near their integer counterparts
    if ((mode&LICE_BLIT_FILTER_MASK)!=LICE_BLIT_FILTER_BILINEAR ||
        (fabs(srcx-floor(srcx))<0.03 && fabs(srcy-floor(srcy))<0.03))
    {
      RECT sr={(int)srcx,(int)srcy,};
      sr.right=sr.left+dstw;
      sr.bottom=sr.top+dsth;
      LICE_Blit(dest,src,dstx,dsty,&sr,alpha,mode);
      return;
    }
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

  int clip_x=(int)srcx;
  int clip_y=(int)srcy;
  int clip_r=(int)ceil(srcx+srcw);
  int clip_b=(int)ceil(srcy+srch);

  if (clip_x<0)clip_x=0;
  if (clip_y<0)clip_y=0;
  if (clip_r>src->getWidth()) clip_r=src->getWidth();
  if (clip_b>src->getHeight()) clip_b=src->getHeight();

#define __LICE__ACTION(comb) _LICE_Template_Blit<comb>::scaleBlit(pdest,psrc,dstw,dsth,srcx,srcy,xadvance,yadvance,clip_x,clip_y,clip_r,clip_b,src_span,dest_span,alpha,mode&LICE_BLIT_FILTER_MASK)
    __LICE_ACTIONBYMODE(mode,alpha);
#undef __LICE__ACTION

}

void LICE_DeltaBlit(LICE_IBitmap *dest, LICE_IBitmap *src, 
                    int dstx, int dsty, int dstw, int dsth, 
                    float srcx, float srcy, float srcw, float srch, 
                    double dsdx, double dtdx, double dsdy, double dtdy,
                    double dsdxdy, double dtdxdy,
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

  int sl=(int)(src_left);
  int sr=(int)(src_right);
  int st=(int)(src_top);
  int sb=(int)(src_bottom);
#define __LICE__ACTION(comb) _LICE_Template_Blit<comb>::deltaBlit(pdest,psrc,dstw,dsth,srcx,srcy,dsdx,dtdx,dsdy,dtdy,dsdxdy,dtdxdy,sl,st,sr,sb,src_span,dest_span,alpha,mode&LICE_BLIT_FILTER_MASK)
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

  int sl=(int)(src_left);
  int sr=(int)(src_right);
  int st=(int)(src_top);
  int sb=(int)(src_bottom);
#define __LICE__ACTION(comb) _LICE_Template_Blit<comb>::deltaBlit(pdest,psrc,dstw,dsth,srcx,srcy,dsdx,dtdx,dsdy,dtdy,0,0,sl,st,sr,sb,src_span,dest_span,alpha,mode&LICE_BLIT_FILTER_MASK)
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

void LICE_MultiplyAddRect(LICE_IBitmap *dest, int x, int y, int w, int h, 
                          float rsc, float gsc, float bsc, float asc,
                          float radd, float gadd, float badd, float aadd)
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

  int ir=(int)(rsc*65536.0);
  int ig=(int)(gsc*65536.0);
  int ib=(int)(bsc*65536.0);
  int ia=(int)(asc*65536.0);
  int ir2=(int)(radd*65536.0);
  int ig2=(int)(gadd*65536.0);
  int ib2=(int)(badd*65536.0);
  int ia2=(int)(aadd*65536.0);

  while (h-->0)
  {
    int n=w;
    while (n--) 
    {
      LICE_pixel_chan *ptr=(LICE_pixel_chan *)p++;
      _LICE_MakePixel(ptr,(ptr[LICE_PIXEL_R]*ir+ir2)>>16,
                          (ptr[LICE_PIXEL_G]*ig+ig2)>>16,
                          (ptr[LICE_PIXEL_B]*ib+ib2)>>16,
                          (ptr[LICE_PIXEL_A]*ia+ia2)>>16);
    }
    p+=sp-w;
  }
}

void LICE_FillRect(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel color, float alpha, int mode)
{
  if (!dest) return;
  if (mode & LICE_BLIT_USE_ALPHA) alpha *= LICE_GETA(color)/255.0f;
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
#define __LICE__ACTION(comb) _LICE_Template_Blit<comb>::solidBlit((LICE_pixel_chan*)p,w,h,LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),(int)(alpha*256.0),sp*sizeof(LICE_pixel))
    __LICE_ACTIONBYMODE_NOALPHA(mode);
#undef __LICE__ACTION
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
      if ((*p&LICE_RGBA(255,255,255,0)) == color) *p&=LICE_RGBA(255,255,255,0);
      else *p|=LICE_RGBA(0,0,0,255);
      p++;
    }
    p+=sp-w;
  }
}

LICE_pixel LICE_GetPixel(LICE_IBitmap *bm, int x, int y)
{
  LICE_pixel *px;
  if (!bm || !(px=bm->getBits()) || x < 0 || y < 0 || x >= bm->getWidth() || y>= bm->getHeight()) return 0;
  if (bm->isFlipped()) return px[(bm->getHeight()-1-y) * bm->getRowSpan() + x];
	return px[y * bm->getRowSpan() + x];
}

void LICE_PutPixel(LICE_IBitmap *bm, int x, int y, LICE_pixel color, float alpha, int mode)
{
  LICE_pixel *px;
  if (!bm || !(px=bm->getBits()) || x < 0 || y < 0 || x >= bm->getWidth() || y>= bm->getHeight()) return;

  if (bm->isFlipped()) px+=x+(bm->getHeight()-1-y)*bm->getRowSpan();
  else px+=x+y*bm->getRowSpan();

#define __LICE__ACTION(comb) comb::doPix((LICE_pixel_chan *)px, LICE_GETR(color),LICE_GETG(color),LICE_GETB(color),LICE_GETA(color), (int)(alpha * 256.0f))
  __LICE_ACTIONBYMODE(mode,alpha);
#undef __LICE__ACTION
}


void LICE_TransformBlit(LICE_IBitmap *dest, LICE_IBitmap *src,  
                    int dstx, int dsty, int dstw, int dsth,
                    float *srcpoints, int div_w, int div_h, // srcpoints coords should be div_w*div_h*2 long, and be in source image coordinates
                    float alpha, int mode)
{
  if (!dest || !src || dstw<1 || dsth<1 || div_w<2 || div_h<2) return;

  int cypos=dsty;
  double ypos=dsty;
  double dxpos=dstw/(float)(div_w-1);
  double dypos=dsth/(float)(div_h-1);
  int y;
  float *curpoints=srcpoints;
  for (y = 0; y < div_h-1; y ++)
  {
    int nypos=(int)(ypos+=dypos);
    int x;
    double xpos=dstx;
    int cxpos=dstx;

    if (nypos != cypos)
    {
      double iy=1.0/(double)(nypos-cypos);
      for (x = 0; x < div_w-1; x ++)
      {
        int nxpos=(xpos+=dxpos);
        if (nxpos != cxpos)
        {
          int offs=x*2;
          double sx=curpoints[offs];
          double sy=curpoints[offs+1];
          double sw=curpoints[offs+2]-sx;
          double sh=curpoints[offs+3]-sy;

          offs+=div_w*2;
          double sxdiff=curpoints[offs]-sx;
          double sydiff=curpoints[offs+1]-sy;
          double sw3=curpoints[offs+2]-curpoints[offs];
          double sh3=curpoints[offs+3]-curpoints[offs+1];

          double ix=1.0/(double)(nxpos-cxpos);
          double dsdx=sw*ix;
          double dtdx=sh*ix;
          double dsdx2=sw3*ix;
          double dtdx2=sh3*ix;
          double dsdy=sxdiff*iy;
          double dtdy=sydiff*iy;
          double dsdxdy = (dsdx2-dsdx)*iy;
          double dtdxdy = (dtdx2-dtdx)*iy;

          LICE_DeltaBlit(dest,src,cxpos,cypos,nxpos-cxpos,nypos-cypos,
              sx,sy,sw,sh,
              dsdx,dtdx,dsdy,dtdy,dsdxdy,dtdxdy,false,alpha,mode);
        }

        cxpos=nxpos;
      }
    }
    curpoints+=div_w*2;
    cypos=nypos;
  }

}
