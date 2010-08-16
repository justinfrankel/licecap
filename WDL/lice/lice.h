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
#pragma warning(disable:4244) // float-to-int
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

#define LICE_PIXEL_A 0
#define LICE_PIXEL_R 1
#define LICE_PIXEL_G 2
#define LICE_PIXEL_B 3

#ifdef __ppc__

#define LICE_RGBA(r,g,b,a) (((b)&0xff)|(((g)&0xff)<<8)|(((r)&0xff)<<16)|(((a)&0xff)<<24))
#define LICE_GETB(v) ((v)&0xff)
#define LICE_GETG(v) (((v)>>8)&0xff)
#define LICE_GETR(v) (((v)>>16)&0xff)
#define LICE_GETA(v) (((v)>>24)&0xff)

#else

#define LICE_RGBA(r,g,b,a) (((a)&0xff)|(((r)&0xff)<<8)|(((g)&0xff)<<16)|(((b)&0xff)<<24))
#define LICE_GETA(v) ((v)&0xff)
#define LICE_GETR(v) (((v)>>8)&0xff)
#define LICE_GETG(v) (((v)>>16)&0xff)
#define LICE_GETB(v) (((v)>>24)&0xff)

#endif

#endif


static inline LICE_pixel LICE_RGBA_FROMNATIVE(LICE_pixel col, int alpha=0)
{
  return LICE_RGBA(GetRValue(col),GetGValue(col),GetBValue(col),alpha);
}

// bitmap interface, and some built-in types (memory bitmap and system bitmap)

class LICE_IBitmap
{
public:
  virtual ~LICE_IBitmap() { }

  virtual LICE_pixel *getBits()=0;
  virtual int getWidth()=0;
  virtual int getHeight()=0;
  virtual int getRowSpan()=0; // includes any off-bitmap data
  virtual bool resize(int w, int h)=0;

  virtual HDC getDC() { return 0; } // only sysbitmaps have to implement this

  bool isFlipped() const // non-virtual, we set this per-platform (if a platform needs Flipped(), all bitmaps must be flipped!)
  {
    return false; 
  }
};

class LICE_MemBitmap : public LICE_IBitmap
{
public:
  LICE_MemBitmap(int w=0, int h=0)
  {
    m_allocsize=(m_width=w)*(m_height=h)*sizeof(LICE_pixel);
    m_fb=m_allocsize>0?(LICE_pixel*)malloc(m_allocsize):0;
    if (!m_fb) {m_width=m_height=0; }
  }

  ~LICE_MemBitmap() { free(m_fb); }


  // LICE_IBitmap interface
  LICE_pixel *getBits() { return m_fb; }
  int getWidth() { return m_width; }
  int getHeight() { return m_height; }
  int getRowSpan() { return m_width; }
  bool resize(int w, int h); // returns TRUE if a resize occurred

private:
  LICE_pixel *m_fb;
  int m_width, m_height;
  int m_allocsize;
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
  int getRowSpan() { return m_allocw; }; 
  bool resize(int w, int h); // returns TRUE if a resize occurred

  // sysbitmap specific calls
  HDC getDC() { return m_dc; }


private:
  int m_width, m_height;

  HDC m_dc;
  LICE_pixel *m_bits;
  int m_allocw, m_alloch;
#ifdef _WIN32
  HBITMAP m_bitmap;
  HGDIOBJ m_oldbitmap;
#endif
};


class LICE_SubBitmap : public LICE_IBitmap // note: you should only keep these around as long as they are needed, and don't resize the parent while this is allocated
{
  public:
    LICE_SubBitmap(LICE_IBitmap *parent, int x, int y, int w, int h)
    {
      m_parent=parent;
      if(x<0)x=0; 
      if(y<0)y=0;
      m_x=x;m_y=y;
      resize(w,h);
    }
    ~LICE_SubBitmap() { }

    bool resize(int w, int h)
    {
      m_w=0;m_h=0;
      if (m_parent)
      {
        if(m_x+w>m_parent->getWidth()) w=m_parent->getWidth()-m_x;
        if (w<0)w=0;

        if (m_y+h>m_parent->getHeight()) h=m_parent->getHeight()-m_y;
        if (h<0)h=0;

        m_w=w; 
        m_h=h;
        m_parentptr=m_parent->getBits();
        if (m_parent->isFlipped()) m_parentptr += (m_parent->getHeight() - (m_y+m_h))*m_parent->getRowSpan()+m_x;
        else m_parentptr += m_y*m_parent->getRowSpan()+m_x;
      }

      return true;
    }


