#include "lice_text.h"
#include <math.h>


#ifndef _WIN32
#include "../swell/swell.h"
#endif


#include "lice_combine.h"
#include "lice_extended.h"

#ifdef _WIN32
static char __1ifNT2if98=0; // 2 for iswin98
#endif


static int utf8makechar(char *ptrout, unsigned short charIn)
{
  unsigned char *pout = (unsigned char *)ptrout;
  if (charIn < 128) { *pout = (unsigned char)charIn; return 1; }
  if (charIn < 2048) { pout[0] = 0xC0 + (charIn>>6); pout[1] = 0x80 + (charIn&0x3f); return 2; }
  pout[0] = 0xE0 + (charIn>>12);
  pout[1] = 0x80 + ((charIn>>6)&0x3f);
  pout[2] = 0x80 + (charIn&0x3f);
  return 3;
}

static int utf8char(const char *ptr, unsigned short *charOut) // returns char length
{
  const unsigned char *p = (const unsigned char *)ptr;
  unsigned char tc = *p;

  if (tc < 128) 
  {
    if (charOut) *charOut = (unsigned short) tc;
    return 1;
  }
  else if (tc < 0xC2) // invalid chars (subsequent in sequence, or overlong which we disable for)
  {
  }
  else if (tc < 0xE0) // 2 char seq
  {
    if (p[1] >= 0x80 && p[1] <= 0xC0)
    {
      if (charOut) *charOut = ((tc&0x1f)<<6) | (p[1]&0x3f);
      return 2;
    }
  }
  else if (tc < 0xF0) // 3 char seq
  {
    if (p[1] >= 0x80 && p[1] <= 0xC0 && p[2] >= 0x80 && p[2] <= 0xC0)
    {
      if (charOut) *charOut = ((tc&0xf)<<12) | ((p[1]&0x3f)<<6) | ((p[2]&0x3f));
      return 3;
    }
  }
  else if (tc < 0xF5) // 4 char seq
  {
    if (p[1] >= 0x80 && p[1] <= 0xC0 && p[2] >= 0x80 && p[2] <= 0xC0 && p[3] >= 0x80 && p[3] <= 0xC0)
    {
      if (charOut) *charOut = (unsigned short)' '; // dont support 4 byte sequences yet(ever?)
      return 4;
    }
  }  
  if (charOut) *charOut = (unsigned short) tc;
  return 1;  
}



//not threadsafe ----
static LICE_SysBitmap s_tempbitmap; // keep a sysbitmap around for rendering fonts


int LICE_CachedFont::_charSortFunc(const void *a, const void *b)
{
  charEnt *aa = (charEnt *)a;
  charEnt *bb = (charEnt *)b;
  return aa->charid - bb->charid;
}

LICE_CachedFont::LICE_CachedFont() : m_cachestore(65536)
{
  m_fg=0;
  m_effectcol=m_bg=LICE_RGBA(255,255,255,255); 
  m_comb=0;
  m_alpha=1.0f;
  m_bgmode = TRANSPARENT;
  m_flags=0;
  m_line_height=0;
  m_lsadj=0;
  m_font=0;
  memset(m_lowchars,0,sizeof(m_lowchars));
}

LICE_CachedFont::~LICE_CachedFont()
{
  if ((m_flags&LICE_FONT_FLAG_OWNS_HFONT) && m_font) {
    DeleteObject(m_font);
  }
}

void LICE_CachedFont::SetFromHFont(HFONT font, int flags)
{
  if ((m_flags&LICE_FONT_FLAG_OWNS_HFONT) && m_font && m_font != font)
  {
    DeleteObject(m_font);
  }

  m_flags=flags;
  m_font=font;
  if (font)
  {
    if (s_tempbitmap.getWidth() < 256 || s_tempbitmap.getHeight() < 256)
    {
      s_tempbitmap.resize(256,256);
      ::SetTextColor(s_tempbitmap.getDC(),RGB(255,255,255));
      ::SetBkMode(s_tempbitmap.getDC(),OPAQUE);
      ::SetBkColor(s_tempbitmap.getDC(),RGB(0,0,0));
    }

    TEXTMETRIC tm;
    HGDIOBJ oldFont = 0;
    if (font) oldFont = SelectObject(s_tempbitmap.getDC(),font);
    GetTextMetrics(s_tempbitmap.getDC(),&tm);
    if (oldFont) SelectObject(s_tempbitmap.getDC(),oldFont);

    m_line_height = tm.tmHeight;
  }

  memset(m_lowchars,0,sizeof(m_lowchars));
  m_extracharlist.Resize(0,false);
  m_cachestore.Resize(0);
  if (flags&LICE_FONT_FLAG_PRECALCALL)
  {
    int x;
    for(x=0;x<128;x++)
      RenderGlyph(x);
  }
}

