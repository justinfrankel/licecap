#ifndef _LICE_H
#define _LICE_H

/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine

  Copyright (C) 2007 and later, Cockos Incorporated
  Portions Copyright (C) 2007 "schwa"

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  

  Notes:

  Currently this library only supports win32. Support for other platforms may come at a later date.

*/


#ifdef _WIN32
#include <windows.h>
#else
#include "../SWELL/swell.h" // use SWELL on other systems
#endif


typedef unsigned int LICE_pixel;
typedef unsigned char LICE_pixel_chan;

#ifdef _WIN32

#define LICE_RGBA(r,g,b,a) (((b)&0xff)|(((g)&0xff)<<8)|(((r)&0xff)<<16)|(((a)&0xff)<<24))
#define LICE_GETB(v) ((v)&0xff)
#define LICE_GETG(v) (((v)>>8)&0xff)
#define LICE_GETR(v) (((v)>>16)&0xff)
#define LICE_GETA(v) (((v)>>24)&0xff)


#define LICE_PIXEL_B 0
#define LICE_PIXEL_G 1
#define LICE_PIXEL_R 2
#define LICE_PIXEL_A 3

#else

#define LICE_RGBA(r,g,b,a) (((a)&0xff)|(((r)&0xff)<<8)|(((g)&0xff)<<16)|(((b)&0xff)<<24))
#define LICE_GETA(v) ((v)&0xff)
#define LICE_GETR(v) (((v)>>8)&0xff)
#define LICE_GETG(v) (((v)>>16)&0xff)
#define LICE_GETB(v) (((v)>>24)&0xff)

#ifdef __powerpc__
#define LICE_PIXEL_A 3
#define LICE_PIXEL_R 2
#define LICE_PIXEL_G 1
#define LICE_PIXEL_B 0
#else
#define LICE_PIXEL_A 0
#define LICE_PIXEL_R 1
#define LICE_PIXEL_G 2
#define LICE_PIXEL_B 3
#endif

#endif



// bitmap interface, and some built-in types (memory bitmap and system bitmap)

class LICE_IBitmap
{
public:
  virtual ~LICE_IBitmap() { }

  virtual LICE_pixel *getBits()=0;
  virtual int getWidth()=0;
  virtual int getHeight()=0;
  virtual int getRowSpan()=0; // includes any off-bitmap data
  virtual bool isFlipped()=0;
  virtual bool resize(int w, int h)=0;
};

class LICE_MemBitmap : public LICE_IBitmap
{
public:
  LICE_MemBitmap(int w=0, int h=0)
  {
    int sz=(m_width=w)*(m_height=h)*sizeof(LICE_pixel);
    m_fb=sz>0?(LICE_pixel*)malloc(sz):0;
  }

  ~LICE_MemBitmap() { free(m_fb); }


  // LICE_IBitmap interface
  LICE_pixel *getBits() { return m_fb; };
  int getWidth() { return m_width; }
  int getHeight() { return m_height; }
  int getRowSpan() { return m_width; }; 
  bool resize(int w, int h); // returns TRUE if a resize occurred

  bool isFlipped() { return false; }

private:
  LICE_pixel *m_fb;
  int m_width, m_height;
};

class LICE_SysBitmap : public LICE_IBitmap
{
public:
  LICE_SysBitmap(int w=0, int h=0);
  ~LICE_SysBitmap();
  
  // LICE_IBitmap interface
  LICE_pixel *getBits() { return m_bits; }
  int getWidth() { return m_width; }
  int getHeight() { return m_height; }
  int getRowSpan() { return m_width; }; 
  bool resize(int w, int h); // returns TRUE if a resize occurred

  bool isFlipped() 
  {
#ifdef MAC
    return true;
#else
    return false; 
#endif
  }

  // sysbitmap specific calls
  HDC getDC() { return m_dc; }


private:
  int m_width, m_height;

  HDC m_dc;
  LICE_pixel *m_bits;
#ifdef _WIN32
  HBITMAP m_bitmap;
  HGDIOBJ m_oldbitmap;
#endif
};


// bitmap loaders

