/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice_jpg.cpp (JPG loading for LICE)
  See lice.h for license and other information
*/

#include <stdio.h>
#include "lice.h"
#include <setjmp.h>

extern "C" {
#include "../jpeglib/jpeglib.h"
};

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

LICE_IBitmap *LICE_LoadJPG(const char *filename, LICE_IBitmap *bmp)
{
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  JSAMPARRAY buffer;
  int row_stride;

  FILE *fp=fopen(filename,"rb");
  if (!fp) return 0;

  cinfo.err = jpeg_std_error(&jerr.pub);

  if (setjmp(jerr.setjmp_buffer)) 
  {
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);
    return 0;
  }
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo, fp);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  row_stride = cinfo.output_width * cinfo.output_components;

  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  if (bmp)
  {
    bmp->resize(cinfo.output_width,cinfo.output_height);
    if (bmp->getWidth() != (int)cinfo.output_width || bmp->getHeight() != (int)cinfo.output_height) 
    {
      jpeg_finish_decompress(&cinfo);
      jpeg_destroy_decompress(&cinfo);
      fclose(fp);
      return 0;
    }
  }
  else bmp=new LICE_MemBitmap(cinfo.output_width,cinfo.output_height);

  LICE_pixel *bmpptr = bmp->getBits();
  int dbmpptr=bmp->getRowSpan();
  if (bmp->isFlipped())
  {
    bmpptr += dbmpptr*(bmp->getHeight()-1);
    dbmpptr=-dbmpptr;
  }


  while (cinfo.output_scanline < cinfo.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
    jpeg_read_scanlines(&cinfo, buffer, 1);
    /* Assume put_scanline_someplace wants a pointer and sample count. */
//    put_scanline_someplace(buffer[0], row_stride);
    if (cinfo.output_components==3)
    {
      int x;
      for (x = 0; x < cinfo.output_width; x++)
      {
        bmpptr[x]=LICE_RGBA(buffer[0][x*3],buffer[0][x*3+1],buffer[0][x*3+2],255);
      }
    }
    else if (cinfo.output_components==1)
    {
      int x;
      for (x = 0; x < cinfo.output_width; x++)
      {
        int v=buffer[0][x];
        bmpptr[x]=LICE_RGBA(v,v,v,255);
      }
    }
    else
      memset(bmpptr,0,4*cinfo.output_width);
    bmpptr+=dbmpptr;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(fp);

  return bmp;
}