bool LICE_CachedFont::RenderGlyph(unsigned short idx) // return TRUE if ok
{
  bool needSort=false;
  charEnt *ent;
  if (idx>=128)
  {
#ifdef _WIN32
    if (!__1ifNT2if98) __1ifNT2if98 = GetVersion()<0x80000000 ? 1 : 2;

    if (__1ifNT2if98==2) return false;
#endif
    ent=findChar(idx);
    if (!ent)
    {
      if (m_flags & LICE_FONT_FLAG_PRECALCALL) return false;

      int oldsz=m_extracharlist.GetSize();
      ent = m_extracharlist.Resize(oldsz+1) + oldsz;
      memset(ent,0,sizeof(*ent));
      ent->charid = idx;

      needSort=true;
    }
  }
  else ent = m_lowchars+idx;

  const int bmsz=max(m_line_height,1) * 2 + 8;
  if (s_tempbitmap.getWidth() < bmsz || s_tempbitmap.getHeight() < bmsz)
  {
    s_tempbitmap.resize(bmsz,bmsz);
    ::SetTextColor(s_tempbitmap.getDC(),RGB(255,255,255));
    ::SetBkMode(s_tempbitmap.getDC(),OPAQUE);
    ::SetBkColor(s_tempbitmap.getDC(),RGB(0,0,0));
  }

  LICE_Clear(&s_tempbitmap,0);

  HGDIOBJ oldFont=0;
  if (m_font) oldFont = SelectObject(s_tempbitmap.getDC(),m_font);
  RECT r={0,0,0,0,};
  int advance;

#ifdef _WIN32
  if (__1ifNT2if98==1) 
  {
    WCHAR tmpstr[2]={(WCHAR)idx,0};
    ::DrawTextW(s_tempbitmap.getDC(),tmpstr,1,&r,DT_CALCRECT|DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX);
    advance=r.right;
    if (idx>='A' && idx<='Z') r.right+=2; // extra space for A-Z
    ::DrawTextW(s_tempbitmap.getDC(),tmpstr,1,&r,DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX);
  }
  else
#endif
  {
    
    char tmpstr[6]={(char)idx,0};
#ifndef _WIN32
    if (idx>=128) utf8makechar(tmpstr,idx);
#endif
    ::DrawText(s_tempbitmap.getDC(),tmpstr,-1,&r,DT_CALCRECT|DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX);
    advance=r.right;
    if (idx>='A' && idx<='Z') r.right+=2; // extra space for A-Z
    ::DrawText(s_tempbitmap.getDC(),tmpstr,-1,&r,DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX);
  }

  if (advance > s_tempbitmap.getWidth()) advance=s_tempbitmap.getWidth();
  if (r.right > s_tempbitmap.getWidth()) r.right=s_tempbitmap.getWidth();
  if (r.bottom > s_tempbitmap.getHeight()) r.bottom=s_tempbitmap.getHeight();

  if (oldFont) SelectObject(s_tempbitmap.getDC(),oldFont);

  if (r.right < 1 || r.bottom < 1) 
  {
    ent->base_offset=-1;
    ent->advance=ent->width=ent->height=0;
  }
  else
  {
    ent->advance=advance;
    int flags=m_flags;
    if (flags&LICE_FONT_FLAG_FX_BLUR)
    {
      LICE_Blur(&s_tempbitmap,&s_tempbitmap,0,0,0,0,r.right,r.bottom);
    }
    LICE_pixel *srcbuf = s_tempbitmap.getBits();
    int span=s_tempbitmap.getRowSpan();

    ent->base_offset=m_cachestore.GetSize()+1;
    const int newsz = ent->base_offset-1+r.right*r.bottom;
    unsigned char *destbuf = m_cachestore.Resize(newsz) + ent->base_offset-1;
    if (m_cachestore.GetSize() != newsz)
    {
      ent->base_offset=-1;
      ent->advance=ent->width=ent->height=0;
    }
    else
    {
      if (s_tempbitmap.isFlipped())
      {
        srcbuf += (s_tempbitmap.getHeight()-1)*span;
        span=-span;
      }
      int x,y;
      for(y=0;y<r.bottom;y++)
      {
        if (flags&LICE_FONT_FLAG_FX_INVERT)
          for (x=0;x<r.right;x++)
            *destbuf++ = 255-((unsigned char*)(srcbuf+x))[LICE_PIXEL_R];
        else
          for (x=0;x<r.right;x++)
            *destbuf++ = ((unsigned char*)(srcbuf+x))[LICE_PIXEL_R];

        srcbuf += span;
      }
      destbuf -= r.right*r.bottom;

      if (flags&LICE_FONT_FLAG_VERTICAL)
      {
        int a=r.bottom; r.bottom=r.right; r.right=a;
  
        unsigned char *tmpbuf = (unsigned char *)s_tempbitmap.getBits();
        memcpy(tmpbuf,destbuf,r.right*r.bottom);
  
        int dptr=r.bottom;
        int dtmpbuf=1;
        if (!(flags&LICE_FONT_FLAG_VERTICAL_BOTTOMUP))
        {
          tmpbuf += (r.right-1)*dptr;
          dptr=-dptr;
        }
        else
        {
          tmpbuf+=r.bottom-1;
          dtmpbuf=-1;
        }
  
        for(y=0;y<r.bottom;y++)
        {
          unsigned char *ptr=tmpbuf;
          tmpbuf+=dtmpbuf;
          for(x=0;x<r.right;x++)
          {
            *destbuf++=*ptr;
            ptr+=dptr;
          }
        }
        destbuf -= r.right*r.bottom;
      }
      if (flags&LICE_FONT_FLAG_FX_MONO)
      {
        for(y=0;y<r.bottom;y++) for (x=0;x<r.right;x++) 
        {
          *destbuf = *destbuf>130 ? 255:0;
          destbuf++;
        }
  
        destbuf -= r.right*r.bottom;
      }
      if (flags&(LICE_FONT_FLAG_FX_SHADOW|LICE_FONT_FLAG_FX_OUTLINE))
      {
        for(y=0;y<r.bottom;y++)
          for (x=0;x<r.right;x++)
          {
            *destbuf = *destbuf>130 ? 255:0;
            destbuf++;
          }
  
        destbuf -= r.right*r.bottom;
        if (flags&LICE_FONT_FLAG_FX_SHADOW)
        {
          for(y=0;y<r.bottom;y++)
            for (x=0;x<r.right;x++)
            {
              if (!*destbuf)
              {
                if (x>0)
                {
                  if (destbuf[-1]==255) *destbuf=128;
                  else if (y>0 && destbuf[-r.right-1]==255) *destbuf=128;
                }
                if (y>0 && destbuf[-r.right]==255) *destbuf=128;
              }
              destbuf++;
            }
        }
        else
        {
          for(y=0;y<r.bottom;y++)
            for (x=0;x<r.right;x++)
            {
              if (!*destbuf)
              {
                if (y>0 && destbuf[-r.right]==255) *destbuf=128;
                else if (y<r.bottom-1 && destbuf[r.right]==255) *destbuf=128;
                else if (x>0)
                {
                  if (destbuf[-1]==255) *destbuf=128;
      //          else if (y>0 && destbuf[-r.right-1]==255) *destbuf=128;
    //            else if (y<r.bottom-1 && destbuf[r.right-1]==255) *destbuf=128;
                }
  
                if (x<r.right-1)
                {
                  if (destbuf[1]==255) *destbuf=128;
//                else if (y>0 && destbuf[-r.right+1]==255) *destbuf=128;
  //              else if (y<r.bottom-1 && destbuf[r.right+1]==255) *destbuf=128;
                }
              }
              destbuf++;
            }
        }
      }
      ent->width = r.right;
      ent->height = r.bottom;
    }
  }
  if (needSort&&m_extracharlist.GetSize()>1) qsort(m_extracharlist.Get(),m_extracharlist.GetSize(),sizeof(charEnt),_charSortFunc);

  return true;
}

