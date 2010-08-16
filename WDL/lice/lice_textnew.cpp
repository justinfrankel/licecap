#include "lice_text.h"
#include <math.h>


#include "lice_combine.h"


//not threadsafe ----
static LICE_SysBitmap s_tempbitmap; // keep a sysbitmap around for rendering fonts


LICE_CachedFont::LICE_CachedFont() : m_cachestore(65536)
{
  m_fg=0;
  m_effectcol=m_bg=LICE_RGBA(255,255,255,255); 
  m_comb=0;
  m_alpha=1.0f;
  m_bgmode = TRANSPARENT;
  m_flags=0;
  m_line_height=0;
  m_font=0;
  memset(m_chars,0,sizeof(m_chars));
}

LICE_CachedFont::~LICE_CachedFont()
{
  if ((m_flags&LICE_FONT_FLAG_OWNS_HFONT) && m_font) {
    DeleteObject(m_font);
  }
}

void LICE_CachedFont::SetFromHFont(HFONT font, int flags)
{
  if ((flags&LICE_FONT_FLAG_OWNS_HFONT) && m_font) {
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

  memset(m_chars,0,sizeof(m_chars));
  m_cachestore.Resize(0);
  if (flags&LICE_FONT_FLAG_PRECALCALL)
  {
    int x;
    for(x=0;x<256;x++)
      RenderGlyph(x);
  }
}

bool LICE_CachedFont::RenderGlyph(unsigned char idx) // return TRUE if ok
{
  int bmsz=m_line_height;
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
  char tmpstr[2]={(char)idx,0};
  RECT r={0,0,0,0,};
  ::DrawText(s_tempbitmap.getDC(),tmpstr,1,&r,DT_CALCRECT|DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX);
  int advance=r.right;
  if (idx>='A' && idx<='Z') r.right+=2; // extra space for A-Z
  ::DrawText(s_tempbitmap.getDC(),tmpstr,1,&r,DT_SINGLELINE|DT_LEFT|DT_TOP|DT_NOPREFIX);

  if (oldFont) SelectObject(s_tempbitmap.getDC(),oldFont);

  charEnt *ent = m_chars+idx;

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
    unsigned char *destbuf = m_cachestore.Resize(ent->base_offset-1+r.right*r.bottom) + ent->base_offset-1;
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
      for(y=0;y<r.bottom;y++) for (x=0;x<r.right;x++) *destbuf++ = *destbuf>130 ? 255:0;

      destbuf -= r.right*r.bottom;
    }
    if (flags&(LICE_FONT_FLAG_FX_SHADOW|LICE_FONT_FLAG_FX_OUTLINE))
    {
      for(y=0;y<r.bottom;y++)
        for (x=0;x<r.right;x++)
          *destbuf++ = *destbuf>130 ? 255:0;

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

  return true;
}

template<class T> class GlyphRenderer
{
public:
  static void Normal(unsigned char *gsrc, LICE_pixel *pout,
              int src_span, int dest_span, int width, int height,
              int red, int green, int blue, float alpha)
  {
    int y;
    if (alpha==1.0)
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
      int a256=(int) (alpha*256.0);
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
bool LICE_CachedFont::DrawGlyph(LICE_IBitmap *bm, unsigned char c, 
                                int xpos, int ypos, RECT *clipR)
{
  charEnt *ch = m_chars+c;
  if (xpos+ch->width <= clipR->left || xpos >= clipR->right || 
      ypos+ch->height <= clipR->top || ypos >= clipR->bottom) return false;

  unsigned char *gsrc = m_cachestore.Get() + ch->base_offset-1;
  int src_span = ch->width;
  int width = ch->width;
  int height = ch->height;

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
      __LICE_ACTIONBYMODE_NOSRCALPHA(mode,alpha);
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
      __LICE_ACTIONBYMODE_NOSRCALPHA(mode,alpha);
      #undef __LICE__ACTION
    }
  }
  else
  {
    #define __LICE__ACTION(comb) GlyphRenderer<comb>::Normal(gsrc,pout,src_span,dest_span,width,height,red,green,blue,alpha)
    __LICE_ACTIONBYMODE_NOSRCALPHA(mode,alpha);
    #undef __LICE__ACTION
  }

  return true; // drew glyph at all (for updating max extents)
}

int LICE_CachedFont::DrawText(LICE_IBitmap *bm, const char *str, int strcnt, 
                               RECT *rect, UINT dtFlags)
{
  if (!bm) return 0;

  if ((m_flags&LICE_FONT_FLAG_ALLOW_NATIVE) && 
      !(m_flags&LICE_FONT_FLAG_PRECALCALL))
  {
    HDC hdc=bm->getDC();
    if (hdc)
    {
      ::SetTextColor(hdc,RGB(LICE_GETR(m_fg),LICE_GETG(m_fg),LICE_GETB(m_fg)));
      ::SetBkMode(hdc,m_bgmode);
      if (m_bgmode==OPAQUE)
        ::SetBkColor(hdc,RGB(LICE_GETR(m_bg),LICE_GETG(m_bg),LICE_GETB(m_bg)));

      return ::DrawText(hdc,str,strcnt,rect,dtFlags|DT_NOPREFIX);
    }
  }

  if (dtFlags & DT_CALCRECT)
  {
    int xpos=0;
    int ypos=0;
    int max_xpos=0;
    int max_ypos=0;
    for (; *str && strcnt--; str++)
    {
      unsigned char c = *(unsigned char *)str;
      if (c == '\r') continue;
      if (c == '\n')
      {
        if (dtFlags & DT_SINGLELINE) c=' ';
        else
        {
          if (m_flags&LICE_FONT_FLAG_VERTICAL) 
          {
            xpos+=m_line_height;
            ypos=0;
          }
          else
          {
            ypos+=m_line_height;
            xpos=0;
          }
          continue;
        }
      }


      if (m_chars[c].base_offset>=0)
      {
        if (m_chars[c].base_offset == 0) RenderGlyph(c);

        if (m_chars[c].base_offset > 0)
        {
          if (m_flags&LICE_FONT_FLAG_VERTICAL) 
          {
            ypos += m_chars[c].advance;
            if (xpos+m_chars[c].width>max_xpos) max_xpos=xpos+m_chars[c].width;
            if (ypos>max_ypos) max_ypos=ypos;
          }
          else
          {
            xpos += m_chars[c].advance;
            if (ypos+m_chars[c].height>max_ypos) max_ypos=ypos+m_chars[c].height;         
            if (xpos>max_xpos) max_xpos=xpos;
          }
        }
      }
    }

    rect->right = rect->left+max_xpos;
    rect->bottom = rect->top+max_ypos;

    return (m_flags&LICE_FONT_FLAG_VERTICAL) ? max_xpos : max_ypos;
  }

  if (m_alpha==0.0) return 0;

  RECT use_rect=*rect;
  int xpos=use_rect.left;
  int ypos=use_rect.top;

  bool isVertRev = false;
  if ((m_flags&(LICE_FONT_FLAG_VERTICAL|LICE_FONT_FLAG_VERTICAL_BOTTOMUP)) == (LICE_FONT_FLAG_VERTICAL|LICE_FONT_FLAG_VERTICAL_BOTTOMUP))
    isVertRev=true;

  if (dtFlags & (DT_CENTER|DT_VCENTER|DT_RIGHT|DT_BOTTOM))
  {
    RECT tr={0,};
    DrawText(bm,str,strcnt,&tr,DT_CALCRECT|(dtFlags & DT_SINGLELINE));
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
    DrawText(bm,str,strcnt,&tr,DT_CALCRECT|(dtFlags & DT_SINGLELINE));
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
      return 0;
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

  for (; *str && strcnt--; str++)
  {
    unsigned char c = *(unsigned char *)str;
    if (c == '\r') continue;
    if (c == '\n')
    {
      if (dtFlags & DT_SINGLELINE) c=' ';
      else
      {
        if (m_flags&LICE_FONT_FLAG_VERTICAL) 
        {
          xpos+=m_line_height;
          ypos=start_y;
        }
        else
        {
          ypos+=m_line_height;
          xpos=start_x;
        }
        continue;
      }
    }

    if (m_chars[c].base_offset>=0)
    {
      if (m_chars[c].base_offset==0) RenderGlyph(c);

      if (m_chars[c].base_offset > 0 && m_chars[c].base_offset < m_cachestore.GetSize())
      {
        if (isVertRev) ypos -= m_chars[c].height;
       
        bool drawn = DrawGlyph(bm,c,xpos,ypos,&use_rect);

        if (m_flags&LICE_FONT_FLAG_VERTICAL) 
        {
          if (!isVertRev)
          {
            ypos += m_chars[c].advance;
          }
          else ypos += m_chars[c].height - m_chars[c].advance;
          if (drawn && xpos+m_chars[c].width > max_xpos) max_xpos=xpos;
        }
        else
        {
          xpos += m_chars[c].advance;
          if (drawn && ypos+m_chars[c].height>max_ypos) max_ypos=ypos+m_chars[c].height;         
        }
      }
    }
  }

  return (m_flags&LICE_FONT_FLAG_VERTICAL) ? max_xpos - start_x : max_ypos - start_y;
}