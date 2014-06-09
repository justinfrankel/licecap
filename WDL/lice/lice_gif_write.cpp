/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice_gif.cpp (GIF loading for LICE)
  See lice.h for license and other information
*/

#include "lice.h"

extern "C" {

#include "../giflib/gif_lib.h"
//int _GifError;
};


struct liceGifWriteRec
{
  GifFileType *f;
  ColorMapObject *cmap;
  unsigned char from15to8bit[32][32][32];//r,g,b
  GifPixelType *linebuf;
  int transalpha;
  int w,h;
  bool dither;
  bool has_had_frame;
  bool has_global_cmap; 
  LICE_IBitmap *prevframe; // used when multiframe, transalpha<0
};

static inline GifPixelType QuantPixel(LICE_pixel p, liceGifWriteRec *wr)
{
  return wr->from15to8bit[LICE_GETR(p)>>3][LICE_GETG(p)>>3][LICE_GETB(p)>>3];
}


int LICE_SetGIFColorMapFromOctree(void *ww, void *octree, int numcolors)
{
  liceGifWriteRec  *wr = (liceGifWriteRec *)ww;
  if (!octree||!ww) return 0;

  ColorMapObject *cmap = wr->cmap;
  
  int palette_sz=0;

  // store palette
  {
    LICE_pixel* palette=0;
    LICE_pixel tpalette[256];
    if (numcolors <= 256) palette = tpalette;
    else palette = (LICE_pixel*)malloc(numcolors*sizeof(LICE_pixel));
    
    palette_sz = LICE_ExtractOctreePalette(octree, palette);

    int i;
    for (i = 0; i < palette_sz; ++i)
    {
      cmap->Colors[i].Red = LICE_GETR(palette[i]);
      cmap->Colors[i].Green = LICE_GETG(palette[i]);
      cmap->Colors[i].Blue = LICE_GETB(palette[i]);
    }
    for (i = palette_sz; i < numcolors; ++i)
    {
      cmap->Colors[i].Red = cmap->Colors[i].Green = cmap->Colors[i].Blue = 0;   
    }
  
    if (palette != tpalette) free(palette);
  }

  // map palette to 16 bit
  GifColorType *ct = cmap->Colors;
  unsigned char r,g,b;
  for(r=0;r<32;r++)  
  {
    unsigned char cr = r<<3;
    for (g=0;g<32;g++)
    {
      unsigned char cg = g<<3;
      for (b=0;b<32;b++)
      {
        unsigned char cb = b<<3;
        LICE_pixel col = LICE_RGBA(cr,cg,cb,0);
        wr->from15to8bit[r][g][b] = LICE_FindInOctree(octree, col);
      }
    }
  }

  wr->has_global_cmap=true;

  return palette_sz;
}


bool LICE_WriteGIFFrame(void *handle, LICE_IBitmap *frame, int xpos, int ypos, bool perImageColorMap, int frame_delay, int nreps)
{
  liceGifWriteRec *wr = (liceGifWriteRec*)handle;
  if (!wr) return false;

  bool isFirst=false;
  if (!wr->has_had_frame)
  {
    wr->has_had_frame=true;
    isFirst=true;

    if (!perImageColorMap && !wr->has_global_cmap)
    {
      int ccnt = wr->cmap->ColorCount;
      if (wr->transalpha && ccnt>255) ccnt=255;
      void* octree = LICE_CreateOctree(ccnt);
      if (octree) 
      {
        LICE_BuildOctree(octree, frame);
        // sets has_global_cmap
        LICE_SetGIFColorMapFromOctree(wr, octree, ccnt);
        LICE_DestroyOctree(octree);
      }
    }

    EGifPutScreenDesc(wr->f,wr->w,wr->h,8,0,wr->has_global_cmap ? wr->cmap : 0);

  }

  int usew=frame->getWidth(), useh=frame->getHeight();
  if (xpos+usew > wr->w) usew = wr->w-xpos;
  if (ypos+useh > wr->h) useh = wr->h-ypos;
  if (usew<1||useh<1) return false;

  unsigned char gce[4] = { 0, };
  if (wr->transalpha)
  {
    gce[0] |= 1;
    gce[3] = 255;
  }

  int a = frame_delay/10;
  if(a<1&&frame_delay)a=1;
  else if (a>60000) a=60000;
  gce[1]=(a)&255;
  gce[2]=(a)>>8;

  if (isFirst && frame_delay && nreps!=1)
  {
    int nr = nreps > 1 && nreps <= 65536 ? nreps-1 : 0;
    unsigned char ext[]={0xB, 'N','E','T','S','C','A','P','E','2','.','0',3,1,(unsigned char) (nr&0xff), (unsigned char) ((nr>>8)&0xff)};
    EGifPutExtension(wr->f,0xFF, sizeof(ext),ext);
  }

  if (gce[0]||gce[1]||gce[2])
    EGifPutExtension(wr->f, 0xF9, sizeof(gce), gce);


  if (perImageColorMap && !wr->has_global_cmap)
  {
    int ccnt = wr->cmap->ColorCount;
    if (wr->transalpha && ccnt>255) ccnt=255;
    void* octree = LICE_CreateOctree(ccnt);
    if (octree) 
    {
      LICE_BuildOctree(octree, frame);
        // sets has_global_cmap (clear below)
      LICE_SetGIFColorMapFromOctree(wr, octree, ccnt);
      LICE_DestroyOctree(octree);
      wr->has_global_cmap=false;
    }
  }

  EGifPutImageDesc(wr->f, xpos, ypos, usew,useh, 0, wr->has_global_cmap ? NULL : wr->cmap); 

  GifPixelType *linebuf = wr->linebuf;
  int y;
  int ta=wr->transalpha>0;

  if ((!isFirst || frame_delay) && wr->transalpha<0)
  {
    bool ignFr=false;
    if (!wr->prevframe)
    {
      ignFr=true;
      wr->prevframe = new LICE_MemBitmap(wr->w,wr->h);
      LICE_Clear(wr->prevframe,0);
    }

    LICE_SubBitmap tmp(wr->prevframe,xpos,ypos,usew,useh);

    for(y=0;y<useh;y++)
    {
      int rdy=y,rdy2=y;
      if (frame->isFlipped()) rdy = frame->getHeight()-1-y;
      if (tmp.isFlipped()) rdy2 = tmp.getHeight()-1-y;
      LICE_pixel *in = frame->getBits() + rdy*frame->getRowSpan();
      LICE_pixel *in2 = tmp.getBits() + rdy2*tmp.getRowSpan();
      int x;
      for(x=0;x<usew;x++)
      {
        LICE_pixel p = *in++;
        if (ignFr || p != *in2)
        {
          linebuf[x] = QuantPixel(p,wr);
        }
        else 
        {
          linebuf[x]=255;
        }
        in2++;
      }
      EGifPutLine(wr->f, linebuf, usew);
    }


    LICE_Blit(&tmp,frame,0,0,0,0,usew,useh,1.0f,LICE_BLIT_MODE_COPY);
    
  }
  else for(y=0;y<useh;y++)
  {
    int rdy=y;
    if (frame->isFlipped()) rdy = frame->getHeight()-1-y;
    LICE_pixel *in = frame->getBits() + rdy*frame->getRowSpan();
    int x;
    for(x=0;x<usew;x++)
    {
      LICE_pixel p = *in++;
      GifPixelType val = QuantPixel(p,wr);
      if (ta && LICE_GETA(p) < ta)
        linebuf[x]=255;
      else
        linebuf[x] = val;
    }
    EGifPutLine(wr->f, linebuf, usew);
  }

  return true;
}