template<class T> class GlyphRenderer
{
public:
  static void Normal(unsigned char *gsrc, LICE_pixel *pout,
              int src_span, int dest_span, int width, int height,
              int red, int green, int blue, int a256)
  {
    int y;
    if (a256==256)
    {
      for(y=0;y<height;y++)
      {
        int x;
        for(x=0;x<width;x++)
        {
          unsigned char v=gsrc[x];
          if (v) T::doPix((unsigned char *)(pout+x),red,green,blue,255,(int)v+1);
        }
        gsrc += src_span;
        pout += dest_span;
      }
    }
    else
    {
      for(y=0;y<height;y++)
      {
        int x;
        for(x=0;x<width;x++)
        {
          unsigned char v=gsrc[x];
          if (v) 
          {
            int a=(v*a256)/256;
            if (a>256)a=256;
            T::doPix((unsigned char *)(pout+x),red,green,blue,255,a);
          }
        }
        gsrc += src_span;
        pout += dest_span;
      }
    }
  }
  static void Mono(unsigned char *gsrc, LICE_pixel *pout,
              int src_span, int dest_span, int width, int height,
              int red, int green, int blue, int alpha)
  {
    int y;
    for(y=0;y<height;y++)
    {
      int x;
      for(x=0;x<width;x++)
        if (gsrc[x]) T::doPix((unsigned char *)(pout+x),red,green,blue,255,alpha);
      gsrc += src_span;
      pout += dest_span;
    }
  }
  static void Effect(unsigned char *gsrc, LICE_pixel *pout,
              int src_span, int dest_span, int width, int height,
              int red, int green, int blue, int alpha, int r2, int g2, int b2)
  {
    int y;
    for(y=0;y<height;y++)
    {
      int x;
      for(x=0;x<width;x++)
      {
        unsigned char v=gsrc[x];
        if (v) 
        {
          if (v==255) T::doPix((unsigned char *)(pout+x),red,green,blue,255,alpha);
          else T::doPix((unsigned char *)(pout+x),r2,g2,b2,255,alpha);
        }
      }
      gsrc += src_span;
      pout += dest_span;
    }
  }
};