// pass a bmp if you wish to load it into that bitmap. note that if it fails bmp will not be deleted.
LICE_IBitmap *LICE_LoadPNG(const char *filename, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success
LICE_IBitmap *LICE_LoadPNGFromResource(HINSTANCE hInst, int resid, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success

LICE_IBitmap *LICE_LoadBMP(const char *filename, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success
LICE_IBitmap *LICE_LoadBMPFromResource(HINSTANCE hInst, int resid, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success

LICE_IBitmap *LICE_LoadIcon(const char *filename, int iconnb=0, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success
LICE_IBitmap *LICE_LoadIconFromResource(HINSTANCE hInst, int resid, int iconnb=0, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success

// flags that most blit functions can take

#define LICE_BLIT_MODE_MASK 0xff
#define LICE_BLIT_MODE_COPY 0
#define LICE_BLIT_MODE_ADD 1

#define LICE_BLIT_MODE_CHANCOPY 0xf0 // in this mode, only available for LICE_Blit(), the low nibble is 2 bits of source channel (low 2), 2 bits of dest channel (high 2)

#define LICE_BLIT_FILTER_MASK 0xff00
#define LICE_BLIT_FILTER_NONE 0
#define LICE_BLIT_FILTER_BILINEAR 0x100 // currently pretty slow! ack


#define LICE_BLIT_USE_ALPHA 0x10000 // use source's alpha channel


// basic primitives
void LICE_PutPixel(LICE_IBitmap *bm, int x, int y, LICE_pixel color, float alpha, int mode);
LICE_pixel LICE_GetPixel(LICE_IBitmap *bm, int x, int y);

// blit functions

void LICE_Copy(LICE_IBitmap *dest, LICE_IBitmap *src); // resizes dest to fit


//alpha parameter = const alpha (combined with source alpha if spcified)
void LICE_Blit(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, RECT *srcrect, float alpha, int mode);

// dstw/dsty can be negative, srcw/srch can be as well (for flipping)
void LICE_ScaledBlit(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, int dstw, int dsth, 
                     float srcx, float srcy, float srcw, float srch, float alpha, int mode);


// if cliptosourcerect is false, then areas outside the source rect can get in (otherwise they are not drawn)
void LICE_RotatedBlit(LICE_IBitmap *dest, LICE_IBitmap *src, 
                      int dstx, int dsty, int dstw, int dsth, 
                      float srcx, float srcy, float srcw, float srch, 
                      float angle, 
                      bool cliptosourcerect, float alpha, int mode,
                      float rotxcent=0.0, float rotycent=0.0); // these coordinates are offset from the center of the image, in source pixel coordinates


// if cliptosourcerect is false, then areas outside the source rect can get in (otherwise they are not drawn)
void LICE_DeltaBlit(LICE_IBitmap *dest, LICE_IBitmap *src, 
                    int dstx, int dsty, int dstw, int dsth,                     
                    float srcx, float srcy, float srcw, float srch, 
                    double dsdx, double dtdx, double dsdy, double dtdy,                   
                    bool cliptosourcerect, float alpha, int mode);


// only LICE_BLIT_MODE_ADD or LICE_BLIT_MODE_COPY are used by this, for flags
// ir-ia should be 0.0..1.0 (or outside that and they'll be clamped)
// drdx should be X/dstw, drdy X/dsth etc
void LICE_GradRect(LICE_IBitmap *dest, int dstx, int dsty, int dstw, int dsth, 
                      float ir, float ig, float ib, float ia,
                      float drdx, float dgdx, float dbdx, float dadx,
                      float drdy, float dgdy, float dbdy, float dady,
                      int mode);

void LICE_Clear(LICE_IBitmap *dest, LICE_pixel color);
void LICE_ClearRect(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel mask=0, LICE_pixel orbits=0);
void LICE_MultiplyAddRect(LICE_IBitmap *dest, int x, int y, int w, int h, 
                          float rsc, float gsc, float bsc, float asc,
                          float radd, float gadd, float badd, float aadd);

void LICE_SetAlphaFromColorMask(LICE_IBitmap *dest, LICE_pixel color);



// texture generators
void LICE_TexGen_Marble(LICE_IBitmap *dest, RECT *rect, float rv, float gv, float bv, float intensity); //fills whole bitmap if rect == NULL

//this function generates a Perlin noise
//fills whole bitmap if rect == NULL
//smooth needs to be a multiple of 2
enum
{
  NOISE_MODE_NORMAL = 0,
  NOISE_MODE_WOOD,
};
void LICE_TexGen_Noise(LICE_IBitmap *dest, RECT *rect, float rv, float gv, float bv, float intensity, int mode=NOISE_MODE_NORMAL, int smooth=1); 

//this function generates a Perlin noise in a circular fashion
//fills whole bitmap if rect == NULL
//size needs to be a multiple of 2
void LICE_TexGen_CircNoise(LICE_IBitmap *dest, RECT *rect, float rv, float gv, float bv, float nrings, float power, int size);


// bitmapped text drawing:
void LICE_DrawChar(LICE_IBitmap *bm, int x, int y, char c, 
                   LICE_pixel color, float alpha, int mode);
void LICE_DrawText(LICE_IBitmap *bm, int x, int y, const char *string, 
                   LICE_pixel color, float alpha, int mode);
void LICE_MeasureText(const char *string, int *w, int *h);

// line drawing functions
void LICE_Line(LICE_IBitmap *dest, float x1, float y1, float x2, float y2, LICE_pixel color, float alpha=1.0f, int mode=0, bool aa=true);
void LICE_Arc(LICE_IBitmap* dest, float cx, float cy, float r, float minAngle, float maxAngle, 
              LICE_pixel color, float alpha=1.0f, int mode=0, bool aa=true);
void LICE_Circle(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha=1.0f, int mode=0, bool aa=true);

/*
  Stuff planned:

  
  void LICE_PutPixelAA(LICE_IBitmap *dest, float x, float y, LICE_pixel color, float alpha=1.0); // antialiased putpixel (can affect up to 4 pixels)
  void LICE_Rectangle(LICE_IBitmap *dest, float x1, float y1, float x2, float y2, LICE_pixel color, float alpha=1.0, bool aa=true);
  void LICE_Triangle(LICE_IBitmap *dest, float x1, float y1, float x2, float y2, float x3, float y3, LICE_pixel color, float alpha=1.0, bool aa=true);

  ScaleTransform() 
    optional filtering, you specify grid size and a transformation function (i.e. AVS Dynamic Movement). optional transformation specifying alpha at each point as well.

  TextureTriangle() 
    specify a dest triangle, and source coordinates.. affine texture mapped. optional filtering, optional alpha, etc.

  */
#endif