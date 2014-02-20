#ifndef _EEL_LICE_H_
#define _EEL_LICE_H_

// #define EEL_LICE_GET_FILENAME_FOR_STRING(idx, fs, p) (((sInst*)opaque)->GetFilenameForParameter(idx,fs,p))
// #define EEL_LICE_GET_CONTEXT(opaque) (((opaque) ? (((sInst *)opaque)->m_gfx_state) : NULL)


void eel_lice_register();

#ifdef DYNAMIC_LICE
  #define LICE_IBitmap LICE_IBitmap_disabledAPI
    #include "../lice/lice.h"

  #undef LICE_IBitmap
  typedef void LICE_IBitmap; // prevent us from using LICE api directly, in case it ever changes

class LICE_IFont;

#ifdef EEL_LICE_API_ONLY
#define EEL_LICE_FUNCDEF extern
#else
#define EEL_LICE_FUNCDEF
#endif

#define LICE_FUNCTION_VALID(x) (x)

EEL_LICE_FUNCDEF LICE_IBitmap *(*__LICE_CreateBitmap)(int, int, int);
EEL_LICE_FUNCDEF void (*__LICE_Clear)(LICE_IBitmap *dest, LICE_pixel color);
EEL_LICE_FUNCDEF void (*__LICE_Line)(LICE_IBitmap *dest, int x1, int y1, int x2, int y2, LICE_pixel color, float alpha, int mode, bool aa);
EEL_LICE_FUNCDEF bool (*__LICE_ClipLine)(int* pX1, int* pY1, int* pX2, int* pY2, int xLo, int yLo, int xHi, int yHi);
EEL_LICE_FUNCDEF void (*__LICE_DrawText)(LICE_IBitmap *bm, int x, int y, const char *string, 
                   LICE_pixel color, float alpha, int mode);
EEL_LICE_FUNCDEF void (*__LICE_DrawChar)(LICE_IBitmap *bm, int x, int y, char c, 
                   LICE_pixel color, float alpha, int mode);
EEL_LICE_FUNCDEF void (*__LICE_MeasureText)(const char *string, int *w, int *h);
EEL_LICE_FUNCDEF void (*__LICE_PutPixel)(LICE_IBitmap *bm, int x, int y, LICE_pixel color, float alpha, int mode);
EEL_LICE_FUNCDEF LICE_pixel (*__LICE_GetPixel)(LICE_IBitmap *bm, int x, int y);
EEL_LICE_FUNCDEF void (*__LICE_FillRect)(LICE_IBitmap *dest, int x, int y, int w, int h, LICE_pixel color, float alpha, int mode);
EEL_LICE_FUNCDEF LICE_IBitmap *(*__LICE_LoadImage)(const char* filename, LICE_IBitmap* bmp, bool tryIgnoreExtension);
EEL_LICE_FUNCDEF void (*__LICE_Blur)(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, int srcx, int srcy, int srcw, int srch); // src and dest can overlap, however it may look fudgy if they do
EEL_LICE_FUNCDEF void (*__LICE_ScaledBlit)(LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, int dstw, int dsth, 
                     float srcx, float srcy, float srcw, float srch, float alpha, int mode);
EEL_LICE_FUNCDEF void (*__LICE_Circle)(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, int mode, bool aa);
EEL_LICE_FUNCDEF void (*__LICE_FillCircle)(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, int mode, bool aa);
EEL_LICE_FUNCDEF void (*__LICE_RoundRect)(LICE_IBitmap *drawbm, float xpos, float ypos, float w, float h, int cornerradius, LICE_pixel col, float alpha, int mode, bool aa);
EEL_LICE_FUNCDEF void (*__LICE_Arc)(LICE_IBitmap* dest, float cx, float cy, float r, float minAngle, float maxAngle, LICE_pixel color, float alpha, int mode, bool aa);

// if cliptosourcerect is false, then areas outside the source rect can get in (otherwise they are not drawn)
EEL_LICE_FUNCDEF void (*__LICE_RotatedBlit)(LICE_IBitmap *dest, LICE_IBitmap *src, 
                      int dstx, int dsty, int dstw, int dsth, 
                      float srcx, float srcy, float srcw, float srch, 
                      float angle, 
                      bool cliptosourcerect, float alpha, int mode,
                      float rotxcent, float rotycent); // these coordinates are offset from the center of the image, in source pixel coordinates


EEL_LICE_FUNCDEF void (*__LICE_MultiplyAddRect)(LICE_IBitmap *dest, int x, int y, int w, int h, 
                          float rsc, float gsc, float bsc, float asc, // 0-1, or -100 .. +100 if you really are insane
                          float radd, float gadd, float badd, float aadd); // 0-255 is the normal range on these.. of course its clamped

EEL_LICE_FUNCDEF void (*__LICE_GradRect)(LICE_IBitmap *dest, int dstx, int dsty, int dstw, int dsth, 
                      float ir, float ig, float ib, float ia,
                      float drdx, float dgdx, float dbdx, float dadx,
                      float drdy, float dgdy, float dbdy, float dady,
                      int mode);

EEL_LICE_FUNCDEF void (*__LICE_TransformBlit2)(LICE_IBitmap *dest, LICE_IBitmap *src,  
                    int dstx, int dsty, int dstw, int dsth,
                    double *srcpoints, int div_w, int div_h, // srcpoints coords should be div_w*div_h*2 long, and be in source image coordinates
                    float alpha, int mode);

EEL_LICE_FUNCDEF void (*__LICE_DeltaBlit)(LICE_IBitmap *dest, LICE_IBitmap *src, 
                    int dstx, int dsty, int dstw, int dsth,                     
                    float srcx, float srcy, float srcw, float srch, 
                    double dsdx, double dtdx, double dsdy, double dtdy,         
                    double dsdxdy, double dtdxdy,
                    bool cliptosourcerect, float alpha, int mode);


#define LICE_Blur __LICE_Blur
#define LICE_Clear __LICE_Clear
#define LICE_Line __LICE_Line
#define LICE_ClipLine __LICE_ClipLine
#define LICE_FillRect __LICE_FillRect
#define LICE_PutPixel __LICE_PutPixel
#define LICE_GetPixel __LICE_GetPixel
#define LICE_DrawText __LICE_DrawText
#define LICE_DrawChar __LICE_DrawChar
#define LICE_MeasureText __LICE_MeasureText
#define LICE_LoadImage __LICE_LoadImage
#define LICE_RotatedBlit __LICE_RotatedBlit
#define LICE_ScaledBlit __LICE_ScaledBlit
#define LICE_MultiplyAddRect __LICE_MultiplyAddRect
#define LICE_GradRect __LICE_GradRect
#define LICE_TransformBlit2 __LICE_TransformBlit2
#define LICE_DeltaBlit __LICE_DeltaBlit
#define LICE_Circle __LICE_Circle
#define LICE_FillCircle __LICE_FillCircle
#define LICE_RoundRect __LICE_RoundRect
#define LICE_Arc __LICE_Arc

EEL_LICE_FUNCDEF HDC (*LICE__GetDC)(LICE_IBitmap *bm);
EEL_LICE_FUNCDEF int (*LICE__GetWidth)(LICE_IBitmap *bm);
EEL_LICE_FUNCDEF int (*LICE__GetHeight)(LICE_IBitmap *bm);
EEL_LICE_FUNCDEF void (*LICE__Destroy)(LICE_IBitmap *bm);
EEL_LICE_FUNCDEF bool (*LICE__resize)(LICE_IBitmap *bm, int w, int h);

EEL_LICE_FUNCDEF void (*LICE__DestroyFont)(LICE_IFont* font);
EEL_LICE_FUNCDEF LICE_IFont *(*LICE_CreateFont)();
EEL_LICE_FUNCDEF void (*LICE__SetFromHFont)(LICE_IFont* ifont, HFONT font, int flags);
EEL_LICE_FUNCDEF LICE_pixel (*LICE__SetTextColor)(LICE_IFont* ifont, LICE_pixel color);
EEL_LICE_FUNCDEF LICE_pixel (*LICE__SetBkColor)(LICE_IFont* ifont, LICE_pixel color);
EEL_LICE_FUNCDEF void (*LICE__SetTextCombineMode)(LICE_IFont* ifont, int mode, float alpha);
EEL_LICE_FUNCDEF int (*LICE__DrawText)(LICE_IFont* ifont, LICE_IBitmap *bm, const char *str, int strcnt, RECT *rect, UINT dtFlags);

#else

#include "../lice/lice.h"
#include "../lice/lice_text.h"
#define LICE_FUNCTION_VALID(x) (sizeof(int) > 0)

static HDC LICE__GetDC(LICE_IBitmap *bm)
{
  return bm->getDC();
}
static int LICE__GetWidth(LICE_IBitmap *bm)
{
  return bm->getWidth();
}
static int LICE__GetHeight(LICE_IBitmap *bm)
{
  return bm->getHeight();
}
static void LICE__Destroy(LICE_IBitmap *bm)
{
  delete bm;
}
static void LICE__SetFromHFont(LICE_IFont * ifont, HFONT font, int flags)
{
  if (ifont) ifont->SetFromHFont(font,flags);
}
static LICE_pixel LICE__SetTextColor(LICE_IFont* ifont, LICE_pixel color)
{
  if (ifont) return ifont->SetTextColor(color);
  return 0;
}
static LICE_pixel LICE__SetBkColor(LICE_IFont* ifont, LICE_pixel color)
{
  if (ifont) return ifont->SetBkColor(color);
  return 0;
}
static void LICE__SetTextCombineMode(LICE_IFont* ifont, int mode, float alpha)
{
  if (ifont) ifont->SetCombineMode(mode, alpha);
}
static int LICE__DrawText(LICE_IFont* ifont, LICE_IBitmap *bm, const char *str, int strcnt, RECT *rect, UINT dtFlags)
{
  if (ifont) return ifont->DrawText(bm, str, strcnt, rect, dtFlags);
  return 0;
}


static LICE_IFont *LICE_CreateFont()
{
  return new LICE_CachedFont();
}
static void LICE__DestroyFont(LICE_IFont *bm)
{
  delete bm;
}
static bool LICE__resize(LICE_IBitmap *bm, int w, int h)
{
  return bm->resize(w,h);
}

static LICE_IBitmap *__LICE_CreateBitmap(int mode, int w, int h)
{
  if (mode==1) return new LICE_SysBitmap(w,h);
  return new LICE_MemBitmap(w,h);
}


#endif




class eel_lice_state
{
public:

  eel_lice_state(NSEEL_VMCTX vm, void *ctx, int image_slots, int font_slots);
  ~eel_lice_state();

  void resetVarsToStock()
  {
    if (m_gfx_a&&m_gfx_r&&m_gfx_g&&m_gfx_b) *m_gfx_r=*m_gfx_g=*m_gfx_b=*m_gfx_a=1.0;
    if (m_gfx_dest) *m_gfx_dest=-1.0;
    if (m_mouse_wheel) *m_mouse_wheel=0.0;
    if (m_mouse_hwheel) *m_mouse_hwheel=0.0;
    // todo: reset others?
  }

  LICE_IBitmap *m_framebuffer, *m_framebuffer_extra;
  int m_framebuffer_refstate;
  WDL_TypedBuf<LICE_IBitmap *> m_gfx_images;
  struct gfxFontStruct {
    LICE_IFont *font;
    char last_fontname[128];
    int last_fontsize;
    int last_fontflag;

