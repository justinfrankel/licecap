/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice_gif.cpp (GIF loading for LICE)
  See lice.h for license and other information
*/

#include "lice.h"
#include <stdio.h>

extern "C" {

#include "../giflib/gif_lib.h"
int _GifError;
};

static int readfunc_fh(GifFileType *fh, GifByteType *buf, int sz) { return (int)fread(buf, 1, sz, (FILE *)fh->UserData); }

LICE_IBitmap *LICE_LoadGIF(const char *filename, LICE_IBitmap *bmp, int *nframes)
{
  FILE *fpp = NULL;
#ifdef _WIN32
  if (GetVersion()<0x80000000)
  {
    WCHAR wf[2048];
    if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,filename,-1,wf,2048))
      fpp = _wfopen(wf,L"rb");
  }
#endif

  if (!fpp) fpp = fopen(filename,"rb");
  if(!fpp) return 0;

  GifFileType *fp=DGifOpen(fpp, readfunc_fh);
  if (!fp)
  {
    fclose(fpp);
    return 0;
  }
  
  GifRecordType RecordType;
  GifByteType *Extension;
  int ExtCode;
  do 
  {
    if (DGifGetRecordType(fp, &RecordType) == GIF_ERROR) break;

  	switch (RecordType) 
    {
	    case IMAGE_DESC_RECORD_TYPE:
		    if (DGifGetImageDesc(fp) == GIF_ERROR) 
        {
          RecordType = TERMINATE_RECORD_TYPE;
          break;
  		  }

        // todo: support transparency
        // todo: have it buffer all frames and output frame count
        if (nframes) *nframes=1; // placeholder 

        {
          int width=fp->Image.Width,height=fp->Image.Height;
          if (bmp)
          {
            bmp->resize(width,height);
            if (bmp->getWidth() != (int)width || bmp->getHeight() != (int)height) 
            {
              RecordType = TERMINATE_RECORD_TYPE;
              break;
            }
          }
          else bmp=new LICE_MemBitmap(width,height);

          LICE_pixel *bmpptr = bmp->getBits();
          int dbmpptr=bmp->getRowSpan();
          if (bmp->isFlipped())
          {
            bmpptr += dbmpptr*(bmp->getHeight()-1);
            dbmpptr=-dbmpptr;
          }
          GifPixelType *linebuf=(GifPixelType*)malloc(width*sizeof(GifPixelType));
          int y;
          int cmap[256];
          for (y=0;y<256;y++)
          {
            if (fp->SColorMap&&y<fp->SColorMap->ColorCount&&fp->SColorMap->Colors)
            {
              GifColorType *ct=fp->SColorMap->Colors+y;
              cmap[y]=LICE_RGBA(ct->Red,ct->Green,ct->Blue,255);
            }
            else cmap[y]=0;
          }

          for (y=0; y < height; y ++)
          {
            if (DGifGetLine(fp,linebuf,width)==GIF_ERROR) break;

            int x;
            for (x = 0; x < width; x ++)
            {
              bmpptr[x]=cmap[linebuf[x]&0xff];
            }
            bmpptr += dbmpptr;
          }



          free(linebuf);
          DGifCloseFile(fp);
          fclose(fpp);
          return bmp;
        }
  		break;
	    case EXTENSION_RECORD_TYPE:
    		if (DGifGetExtension(fp, &ExtCode, &Extension) == GIF_ERROR) 
        {
          RecordType = TERMINATE_RECORD_TYPE;
        }
        else
        {
    		  while (Extension != NULL) 
          {
		        if (DGifGetExtensionNext(fp, &Extension) == GIF_ERROR) 
            {
              RecordType = TERMINATE_RECORD_TYPE;
              break;
  		      }
		      }
        }
      break;
	    case TERMINATE_RECORD_TYPE:
  		break;
	    default:		    /* Should be traps by DGifGetRecordType. */
	  	break;
	  }
  }
  while (RecordType != TERMINATE_RECORD_TYPE);

  DGifCloseFile(fp);
  fclose(fpp);
  return 0;

}



class LICE_GIFLoader
{
public:
  _LICE_ImageLoader_rec rec;
  LICE_GIFLoader() 
  {
    rec.loadfunc = loadfunc;
    rec.get_extlist = get_extlist;
    rec._next = LICE_ImageLoader_list;
    LICE_ImageLoader_list = &rec;
  }

  static LICE_IBitmap *loadfunc(const char *filename, bool checkFileName, LICE_IBitmap *bmpbase)
  {
    if (checkFileName)
    {
      const char *p=filename;
      while (*p)p++;
      while (p>filename && *p != '\\' && *p != '/' && *p != '.') p--;
      if (stricmp(p,".gif")) return 0;
    }
    return LICE_LoadGIF(filename,bmpbase,NULL);
  }
  static const char *get_extlist()
  {
    return "GIF files (*.GIF)\0*.GIF\0";
  }

};

LICE_GIFLoader LICE_gifldr;