void *LICE_WriteGIFBeginNoFrame(const char *filename, int w, int h, int transparent_alpha, bool dither)
{

  EGifSetGifVersion("89a");

  GifFileType *f = EGifOpenFileName(filename,0);
  if (!f) return NULL;

  liceGifWriteRec *wr = (liceGifWriteRec*)calloc(sizeof(liceGifWriteRec),1);
  wr->f = f;
  wr->dither = dither;
  wr->w=w;
  wr->h=h;
  wr->cmap = (ColorMapObject*)calloc(sizeof(ColorMapObject)+256*sizeof(GifColorType),1);
  wr->cmap->Colors = (GifColorType*)(wr->cmap+1);
  wr->cmap->ColorCount=256;
  wr->cmap->BitsPerPixel=8;
  wr->has_had_frame=false;
  wr->has_global_cmap=false;

  wr->linebuf = (GifPixelType*)malloc(wr->w*sizeof(GifPixelType));
  wr->transalpha = transparent_alpha;

  return wr;
}
void *LICE_WriteGIFBegin(const char *filename, LICE_IBitmap *firstframe, int transparent_alpha, int frame_delay, bool dither, int nreps)
{
  if (!firstframe) return NULL;

  void *wr=LICE_WriteGIFBeginNoFrame(filename,firstframe->getWidth(),firstframe->getHeight(),transparent_alpha,dither);
  if (wr) LICE_WriteGIFFrame(wr,firstframe,0,0,true,frame_delay,nreps);

  return wr;
}



bool LICE_WriteGIFEnd(void *handle)
{
  liceGifWriteRec *wr = (liceGifWriteRec*)handle;
  if (!wr) return false;

  int ret = EGifCloseFile(wr->f);

  free(wr->linebuf);
  free(wr->cmap);

  delete wr->prevframe;

  free(wr);

  return ret!=GIF_ERROR;
}


bool LICE_WriteGIF(const char *filename, LICE_IBitmap *bmp, int transparent_alpha, bool dither)
{
  // todo: alpha?
  if (!bmp) return false;

  int has_transparent = 0;
  if (transparent_alpha>0)
  {
    int y=bmp->getHeight();
    LICE_pixel *p = bmp->getBits();
    int w = bmp->getWidth();
    while (y--&&!has_transparent)
    {
      int x=w;
      while(x--)
      {
        if (LICE_GETA(*p) < transparent_alpha) 
        {
          has_transparent=1;
          break;
        }
        p++;
      }
      p+=bmp->getRowSpan()-w;
    }
  }


  void *wr=LICE_WriteGIFBeginNoFrame(filename,bmp->getWidth(),bmp->getHeight(),has_transparent?transparent_alpha:0,dither);
  if (!wr)  return false;
  
  LICE_WriteGIFFrame(wr,bmp,0,0,false,0);

  return LICE_WriteGIFEnd(wr);
}