    int use_fonth;
  }; 
  WDL_TypedBuf<gfxFontStruct> m_gfx_fonts;
  int m_gfx_font_active; // -1 for default, otherwise index into gfx_fonts (NOTE: this differs from the exposed API, which defines 0 as default, 1-n)
  LICE_IFont *GetActiveFont() { return m_gfx_font_active>=0&&m_gfx_font_active<m_gfx_fonts.GetSize() && m_gfx_fonts.Get()[m_gfx_font_active].use_fonth ? m_gfx_fonts.Get()[m_gfx_font_active].font : NULL; }

  LICE_IBitmap *GetImageForIndex(EEL_F idx, const char *callername) 
  { 
    if (idx>-2.0)
    {
      if (idx < 0.0) return m_framebuffer;

      const int a = (int)idx;
      if (a >= 0 && a < m_gfx_images.GetSize()) return m_gfx_images.Get()[a];
    }
    return NULL;
  };

  void SetImageDirty(LICE_IBitmap *bm)
  {
    if (bm == m_framebuffer) m_framebuffer_refstate=1;
  }

  // R, G, B, A, w, h, x, y, mode(1=add,0=copy)
  EEL_F *m_gfx_r, *m_gfx_g, *m_gfx_b, *m_gfx_w, *m_gfx_h, *m_gfx_a, *m_gfx_x, *m_gfx_y, *m_gfx_mode, *m_gfx_clear, *m_gfx_texth,*m_gfx_dest;
  EEL_F *m_mouse_x, *m_mouse_y, *m_mouse_cap, *m_mouse_wheel, *m_mouse_hwheel;

  NSEEL_VMCTX m_vmref;
  void *m_user_ctx;

  int setup_frame(HWND hwnd, RECT r);

  void gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F aaflag);
  void gfx_rectto(EEL_F xpos, EEL_F ypos);
  void gfx_line(int np, EEL_F **parms);
  void gfx_rect(int np, EEL_F **parms);
  void gfx_roundrect(int np, EEL_F **parms);
  void gfx_arc(int np, EEL_F **parms);
  void gfx_grad_or_muladd_rect(int mode, int np, EEL_F **parms);
  void gfx_setpixel(EEL_F r, EEL_F g, EEL_F b);
  void gfx_getpixel(EEL_F *r, EEL_F *g, EEL_F *b);
  void gfx_drawnumber(EEL_F n, EEL_F ndigits);
  void gfx_drawchar(EEL_F ch);
  void gfx_getimgdim(EEL_F img, EEL_F *w, EEL_F *h);
  EEL_F gfx_setimgdim(int img, EEL_F *w, EEL_F *h);
  void gfx_blurto(EEL_F x, EEL_F y);
  void gfx_blit(EEL_F img, EEL_F scale, EEL_F rotate);
  void gfx_blitext(EEL_F img, EEL_F *coords, EEL_F angle);
  void gfx_blitext2(int np, EEL_F **parms, int mode); // 0=blit, 1=deltablit
  void gfx_transformblit(EEL_F **parms, int div_w, int div_h, EEL_F *tab); // parms[0]=src, 1-4=x,y,w,h
  void gfx_circle(float x, float y, float r, bool fill, bool aaflag);
 
  void gfx_drawstr(void *opaque, EEL_F str, int formatmode, int nfmtparms, EEL_F **fmtparms); // formatmode=1 for format, 2 for purely measure no format
  EEL_F gfx_loadimg(void *opaque, int img, EEL_F loadFrom);
  EEL_F gfx_setfont(void *opaque, int np, EEL_F **parms);
  EEL_F gfx_getfont(void *opaque, int np, EEL_F **parms);

  LICE_pixel getCurColor();
  int getCurMode();
  int getCurModeForBlit(bool isFBsrc);


#ifdef EEL_LICE_WANT_STANDALONE
  HWND create_wnd(HWND par, int isChild);
  HWND hwnd_standalone;
  int hwnd_standalone_kb_state[32]; // pressed keys, if any

  int m_kb_queue[64];
  unsigned char m_kb_queue_valid;
  unsigned char m_kb_queue_pos;

#endif
  bool m_has_had_getch; // set on first gfx_getchar(), makes mouse_cap updated with modifiers even when no mouse click is down
};


#ifndef EEL_LICE_API_ONLY

eel_lice_state::eel_lice_state(NSEEL_VMCTX vm, void *ctx, int image_slots, int font_slots)
{
#ifdef EEL_LICE_WANT_STANDALONE
  hwnd_standalone=NULL;
  memset(hwnd_standalone_kb_state,0,sizeof(hwnd_standalone_kb_state));
  m_kb_queue_valid=0;
#endif
  m_user_ctx=ctx;
  m_vmref= vm;
  m_gfx_font_active=-1;
  m_gfx_fonts.Resize(font_slots);
  memset(m_gfx_fonts.Get(),0,m_gfx_fonts.GetSize()*sizeof(m_gfx_fonts.Get()[0]));

  m_gfx_images.Resize(image_slots);
  memset(m_gfx_images.Get(),0,m_gfx_images.GetSize()*sizeof(m_gfx_images.Get()[0]));
  m_framebuffer=m_framebuffer_extra=0;
  m_framebuffer_refstate=0;

  m_gfx_r = NSEEL_VM_regvar(vm,"gfx_r");
  m_gfx_g = NSEEL_VM_regvar(vm,"gfx_g");
  m_gfx_b = NSEEL_VM_regvar(vm,"gfx_b");
  m_gfx_a = NSEEL_VM_regvar(vm,"gfx_a");

  m_gfx_w = NSEEL_VM_regvar(vm,"gfx_w");
  m_gfx_h = NSEEL_VM_regvar(vm,"gfx_h");
  m_gfx_x = NSEEL_VM_regvar(vm,"gfx_x");
  m_gfx_y = NSEEL_VM_regvar(vm,"gfx_y");
  m_gfx_mode = NSEEL_VM_regvar(vm,"gfx_mode");
  m_gfx_clear = NSEEL_VM_regvar(vm,"gfx_clear");
  m_gfx_texth = NSEEL_VM_regvar(vm,"gfx_texth");
  m_gfx_dest = NSEEL_VM_regvar(vm,"gfx_dest");

  m_mouse_x = NSEEL_VM_regvar(vm,"mouse_x");
  m_mouse_y = NSEEL_VM_regvar(vm,"mouse_y");
  m_mouse_cap = NSEEL_VM_regvar(vm,"mouse_cap");
  m_mouse_wheel=NSEEL_VM_regvar(vm,"mouse_wheel");
  m_mouse_hwheel=NSEEL_VM_regvar(vm,"mouse_hwheel");

  if (m_gfx_texth) *m_gfx_texth=8;

  m_has_had_getch=0;
}
eel_lice_state::~eel_lice_state()
{
#ifdef EEL_LICE_WANT_STANDALONE
  if (hwnd_standalone) DestroyWindow(hwnd_standalone);
#endif
  if (LICE_FUNCTION_VALID(LICE__Destroy)) 
  {
    LICE__Destroy(m_framebuffer_extra);
    LICE__Destroy(m_framebuffer);
    int x;
    for (x=0;x<m_gfx_images.GetSize();x++)
    {
      LICE__Destroy(m_gfx_images.Get()[x]);
    }
  }
  if (LICE_FUNCTION_VALID(LICE__DestroyFont))
  {
    int x;
    for (x=0;x<m_gfx_fonts.GetSize();x++)
    {
      if (m_gfx_fonts.Get()[x].font) LICE__DestroyFont(m_gfx_fonts.Get()[x].font);
    }
  }
}

int eel_lice_state::getCurMode()
{
  const int gmode = (int) (*m_gfx_mode);
  const int sm=(gmode>>4)&0xf;
  if (sm > LICE_BLIT_MODE_COPY && sm <= LICE_BLIT_MODE_HSVADJ) return sm;

  return (gmode&1) ? LICE_BLIT_MODE_ADD : LICE_BLIT_MODE_COPY;
}
int eel_lice_state::getCurModeForBlit(bool isFBsrc)
{
  const int gmode = (int) (*m_gfx_mode);
 
  const int sm=(gmode>>4)&0xf;

  int mode;
  if (sm > LICE_BLIT_MODE_COPY && sm <= LICE_BLIT_MODE_HSVADJ) mode=sm;
  else mode=((gmode&1) ? LICE_BLIT_MODE_ADD : LICE_BLIT_MODE_COPY);


  if (!isFBsrc && !(gmode&2)) mode|=LICE_BLIT_USE_ALPHA;
  if (!(gmode&4)) mode|=LICE_BLIT_FILTER_BILINEAR;
 
  return mode;
}
LICE_pixel eel_lice_state::getCurColor()
{
  int red=(int) (*m_gfx_r*255.0);
  int green=(int) (*m_gfx_g*255.0);
  int blue=(int) (*m_gfx_b*255.0);
  if (red<0) red=0;else if (red>255)red=255;
  if (green<0) green=0;else if (green>255)green=255;
  if (blue<0) blue=0; else if (blue>255) blue=255;
  return LICE_RGBA(red,green,blue,255);
}


static EEL_F * NSEEL_CGEN_CALL _gfx_lineto(void *opaque, EEL_F *xpos, EEL_F *ypos, EEL_F *useaa)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_lineto(*xpos, *ypos, *useaa);
  return xpos;
}
static EEL_F * NSEEL_CGEN_CALL _gfx_lineto2(void *opaque, EEL_F *xpos, EEL_F *ypos)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_lineto(*xpos, *ypos, 1.0f);
  return xpos;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_rectto(void *opaque, EEL_F *xpos, EEL_F *ypos)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_rectto(*xpos, *ypos);
  return xpos;
}

