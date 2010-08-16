#ifndef _LICE_TEXT_H_
#define _LICE_TEXT_H_

#include "lice.h"

#include "../heapbuf.h"

#define LICE_FONT_FLAG_VERTICAL 1 // rotate text to vertical (do not set the windows font to vertical though)
#define LICE_FONT_FLAG_VERTICAL_BOTTOMUP 2

#define LICE_FONT_FLAG_PRECALCALL 4
#define LICE_FONT_FLAG_ALLOW_NATIVE 8

#define LICE_FONT_FLAG_FX_BLUR 16
#define LICE_FONT_FLAG_FX_INVERT 32
#define LICE_FONT_FLAG_FX_MONO 64 // faster but no AA/etc

#define LICE_FONT_FLAG_FX_SHADOW 128 // these imply MONO
#define LICE_FONT_FLAG_FX_OUTLINE 256

#define LICE_FONT_FLAG_OWNS_HFONT 512

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
};


class LICE_CachedFont : public LICE_IFont
{
  public:
    LICE_CachedFont();
    ~LICE_CachedFont();

    void SetFromHFont(HFONT font, int flags=0);

    LICE_pixel SetTextColor(LICE_pixel color) { LICE_pixel ret=m_fg; m_fg=color; return ret; }
    LICE_pixel SetBkColor(LICE_pixel color) { LICE_pixel ret=m_bg; m_bg=color; return ret; }
    LICE_pixel SetEffectColor(LICE_pixel color) { LICE_pixel ret=m_effectcol; m_effectcol=color; return ret; }
    int SetBkMode(int bkmode) { int bk = m_bgmode; m_bgmode=bkmode; return bk; }
    void SetCombineMode(int combine, float alpha=1.0f) { m_comb=combine; m_alpha=alpha; }

    int DrawText(LICE_IBitmap *bm, const char *str, int strcnt, RECT *rect, UINT dtFlags);

  private:

    bool DrawGlyph(LICE_IBitmap *bm, unsigned char c, int xpos, int ypos, RECT *clipR);
    bool RenderGlyph(unsigned char idx);

    LICE_pixel m_fg,m_bg,m_effectcol;
    int m_bgmode;
    int m_comb;
    float m_alpha;
    int m_flags;

    int m_line_height;
    struct charEnt
    {
      int base_offset; // offset in m_cachestore+1, so 1=offset0, 0=unset, -1=failed to render
      int width, height;
      int advance;
    };

    charEnt m_chars[256]; // todo: MBCS support?
    WDL_TypedBuf<unsigned char> m_cachestore;

    HFONT m_font;

};

#endif//_LICE_TEXT_H_