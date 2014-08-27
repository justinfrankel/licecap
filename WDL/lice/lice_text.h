#ifndef _LICE_TEXT_H_
#define _LICE_TEXT_H_

#include "lice.h"

#include "../heapbuf.h"

#define LICE_FONT_FLAG_VERTICAL 1 // rotate text to vertical (do not set the windows font to vertical though)
#define LICE_FONT_FLAG_VERTICAL_BOTTOMUP 2

#define LICE_FONT_FLAG_PRECALCALL 4
//#define LICE_FONT_FLAG_ALLOW_NATIVE 8
#define LICE_FONT_FLAG_FORCE_NATIVE 1024

#define LICE_FONT_FLAG_FX_BLUR 16
#define LICE_FONT_FLAG_FX_INVERT 32
#define LICE_FONT_FLAG_FX_MONO 64 // faster but no AA/etc

#define LICE_FONT_FLAG_FX_SHADOW 128 // these imply MONO
#define LICE_FONT_FLAG_FX_OUTLINE 256

#define LICE_FONT_FLAG_OWNS_HFONT 512

// could do a mask for these flags
#define LICE_FONT_FLAGS_HAS_FX(flag) \
  (flag&(LICE_FONT_FLAG_VERTICAL|LICE_FONT_FLAG_VERTICAL_BOTTOMUP| \
         LICE_FONT_FLAG_FX_BLUR|LICE_FONT_FLAG_FX_INVERT|LICE_FONT_FLAG_FX_MONO| \
         LICE_FONT_FLAG_FX_SHADOW|LICE_FONT_FLAG_FX_OUTLINE))

#define LICE_DT_NEEDALPHA  0x80000000 // include in DrawText() if the output alpha channel is important
#define LICE_DT_USEFGALPHA 0x40000000 // uses alpha channel in fg color 

class LICE_IFont
{
  public:
    virtual ~LICE_IFont() {}

    virtual void SetFromHFont(HFONT font, int flags=0)=0; // hfont must REMAIN valid, unless LICE_FONT_FLAG_PRECALCALL or LICE_FONT_FLAG_OWNS_HFONT set (OWNS means LICE_IFont will clean up hfont on font change or exit)

    virtual LICE_pixel SetTextColor(LICE_pixel color)=0;
    virtual LICE_pixel SetBkColor(LICE_pixel color)=0;
    virtual LICE_pixel SetEffectColor(LICE_pixel color)=0;
    virtual int SetBkMode(int bkmode)=0;
    virtual void SetCombineMode(int combine, float alpha=1.0f)=0;

    virtual int DrawText(LICE_IBitmap *bm, const char *str, int strcnt, RECT *rect, UINT dtFlags)=0;

    virtual LICE_pixel GetTextColor()=0;
    virtual HFONT GetHFont()=0;
};


class LICE_CachedFont : public LICE_IFont
{
  public:
    LICE_CachedFont();
    virtual ~LICE_CachedFont();

    virtual void SetFromHFont(HFONT font, int flags=0);

    virtual LICE_pixel SetTextColor(LICE_pixel color) { LICE_pixel ret=m_fg; m_fg=color; return ret; }
    virtual LICE_pixel SetBkColor(LICE_pixel color) { LICE_pixel ret=m_bg; m_bg=color; return ret; }
    virtual LICE_pixel SetEffectColor(LICE_pixel color) { LICE_pixel ret=m_effectcol; m_effectcol=color; return ret; }
    virtual int SetBkMode(int bkmode) { int bk = m_bgmode; m_bgmode=bkmode; return bk; }
    virtual void SetCombineMode(int combine, float alpha=1.0f) { m_comb=combine; m_alpha=alpha; }

    virtual int DrawText(LICE_IBitmap *bm, const char *str, int strcnt, RECT *rect, UINT dtFlags)
    {
      return DrawTextImpl(bm,str,strcnt,rect,dtFlags);
    }

    virtual LICE_pixel GetTextColor() { return m_fg; }
    virtual HFONT GetHFont() { return m_font; }

    void SetLineSpacingAdjust(int amt) { m_lsadj=amt; }

  private:

    int DrawTextImpl(LICE_IBitmap *bm, const char *str, int strcnt, RECT *rect, UINT dtFlags); // cause swell defines DrawText to SWELL_DrawText etc

    bool DrawGlyph(LICE_IBitmap *bm, unsigned short c, int xpos, int ypos, RECT *clipR);
    bool RenderGlyph(unsigned short idx);

    LICE_pixel m_fg,m_bg,m_effectcol;
    int m_bgmode;
    int m_comb;
    float m_alpha;
    int m_flags;

    int m_line_height,m_lsadj;
    struct charEnt
    {
      int base_offset; // offset in m_cachestore+1, so 1=offset0, 0=unset, -1=failed to render
      int width, height;
      int advance;
      int charid; // used by m_extracharlist
    };
    charEnt *findChar(unsigned short c);

    charEnt m_lowchars[128]; // first 128 chars cached here
    WDL_TypedBuf<charEnt> m_extracharlist;
    WDL_TypedBuf<unsigned char> m_cachestore;
    
    static int _charSortFunc(const void *a, const void *b);

    HFONT m_font;

};

#endif//_LICE_TEXT_H_
