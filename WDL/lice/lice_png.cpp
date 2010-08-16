/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice_png.cpp (PNG loading for LICE)
  See lice.h for license and other information
*/

#include "lice.h"


#include <stdio.h>
#include "../libpng/png.h"

LICE_IBitmap *LICE_LoadPNG(const char *filename, LICE_IBitmap *bmp)
{
  FILE *fp = fopen(filename,"rb");
  if(!fp) return 0;

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); 
  if(!png_ptr) 
  {
    fclose(fp);
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr); 
  if(!info_ptr)
  {
    png_destroy_read_struct(&png_ptr, NULL, NULL); 
    fclose(fp);
    return 0;
  }
  
  if (setjmp(png_jmpbuf(png_ptr)))
  { 
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL); 
    fclose(fp);
    return 0;
  }

  png_init_io(png_ptr, fp); 

  png_read_info(png_ptr, info_ptr);

  unsigned long width, height;
  int bit_depth, color_type, interlace_type, compression_type, filter_method;
  png_get_IHDR(png_ptr, info_ptr, &width, &height,
       &bit_depth, &color_type, &interlace_type,
       &compression_type, &filter_method);

  //convert whatever it is to RGBA
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
    png_set_gray_1_2_4_to_8(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) 
    png_set_tRNS_to_alpha(png_ptr);

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  if (bit_depth < 8)
    png_set_packing(png_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB)
    png_set_filler(png_ptr, 0xff, PNG_FILLER_BEFORE);

  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    png_set_swap_alpha(png_ptr);

  //get the bits
  if (bmp)
  {
    bmp->resize(width,height);
    if (bmp->getWidth() != (int)width || bmp->getHeight() != (int)height) 
    {
      fclose(fp);
      return 0;
    }
  }
  else bmp=new LICE_MemBitmap(width,height);

  unsigned char **row_pointers=(unsigned char **)malloc(height*sizeof(unsigned char *));;
  LICE_pixel *bmpptr = bmp->getBits();
  int dbmpptr=bmp->getRowSpan();
  if (bmp->isFlipped())
  {
    bmpptr += dbmpptr*(bmp->getHeight()-1);
    dbmpptr=-dbmpptr;
  }
  unsigned int i;
  for(i=0;i<height;i++)
  {
    row_pointers[i]=(unsigned char *)bmpptr;
    bmpptr+=dbmpptr;
  }
  png_read_image(png_ptr, row_pointers);
  png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
  fclose(fp);

  //put shit in correct order
  for(i=0;i<height;i++)
  {
    LICE_pixel *bmpptr = (LICE_pixel *)row_pointers[i];
    unsigned int j;
    for(j=0;j<width;j++)
    {
      LICE_pixel p = bmpptr[j];
      unsigned char a = p&0xff;
      unsigned char r = (p>>8)&0xff;
      unsigned char g = (p>>16)&0xff;
      unsigned char b = (p>>24)&0xff;
      bmpptr[j] = LICE_RGBA(r,g,b,a);
    }
  }
  free(row_pointers);
  
  return bmp;
}

typedef struct 
{
  unsigned char *data;
  int len;
} pngReadStruct;

static void staticPngReadFunc(png_structp png_ptr, png_bytep data, png_size_t length)
{
  pngReadStruct *readStruct = (pngReadStruct *)png_get_io_ptr(png_ptr);
  memset(data, 0, length);

  int l = min((int)length, readStruct->len);
  memcpy(data, readStruct->data, l);
  readStruct->data += l;
  readStruct->len -= l;
}

LICE_IBitmap *LICE_LoadPNGFromResource(HINSTANCE hInst, int resid, LICE_IBitmap *bmp)
{
#ifdef _WIN32
  HRSRC hResource = FindResource(hInst, MAKEINTRESOURCE(resid), "PNG");
  if(!hResource) return NULL;

  DWORD imageSize = SizeofResource(hInst, hResource);
  if(imageSize < 8) return NULL;

  HGLOBAL res = LoadResource(hInst, hResource);
  const void* pResourceData = LockResource(res);
  if(!pResourceData) return NULL;

  unsigned char *data = (unsigned char *)pResourceData;
  if(png_sig_cmp(data, 0, 8)) return NULL;

  pngReadStruct readStruct = {data, imageSize};

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); 
  if(!png_ptr) 
  {
    DeleteObject(res);
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr); 
  if(!info_ptr)
  {
    png_destroy_read_struct(&png_ptr, NULL, NULL); 
    DeleteObject(res);
    return 0;
  }
  
  if (setjmp(png_jmpbuf(png_ptr)))
  { 
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL); 
    DeleteObject(res);
    return 0;
  }

  png_set_read_fn(png_ptr, &readStruct, staticPngReadFunc);

  png_read_info(png_ptr, info_ptr);

  unsigned long width, height;
  int bit_depth, color_type, interlace_type, compression_type, filter_method;
  png_get_IHDR(png_ptr, info_ptr, &width, &height,
       &bit_depth, &color_type, &interlace_type,
       &compression_type, &filter_method);

  //convert whatever it is to RGBA
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
    png_set_gray_1_2_4_to_8(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) 
    png_set_tRNS_to_alpha(png_ptr);

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  if (bit_depth < 8)
    png_set_packing(png_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB)
    png_set_filler(png_ptr, 0xff, PNG_FILLER_BEFORE);

  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    png_set_swap_alpha(png_ptr);

  //get the bits
  if (bmp)
  {
    bmp->resize(width,height);
    if (bmp->getWidth() != (int)width || bmp->getHeight() != (int)height) 
    {
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      DeleteObject(res);
      return 0;
    }
  }
  else bmp=new LICE_MemBitmap(width,height);

  unsigned char **row_pointers=(unsigned char **)malloc(height*sizeof(unsigned char *));;
  LICE_pixel *bmpptr = bmp->getBits();
  int dbmpptr=bmp->getRowSpan();
  unsigned int i;
  for(i=0;i<height;i++)
  {
    row_pointers[i]=(unsigned char *)bmpptr;
    bmpptr+=dbmpptr;
  }
  png_read_image(png_ptr, row_pointers);
  png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

  DeleteObject(res);

  //put shit in correct order
  for(i=0;i<height;i++)
  {
    LICE_pixel *bmpptr = (LICE_pixel *)row_pointers[i];
    unsigned int j;
    for(j=0;j<width;j++)
    {
      LICE_pixel p = bmpptr[j];
      unsigned char a = p&0xff;
      unsigned char r = (p>>8)&0xff;
      unsigned char g = (p>>16)&0xff;
      unsigned char b = (p>>24)&0xff;
      bmpptr[j] = LICE_RGBA(r,g,b,a);
    }
  }
  free(row_pointers);
  return bmp;
  
#else
  return 0;
#endif
}