static EEL_F NSEEL_CGEN_CALL _gfx_line(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_line((int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_rect(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_rect((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL _gfx_roundrect(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_roundrect((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL _gfx_arc(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_arc((int)np,parms);
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL _gfx_gradrect(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_grad_or_muladd_rect(0,(int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_muladdrect(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_grad_or_muladd_rect(1,(int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_deltablit(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_blitext2((int)np,parms,1);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_transformblit(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) 
  {
    EEL_F **blocks = ctx->m_vmref  ? ((compileContext*)ctx->m_vmref)->ram_state.blocks : 0;
    if (!blocks || np < 8) return 0.0;

    const int divw = (int) (parms[5][0]+0.5);
    const int divh = (int) (parms[6][0]+0.5);
    if (divw < 1 || divh < 1) return 0.0;

    const int addr1= (int) (parms[7][0]+0.5);
    const int sz = divw*divh*2;
    EEL_F *d=__NSEEL_RAMAlloc(blocks,addr1);
    if (sz>NSEEL_RAM_ITEMSPERBLOCK)
    {
      int x;
      for(x=NSEEL_RAM_ITEMSPERBLOCK;x<sz-1;x+=NSEEL_RAM_ITEMSPERBLOCK)
        if (__NSEEL_RAMAlloc(blocks,addr1+x) != d+x) return 0.0;
    }
    EEL_F *end=__NSEEL_RAMAlloc(blocks,addr1+sz-1);
    if (end != d+sz-1) return 0.0; // buffer not contiguous

    ctx->gfx_transformblit(parms,divw,divh,d);
  }
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_circle(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  bool aa = true, fill = false;
  if (np>3) fill = parms[3][0] > 0.5;
  if (np>4) aa = parms[4][0] > 0.5;
  if (ctx) ctx->gfx_circle((float)parms[0][0], (float)parms[1][0], (float)parms[2][0], fill, aa);
  return 0.0;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_drawnumber(void *opaque, EEL_F *n, EEL_F *nd)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_drawnumber(*n, *nd);
  return n;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_drawchar(void *opaque, EEL_F *n)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_drawchar(*n);
  return n;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_measurestr(void *opaque, EEL_F *str, EEL_F *xOut, EEL_F *yOut)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) 
  {
    EEL_F *p[2]={xOut,yOut};
    ctx->gfx_drawstr(opaque,*str,2,2,p);
  }
  return str;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_drawstr(void *opaque, EEL_F *n)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_drawstr(opaque,*n,0,0,NULL);
  return n;
}
static EEL_F NSEEL_CGEN_CALL _gfx_printf(void *opaque, INT_PTR nparms, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx && nparms>0) 
  {
    EEL_F v= **parms;
    ctx->gfx_drawstr(opaque,v,1,(int)nparms-1,parms+1);
    return v;
  }
  return 0.0;
}
static EEL_F * NSEEL_CGEN_CALL _gfx_setpixel(void *opaque, EEL_F *r, EEL_F *g, EEL_F *b)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_setpixel(*r, *g, *b);
  return r;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_getpixel(void *opaque, EEL_F *r, EEL_F *g, EEL_F *b)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_getpixel(r, g, b);
  return r;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_blit(void *opaque, EEL_F *img, EEL_F *scale, EEL_F *rotate)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_blit(*img,*scale,*rotate);
  return img;
}

static EEL_F NSEEL_CGEN_CALL _gfx_setfont(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) return ctx->gfx_setfont(opaque,(int)np,parms);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_getfont(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) return ctx->m_gfx_font_active+1;
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_blit2(void *opaque, INT_PTR np, EEL_F **parms)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx && np>=3) 
  {
    ctx->gfx_blitext2((int)np,parms,0);
    return *(parms[0]);
  }
  return 0.0;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_blitext(void *opaque, EEL_F *img, EEL_F *coordidx, EEL_F *rotate)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) 
  {
    EEL_F fc = *coordidx;
    if (fc < -0.5 || fc >= NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK) return img;
    int a=(int)fc;
    if (a<0) return img;
        
    EEL_F buf[10];
    int x;
    EEL_F **blocks = ctx->m_vmref  ? ((compileContext*)ctx->m_vmref)->ram_state.blocks : 0;
    if (!blocks) return img;
    for (x = 0;x < 10; x ++)
    {
      EEL_F *d=__NSEEL_RAMAlloc(blocks,a++);
      if (!d || d==&nseel_ramalloc_onfail) return img;
      buf[x]=*d;
    }
    // read megabuf
    ctx->gfx_blitext(*img,buf,*rotate);
  }
  return img;
}


static EEL_F * NSEEL_CGEN_CALL _gfx_blurto(void *opaque, EEL_F *x, EEL_F *y)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_blurto(*x,*y);
  return x;
}

static EEL_F * NSEEL_CGEN_CALL _gfx_getimgdim(void *opaque, EEL_F *img, EEL_F *w, EEL_F *h)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) ctx->gfx_getimgdim(*img,w,h);
  return img;
}

static EEL_F NSEEL_CGEN_CALL _gfx_loadimg(void *opaque, EEL_F *img, EEL_F *fr)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) return ctx->gfx_loadimg(opaque,(int)*img,*fr);
  return 0.0;
}

static EEL_F NSEEL_CGEN_CALL _gfx_setimgdim(void *opaque, EEL_F *img, EEL_F *w, EEL_F *h)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx) return ctx->gfx_setimgdim((int)*img,w,h);
  return 0.0;
}




void eel_lice_state::gfx_lineto(EEL_F xpos, EEL_F ypos, EEL_F aaflag)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_lineto");
  if (!dest) return;

  int x1=(int)floor(xpos),y1=(int)floor(ypos),x2=(int)floor(*m_gfx_x), y2=(int)floor(*m_gfx_y);
  if (LICE_FUNCTION_VALID(LICE__GetWidth) && LICE_FUNCTION_VALID(LICE__GetHeight) && LICE_FUNCTION_VALID(LICE_Line) && 
      LICE_FUNCTION_VALID(LICE_ClipLine) && 
      LICE_ClipLine(&x1,&y1,&x2,&y2,0,0,LICE__GetWidth(dest),LICE__GetHeight(dest))) 
  {
    LICE_Line(dest,x1,y1,x2,y2,getCurColor(),(float) *m_gfx_a,getCurMode(),aaflag > 0.5);
    SetImageDirty(dest);
  }
  *m_gfx_x = xpos;
  *m_gfx_y = ypos;
  
}

void eel_lice_state::gfx_circle(float x, float y, float r, bool fill, bool aaflag)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_circle");
  if (!dest) return;

  if (LICE_FUNCTION_VALID(LICE_Circle) && LICE_FUNCTION_VALID(LICE_FillCircle))
  {
    if(fill)
      LICE_FillCircle(dest, x, y, r, getCurColor(), (float) *m_gfx_a, getCurMode(), aaflag);
    else
      LICE_Circle(dest, x, y, r, getCurColor(), (float) *m_gfx_a, getCurMode(), aaflag);
    SetImageDirty(dest);
  }
}

void eel_lice_state::gfx_rectto(EEL_F xpos, EEL_F ypos)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_rectto");
  if (!dest) return;

  EEL_F x1=xpos,y1=ypos,x2=*m_gfx_x, y2=*m_gfx_y;
  if (x2<x1) { x1=x2; x2=xpos; }
  if (y2<y1) { y1=y2; y2=ypos; }

  if (LICE_FUNCTION_VALID(LICE_FillRect) && x2-x1 > 0.5 && y2-y1 > 0.5)
  {
    LICE_FillRect(dest,(int)x1,(int)y1,(int)(x2-x1),(int)(y2-y1),getCurColor(),(float)*m_gfx_a,getCurMode());

    SetImageDirty(dest);
  }
  *m_gfx_x = xpos;
  *m_gfx_y = ypos;
}


void eel_lice_state::gfx_line(int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_line");
  if (!dest) return;

  int x1=(int)floor(parms[0][0]),y1=(int)floor(parms[1][0]),x2=(int)floor(parms[2][0]), y2=(int)floor(parms[3][0]);
  if (LICE_FUNCTION_VALID(LICE__GetWidth) && 
      LICE_FUNCTION_VALID(LICE__GetHeight) && 
      LICE_FUNCTION_VALID(LICE_Line) && 
      LICE_FUNCTION_VALID(LICE_ClipLine) && LICE_ClipLine(&x1,&y1,&x2,&y2,0,0,LICE__GetWidth(dest),LICE__GetHeight(dest))) 
  {
    LICE_Line(dest,x1,y1,x2,y2,getCurColor(),(float)*m_gfx_a,getCurMode(),np< 5 || parms[4][0] > 0.5);
    SetImageDirty(dest);
  } 
}

void eel_lice_state::gfx_rect(int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_rect");
  if (!dest) return;

  int x1=(int)floor(parms[0][0]),y1=(int)floor(parms[1][0]),w=(int)floor(parms[2][0]), h=(int)floor(parms[3][0]);

  if (LICE_FUNCTION_VALID(LICE_FillRect) && w>0 && h>0)
  {
    LICE_FillRect(dest,x1,y1,w,h,getCurColor(),(float)*m_gfx_a,getCurMode());

    SetImageDirty(dest);
  }
}

void eel_lice_state::gfx_roundrect(int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_roundrect");
  if (!dest) return;

  const bool aa = np <= 5 || parms[5][0]>0.5;

  if (LICE_FUNCTION_VALID(LICE_RoundRect) && parms[2][0]>0 && parms[3][0]>0)
  {
    LICE_RoundRect(dest, (float)parms[0][0], (float)parms[1][0], (float)parms[2][0], (float)parms[3][0], (int)parms[4][0], getCurColor(), (float)*m_gfx_a, getCurMode(), aa);
    SetImageDirty(dest);
  }
}

void eel_lice_state::gfx_arc(int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_arc");
  if (!dest) return;

  const bool aa = np <= 5 || parms[5][0]>0.5;

  if (LICE_FUNCTION_VALID(LICE_Arc))
  {
    LICE_Arc(dest, (float)parms[0][0], (float)parms[1][0], (float)parms[2][0], (float)parms[3][0], (float)parms[4][0], getCurColor(), (float)*m_gfx_a, getCurMode(), aa);
    SetImageDirty(dest);
  }
}

void eel_lice_state::gfx_grad_or_muladd_rect(int whichmode, int np, EEL_F **parms)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,whichmode==0?"gfx_gradrect":"gfx_muladdrect");
  if (!dest) return;

  const int x1=(int)floor(parms[0][0]),y1=(int)floor(parms[1][0]),w=(int)floor(parms[2][0]), h=(int)floor(parms[3][0]);

  if (w>0 && h>0)
  {
    if (whichmode==0 && LICE_FUNCTION_VALID(LICE_GradRect) && np > 7)
    {
      LICE_GradRect(dest,x1,y1,w,h,(float)parms[4][0],(float)parms[5][0],(float)parms[6][0],(float)parms[7][0],
                                   np > 8 ? (float)parms[8][0]:0.0f, np > 9 ? (float)parms[9][0]:0.0f,  np > 10 ? (float)parms[10][0]:0.0f, np > 11 ? (float)parms[11][0]:0.0f,  
                                   np > 12 ? (float)parms[12][0]:0.0f, np > 13 ? (float)parms[13][0]:0.0f,  np > 14 ? (float)parms[14][0]:0.0f, np > 15 ? (float)parms[15][0]:0.0f,  
                                   getCurMode());

      SetImageDirty(dest);
    }
    else if (whichmode==1 && LICE_FUNCTION_VALID(LICE_MultiplyAddRect) && np > 6)
    {
      const double sc = 255.0;
      LICE_MultiplyAddRect(dest,x1,y1,w,h,(float)parms[4][0],(float)parms[5][0],(float)parms[6][0],np>7 ? (float)parms[7][0]:1.0f,
        (float)(np > 8 ? sc*parms[8][0]:0.0), (float)(np > 9 ? sc*parms[9][0]:0.0),  (float)(np > 10 ? sc*parms[10][0]:0.0), (float)(np > 11 ? sc*parms[11][0]:0.0));
      SetImageDirty(dest);
    }
  }
}



void eel_lice_state::gfx_setpixel(EEL_F r, EEL_F g, EEL_F b)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_setpixel");
  if (!dest) return;

  int red=(int) (r*255.0);
  int green=(int) (g*255.0);
  int blue=(int) (b*255.0);
  if (red<0) red=0;else if (red>255)red=255;
  if (green<0) green=0;else if (green>255)green=255;
  if (blue<0) blue=0; else if (blue>255) blue=255;

  if (LICE_FUNCTION_VALID(LICE_PutPixel)) 
  {
    LICE_PutPixel(dest,(int)*m_gfx_x, (int)*m_gfx_y,LICE_RGBA(red,green,blue,255), (float)*m_gfx_a,getCurMode());
    SetImageDirty(dest);
  }
}

void eel_lice_state::gfx_getimgdim(EEL_F img, EEL_F *w, EEL_F *h)
{
  *w=*h=0;
  if (!this
#ifdef DYNAMIC_LICE
    ||!LICE__GetWidth || !LICE__GetHeight
#endif
    ) return;

  LICE_IBitmap *bm=GetImageForIndex(img,"gfx_getimgdim"); 
  if (bm)
  {
    *w=LICE__GetWidth(bm);
    *h=LICE__GetHeight(bm);
  }
}


