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
  GifPixelType *linebuf;
  GifPixelType *framestate;
  int transalpha;
  int w,h;
  bool dither;
};

static GifPixelType QuantPixel(LICE_pixel p, liceGifWriteRec *wr)
{
  int rv=LICE_GETR(p)*6;
  int gv=LICE_GETG(p)*7;
  int bv=LICE_GETB(p)*6;

  if (wr->dither)
  {
    rv+=(rand()&127)-63;
    gv+=(rand()&127)-63;
    bv+=(rand()&127)-63;
    if (rv<0) rv=0;
    else if(rv>256*5) rv=256*5;
    if (gv<0) gv=0;
    else if(gv>256*6) gv=256*6;
    if (bv<0) bv=0;
    else if(bv>256*5) bv=256*5;
  }
  return 1 + (rv)/256 + ((gv)/256)*6 + ((bv)/256)*42;
}

static void MakeColorMap(ColorMapObject *cmap, LICE_IBitmap *bm)
{
  int x;
  int r=0;
  int g=0;
  int b=0;
  cmap->Colors[0].Red=cmap->Colors[0].Green=cmap->Colors[0].Blue=0;
  for(x=1;x<253;x++)
  {
    cmap->Colors[x].Red = (r*255)/5;
    cmap->Colors[x].Green = (g*255)/6;
    cmap->Colors[x].Blue = (b*255)/5;
    if (++r==6)
    {
      r=0;
      if (++g==7)
      {
        g=0;
        b++;
      }
      
    }
  }
}

bool LICE_WriteGIFFrame(void *handle, LICE_IBitmap *frame, int xpos, int ypos, bool wantNewColorMap)
{
  liceGifWriteRec *wr = (liceGifWriteRec*)handle;
  if (!wr) return false;

  int usew=frame->getWidth(), useh=frame->getHeight();
  if (xpos+usew > wr->w) usew = wr->w-xpos;
  if (ypos+useh > wr->h) useh = wr->h-ypos;
  if (usew<1||useh<1) return false;

  if (wantNewColorMap)
  {
    // generate new color map yo
  }

  EGifPutImageDesc(wr->f, xpos, ypos, usew,useh, 0, wantNewColorMap ? wr->cmap : NULL); 

  GifPixelType *linebuf = wr->linebuf;
  int y;
  bool chkFrame= false;
  int ta=wr->transalpha>0;

  if (wr->transalpha < 0)
  {
    if (!wr->framestate) wr->framestate=(GifPixelType *) calloc(wr->w*wr->h,sizeof(GifPixelType));
    else chkFrame=true;
  }

  for(y=0;y<useh;y++)
  {
    int rdy=y;
    if (frame->isFlipped()) rdy = frame->getHeight()-1-y;
    LICE_pixel *in = frame->getBits() + rdy*frame->getRowSpan();
    int x;
    for(x=0;x<usew;x++)
    {
      LICE_pixel p = *in++;
      GifPixelType val = QuantPixel(p,wr);
      if (wr->framestate)
      {
        if (!chkFrame || wr->framestate[xpos+x + (ypos+y)*wr->w] != val)
        {
          wr->framestate[xpos+x + (ypos+y)*wr->w]=val;
          linebuf[x] = val;
        }
        else linebuf[x]=0; // transparent
      }
      else // single frame mode
      {
        if (ta && LICE_GETA(p) < ta)
          linebuf[x]=0;
        else
          linebuf[x] = val;
      }
    }
    EGifPutLine(wr->f, linebuf, usew);
  }

  return true;
}
void *LICE_WriteGIFBegin(const char *filename, LICE_IBitmap *firstframe, int transparent_alpha, int frame_delay, bool dither)
{
  if (!firstframe) return NULL;

  GifFileType *f = EGifOpenFileName(filename,0);
  if (!f) return NULL;

  liceGifWriteRec *wr = (liceGifWriteRec*)malloc(sizeof(liceGifWriteRec));
  wr->f = f;
  wr->dither = dither;
  wr->w=firstframe->getWidth();
  wr->h=firstframe->getHeight();
  wr->cmap = (ColorMapObject*)calloc(sizeof(ColorMapObject)+256*sizeof(GifColorType),1);
  wr->cmap->Colors = (GifColorType*)(wr->cmap+1);
  wr->cmap->ColorCount=256;
  wr->cmap->BitsPerPixel=8;
  wr->framestate=0;
  MakeColorMap(wr->cmap,firstframe);

  EGifPutScreenDesc(wr->f,wr->w,wr->h,8,0,wr->cmap);

  wr->linebuf = (GifPixelType*)malloc(wr->w*sizeof(GifPixelType));
  wr->transalpha = transparent_alpha;

  unsigned char gce[4] = { 0, };
  if (wr->transalpha)
  {
    gce[0] |= 1;
    gce[3] = 0;
  }

  gce[1]=(frame_delay/10)&255;
  gce[2]=(frame_delay/10)>>8;

  if (gce[0]||gce[1]||gce[2])
    EGifPutExtension(f, 0xF9, sizeof(gce), gce);

  LICE_WriteGIFFrame(wr,firstframe,0,0,false);

  return wr;
}



bool LICE_WriteGIFEnd(void *handle)
{
  liceGifWriteRec *wr = (liceGifWriteRec*)handle;
  if (!wr) return false;

  int ret = EGifCloseFile(wr->f);

  free(wr->linebuf);
  free(wr->cmap);
  free(wr->framestate);

  free(wr);

  return ret!=GIF_ERROR;
}


bool LICE_WriteGIF(const char *filename, LICE_IBitmap *bmp, int transparent_alpha, bool dither)
{
  // todo: alpha?
  if (!bmp) return false;

  EGifSetGifVersion("89a");
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

  void *h=LICE_WriteGIFBegin(filename,bmp,has_transparent?transparent_alpha:0,0,dither);
  if (!h) return false;

  return LICE_WriteGIFEnd(h);
}