LICE_CachedFont::charEnt *LICE_CachedFont::findChar(unsigned short c)
{
  if (c<128) return m_lowchars+c;
  if (!m_extracharlist.GetSize()) return 0;
  charEnt a={0,};
  a.charid=c;
  return (charEnt *)bsearch(&a,m_extracharlist.Get(),m_extracharlist.GetSize(),sizeof(charEnt),_charSortFunc);
}

bool LICE_CachedFont::DrawGlyph(LICE_IBitmap *bm, unsigned short c, 
                                int xpos, int ypos, RECT *clipR)
{
  charEnt *ch = findChar(c);

  if (!ch || xpos+ch->width <= clipR->left || xpos >= clipR->right || 
      ypos+ch->height <= clipR->top || ypos >= clipR->bottom) return false;

  unsigned char *gsrc = m_cachestore.Get() + ch->base_offset-1;
  int src_span = ch->width;
  int width = ch->width;
  int height = ch->height;

#ifndef DISABLE_LICE_EXTENSIONS
  if (bm->Extended(LICE_EXT_SUPPORTS_ID, (void*) LICE_EXT_DRAWGLYPH_ACCEL))
  {
    LICE_Ext_DrawGlyph_acceldata data(xpos, ypos, m_fg, gsrc, width, height, m_alpha, m_comb);
    if (bm->Extended(LICE_EXT_DRAWGLYPH_ACCEL, &data)) return true;
  }
#endif

  if (xpos < clipR->left) 
  { 
    width += (xpos-clipR->left); 
    gsrc += clipR->left-xpos; 
    xpos=clipR->left; 
  }
  if (ypos < clipR->top) 
  { 
    gsrc += src_span*(clipR->top-ypos);
    height += (ypos-clipR->top); 
    ypos=clipR->top; 
  }
  int dest_span = bm->getRowSpan();
  LICE_pixel *pout = bm->getBits();

  if (bm->isFlipped())
  {
    pout += (bm->getHeight()-1)*dest_span;
    dest_span=-dest_span;
  }
  
  pout += xpos + ypos * dest_span;

  if (xpos+width >= clipR->right) width = clipR->right-xpos;

  if (ypos+height >= clipR->bottom) height = clipR->bottom-ypos;

  int mode=m_comb&~LICE_BLIT_USE_ALPHA;
  float alpha=m_alpha;
  
  if (m_bgmode==OPAQUE)
    LICE_FillRect(bm,xpos,ypos,width,height,m_bg,alpha,mode);

  int red=LICE_GETR(m_fg);
  int green=LICE_GETG(m_fg);
  int blue=LICE_GETB(m_fg);

  if (m_flags&LICE_FONT_FLAG_FX_MONO)
  {
    if (alpha==1.0 && (mode&LICE_BLIT_MODE_MASK)==LICE_BLIT_MODE_COPY) // fast simple
    {
      LICE_pixel col=m_fg;
      int y;
      for(y=0;y<height;y++)
      {
        int x;
        for(x=0;x<width;x++) if (gsrc[x]) pout[x]=col;
        gsrc += src_span;
        pout += dest_span;
      }
    }
    else 
    {
      int avalint = (int) (alpha*256.0);
      if (avalint>256)avalint=256;

      #define __LICE__ACTION(comb) GlyphRenderer<comb>::Mono(gsrc,pout,src_span,dest_span,width,height,red,green,blue,avalint)
      __LICE_ACTION_NOSRCALPHA(mode,avalint, false);
      #undef __LICE__ACTION
    }
  }
  else if (m_flags&(LICE_FONT_FLAG_FX_SHADOW|LICE_FONT_FLAG_FX_OUTLINE))
  {
    LICE_pixel col=m_fg;
    LICE_pixel bkcol=m_effectcol;

    if (alpha==1.0 && (mode&LICE_BLIT_MODE_MASK)==LICE_BLIT_MODE_COPY)
    {
      int y;
      for(y=0;y<height;y++)
      {
        int x;
        for(x=0;x<width;x++) 
        {
          unsigned char c=gsrc[x];
          if (c) pout[x]=c==255? col : bkcol;
        }
        gsrc += src_span;
        pout += dest_span;
      }
    }
    else 
    {
      int avalint = (int) (alpha*256.0);
      if (avalint>256)avalint=256;
      int r2=LICE_GETR(bkcol);
      int g2=LICE_GETG(bkcol);
      int b2=LICE_GETB(bkcol);
      #define __LICE__ACTION(comb) GlyphRenderer<comb>::Effect(gsrc,pout,src_span,dest_span,width,height,red,green,blue,avalint,r2,g2,b2)
      __LICE_ACTION_NOSRCALPHA(mode,avalint, false);
      #undef __LICE__ACTION
    }
  }
  else
  {
    int avalint = (int) (alpha*256.0);
    #define __LICE__ACTION(comb) GlyphRenderer<comb>::Normal(gsrc,pout,src_span,dest_span,width,height,red,green,blue,avalint)
    __LICE_ACTION_NOSRCALPHA(mode,avalint, false);
    #undef __LICE__ACTION
  }

  return true; // drew glyph at all (for updating max extents)
}