EEL_F eel_lice_state::gfx_loadimg(void *opaque, int img, EEL_F loadFrom)
{
  if (!this
#ifdef DYNAMIC_LICE
    ||!__LICE_LoadImage || !LICE__Destroy
#endif
    ) return 0.0;

  if (img >= 0 && img < m_gfx_images.GetSize()) 
  {
    WDL_FastString fs;
    bool ok = EEL_LICE_GET_FILENAME_FOR_STRING(loadFrom,&fs,0);

    if (ok && fs.GetLength())
    {
      LICE_IBitmap *bm = LICE_LoadImage(fs.Get(),NULL,false);
      if (bm)
      {
        LICE__Destroy(m_gfx_images.Get()[img]);
        m_gfx_images.Get()[img]=bm;
        return img;
      }
    }
  }
  return -1.0;

}

EEL_F eel_lice_state::gfx_setimgdim(int img, EEL_F *w, EEL_F *h)
{
  int rv=0;
  if (!this
#ifdef DYNAMIC_LICE
    ||!LICE__resize ||!LICE__GetWidth || !LICE__GetHeight||!__LICE_CreateBitmap
#endif
    ) return 0.0;

  int use_w = (int)*w;
  int use_h = (int)*h;
  if (use_w<1 || use_h < 1) use_w=use_h=0;
  if (use_w > 2048) use_w=2048;
  if (use_h > 2048) use_h=2048;
  
  LICE_IBitmap *bm=NULL;
  if (img >= 0 && img < m_gfx_images.GetSize()) 
  {
    bm=m_gfx_images.Get()[img];  
    if (!bm) 
    {
      m_gfx_images.Get()[img] = bm = __LICE_CreateBitmap(1,use_w,use_h);
      rv=!!bm;
    }
    else 
    {
      rv=LICE__resize(bm,use_w,use_h);
    }
  }

  return rv?1.0:0.0;
}

void eel_lice_state::gfx_blurto(EEL_F x, EEL_F y)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_blurto");
  if (!dest
#ifdef DYNAMIC_LICE
    ||!LICE_Blur
#endif
    ) return;
  
  int srcx = (int)x;
  int srcy = (int)y;
  int srcw=(int) (*m_gfx_x-x);
  int srch=(int) (*m_gfx_y-y);
  if (srch < 0) { srch=-srch; srcy = (int)*m_gfx_y; }
  if (srcw < 0) { srcw=-srcw; srcx = (int)*m_gfx_x; }
  LICE_Blur(dest,dest,srcx,srcy,srcx,srcy,srcw,srch);
  *m_gfx_x = x;
  *m_gfx_y = y;
  SetImageDirty(dest);
}

static bool CoordsSrcDestOverlap(EEL_F *coords)
{
  if (coords[0]+coords[2] < coords[4]) return false;
  if (coords[0] > coords[4] + coords[6]) return false;
  if (coords[1]+coords[3] < coords[5]) return false;
  if (coords[1] > coords[5] + coords[7]) return false;
  return true;
}

void eel_lice_state::gfx_transformblit(EEL_F **parms, int div_w, int div_h, EEL_F *tab)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_transformblit");

  if (!dest
#ifdef DYNAMIC_LICE
    ||!LICE_ScaledBlit || !LICE_TransformBlit2 ||!LICE__GetWidth||!LICE__GetHeight
#endif 
    ) return;

  LICE_IBitmap *bm=GetImageForIndex(parms[0][0],"gfx_transformblit:src"); 
  if (!bm) return;

  const int bmw=LICE__GetWidth(bm);
  const int bmh=LICE__GetHeight(bm);
 
  const bool isFromFB = bm==m_framebuffer;

  if (bm == dest)
  {
    if (!m_framebuffer_extra && LICE_FUNCTION_VALID(__LICE_CreateBitmap)) m_framebuffer_extra=__LICE_CreateBitmap(0,bmw,bmh);
    if (m_framebuffer_extra)
    {
    
      LICE__resize(bm=m_framebuffer_extra,bmw,bmh);
      LICE_ScaledBlit(bm,dest, // copy the entire image
        0,0,bmw,bmh,
        0.0f,0.0f,(float)bmw,(float)bmh,
        1.0f,LICE_BLIT_MODE_COPY);      
    }
  }
  LICE_TransformBlit2(dest,bm,(int)floor(parms[1][0]),(int)floor(parms[2][0]),(int)floor(parms[3][0]),(int)floor(parms[4][0]),tab,div_w,div_h, (float)*m_gfx_a,getCurModeForBlit(isFromFB));

  SetImageDirty(dest);
}

EEL_F eel_lice_state::gfx_setfont(void *opaque, int np, EEL_F **parms)
{
  int a = np>0 ? ((int)floor(parms[0][0]))-1 : -1;

  if (a>=0 && a < m_gfx_fonts.GetSize())
  {
    gfxFontStruct *s = m_gfx_fonts.Get()+a;
    if (np>1 && LICE_FUNCTION_VALID(LICE_CreateFont) && LICE_FUNCTION_VALID(LICE__SetFromHFont))
    {
      const int sz=np>2 ? (int)parms[2][0] : 10;
      
      bool doCreate=false;
      int fontflag=0;

      {
        EEL_STRING_MUTEXLOCK_SCOPE
      
        const char *face=EEL_STRING_GET_FOR_INDEX(parms[1][0],NULL);
        #ifdef EEL_STRING_DEBUGOUT
          if (!face) EEL_STRING_DEBUGOUT("gfx_setfont: invalid string identifier %f",parms[1][0]);
        #endif
        if (!face || !*face) face="Arial";

        {
          unsigned int c = np > 3 ? (unsigned int) parms[3][0] : 0;
          while (c)
          {
            if (toupper(c&0xff)=='B') fontflag|=1;
            else if (toupper(c&0xff)=='I') fontflag|=2;
            else if (toupper(c&0xff)=='U') fontflag|=4;
            c>>=8;
          }
        }
      

        if (fontflag != s->last_fontflag || sz!=s->last_fontsize || strncmp(s->last_fontname,face,sizeof(s->last_fontname)-1))
        {
          lstrcpyn_safe(s->last_fontname,face,sizeof(s->last_fontname));
          s->last_fontsize=sz;
          s->last_fontflag=fontflag;
          doCreate=1;
        }
      }

      if (doCreate)
      {
        if (!s->font) s->font=LICE_CreateFont();
        if (s->font)
        {
          HFONT hf=CreateFont(sz,0,0,0,(fontflag&1) ? FW_BOLD : FW_NORMAL,!!(fontflag&2),!!(fontflag&4),FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,s->last_fontname);
          if (!hf)
          {
            s->use_fonth=0; // disable this font
          }
          else
          {
            TEXTMETRIC tm;
            tm.tmHeight = sz;

            if (!m_framebuffer && LICE_FUNCTION_VALID(__LICE_CreateBitmap)) m_framebuffer=__LICE_CreateBitmap(0,64,64);

            if (m_framebuffer && LICE_FUNCTION_VALID(LICE__GetDC))
            {
              HGDIOBJ oldFont = 0;
              HDC hdc=LICE__GetDC(m_framebuffer);
              if (hdc)
              {
                oldFont = SelectObject(hdc,hf);
                GetTextMetrics(hdc,&tm);
                SelectObject(hdc,oldFont);
              }
            }

            s->use_fonth=max(tm.tmHeight,1);
            LICE__SetFromHFont(s->font,hf,512);//LICE_FONT_FLAG_OWNS_HFONT);
          }
        }
      }
    }

    
    if (s->font && s->use_fonth)
    {
      m_gfx_font_active=a;
      if (m_gfx_texth) *m_gfx_texth=s->use_fonth;
      return 1.0;
    }
    // try to init this font
  }
  #ifdef EEL_STRING_DEBUGOUT
  if (a >= m_gfx_fonts.GetSize()) EEL_STRING_DEBUGOUT("gfx_setfont: invalid font %d specified",a);
  #endif

  if (a<0||a>=m_gfx_fonts.GetSize()||!m_gfx_fonts.Get()[a].font)
  {
    m_gfx_font_active=-1;
    if (m_gfx_texth) *m_gfx_texth=8;
    return 1.0;
  }
  return 0.0;
}

void eel_lice_state::gfx_blitext2(int np, EEL_F **parms, int blitmode)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_blitext2");

  if (!dest
#ifdef DYNAMIC_LICE
    ||!LICE_ScaledBlit || !LICE_RotatedBlit||!LICE__GetWidth||!LICE__GetHeight
#endif 
    ) return;

  LICE_IBitmap *bm=GetImageForIndex(parms[0][0],"gfx_blitext2:src"); 
  if (!bm) return;

  const int bmw=LICE__GetWidth(bm);
  const int bmh=LICE__GetHeight(bm);
  
  // 0=img, 1=scale, 2=rotate
  double coords[8];
  const double sc = blitmode==0 ? parms[1][0] : 1.0,
            angle = blitmode==0 ? parms[2][0] : 0.0;
  if (blitmode==0)
  {
    parms+=2;
    np -= 2;
  }

  coords[0]=np > 1 ? parms[1][0] : 0.0f;
  coords[1]=np > 2 ? parms[2][0] : 0.0f;
  coords[2]=np > 3 ? parms[3][0] : bmw;
  coords[3]=np > 4 ? parms[4][0] : bmh;
  coords[4]=np > 5 ? parms[5][0] : *m_gfx_x;
  coords[5]=np > 6 ? parms[6][0] : *m_gfx_y;
  coords[6]=np > 7 ? parms[7][0] : coords[2]*sc;
  coords[7]=np > 8 ? parms[8][0] : coords[3]*sc;
 
  const bool isFromFB = bm == m_framebuffer;
 
  if (bm == dest && CoordsSrcDestOverlap(coords))
  {
    if (!m_framebuffer_extra && LICE_FUNCTION_VALID(__LICE_CreateBitmap)) m_framebuffer_extra=__LICE_CreateBitmap(0,bmw,bmh);
    if (m_framebuffer_extra)
    {
    
      LICE__resize(bm=m_framebuffer_extra,bmw,bmh);
      LICE_ScaledBlit(bm,dest, // copy the source portion
        (int)coords[0],(int)coords[1],(int)coords[2],(int)coords[3],
        (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],
        1.0f,LICE_BLIT_MODE_COPY);      
    }
  }
  
  if (blitmode==1)
  {
    if (LICE_FUNCTION_VALID(LICE_DeltaBlit))
      LICE_DeltaBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
                (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],
                np > 9 ? (float)parms[9][0]:1.0f, // dsdx
                np > 10 ? (float)parms[10][0]:0.0f, // dtdx
                np > 11 ? (float)parms[11][0]:0.0f, // dsdy
                np > 12 ? (float)parms[12][0]:1.0f, // dtdy
                np > 13 ? (float)parms[13][0]:0.0f, // dsdxdy
                np > 14 ? (float)parms[14][0]:0.0f, // dtdxdy
                true, (float)*m_gfx_a,getCurModeForBlit(isFromFB));
  }
  else if (fabs(angle)>0.000000001)
  {
    LICE_RotatedBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
      (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],
      (float)angle,true, (float)*m_gfx_a,getCurModeForBlit(isFromFB),
       np > 9 ? (float)parms[9][0] : 0.0f,
       np > 10 ? (float)parms[10][0] : 0.0f);
  }
  else
  {
    LICE_ScaledBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
      (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3], (float)*m_gfx_a,getCurModeForBlit(isFromFB));
  }
  SetImageDirty(dest);
}