    LICE_pixel *getBits() { return m_parentptr; }
    int getWidth() { return m_w; }
    int getHeight() { return m_h; }
    int getRowSpan() { return m_parent ? m_parent->getRowSpan() : 0; }

    HDC getDC() { return NULL; }

    int m_w,m_h,m_x,m_y;
    LICE_IBitmap *m_parent;
    LICE_pixel *m_parentptr;
};
// bitmap loaders


// dispatch to a linked loader implementation based on file extension 
LICE_IBitmap* LICE_LoadImage(const char* filename, LICE_IBitmap* bmp=NULL, bool tryIgnoreExtension=false);
char *LICE_GetImageExtensionList(bool wantAllSup=true, bool wantAllFiles=true); // returns doublenull terminated GetOpenFileName() style list -- free() when done.



// pass a bmp if you wish to load it into that bitmap. note that if it fails bmp will not be deleted.
LICE_IBitmap *LICE_LoadPNG(const char *filename, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success
LICE_IBitmap *LICE_LoadPNGFromResource(HINSTANCE hInst, int resid, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success

LICE_IBitmap *LICE_LoadBMP(const char *filename, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success
LICE_IBitmap *LICE_LoadBMPFromResource(HINSTANCE hInst, int resid, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success

LICE_IBitmap *LICE_LoadIcon(const char *filename, int reqiconsz=16, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success
LICE_IBitmap *LICE_LoadIconFromResource(HINSTANCE hInst, int resid, int reqiconsz=16, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success

LICE_IBitmap *LICE_LoadJPG(const char *filename, LICE_IBitmap *bmp=NULL);
LICE_IBitmap *LICE_LoadGIF(const char *filename, LICE_IBitmap *bmp=NULL, int *nframes=NULL); // if nframes set, will be set to number of images (stacked vertically), otherwise first frame used

LICE_IBitmap *LICE_LoadPCX(const char *filename, LICE_IBitmap *bmp=NULL); // returns a bitmap (bmp if nonzero) on success


// bitmap saving
bool LICE_WritePNG(const char *filename, LICE_IBitmap *bmp, bool wantalpha=true);
bool LICE_WriteJPG(const char *filename, LICE_IBitmap *bmp, int quality=95, bool force_baseline=true);

// flags that most blit functions can take

#define LICE_BLIT_MODE_MASK 0xff
#define LICE_BLIT_MODE_COPY 0
#define LICE_BLIT_MODE_ADD 1
#define LICE_BLIT_MODE_DODGE 2
#define LICE_BLIT_MODE_MUL 3

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
void LICE_Blit(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, int srcx, int srcy, int srcw, int srch, float alpha, int mode);

void LICE_Blur(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, int srcx, int srcy, int srcw, int srch);

// dstw/dsty can be negative, srcw/srch can be as well (for flipping)
void LICE_ScaledBlit(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, int dstw, int dsth, 
                     float srcx, float srcy, float srcw, float srch, float alpha, int mode);


void LICE_HalveBlitAA(LICE_IBitmap *dest, LICE_IBitmap *src); // AA's src down to dest. uses the minimum size of both (use with LICE_SubBitmap to do sections)

// if cliptosourcerect is false, then areas outside the source rect can get in (otherwise they are not drawn)
void LICE_RotatedBlit(LICE_IBitmap *dest, LICE_IBitmap *src, 
                      int dstx, int dsty, int dstw, int dsth, 
                      float srcx, float srcy, float srcw, float srch, 
                      float angle, 
                      bool cliptosourcerect, float alpha, int mode,
                      float rotxcent=0.0, float rotycent=0.0); // these coordinates are offset from the center of the image, in source pixel coordinates


void LICE_TransformBlit(LICE_IBitmap *dest, LICE_IBitmap *src,  
                    int dstx, int dsty, int dstw, int dsth,
                    float *srcpoints, int div_w, int div_h, // srcpoints coords should be div_w*div_h*2 long, and be in source image coordinates
                    float alpha, int mode);

// if cliptosourcerect is false, then areas outside the source rect can get in (otherwise they are not drawn)
void LICE_DeltaBlit(LICE_IBitmap *dest, LICE_IBitmap *src, 
                    int dstx, int dsty, int dstw, int dsth,                     
                    float srcx, float srcy, float srcw, float srch, 
                    double dsdx, double dtdx, double dsdy, double dtdy,         
                    double dsdxdy, double dtdxdy,
                    bool cliptosourcerect, float alpha, int mode);


// only LICE_BLIT_MODE_ADD or LICE_BLIT_MODE_COPY are used by this, for flags
// ir-ia should be 0.0..1.0 (or outside that and they'll be clamped)
// drdx should be X/dstw, drdy X/dsth etc
void LICE_GradRect(LICE_IBitmap *dest, int dstx, int dsty, int dstw, int dsth, 
                      float ir, float ig, float ib, float ia,
                      float drdx, float dgdx, float dbdx, float dadx,
                      float drdy, float dgdy, float dbdy, float dady,
                      int mode);

void LICE_FillRect(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel color, float alpha = 1.0f, int mode = 0);

void LICE_Clear(LICE_IBitmap *dest, LICE_pixel color);
void LICE_ClearRect(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel mask=0, LICE_pixel orbits=0);
void LICE_MultiplyAddRect(LICE_IBitmap *dest, int x, int y, int w, int h, 
                          float rsc, float gsc, float bsc, float asc, // 0-1, or -100 .. +100 if you really are insane
                          float radd, float gadd, float badd, float aadd); // 0-255 is the normal range on these.. of course its clamped

void LICE_SetAlphaFromColorMask(LICE_IBitmap *dest, LICE_pixel color);


// non-flood fill. simply scans up/down and left/right
void LICE_SimpleFill(LICE_IBitmap *dest, int x, int y, LICE_pixel newcolor,  
                     LICE_pixel comparemask=LICE_RGBA(255,255,255,0), 
                     LICE_pixel keepmask=LICE_RGBA(0,0,0,0));


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
void LICE_FillTriangle(LICE_IBitmap *dest, int x1, int y1, int x2, int y2, int x3, int y3, LICE_pixel color, float alpha=1.0f, int mode=0);

// Returns false if the line is entirely offscreen.
bool LICE_ClipLine(float* pX1, float* pY1, float* pX2, float* pY2, int xLo, int yLo, int xHi, int yHi);

void LICE_Arc(LICE_IBitmap* dest, float cx, float cy, float r, float minAngle, float maxAngle, 
              LICE_pixel color, float alpha=1.0f, int mode=0, bool aa=true);
void LICE_Circle(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha=1.0f, int mode=0, bool aa=true);
void LICE_FillCircle(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha=1.0f, int mode=0);
void LICE_RoundRect(LICE_IBitmap *drawbm, float xpos, float ypos, float w, float h, int cornerradius,
                    LICE_pixel col, float alpha, int mode, bool aa);

// useful for drawing shapes from a cache
void LICE_DrawGlyph(LICE_IBitmap* dest, int x, int y, LICE_pixel color, LICE_pixel_chan* alphas, int glyph_w, int glyph_h, float alpha=1.0f, int mode = 0);

// quadratic bezier
void LICE_DrawBezier(LICE_IBitmap* dest, float xstart, float ystart, float xctl, float yctl, float xend, float yend, 
  LICE_pixel color, float alpha=1.0f, int mode=0, bool aa=true);

// cubic bezier
void LICE_DrawCBezier(LICE_IBitmap* dest, float xstart, float ystart, float xctl1, float yctl1,
  float xctl2, float yctl2, float xend, float yend, LICE_pixel color, float alpha=1.0f, int mode=0, bool aa=true);


// convenience functions
void LICE_DrawRect(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel color, float alpha=1.0f, int mode=0);
void LICE_BorderedRect(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel bgcolor, LICE_pixel fgcolor, float alpha=1.0f, int mode=0);

// bitmap compare-by-value function
int LICE_BitmapCmp(LICE_IBitmap* a, LICE_IBitmap* b);








struct _LICE_ImageLoader_rec
{
  LICE_IBitmap *(*loadfunc)(const char *filename, bool checkFileName, LICE_IBitmap *bmpbase); 
  const char *(*get_extlist)(); // returns GetOpenFileName sort of list "JPEG files (*.jpg)\0*.jpg\0"

  struct _LICE_ImageLoader_rec *_next;
};
extern _LICE_ImageLoader_rec *LICE_ImageLoader_list;



#endif