static int LICE_Text_IsWine()
{
  static int isWine=-1;
#ifdef _WIN32
  if (isWine<0)
  {
    HKEY hk;
    if (RegOpenKey(HKEY_LOCAL_MACHINE,"Software\\Wine",&hk)==ERROR_SUCCESS)
    {
      isWine=1;
      RegCloseKey(hk);
    }
    else isWine=0;
  }
#endif
  return isWine>0;
}

#ifdef _WIN32
static BOOL LICE_Text_HasUTF8(const char *_str)
{
  const unsigned char *str = (const unsigned char *)_str;
  if (!str) return FALSE;
  while (*str) 
  {
    unsigned char c = *str;
    if (c >= 0xC2) // fuck overlongs
    {
      if (c <= 0xDF && str[1] >=0x80 && str[1] <= 0xBF) return TRUE;
      else if (c <= 0xEF && str[1] >=0x80 && str[1] <= 0xBF && str[2] >=0x80 && str[2] <= 0xBF) return TRUE;
      else if (c <= 0xF4 && str[1] >=0x80 && str[1] <= 0xBF && str[2] >=0x80 && str[2] <= 0xBF) return TRUE;
    }
    str++;
  }
  return FALSE;
}
#endif


int LICE_CachedFont::DrawTextImpl(LICE_IBitmap *bm, const char *str, int strcnt, 
                               RECT *rect, UINT dtFlags)
{
  if (!bm && !(dtFlags&DT_CALCRECT)) return 0;

  bool forceWantAlpha=false;

  if (dtFlags & LICE_DT_NEEDALPHA)
  {
    forceWantAlpha=true;
    dtFlags &= ~LICE_DT_NEEDALPHA;
  }

#if 0
  if ((m_flags&LICE_FONT_FLAG_ALLOW_NATIVE) && 
      !(m_flags&LICE_FONT_FLAG_PRECALCALL))
  {
    HDC hdc = (bm ? bm->getDC() : 0);
    if (hdc)
    {
      ::SetTextColor(hdc,RGB(LICE_GETR(m_fg),LICE_GETG(m_fg),LICE_GETB(m_fg)));
      ::SetBkMode(hdc,m_bgmode);
      if (m_bgmode==OPAQUE)
        ::SetBkColor(hdc,RGB(LICE_GETR(m_bg),LICE_GETG(m_bg),LICE_GETB(m_bg)));

      return ::DrawText(hdc,str,strcnt,rect,dtFlags|DT_NOPREFIX);
    }
  }
#endif

  // if using line-spacing adjustments (m_lsadj), don't allow native rendering 
  // todo: split rendering up into invidual lines and DrawText calls
  if ((m_flags&LICE_FONT_FLAG_FORCE_NATIVE) && m_font && !forceWantAlpha &&!LICE_Text_IsWine() && 
      !(dtFlags & LICE_DT_USEFGALPHA) &&
      !(m_flags&LICE_FONT_FLAG_PRECALCALL) && !LICE_FONT_FLAGS_HAS_FX(m_flags) &&
     (!m_lsadj || (dtFlags&DT_SINGLELINE))) 
  {

    // on Win2000+, use wide versions if needed for UTF
#ifdef _WIN32
    WCHAR wtmpbuf[1024];
    WCHAR *wtmp=NULL;
    static int win9x;
    if (!win9x) win9x = GetVersion() < 0x80000000 ? -1 : 1; //>0 if win9x
    if (win9x<0 && LICE_Text_HasUTF8(str))
    {
      int req = MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,str,strcnt,NULL,0);
      if (req < 1000) 
      {
        int cnt=0;
        if ((cnt=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,str,strcnt,wtmpbuf,1024)))
        {
          wtmp=wtmpbuf;
          wtmp[cnt]=0;
        }
      }
      else
      {
        wtmp = (WCHAR *)malloc((req + 32)*sizeof(WCHAR));
        int cnt=-1;
        if (wtmp && !(cnt=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,str,strcnt,wtmp,req+1)))
          free(wtmp);
        else if (cnt>0) wtmp[cnt]=0;
      }
    }