void eel_lice_state::gfx_blitext(EEL_F img, EEL_F *coords, EEL_F angle)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_blitext");

  if (!dest
#ifdef DYNAMIC_LICE
    ||!LICE_ScaledBlit || !LICE_RotatedBlit||!LICE__GetWidth||!LICE__GetHeight
#endif 
    ) return;

  LICE_IBitmap *bm=GetImageForIndex(img,"gfx_blitext:src");
  if (!bm) return;
  
  const bool isFromFB = bm == m_framebuffer;
 
  int bmw=LICE__GetWidth(bm);
  int bmh=LICE__GetHeight(bm);
  
  if (bm == dest && CoordsSrcDestOverlap(coords))
  {
    if (!m_framebuffer_extra && LICE_FUNCTION_VALID(__LICE_CreateBitmap)) m_framebuffer_extra=__LICE_CreateBitmap(0,bmw,bmh);
    if ( m_framebuffer_extra)
    {
    
      LICE__resize(bm=m_framebuffer_extra,bmw,bmh);
      LICE_ScaledBlit(bm,dest, // copy the source portion
        (int)coords[0],(int)coords[1],(int)coords[2],(int)coords[3],
        (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],
        1.0f,LICE_BLIT_MODE_COPY);      
    }
  }
  
  if (fabs(angle)>0.000000001)
  {
    LICE_RotatedBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
      (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3],(float)angle,
      true, (float)*m_gfx_a,getCurModeForBlit(isFromFB),
          (float)coords[8],(float)coords[9]);
  }
  else
  {
    LICE_ScaledBlit(dest,bm,(int)coords[4],(int)coords[5],(int)coords[6],(int)coords[7],
      (float)coords[0],(float)coords[1],(float)coords[2],(float)coords[3], (float)*m_gfx_a,getCurModeForBlit(isFromFB));
  }
  SetImageDirty(dest);
}

void eel_lice_state::gfx_blit(EEL_F img, EEL_F scale, EEL_F rotate)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_blit");
  if (!dest
#ifdef DYNAMIC_LICE
    ||!LICE_ScaledBlit || !LICE_RotatedBlit||!LICE__GetWidth||!LICE__GetHeight
#endif
    ) return;

  LICE_IBitmap *bm=GetImageForIndex(img,"gfx_blit:src");
  
  if (!bm) return;
  
  const bool isFromFB = bm == m_framebuffer;
  
  int bmw=LICE__GetWidth(bm);
  int bmh=LICE__GetHeight(bm);
  if (fabs(rotate)>0.000000001)
  {
    LICE_RotatedBlit(dest,bm,(int)*m_gfx_x,(int)*m_gfx_y,(int) (bmw*scale),(int) (bmh*scale),0.0f,0.0f,(float)bmw,(float)bmh,(float)rotate,true, (float)*m_gfx_a,getCurModeForBlit(isFromFB),
        0.0f,0.0f);
  }
  else
  {
    LICE_ScaledBlit(dest,bm,(int)*m_gfx_x,(int)*m_gfx_y,(int) (bmw*scale),(int) (bmh*scale),0.0f,0.0f,(float)bmw,(float)bmh, (float)*m_gfx_a,getCurModeForBlit(isFromFB));
  }
  SetImageDirty(dest);
}


void eel_lice_state::gfx_getpixel(EEL_F *r, EEL_F *g, EEL_F *b)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_getpixel");
  if (!dest) return;

  int ret=LICE_FUNCTION_VALID(LICE_GetPixel)?LICE_GetPixel(dest,(int)*m_gfx_x, (int)*m_gfx_y):0;

  *r=LICE_GETR(ret)/255.0;
  *g=LICE_GETG(ret)/255.0;
  *b=LICE_GETB(ret)/255.0;

}


static int __drawTextWithFont(LICE_IBitmap *dest, int xpos, int ypos, LICE_IFont *font, const char *buf, int buflen, int fg, int mode, float alpha, EEL_F *wantYoutput, EEL_F **measureOnly)
{
  if (font && LICE_FUNCTION_VALID(LICE__DrawText))
  {
    LICE__SetTextColor(font,fg);
    LICE__SetTextCombineMode(font,mode,alpha);

    int maxx=0;
    RECT r={0,0,xpos,0};
    while (buflen>0)
    {
      int thislen = 0;
      while (thislen < buflen && buf[thislen] != '\n') thislen++;
      memset(&r,0,sizeof(r));
      int lineh = LICE__DrawText(font,dest,buf,thislen?thislen:1,&r,DT_SINGLELINE|DT_NOPREFIX|DT_CALCRECT);
      if (!measureOnly)
      {
        r.left = xpos;
        r.top = ypos;
        r.right += xpos;
        r.bottom += ypos;
        lineh = LICE__DrawText(font,dest,buf,thislen?thislen:1,&r,DT_SINGLELINE|DT_NOPREFIX|DT_LEFT|DT_TOP);

        if (wantYoutput) *wantYoutput = ypos;
      }
      else
      {
        if (r.right > maxx) maxx=r.right;
      }
      ypos += lineh;

      buflen -= thislen+1;
      buf += thislen+1;      
    }
    if (measureOnly) 
    {
      measureOnly[0][0] = maxx;
      measureOnly[1][0] = ypos;
    }
    return r.right;
  }
  else
  { 
    int x;
    const int sxpos = xpos;
    int maxx=0,maxy=0;
    if (LICE_FUNCTION_VALID(LICE_DrawChar)) for(x=0;x<buflen;x++)
    {
      switch (buf[x])
      {
        case '\n': 
          ypos += 8; 
        case '\r': 
          xpos = sxpos; 
        break;
        case ' ': xpos += 8; break;
        case '\t': xpos += 8*5; break;
        default:
          if (!measureOnly) LICE_DrawChar(dest,xpos,ypos,buf[x], fg,alpha,mode);
          xpos += 8;
          if (xpos > maxx) maxx=xpos;
          maxy = ypos + 8;
        break;
      }
    }
    if (measureOnly)
    {
      measureOnly[0][0]=maxx;
      measureOnly[1][0]=maxy;
    }
    else
    {
      if (wantYoutput) *wantYoutput=ypos;
    }
    return xpos;
  }
}

void eel_lice_state::gfx_drawstr(void *opaque, EEL_F ch, int formatmode, int nfmtparms, EEL_F **fmtparms)// formatmode=1 for format, 2 for purely measure no format
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,formatmode==1?"gfx_printf":formatmode==2?"gfx_measurestr":"gfx_drawstr");
  if (!dest) return;

#ifdef DYNAMIC_LICE
  if (!LICE__GetWidth || !LICE__GetHeight) return;
#endif

  EEL_STRING_MUTEXLOCK_SCOPE

  WDL_FastString *fs=NULL;
  const char *s=EEL_STRING_GET_FOR_INDEX(ch,&fs);
  #ifdef EEL_STRING_DEBUGOUT
    if (!s) EEL_STRING_DEBUGOUT("gfx_%s: invalid string identifier %f",formatmode==1?"printf":formatmode==2?"measurestr":"drawstr",ch);
  #endif
  char buf[4096];
  int s_len=0;
  if (!s) 
  {
    s="<bad string>";
    s_len = 12;
  }
  else if (formatmode==1)
  {
    extern int eel_format_strings(void *, const char *s, const char *ep, char *, int, int, EEL_F **);
    s_len = eel_format_strings(opaque,s,fs?(s+fs->GetLength()):NULL,buf,sizeof(buf),nfmtparms,fmtparms);
    if (s_len<1) return;
    s=buf;
  }
  else
  {
    s_len = fs?fs->GetLength():(int)strlen(s);
  }

  if (s_len)
  {
    if (formatmode==2)
    {
      if (nfmtparms==2)
        __drawTextWithFont(dest,0,0,GetActiveFont(),s,s_len,getCurColor(),getCurMode(),(float) *m_gfx_a,NULL, fmtparms);
    }
    else
    {
      *m_gfx_x = __drawTextWithFont(dest,(int)floor(*m_gfx_x),(int)floor(*m_gfx_y),GetActiveFont(),s,s_len,getCurColor(),getCurMode(),(float) *m_gfx_a,m_gfx_y,NULL);
    }

    SetImageDirty(dest);
  }
}

void eel_lice_state::gfx_drawchar(EEL_F ch)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_drawchar");
  if (!dest) return;

  int a=(int)(ch+0.5);
  if (a == '\r' || a=='\n') a=' ';

  char buf[2]={a,0};
  *m_gfx_x = __drawTextWithFont(dest,(int)floor(*m_gfx_x),(int)floor(*m_gfx_y),
                         GetActiveFont(),buf,1,
                         getCurColor(),getCurMode(),(float)*m_gfx_a, NULL,NULL);

  SetImageDirty(dest);
}


void eel_lice_state::gfx_drawnumber(EEL_F n, EEL_F ndigits)
{
  LICE_IBitmap *dest = GetImageForIndex(*m_gfx_dest,"gfx_drawnumber");
  if (!dest) return;

  char buf[512];
  int a=(int)(ndigits+0.5);
  if (a <0)a=0;
  else if (a > 16) a=16;
  snprintf(buf,sizeof(buf),"%.*f",a,n);

  *m_gfx_x = __drawTextWithFont(dest,(int)floor(*m_gfx_x),(int)floor(*m_gfx_y),
                           GetActiveFont(),buf,(int)strlen(buf),
                           getCurColor(),getCurMode(),(float)*m_gfx_a, NULL,NULL);

  SetImageDirty(dest);
}

int eel_lice_state::setup_frame(HWND hwnd, RECT r)
{
  int dr=0;
  if (!m_framebuffer && LICE_FUNCTION_VALID(__LICE_CreateBitmap)) 
  {
    m_framebuffer=__LICE_CreateBitmap(1,r.right-r.left,r.bottom-r.top);
    dr=1;
  }

  if (!m_framebuffer || !LICE_FUNCTION_VALID(LICE__GetHeight) || !LICE_FUNCTION_VALID(LICE__GetWidth)) return -1;

  if (r.right-r.left != LICE__GetWidth(m_framebuffer) ||
      r.bottom-r.top != LICE__GetHeight(m_framebuffer))
  {
    LICE__resize(m_framebuffer,r.right-r.left,r.bottom-r.top);
    dr=1;
  }
  *m_gfx_w = r.right-r.left;
  *m_gfx_h = r.bottom-r.top;
  
  if (*m_gfx_clear > -1.0)
  {
    int a=(int)*m_gfx_clear;
    int r=a&0xff;
    int g=(a>>8)&0xff;
    int b=(a>>16)&0xff;
    if (LICE_FUNCTION_VALID(LICE_Clear)) LICE_Clear(m_framebuffer,LICE_RGBA(r,g,b,0));
  }

  POINT pt;
  GetCursorPos(&pt);
  ScreenToClient(hwnd,&pt);
  *m_mouse_x=pt.x-r.left;
  *m_mouse_y=pt.y-r.top;
  int vflags=0;
  const bool hasCap=GetCapture()==hwnd;
  if (hasCap)
  {
    if (GetAsyncKeyState(VK_LBUTTON)&0x8000) vflags|=1;
    if (GetAsyncKeyState(VK_RBUTTON)&0x8000) vflags|=2;
    if (GetAsyncKeyState(VK_MBUTTON)&0x8000) vflags|=64;
  }
  if (hasCap||(m_has_had_getch && GetFocus()==hwnd))
  {
    if (GetAsyncKeyState(VK_CONTROL)&0x8000) vflags|=4;
    if (GetAsyncKeyState(VK_SHIFT)&0x8000) vflags|=8;
    if (GetAsyncKeyState(VK_MENU)&0x8000) vflags|=16;
    if (GetAsyncKeyState(VK_LWIN)&0x8000) vflags|=32;
  }
  *m_mouse_cap=(EEL_F)vflags;

  *m_gfx_dest = -1.0; // m_framebuffer
  *m_gfx_a= 1.0; // default to full alpha every call
  int fh;
  if (m_gfx_font_active>=0&&m_gfx_font_active<m_gfx_fonts.GetSize() && (fh=m_gfx_fonts.Get()[m_gfx_font_active].use_fonth)>0)
    *m_gfx_texth=fh;
  else 
    *m_gfx_texth = 8;
  
  return dr;

}


