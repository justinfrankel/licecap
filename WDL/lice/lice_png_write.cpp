/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice_png_write.cpp (PNG saving for LICE)
  See lice.h for license and other information
*/

#include "lice.h"


#include <stdio.h>
#include "../libpng/png.h"


bool LICE_WritePNG(const char *filename, LICE_IBitmap *bmp, bool wantalpha /*=true*/)
{
  if (!bmp || !filename) return false;
  /*
  **  Joshua Teitelbaum 1/1/2008
  **  Gifted to cockos for toe nail clippings.
  **
  ** JF> tweaked some
  */
  png_structp png_ptr=NULL;
  png_infop info_ptr=NULL;

  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) return false;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL, NULL, NULL);

  if (png_ptr == NULL) {
    fclose(fp);
    return false;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fclose(fp);
    png_destroy_write_struct(&png_ptr,  png_infopp_NULL);
    return false;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    /* If we get here, we had a problem reading the file */
    fclose(fp);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return false;
  }

  png_init_io(png_ptr, fp);

#define BITDEPTH 8
  png_set_IHDR(png_ptr, info_ptr, bmp->getWidth(), bmp->getHeight(), BITDEPTH, wantalpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, info_ptr);

  png_set_bgr(png_ptr);

  // kill alpha channel bytes if not wanted
  if (!wantalpha) png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

  unsigned char **row_pointers = (unsigned char **)png_malloc(png_ptr,bmp->getHeight()*sizeof(int*));
  LICE_pixel *ptr=(LICE_pixel *)bmp->getBits();
  int rowspan=bmp->getRowSpan();
  if (bmp->isFlipped()) 
  {
    ptr+=rowspan*(bmp->getHeight()-1); 
    rowspan=-rowspan;
  }

  int k;
  for (k = 0; k < bmp->getHeight(); k++)
  {
    row_pointers[k] = (unsigned char*) ptr;
    ptr += rowspan;
  }

  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, info_ptr);
  png_free(png_ptr,row_pointers);
  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(fp);

  return true;
}