#endif

    HDC hdc = (bm ? bm->getDC() : 0);
    int w = rect->right-rect->left;
    int h = rect->bottom-rect->top;
    HGDIOBJ oldfont = 0;
    RECT srcr={0,};
    bool isTmp=false;
    POINT blitPos={0,};

    static LICE_SysBitmap s_nativerender_tempbitmap; 

    if (!hdc)  // use temp buffer
    {
      isTmp=true;
      if (w<1)w=1;
      if (h<1)h=1;
      if (s_nativerender_tempbitmap.getWidth() < w ||
          s_nativerender_tempbitmap.getHeight() < h)
        s_nativerender_tempbitmap.resize(w, h);


      hdc = s_nativerender_tempbitmap.getDC();

      oldfont = SelectObject(hdc, m_font);
  
      RECT blit_r = {0,0};
      int rv=
#ifdef _WIN32
        wtmp ? 
          ::DrawTextW(hdc,wtmp,-1,&blit_r,(dtFlags&~(DT_CENTER|DT_VCENTER|DT_TOP|DT_BOTTOM|DT_LEFT|DT_RIGHT))|DT_CALCRECT|DT_NOPREFIX)
          : 
#endif
          ::DrawText(hdc,str,strcnt,&blit_r,(dtFlags&~(DT_CENTER|DT_VCENTER|DT_TOP|DT_BOTTOM|DT_LEFT|DT_RIGHT))|DT_CALCRECT|DT_NOPREFIX);
      if (dtFlags & DT_CALCRECT)
      {
        SelectObject(hdc,oldfont);
        rect->right = rect->left + blit_r.right - blit_r.left;
        rect->bottom = rect->top + blit_r.bottom - blit_r.top;

#ifdef _WIN32
        if (wtmp!=wtmpbuf)free(wtmp);
#endif
        return rv;
      }

      if (!bm) return 0;

      // if noclip, resize up
      if (dtFlags&DT_NOCLIP)
      {
        w=blit_r.right-blit_r.left;
        h=blit_r.bottom-blit_r.top;
      }

      // set new width/height (if shrinking down)
      if (blit_r.right-blit_r.left < w) 
      {
        if (dtFlags & DT_RIGHT) blitPos.x =  w-(blit_r.right-blit_r.left);
        else if (dtFlags & DT_CENTER) blitPos.x=(w-(blit_r.right-blit_r.left))/2;

        w=blit_r.right-blit_r.left;
      }
      if (blit_r.bottom-blit_r.top < h) 
      {
        if (dtFlags & DT_BOTTOM) blitPos.y =  h-(blit_r.bottom-blit_r.top);
        else if (dtFlags & DT_VCENTER) blitPos.y=(h-(blit_r.bottom-blit_r.top))/2;

        h=blit_r.bottom-blit_r.top;
      }

      if (w > s_nativerender_tempbitmap.getWidth() || 
         h > s_nativerender_tempbitmap.getHeight())
      {
        SelectObject(hdc,oldfont);
        s_nativerender_tempbitmap.resize(w, h);
        hdc = s_nativerender_tempbitmap.getDC();
        oldfont = SelectObject(hdc, m_font);
      }

      blit_r.left=rect->left + blitPos.x;
      blit_r.top = rect->top + blitPos.y;
      blit_r.right = blit_r.left+w;
      blit_r.bottom = blit_r.top+h;
      LICE_Blit(&s_nativerender_tempbitmap, bm, 0, 0, &blit_r, 1.0f, LICE_BLIT_MODE_COPY);
      srcr.right=w;
      srcr.bottom=h;
    }
    else
    {
      oldfont = SelectObject(hdc, m_font);
      srcr = *rect;
    }

    ::SetTextColor(hdc,RGB(LICE_GETR(m_fg),LICE_GETG(m_fg),LICE_GETB(m_fg)));
    ::SetBkMode(hdc,m_bgmode);
    if (m_bgmode==OPAQUE) ::SetBkColor(hdc,RGB(LICE_GETR(m_bg),LICE_GETG(m_bg),LICE_GETB(m_bg)));

    int ret = 