void eel_lice_register()
{
  NSEEL_addfunc_retptr("gfx_lineto",3,NSEEL_PProc_THIS,&_gfx_lineto);
  NSEEL_addfunc_retptr("gfx_lineto",2,NSEEL_PProc_THIS,&_gfx_lineto2);
  NSEEL_addfunc_retptr("gfx_rectto",2,NSEEL_PProc_THIS,&_gfx_rectto);
  NSEEL_addfunc_exparms("gfx_rect",4,NSEEL_PProc_THIS,&_gfx_rect);
  NSEEL_addfunc_varparm("gfx_line",4,NSEEL_PProc_THIS,&_gfx_line); // 5th param is optionally AA
  NSEEL_addfunc_varparm("gfx_gradrect",8,NSEEL_PProc_THIS,&_gfx_gradrect);
  NSEEL_addfunc_varparm("gfx_muladdrect",7,NSEEL_PProc_THIS,&_gfx_muladdrect);
  NSEEL_addfunc_varparm("gfx_deltablit",9,NSEEL_PProc_THIS,&_gfx_deltablit);
  NSEEL_addfunc_exparms("gfx_transformblit",8,NSEEL_PProc_THIS,&_gfx_transformblit);
  NSEEL_addfunc_varparm("gfx_circle",3,NSEEL_PProc_THIS,&_gfx_circle);
  NSEEL_addfunc_varparm("gfx_roundrect",5,NSEEL_PProc_THIS,&_gfx_roundrect);
  NSEEL_addfunc_varparm("gfx_arc",5,NSEEL_PProc_THIS,&_gfx_arc);
  NSEEL_addfunc_retptr("gfx_blurto",2,NSEEL_PProc_THIS,&_gfx_blurto);
  NSEEL_addfunc_retptr("gfx_drawnumber",2,NSEEL_PProc_THIS,&_gfx_drawnumber);
  NSEEL_addfunc_retptr("gfx_drawchar",1,NSEEL_PProc_THIS,&_gfx_drawchar);
  NSEEL_addfunc_retptr("gfx_drawstr",1,NSEEL_PProc_THIS,&_gfx_drawstr);
  NSEEL_addfunc_retptr("gfx_measurestr",3,NSEEL_PProc_THIS,&_gfx_measurestr);
  NSEEL_addfunc_varparm("gfx_printf",1,NSEEL_PProc_THIS,&_gfx_printf);
  NSEEL_addfunc_retptr("gfx_setpixel",3,NSEEL_PProc_THIS,&_gfx_setpixel);
  NSEEL_addfunc_retptr("gfx_getpixel",3,NSEEL_PProc_THIS,&_gfx_getpixel);
  NSEEL_addfunc_retptr("gfx_getimgdim",3,NSEEL_PProc_THIS,&_gfx_getimgdim);
  NSEEL_addfunc_retval("gfx_setimgdim",3,NSEEL_PProc_THIS,&_gfx_setimgdim);
  NSEEL_addfunc_retval("gfx_loadimg",2,NSEEL_PProc_THIS,&_gfx_loadimg);
  NSEEL_addfunc_retptr("gfx_blit",3,NSEEL_PProc_THIS,&_gfx_blit);
  NSEEL_addfunc_retptr("gfx_blitext",3,NSEEL_PProc_THIS,&_gfx_blitext);
  NSEEL_addfunc_varparm("gfx_blit",4,NSEEL_PProc_THIS,&_gfx_blit2);
  NSEEL_addfunc_varparm("gfx_setfont",1,NSEEL_PProc_THIS,&_gfx_setfont);
  NSEEL_addfunc_varparm("gfx_getfont",1,NSEEL_PProc_THIS,&_gfx_getfont);
}

#ifdef EEL_LICE_WANT_STANDALONE

#ifdef _WIN32
static HINSTANCE eel_lice_hinstance;
static const char *eel_lice_standalone_classname;
#endif

HWND eel_lice_standalone_owner;

#ifdef EEL_LICE_WANT_STANDALONE_UPDATE
static EEL_F * NSEEL_CGEN_CALL _gfx_update(void *opaque, EEL_F *n)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx)
  {
    if (ctx->hwnd_standalone) 
    {
      if (ctx->m_framebuffer_refstate) 
      {
#ifdef __APPLE__
        void *p = SWELL_InitAutoRelease();
#endif

        InvalidateRect(ctx->hwnd_standalone,NULL,FALSE);
        UpdateWindow(ctx->hwnd_standalone);

#ifdef __APPLE__
        SWELL_QuitAutoRelease(p);
#endif
      }
      // run message pump
#ifndef EEL_LICE_WANT_STANDALONE_UPDATE_NO_MSGPUMP

#ifdef _WIN32
      MSG msg;
      while (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) 
      {
	      TranslateMessage(&msg);
	      DispatchMessage(&msg);
      }
#else
      void SWELL_RunEvents();
      SWELL_RunEvents();
#endif
#endif
      RECT r;
      GetClientRect(ctx->hwnd_standalone,&r);
      ctx->setup_frame(ctx->hwnd_standalone,r);
    }
  }
  return n;
}
#endif



static EEL_F NSEEL_CGEN_CALL _gfx_getchar(void *opaque, EEL_F *p)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx)
  {
    ctx->m_has_had_getch=true;
    if (*p >= 2.0)
    {
      int x;
      const int n = sizeof(ctx->hwnd_standalone_kb_state) / sizeof(ctx->hwnd_standalone_kb_state[0]);
      int *st = ctx->hwnd_standalone_kb_state;
      int a = (int)*p;
      for (x=0;x<n && st[x] != a;x++);
      return x<n ? 1.0 : 0.0;
    }

    if (!ctx->hwnd_standalone) return -1.0;

    if (ctx->m_kb_queue_valid)
    {
      const int qsize = sizeof(ctx->m_kb_queue)/sizeof(ctx->m_kb_queue[0]);
      const int a = ctx->m_kb_queue[ctx->m_kb_queue_pos & (qsize-1)];
      ctx->m_kb_queue_pos++;
      ctx->m_kb_queue_valid--;
      return a;
    }
  }
  return 0.0;
}

static int eel_lice_key_xlate(int msg, int wParam, int lParam, bool *isAltOut)
{
#define EEL_MB_C(a) (sizeof(a)<=2 ? a[0] : \
                     sizeof(a)==3 ?  (((a[0])<<8)+(a[1])) : \
                     sizeof(a)==4 ? (((a[0])<<16)+((a[1])<<8)+(a[2])) : \
                     (((a[0])<<24)+((a[1])<<16)+((a[2])<<8)+(a[3])))

  if (msg != WM_CHAR)
  {
#ifndef _WIN32
    if (lParam & FVIRTKEY)
#endif
    switch (wParam)
	  {
      case VK_HOME: return EEL_MB_C("home");
      case VK_UP: return EEL_MB_C("up");
      case VK_PRIOR: return EEL_MB_C("pgup");
      case VK_LEFT: return EEL_MB_C("left");
      case VK_RIGHT: return EEL_MB_C("rght");
      case VK_END: return EEL_MB_C("end");
      case VK_DOWN: return EEL_MB_C("down");
      case VK_NEXT: return EEL_MB_C("pgdn");
      case VK_INSERT: return EEL_MB_C("ins");
      case VK_DELETE: return EEL_MB_C("del");
      case VK_F1: return EEL_MB_C("f1");
      case VK_F2: return EEL_MB_C("f2");
      case VK_F3: return EEL_MB_C("f3");
      case VK_F4: return EEL_MB_C("f4");
      case VK_F5: return EEL_MB_C("f5");
      case VK_F6: return EEL_MB_C("f6");
      case VK_F7: return EEL_MB_C("f7");
      case VK_F8: return EEL_MB_C("f8");
      case VK_F9: return EEL_MB_C("f9");
      case VK_F10: return EEL_MB_C("f10");
      case VK_F11: return EEL_MB_C("f11");
      case VK_F12: return EEL_MB_C("f12");
#ifndef _WIN32
      case VK_SUBTRACT: return '-'; // numpad -
      case VK_ADD: return '+';
      case VK_MULTIPLY: return '*';
      case VK_DIVIDE: return '/';
      case VK_DECIMAL: return '.';
      case VK_NUMPAD0: return '0';
      case VK_NUMPAD1: return '1';
      case VK_NUMPAD2: return '2';
      case VK_NUMPAD3: return '3';
      case VK_NUMPAD4: return '4';
      case VK_NUMPAD5: return '5';
      case VK_NUMPAD6: return '6';
      case VK_NUMPAD7: return '7';
      case VK_NUMPAD8: return '8';
      case VK_NUMPAD9: return '9';
      case (32768|VK_RETURN): return VK_RETURN;
#endif
    }
    
    switch (wParam)
    {
      case VK_RETURN: 
      case VK_BACK: 
      case VK_TAB: 
      case VK_ESCAPE: 
        return wParam;
      
      case VK_CONTROL: break;
    
      default:
        {
          const bool isctrl = !!(GetAsyncKeyState(VK_CONTROL)&0x8000);
          const bool isalt = !!(GetAsyncKeyState(VK_MENU)&0x8000);
          if(isctrl || isalt)
          {
            if (wParam>='a' && wParam<='z') 
            {
              if (isctrl) wParam += 1-'a';
              if (isalt) wParam += 256;
              *isAltOut=isalt;
              return wParam;
            }
            if (wParam>='A' && wParam<='Z') 
            {
              if (isctrl) wParam += 1-'A';
              if (isalt) wParam += 256;
              *isAltOut=isalt;
              return wParam;
            }
          }

          if (isctrl)
          {
            if ((wParam&~0x80) == '[') return 27;
            if ((wParam&~0x80) == ']') return 29;
          }
        }
      break;
    }
  }
    
  if(wParam>=32) 
  {
    #ifdef _WIN32
      if (msg == WM_CHAR) return wParam;
    #else
      if (!(GetAsyncKeyState(VK_SHIFT)&0x8000))
      {
        if (wParam>='A' && wParam<='Z') 
        {
          if ((GetAsyncKeyState(VK_LWIN)&0x8000)) wParam -= 'A'-1;
          else
            wParam += 'a'-'A';
        }
      }
      return wParam;
    #endif
  }      
  return 0;
}
#undef EEL_MB_C

static LRESULT WINAPI eel_lice_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#ifdef __APPLE__
extern "C" 
{
  void *objc_getClass(const char *p);
  void *sel_getUid(const char *p);
  void *objc_msgSend(void *, void *, ...);
};
#endif


HWND eel_lice_state::create_wnd(HWND par, int isChild)
{
  if (hwnd_standalone) return hwnd_standalone;
#ifdef _WIN32
  return CreateWindowEx(0,eel_lice_standalone_classname,"",
                        isChild ? (WS_CHILD|WS_TABSTOP) : (WS_POPUP|WS_CAPTION|WS_THICKFRAME|WS_SYSMENU),CW_USEDEFAULT,CW_USEDEFAULT,100,100,par,NULL,eel_lice_hinstance,this);
#else
  return SWELL_CreateDialog(NULL,isChild ? NULL : ((const char *)(INT_PTR)0x400001),par,(DLGPROC)eel_lice_wndproc,(LPARAM)this);
#endif
}


#ifndef EEL_LICE_STANDALONE_NOINITQUIT

static EEL_F * NSEEL_CGEN_CALL _gfx_quit(void *opaque, EEL_F *n)
{
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque);
  if (ctx)
  {
    if (ctx->hwnd_standalone) DestroyWindow(ctx->hwnd_standalone);
    ctx->hwnd_standalone=0;
  }
  return n; 
}

static EEL_F NSEEL_CGEN_CALL _gfx_init(void *opaque, INT_PTR np, EEL_F **parms)
{
#ifdef EEL_LICE_GET_CONTEXT_INIT
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT_INIT(opaque); 
#else
  eel_lice_state *ctx=EEL_LICE_GET_CONTEXT(opaque); 
#endif
  if (ctx)
  {
    bool wantShow=false;
    if (!ctx->hwnd_standalone)
    {
      #ifdef __APPLE__
        void *nsapp=objc_msgSend( objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
        objc_msgSend(nsapp,sel_getUid("setActivationPolicy:"), 0);
        objc_msgSend(nsapp,sel_getUid("activateIgnoringOtherApps:"), 1);

      #endif

      #ifdef EEL_LICE_STANDALONE_PARENT
        HWND par = EEL_LICE_STANDALONE_PARENT(opaque);
      #elif defined(_WIN32)
        HWND par=GetDesktopWindow();
      #else
        HWND par=NULL;
      #endif

      ctx->create_wnd(par,0);
      // resize client

      if (ctx->hwnd_standalone)
      {
        int sug_w = np > 1 ? (int)parms[1][0] : 640;
        int sug_h = np > 2 ? (int)parms[2][0] : 480;
        
        if (sug_w < 16) sug_w=16;
        else if (sug_w > 2048) sug_w=2048;
        if (sug_h < 16) sug_h=16;
        else if (sug_h > 1600) sug_w=1600;

        RECT r1,r2;
        GetWindowRect(ctx->hwnd_standalone,&r1);
        GetClientRect(ctx->hwnd_standalone,&r2);
        sug_w += (r1.right-r1.left) - r2.right;
        sug_h += abs(r1.bottom-r1.top) - r2.bottom;

        SetWindowPos(ctx->hwnd_standalone,NULL,0,0,sug_w,sug_h,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

        wantShow=true;
        #ifdef EEL_LICE_WANT_STANDALONE_UPDATE
          {
            RECT r;
            GetClientRect(ctx->hwnd_standalone,&r);
            ctx->setup_frame(ctx->hwnd_standalone,r);
          }
        #endif
      }
    }
    if (np>0)
    {
      EEL_STRING_MUTEXLOCK_SCOPE
      const char *title=EEL_STRING_GET_FOR_INDEX(parms[0][0],NULL);
      #ifdef EEL_STRING_DEBUGOUT
        if (!title) EEL_STRING_DEBUGOUT("gfx_init: invalid string identifier %f",parms[0][0]);
      #endif
      if (title&&*title) SetWindowText(ctx->hwnd_standalone,title);
    }

    if (wantShow)
      ShowWindow(ctx->hwnd_standalone,SW_SHOW);
    return !!ctx->hwnd_standalone;
  }
  return 0;  
}
#endif // !EEL_LICE_STANDALONE_NOINITQUIT


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x20A
#endif
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x20E
#endif

LRESULT WINAPI eel_lice_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef WIN32
  static UINT Scroll_Message;
  static bool sm_init;
  if (!sm_init)
  {
    sm_init=true;
    Scroll_Message = RegisterWindowMessage("MSWHEEL_ROLLMSG");
  }
  if (Scroll_Message && uMsg == Scroll_Message)
  {
    uMsg=WM_MOUSEWHEEL;
    wParam<<=16; 
  }
#endif

  switch (uMsg)
  {
    case WM_CREATE: 
      {
#ifdef _WIN32
        LPCREATESTRUCT lpcs= (LPCREATESTRUCT )lParam;
        eel_lice_state *ctx=(eel_lice_state*)lpcs->lpCreateParams;
        SetWindowLongPtr(hwnd,GWLP_USERDATA,(LPARAM)lpcs->lpCreateParams);
#else
        eel_lice_state *ctx=(eel_lice_state*)lParam;
        SetWindowLongPtr(hwnd,GWLP_USERDATA,lParam);
#endif
        ctx->m_kb_queue_valid=0;
        ctx->hwnd_standalone=hwnd;
      }
    return 0;
#ifndef _WIN32
    case WM_CLOSE:
      DestroyWindow(hwnd);
    return 0;
#endif
    case WM_DESTROY:
      {
        eel_lice_state *ctx=(eel_lice_state*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (ctx) ctx->hwnd_standalone=NULL;
      }
    return 0;
    case WM_ACTIVATE:
      {
        eel_lice_state *ctx=(eel_lice_state*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (ctx) memset(&ctx->hwnd_standalone_kb_state,0,sizeof(ctx->hwnd_standalone_kb_state));
      }
    break;
		case WM_MOUSEHWHEEL:   
    case WM_MOUSEWHEEL:
      {
        eel_lice_state *ctx=(eel_lice_state*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (ctx)
        {
          EEL_F *p= uMsg==WM_MOUSEHWHEEL ? ctx->m_mouse_hwheel : ctx->m_mouse_wheel;
          if (p) *p += (EEL_F) (short)HIWORD(wParam);
        }
      }
      return -1;
#ifdef _WIN32
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
#endif
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
      {
        eel_lice_state *ctx=(eel_lice_state*)GetWindowLongPtr(hwnd,GWLP_USERDATA);

        bool hadAltAdj=false;
        int a=eel_lice_key_xlate(uMsg,(int)wParam,(int)lParam, &hadAltAdj);
#ifdef _WIN32
        if (!a && (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) && wParam >= 'A' && wParam <= 'Z') a=(int)wParam + 'a' - 'A';
#endif
        const int mask = hadAltAdj ? ~256 : ~0;

        if (a & mask)
        {
          int a_no_alt = (a&mask);
          const int lowera = a_no_alt >= 1 && a_no_alt < 27 ? (a_no_alt+'a'-1) : tolower(a_no_alt);

          int *st = ctx->hwnd_standalone_kb_state;

          const int n = sizeof(ctx->hwnd_standalone_kb_state) / sizeof(ctx->hwnd_standalone_kb_state[0]);
          int zp=n-1,x;

          for (x=0;x<n && st[x] != lowera;x++) if (x < zp && !st[x]) zp=x;

          if (uMsg==WM_KEYUP
#ifdef _WIN32
             ||uMsg == WM_SYSKEYUP
#endif
            )
          {
            if (x<n) st[x]=0;
          }
          else if (x==n) // key not already down
          {
            st[zp]=lowera;
          }
        }

        if (a && uMsg != WM_KEYUP)
        {
          // add to queue
          const int qsize = sizeof(ctx->m_kb_queue)/sizeof(ctx->m_kb_queue[0]);
          if (ctx->m_kb_queue_valid>=qsize) // queue full, dump an old event!
          {
            ctx->m_kb_queue_valid--;
            ctx->m_kb_queue_pos++;
          }
          ctx->m_kb_queue[(ctx->m_kb_queue_pos + ctx->m_kb_queue_valid++) & (qsize-1)] = a;
        }

      }
    return 0;
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONDOWN:
      SetFocus(hwnd);
      SetCapture(hwnd);
    return 1;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
      ReleaseCapture();
    return 1;
#ifdef _WIN32
    case WM_GETDLGCODE:
      if (GetWindowLong(hwnd,GWL_STYLE)&WS_CHILD) return DLGC_WANTALLKEYS;
    break;
#endif
    case WM_SIZE:
      {
        eel_lice_state *ctx=(eel_lice_state*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (ctx) ctx->m_framebuffer_refstate=0;
      }
    break;

    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          eel_lice_state *ctx=(eel_lice_state*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
          if (ctx && ctx->m_framebuffer && 
              LICE_FUNCTION_VALID(LICE__GetDC) && LICE_FUNCTION_VALID(LICE__GetWidth) && LICE_FUNCTION_VALID(LICE__GetHeight))
          {
            int w = LICE__GetWidth(ctx->m_framebuffer);
            int h = LICE__GetHeight(ctx->m_framebuffer);
            BitBlt(ps.hdc,0,0,w,h,LICE__GetDC(ctx->m_framebuffer),0,0,SRCCOPY);
          }
          if (ctx) ctx->m_framebuffer_refstate=0;
          EndPaint(hwnd,&ps);
        }
      }
    return 0;
  }

  return DefWindowProc(hwnd,uMsg,wParam,lParam);
}



void eel_lice_register_standalone(HINSTANCE hInstance, const char *classname, HWND hwndPar, HICON icon)
{
#ifdef _WIN32
  eel_lice_hinstance=hInstance;
  eel_lice_standalone_classname=classname && *classname ? classname : "EEL_LICE_gfx_standalone";
  WNDCLASS wc={CS_DBLCLKS,eel_lice_wndproc,0,0,hInstance,icon,LoadCursor(NULL,IDC_ARROW), NULL, NULL,eel_lice_standalone_classname};
  RegisterClass(&wc);
#endif

  // gfx_init(title[, w,h, flags])
#ifndef EEL_LICE_STANDALONE_NOINITQUIT
  NSEEL_addfunc_varparm("gfx_init",1,NSEEL_PProc_THIS,&_gfx_init); 
  NSEEL_addfunc_retptr("gfx_quit",1,NSEEL_PProc_THIS,&_gfx_quit);
#endif

#ifdef EEL_LICE_WANT_STANDALONE_UPDATE
  NSEEL_addfunc_retptr("gfx_update",1,NSEEL_PProc_THIS,&_gfx_update);
#endif

  NSEEL_addfunc_retval("gfx_getchar",1,NSEEL_PProc_THIS,&_gfx_getchar);
}


#endif

#endif//!EEL_LICE_API_ONLY




#ifdef DYNAMIC_LICE
static void eel_lice_initfuncs(void *(*getFunc)(const char *name))
{
  if (!getFunc) return;

  *(void **)&__LICE_CreateBitmap = getFunc("LICE_CreateBitmap");
  *(void **)&LICE_Clear = getFunc("LICE_Clear");
  *(void **)&LICE_Line = getFunc("LICE_LineInt");
  *(void **)&LICE_ClipLine = getFunc("LICE_ClipLine");
  *(void **)&LICE_FillRect = getFunc("LICE_FillRect");
  *(void **)&LICE_PutPixel = getFunc("LICE_PutPixel");
  *(void **)&LICE_GetPixel = getFunc("LICE_GetPixel");
  *(void **)&LICE_DrawText = getFunc("LICE_DrawText");
  *(void **)&LICE_DrawChar = getFunc("LICE_DrawChar");
  *(void **)&LICE_MeasureText = getFunc("LICE_MeasureText");
  *(void **)&LICE_LoadImage = getFunc("LICE_LoadImage");
  *(void **)&LICE__GetDC = getFunc("LICE__GetDC");
  *(void **)&LICE__Destroy = getFunc("LICE__Destroy");
  *(void **)&LICE__GetWidth = getFunc("LICE__GetWidth");
  *(void **)&LICE__GetHeight = getFunc("LICE__GetHeight");
  *(void **)&LICE__resize = getFunc("LICE__resize");
  *(void **)&LICE_Blur = getFunc("LICE_Blur");
  *(void **)&LICE_RotatedBlit = getFunc("LICE_RotatedBlit");
  *(void **)&LICE_ScaledBlit = getFunc("LICE_ScaledBlit");
  *(void **)&LICE_Circle = getFunc("LICE_Circle");
  *(void **)&LICE_FillCircle = getFunc("LICE_FillCircle");
  *(void **)&LICE_RoundRect = getFunc("LICE_RoundRect");
  *(void **)&LICE_Arc = getFunc("LICE_Arc");

  *(void **)&LICE_MultiplyAddRect  = getFunc("LICE_MultiplyAddRect");
  *(void **)&LICE_GradRect  = getFunc("LICE_GradRect");
  *(void **)&LICE_TransformBlit2  = getFunc("LICE_TransformBlit2");
  *(void **)&LICE_DeltaBlit  = getFunc("LICE_DeltaBlit");

  *(void **)&LICE__DestroyFont = getFunc("LICE__DestroyFont");    
  *(void **)&LICE_CreateFont = getFunc("LICE_CreateFont");    
  *(void **)&LICE__SetFromHFont = getFunc("LICE__SetFromHFont2");

  *(void **)&LICE__SetTextColor = getFunc("LICE__SetTextColor");    
  *(void **)&LICE__SetBkColor = getFunc("LICE__SetBkColor");    
  *(void **)&LICE__SetTextCombineMode = getFunc("LICE__SetTextCombineMode");    
  *(void **)&LICE__DrawText = getFunc("LICE__DrawText");    
}
#endif

#ifdef EEL_WANT_DOCUMENTATION

#ifdef EELSCRIPT_LICE_MAX_IMAGES
#define MKSTR2(x) #x
#define MKSTR(x) MKSTR2(x)
#define EEL_LICE_DOC_MAXHANDLE MKSTR(EELSCRIPT_LICE_MAX_IMAGES-1)
#else
#define EEL_LICE_DOC_MAXHANDLE "127"
#endif


static const char *eel_lice_function_reference =
#ifdef EEL_LICE_WANT_STANDALONE
#ifndef EEL_LICE_STANDALONE_NOINITQUIT
  "gfx_init\t\"name\"[,width,height]\tInitializes the graphics window with title name. Suggested width and height can be specified.\n\n"
  "Once the graphics window is open, gfx_update() should be called periodically. \0"
  "gfx_quit\t\tCloses the graphics window.\0"
#endif
#ifdef EEL_LICE_WANT_STANDALONE_UPDATE
  "gfx_update\t\tUpdates the graphics display, if opened\0"
#endif
#endif
  "gfx_aaaaa\t\t"
  "The following global variables are special and will be used by the graphics system:\n\n\3"
  "\4gfx_r, gfx_g, gfx_b, gfx_a - These represent the current red, green, blue, and alpha components used by drawing operations (0.0..1.0). \n"
  "\4gfx_w, gfx_h - These are set to the current width and height of the UI framebuffer. \n"
  "\4gfx_x, gfx_y - These set the \"current\" graphics position in x,y. You can set these yourselves, and many of the drawing functions update them as well. \n"
  "\4gfx_mode - Set to 0 for default options. Add 1.0 for additive blend mode (if you wish to do subtractive, set gfx_a to negative and use gfx_mode as additive). Add 2.0 to disable source alpha for gfx_blit(). Add 4.0 to disable filtering for gfx_blit(). \n"
  "\4gfx_clear - If set to a value greater than -1.0, this will result in the framebuffer being cleared to that color. the color for this one is packed RGB (0..255), i.e. red+green*256+blue*65536. The default is 0 (black). \n"
  "\4gfx_dest - Defaults to -1, set to 0.." EEL_LICE_DOC_MAXHANDLE " to have drawing operations go to an offscreen buffer (or loaded image).\n"
  "\4gfx_texth - Set to the height of a line of text in the current font. Do not modify this variable.\n"
  "\4mouse_x, mouse_y - mouse_x and mouse_y are set to the coordinates of the mouse relative to the graphics window.\n"
  "\4mouse_wheel, mouse_hwheel - mouse wheel (and horizontal wheel) positions. These will change typically by 120 or a multiple thereof, the caller should clear the state to 0 after reading it."
  "\4mouse_cap is a bitfield of mouse and keyboard modifier state.\3"
    "\4" "1: left mouse button\n"
    "\4" "2: right mouse button\n"
#ifdef __APPLE__
    "\4" "4: Command key\n"
    "\4" "8: Shift key\n"
    "\4" "16: Option key\n"
    "\4" "32: Control key\n"
#else
    "\4" "4: Control key\n"
    "\4" "8: Shift key\n"
    "\4" "16: Alt key\n"
    "\4" "32: Windows key\n"
#endif
    "\4" "64: middle mouse button\n"
  "\2"
  "\2\0"

"gfx_getchar\t[char]\tIf char is 0 or omitted, returns a character from the keyboard queue, or 0 if no character is available, or -1 if the graphics window is not open. "
     "If char is specified and nonzero, that character's status will be checked, and the function will return greater than 0 if it is pressed.\n\n"
     "Common values are standard ASCII, such as 'a', 'A', '=' and '1', but for many keys multi-byte values are used, including 'home', 'up', 'down', 'left', 'rght', 'f1'.. 'f12', 'pgup', 'pgdn', 'ins', and 'del'. \n\n"
     "Modified and special keys can also be returned, including:\3\n"
     "\4Ctrl/Cmd+A..Ctrl+Z as 1..26\n"
     "\4Ctrl/Cmd+Alt+A..Z as 257..282\n"
     "\4Alt+A..Z as 'A'+256..'Z'+256\n"
     "\4" "27 for ESC\n"
     "\4" "13 for Enter\n"
     "\4' ' for space\n"
     "\2\0"

  "gfx_lineto\tx,y[,aa]\tDraws a line from gfx_x,gfx_y to x,y. if aa is 0.5 or greater, then antialiasing is used. Updates gfx_x and gfx_y to x,y.\0"
  "gfx_line\tx,y,x2,y2[,aa]\tDraws a line from x,y to x2,y2, and if aa is not specified or 0.5 or greater, it will be antialiased. \0"
  "gfx_rectto\tx,y\tFills a rectangle from gfx_x,gfx_y to x,y. Updates gfx_x,gfx_y to x,y. \0"
  "gfx_rect\tx,y,w,h\tFills a rectngle at x,y, w,h pixels in dimension. \0"
  "gfx_setpixel\tr,g,b\tWrites a pixel of r,g,b to gfx_x,gfx_y.\0"
  "gfx_getpixel\tr,g,b\tGets the value of the pixel at gfx_x,gfx_y into r,g,b. \0"
  "gfx_drawnumber\tn,ndigits\tDraws the number n with ndigits of precision to gfx_x, gfx_y, and updates gfx_x to the right side of the drawing. The text height is gfx_texth.\0"
  "gfx_drawchar\tchar\tDraws the character (can be a numeric ASCII code as well), to gfx_x, gfx_y, and moves gfx_x over by the size of the character.\0"
  "gfx_drawstr\t\"str\"\tDraws a string at gfx_x, gfx_y, and updates gfx_x/gfx_y so that subsequent draws will occur in a similar place.\0"
  "gfx_measurestr\t\"str\",&w,&h\tMeasures the drawing dimensions of a string with the current font (as set by gfx_setfont). \0"
  "gfx_setfont\tidx[,\"fontface\", sz, flags]\tCan select a font and optionally configure it. idx=0 for default bitmapped font, no configuration is possible for this font. idx=1..16 for a configurable font, specify fontface such as \"Arial\", sz of 8-100, and optionally specify flags, which is a multibyte character, which can include 'i' for italics, 'u' for underline, or 'b' for bold. These flags may or may not be supported depending on the font and OS. After calling gfx_setfont(), gfx_texth may be updated to reflect the new average line height.\0"
  "gfx_getfont\t\tReturns current font index.\0"
  "gfx_printf\t\"format\"[, ...]\tFormats and draws a string at gfx_x, gfx_y, and updates gfx_x/gfx_y accordingly (the latter only if the formatted string contains newline). For more information on format strings, see sprintf()\0"
  "gfx_blurto\tx,y\tBlurs the region of the screen between gfx_x,gfx_y and x,y, and updates gfx_x,gfx_y to x,y.\0"
  "gfx_blit\tsource,scale,rotation\tIf three parameters are specified, copies the entirity of the source bitmap to gfx_x,gfx_y using current opacity and copy mode (set with gfx_a, gfx_mode). You can specify scale (1.0 is unscaled) and rotation (0.0 is not rotated, angles are in radians).\nFor the \"source\" parameter specify -1 to use the main framebuffer as source, or an image index (see gfx_loadimg()).\0"
  "gfx_blit\tsource, scale, rotation[, srcx, srcy, srcw, srch, destx, desty, destw, desth, rotxoffs, rotyoffs]\t"
                   "srcx/srcy/srcw/srch specify the source rectangle (if omitted srcw/srch default to image size), destx/desty/destw/desth specify dest rectangle (if not specified, these will default to reasonable defaults -- destw/desth default to srcw/srch * scale). \0"
  "gfx_blitext\tsource,coordinatelist,rotation\tDeprecated, use gfx_blit instead.\0"
  "gfx_getimgdim\timage,w,h\tRetreives the dimensions of image (representing a filename: index number) into w and h. Sets these values to 0 if an image failed loading (or if the filename index is invalid).\0"
  "gfx_setimgdim\timage,w,h\tResize image referenced by index 0.." EEL_LICE_DOC_MAXHANDLE ", width and height must be 0-2048. The contents of the image will be undefined after the resize.\0"
  "gfx_loadimg\timage,\"filename\"\tLoad image from filename into slot 0.." EEL_LICE_DOC_MAXHANDLE " specified by image. Returns the image index if success, otherwise -1 if failure. The image will be resized to the dimensions of the image file. \0"
  "gfx_gradrect\tx,y,w,h, r,g,b,a[, drdx, dgdx, dbdx, dadx, drdy, dgdy, dbdy, dady]\tFills a gradient rectangle with the color and alpha specified. drdx-dadx reflect the adjustment (per-pixel) applied for each pixel moved to the right, drdy-dady are the adjustment applied for each pixel moved toward the bottom. Normally drdx=adjustamount/w, drdy=adjustamount/h, etc.\0"
  "gfx_muladdrect\tx,y,w,h,mul_r,mul_g,mul_b[,mul_a,add_r,add_g,add_b,add_a]\tMultiplies each pixel by mul_* and adds add_*, and updates in-place. Useful for changing brightness/contrast, or other effects.\0"
  "gfx_deltablit\tsrcimg,srcx,srcy,srcw,srch,destx,desty,destw,desth,dsdx,dtdx,dsdy,dtdy,dsdxdy,dtdxdy\tBlits from srcimg(srcx,srcy,srcw,srch) "
      "to destination (destx,desty,destw,desth). Source texture coordinates are s/t, dsdx represents the change in s coordinate for each x pixel"
      ", dtdy represents the change in t coordinate for each y pixel, etc. dsdxdy represents the change in dsdx for each line. \0"
  "gfx_transformblit\tsrcimg,destx,desty,destw,desth,div_w,div_h,table\tBlits to destination at (destx,desty), size (destw,desth). "
      "div_w and div_h should be 2..64, and table should point to a table of 2*div_w*div_h values (this table must not cross a "
      "65536 item boundary). Each pair in the table represents a S,T coordinate in the source image, and the table is treated as "
      "a left-right, top-bottom list of texture coordinates, which will then be rendered to the destination.\0"
  "gfx_circle\tx,y,r[,fill,antialias]\tDraws a circle, optionally filling/antialiasing. \0"
  "gfx_roundrect\tx,y,w,h,radius[,antialias]\tDraws a rectangle with rounded corners. \0"
  "gfx_arc\tx,y,r,ang1,ang2[,antialias]\tDraws an arc of the circle centered at x,y, with ang1/ang2 being specified in radians.\0"

;
#ifdef EELSCRIPT_LICE_MAX_IMAGES
#undef MKSTR2
#undef MKSTR
#endif
#endif

#endif//_EEL_LICE_H_