#ifdef _WIN32
      wtmp ? 
      ::DrawTextW(hdc,wtmp,-1,&srcr,dtFlags|DT_NOPREFIX)
      : 
#endif
      ::DrawText(hdc,str,strcnt,&srcr,dtFlags|DT_NOPREFIX);

    if (isTmp) LICE_Blit(bm, &s_nativerender_tempbitmap,  rect->left+blitPos.x, rect->top+blitPos.y, &srcr, m_alpha, LICE_BLIT_MODE_COPY);
    else if (dtFlags & DT_CALCRECT) *rect = srcr;


    SelectObject(hdc, oldfont);
#ifdef _WIN32
    if (wtmp!=wtmpbuf) free(wtmp);
#endif
    return ret;
  }


  if (dtFlags & DT_CALCRECT)
  {
    int xpos=0;
    int ypos=0;
    int max_xpos=0;
    int max_ypos=0;
    while (*str && strcnt)
    {
      unsigned short c=' ';
      int charlen = utf8char(str,&c);
      str += charlen;
      if (strcnt>0)
      {
        strcnt -= charlen;
        if (strcnt<0) strcnt=0;
      }

      if (c == '\r') continue;
      if (c == '\n')
      {
        if (dtFlags & DT_SINGLELINE) c=' ';
        else
        {
          if (m_flags&LICE_FONT_FLAG_VERTICAL) 
          {
            xpos+=m_line_height+m_lsadj;
            ypos=0;
          }
          else
          {
            ypos+=m_line_height+m_lsadj;
            xpos=0;
          }
          continue;
        }
      }

      charEnt *ent = findChar(c);
      if (!ent) 
      {
        int os=m_extracharlist.GetSize();
        RenderGlyph(c);
        if (m_extracharlist.GetSize()!=os)
          ent = findChar(c);
      }

      if (ent && ent->base_offset>=0)
      {
        if (ent->base_offset == 0) RenderGlyph(c);      

        if (ent->base_offset > 0)
        {
          if (m_flags&LICE_FONT_FLAG_VERTICAL) 
          {
            ypos += ent->advance;
            if (xpos+ent->width>max_xpos) max_xpos=xpos+ent->width;
            if (ypos>max_ypos) max_ypos=ypos;
          }
          else
          {
            xpos += ent->advance;
            if (ypos+ent->height>max_ypos) max_ypos=ypos+ent->height;         
            if (xpos>max_xpos) max_xpos=xpos;
          }
        }
      }
    }

    rect->right = rect->left+max_xpos;
    rect->bottom = rect->top+max_ypos;


    return (m_flags&LICE_FONT_FLAG_VERTICAL) ? max_xpos : max_ypos;
  }
  float alphaSave  = m_alpha;

  if (dtFlags & LICE_DT_USEFGALPHA)
  {
    m_alpha *= LICE_GETA(m_fg)/255.0;
  }

  if (m_alpha==0.0) 
  {
    m_alpha=alphaSave;
    return 0;
  }


  RECT use_rect=*rect;
  int xpos=use_rect.left;
  int ypos=use_rect.top;

  bool isVertRev = false;
  if ((m_flags&(LICE_FONT_FLAG_VERTICAL|LICE_FONT_FLAG_VERTICAL_BOTTOMUP)) == (LICE_FONT_FLAG_VERTICAL|LICE_FONT_FLAG_VERTICAL_BOTTOMUP))
    isVertRev=true;

  if (dtFlags & (DT_CENTER|DT_VCENTER|DT_RIGHT|DT_BOTTOM))
  {
    RECT tr={0,};
    DrawTextImpl(bm,str,strcnt,&tr,DT_CALCRECT|(dtFlags & DT_SINGLELINE)|(forceWantAlpha?LICE_DT_NEEDALPHA:0));
    if (dtFlags & DT_CENTER)
    {
      xpos += (use_rect.right-use_rect.left-tr.right)/2;
    }
    else if (dtFlags & DT_RIGHT)
    {
      xpos = use_rect.right - tr.right;
    }

    if (dtFlags & DT_VCENTER)
    {
      ypos += (use_rect.bottom-use_rect.top-tr.bottom)/2;
    }
    else if (dtFlags & DT_BOTTOM)
    {
      ypos = use_rect.bottom - tr.bottom;
    }    
    if (isVertRev)
      ypos += tr.bottom;
  }
  else if (isVertRev) 
  {
    RECT tr={0,};
    DrawTextImpl(bm,str,strcnt,&tr,DT_CALCRECT|(dtFlags & DT_SINGLELINE)|(forceWantAlpha?LICE_DT_NEEDALPHA:0));
    ypos += tr.bottom;
  }


  int start_y=ypos;
  int start_x=xpos;
  int max_ypos=ypos;
  int max_xpos=xpos;

  if (!(dtFlags & DT_NOCLIP))
  {
    if (use_rect.left<0)use_rect.left=0;
    if (use_rect.top<0) use_rect.top=0;
    if (use_rect.right > bm->getWidth()) use_rect.right = bm->getWidth();
    if (use_rect.bottom > bm->getHeight()) use_rect.bottom = bm->getHeight();
    if (use_rect.right <= use_rect.left || use_rect.bottom <= use_rect.top)
    {
      m_alpha=alphaSave;
      return 0;
    }
  }
  else
  {
    use_rect.left=use_rect.top=0;
    use_rect.right = bm->getWidth();
    use_rect.bottom = bm->getHeight();
  }


  // todo: handle DT_END_ELLIPSIS etc 
  // thought: calculate length of "...", then when pos+length+widthofnextchar >= right, switch
  // might need to precalc size to make sure it's needed, though

  while (*str && strcnt)
  {
    unsigned short c=' ';
    int charlen = utf8char(str,&c);
    str += charlen;
    if (strcnt>0)
    {
      strcnt -= charlen;
      if (strcnt<0) strcnt=0;
    }
    if (c == '\r') continue;
    if (c == '\n')
    {
      if (dtFlags & DT_SINGLELINE) c=' ';
      else
      {
        if (m_flags&LICE_FONT_FLAG_VERTICAL) 
        {
          xpos+=m_line_height+m_lsadj;
          ypos=start_y;
        }
        else
        {
          ypos+=m_line_height+m_lsadj;
          xpos=start_x;
        }
        continue;
      }
    }

    charEnt *ent = findChar(c);
    if (!ent) 
    {
      int os=m_extracharlist.GetSize();
      RenderGlyph(c);
      if (m_extracharlist.GetSize()!=os)
        ent = findChar(c);
    }

    if (ent && ent->base_offset>=0)
    {
      if (ent->base_offset==0) RenderGlyph(c);

      if (ent->base_offset > 0 && ent->base_offset < m_cachestore.GetSize())
      {
        if (isVertRev) ypos -= ent->height;
       
        bool drawn = DrawGlyph(bm,c,xpos,ypos,&use_rect);

        if (m_flags&LICE_FONT_FLAG_VERTICAL) 
        {
          if (!isVertRev)
          {
            ypos += ent->advance;
          }
          else ypos += ent->height - ent->advance;
          if (drawn && xpos+ent->width > max_xpos) max_xpos=xpos;
        }
        else
        {
          xpos += ent->advance;
          if (drawn && ypos+ent->height>max_ypos) max_ypos=ypos+ent->height;         
        }
      }
    }
  }

  m_alpha=alphaSave;
  return (m_flags&LICE_FONT_FLAG_VERTICAL) ? max_xpos - start_x : max_ypos - start_y;
}
