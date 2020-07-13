/*
    WDL - virtwnd-listbox.cpp
    Copyright (C) 2006 and later Cockos Incorporated

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
      

    Implementation for virtual window listboxes.

*/

#include "virtwnd-controls.h"
#include "../lice/lice.h"

#define EXPAND_SCROLLBAR_SIZE(sz) ((sz)*2)

WDL_VirtualListBox::WDL_VirtualListBox()
{
  m_scrollbar_color = LICE_RGBA(0,128,255,255);
  m_scrollbar_blendmode=LICE_BLIT_MODE_COPY;
  m_scrollbar_alpha = 1.0f;
  m_scrollbar_size=4;
  m_scrollbar_border=1;
  m_scrollbar_expanded = false;
  m_want_wordwise_cols = false;
  m_cap_startitem=-1;
  m_cap_state=0;
  m_cap_startpos.x = m_cap_startpos.y = 0;
  m_margin_l=m_margin_r=0;
  m_GetItemInfo=0;
  m_CustomDraw=0;
  m_GetItemHeight=NULL;
  m_GetItemInfo_ctx=0;
  m_viewoffs=0;
  m_align=-1;
  m_rh=14;
  m_maxcolwidth=m_mincolwidth=0;
  m_colgap=0;
  m_lsadj=-1000;
  m_font=0;
  m_clickmsg=0;
  m_dropmsg=0;
  m_dragmsg=0;
  m_grayed=false;
}

WDL_VirtualListBox::~WDL_VirtualListBox()
{
}

static bool items_fit(int test_h, const int *heights, int heights_sz, int h, int max_cols, int rh_base)
{
  int y=test_h, cols=1;
  for (int item = 0; item < heights_sz; item ++)
  {
    int rh = heights[item] & WDL_VirtualListBox::ITEMH_MASK;
    if (y > 0 && y + rh > h) 
    {
      if (max_cols == 1 && !(heights[item]&WDL_VirtualListBox::ITEMH_FLAG_NOSQUISH) && 
            y + rh_base <= h) return item == heights_sz-1; // allow partial of larger item in single column mode
      if (++cols > max_cols) return false;
      y=0;
    }
    y += rh;
  }
  return true;
}

void WDL_VirtualListBox::CalcLayout(int num_items, layout_info *layout)
{
  const int w = m_position.right - m_position.left, h = m_position.bottom - m_position.top;

  int max_cols = 1, min_cols = 1, max_cols2 = 1;
  if (m_mincolwidth>0) 
  {
    max_cols = w / m_mincolwidth;
    max_cols2 = (w - m_scrollbar_size - m_colgap) / m_mincolwidth;
    if (max_cols < 1) max_cols = 1;
    if (max_cols2 < 1) max_cols2 = 1;
  }
  if (m_maxcolwidth>0)
  {
    min_cols = w / m_maxcolwidth;
    if (min_cols < 1) min_cols = 1;
  }
  if (max_cols < min_cols) max_cols = min_cols;

  static WDL_TypedBuf<int> s_heights;

  s_heights.Resize(0,false);

  int startitem = wdl_min(m_viewoffs,num_items-1), cols = 1, y = 0;
  if (startitem < 0) startitem = 0;

  const int rh_base = GetRowHeight();
  int item;
  for (item = startitem; item < num_items; item ++)
  {
    int flag=0;
    int rh = GetItemHeight(item, &flag);
    if (y > 0 && y + rh > h) 
    {
      if (max_cols == 1 && !(flag&ITEMH_FLAG_NOSQUISH) && y + rh_base <= h) s_heights.Add(rh | flag); // allow partial of larger item in single column mode
      if (cols >= max_cols) break;
      cols++;
      y=0;
    }
    s_heights.Add(rh | flag);
    y += rh;
  }
  if (item >= num_items)
  {
    int use_col = wdl_max(min_cols,cols);
    if (m_GetItemHeight == NULL && m_want_wordwise_cols && use_col>1 && h >= rh_base*2)
    {
      int viscnt = use_col * (h/rh_base);
      if (startitem + viscnt > num_items + use_col-1)
      {
        startitem = num_items-viscnt + use_col-1;
        if (startitem < 0) startitem=0;
      }
    }
    else
    {
      int use_h = h;
      if (use_col > 1 && num_items >= use_col)
        use_h -= m_scrollbar_size;

      while (startitem > 0)
      {
        int flag=0;
        int rh = GetItemHeight(startitem-1, &flag);
        if (!items_fit(rh,s_heights.Get(),s_heights.GetSize(),use_h,max_cols,rh_base)) break;
        s_heights.Insert(rh | flag,0);
        startitem--;
      }
    }
  }
  const bool has_scroll = item < num_items || startitem > 0;

  if (has_scroll && cols > 1)
  {
    max_cols = max_cols2;
    if (max_cols < min_cols) max_cols = min_cols;
    if (cols > max_cols) cols = max_cols;
  }
  if (cols < min_cols) cols = min_cols;
  layout->startpos = startitem;
  layout->columns = cols;
  layout->heights = &s_heights;

  int scroll_mode = 0;
  if (has_scroll)
  {
    if (cols > 1 && (m_GetItemHeight!=NULL || !m_want_wordwise_cols || h < rh_base*2))
      scroll_mode=2;
    else
      scroll_mode=1;
  }

  layout->vscrollbar_w = scroll_mode == 1 ? m_scrollbar_size : 0;
  layout->hscrollbar_h = scroll_mode == 2 ? m_scrollbar_size : 0;
  layout->item_area_w = w - layout->vscrollbar_w;
  layout->item_area_h = wdl_max(h - layout->hscrollbar_h,rh_base);
  if (AreItemsWordWise(*layout))
  {
    int adj = layout->startpos%layout->columns;
    for (int x = 0; x <adj; x++)
      s_heights.Insert(rh_base,0);
    layout->startpos -= adj;
  }
  else if (scroll_mode == 2 && m_GetItemHeight == NULL && rh_base > 0)
  {
    const int rowcnt = layout->item_area_h / rh_base;
    if (rowcnt > 1)
    {
      int adj = rowcnt - (layout->startpos%rowcnt);
      if (adj < rowcnt)
      {
        if (adj > s_heights.GetSize()) adj = s_heights.GetSize();
        layout->startpos += adj;
        for (int x = 0; x < adj; x++) s_heights.Delete(0);
      }
    }
  }
}

static void maxSizePreservingAspect(int sw, int sh, int dw, int dh, int *outw, int *outh)
{
  *outw=dw;
  *outh=dh;
  if (sw < 1 || sh < 1) return;
  int xwid = (sw * dh) / sh; // calculate width required if we make it dh pixels tall
  if (xwid > dw)
  {
    // too wide, make maximum width and reduce height
    *outh = (dw * sh) / sw;
  }
  else 
  {
    // too narrow, use full height and reduce width
    *outw = (dh * sw) / sh;
  }
}

static void DrawBkImage(LICE_IBitmap *drawbm, WDL_VirtualWnd_BGCfg *bkbm, int drawx, int drawy, int draww, int drawh,
                RECT *cliprect, int drawsrcx, int drawsrcw, int bkbmstate, float alpha, int whichpass=0)
{
  bool haspink=bkbm->bgimage_lt[0]||bkbm->bgimage_lt[1]||bkbm->bgimage_rb[0] || bkbm->bgimage_rb[1];

  if (whichpass==1)
  {
    int sw = (bkbm->bgimage->getWidth() - (haspink? 2 :0))/2;
    int sh = (bkbm->bgimage->getHeight() - (haspink? 2 :0))/3;

    int usew,useh;
    // scale drawing coords by image dimensions
    maxSizePreservingAspect(sw,sh,draww,drawh,&usew,&useh);

    if (usew == sw-1 || usew == sw+1) usew=sw;
    if (useh == sh-1 || useh == sh+1) useh=sh;
    drawx += (draww-usew)/2;
    drawy += (drawh-useh)/2;
    draww = usew;
    drawh = useh;
  }

  int hh=bkbm->bgimage->getHeight()/3;

  if (haspink)
  {
    WDL_VirtualWnd_BGCfg tmp = *bkbm;
    if ((tmp.bgimage_noalphaflags&0xffff)!=0xffff) tmp.bgimage_noalphaflags=0;  // force alpha channel if any alpha

    if (drawsrcx>0) { drawsrcx--; drawsrcw++; }
    LICE_SubBitmap bm(tmp.bgimage,drawsrcx,bkbmstate*hh,drawsrcw+1,hh+2);
    tmp.bgimage = &bm;

    WDL_VirtualWnd_ScaledBlitBG(drawbm,&tmp,
                                  drawx,drawy,draww,drawh,
                                  cliprect->left,cliprect->top,cliprect->right-cliprect->left,cliprect->bottom-cliprect->top,
                                  alpha,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
  }
  else
  {
    LICE_ScaledBlit(drawbm,bkbm->bgimage,
      drawx,drawy,draww,drawh,
      (float)drawsrcx,bkbmstate*(float)hh,
      (float)drawsrcw,(float)hh,alpha,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
  }
}

int WDL_VirtualListBox::ScrollbarGetInfo(int *start, int *size, int num_items, const layout_info &layout)
{
  if (!layout.vscrollbar_w)
  {
    if (layout.hscrollbar_h)
    {
      const int total = wdl_max(num_items,1);
      const int cols = wdl_max(layout.columns,1);
      int vis = (layout.heights->GetSize() + cols - 1);
      vis -= (vis % cols);
      if (vis < cols) vis=cols;

      int startp = (layout.startpos * layout.item_area_w) / total;
      int endp = ((layout.startpos + vis)  * layout.item_area_w) / total;
      *start = startp;
      *size = endp-startp;

      if (*size<2) { if (*start>0) (*start)--; *size=2; }
      if (*start + *size > layout.item_area_w) *start = layout.item_area_w - *size;
      if (*start < 0) *start=0;

      return 2;
    }
    return 0;
  }

  const int num_cols = wdl_max(layout.columns,1);
  const int total_rows = wdl_max((num_items+num_cols-1) / num_cols,1);
  const int rh_base = wdl_max(GetRowHeight(),1);
  const int vis_rows =
    layout.columns > 1 ?  wdl_clamp(layout.item_area_h / rh_base,1,total_rows) :
      wdl_clamp(layout.heights->GetSize(),1,total_rows);

  const int srow = layout.startpos/num_cols;
  int startp = (srow * layout.item_area_h) / total_rows;
  int endp = ((srow + vis_rows)  * layout.item_area_h) / total_rows;
  *start = startp;
  *size = endp-startp;
  if (*size<2) { if (*start>0) (*start)--; *size=2; }
  if (*start + *size > layout.item_area_h) *start = layout.item_area_h - *size;
  if (*start < 0) *start=0;

  return 1;
}

void WDL_VirtualListBox::OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect, int rscale)
{
  RECT r;
  WDL_VWnd::GetPositionPaintExtent(&r,rscale);
  r.left+=origin_x;
  r.right+=origin_x;
  r.top+=origin_y;
  r.bottom+=origin_y;

  WDL_VirtualWnd_BGCfg *mainbk=0;
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,&mainbk) : 0;
  LICE_pixel bgc=GSC(COLOR_BTNFACE);
  bgc=LICE_RGBA_FROMNATIVE(bgc,255);

  layout_info layout;
  CalcLayout(num_items,&layout);
  m_viewoffs = layout.startpos; // latch to new startpos
  const int num_cols = layout.columns;
  const bool do_wordwise = AreItemsWordWise(layout);

  const int usedw = layout.item_area_w * rscale / WDL_VWND_SCALEBASE;
  const int vscrollsize = layout.vscrollbar_w * rscale / WDL_VWND_SCALEBASE;
  const int endpos=layout.item_area_h * rscale / WDL_VWND_SCALEBASE;
  const int startpos = m_viewoffs;
  const int rh_base = GetRowHeight();

  if (r.right > r.left + usedw+vscrollsize) r.right=r.left+usedw+vscrollsize;

  if (mainbk && mainbk->bgimage)
  {
    if (mainbk->bgimage->getWidth()>1 && mainbk->bgimage->getHeight()>1)
    {
      WDL_VirtualWnd_ScaledBlitBG(drawbm,mainbk,
                                    r.left,r.top,r.right-r.left,r.bottom-r.top,
                                    cliprect->left,cliprect->top,cliprect->right-cliprect->left,cliprect->bottom-cliprect->top,
                                    1.0,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
    }
  }
  else
  {
    LICE_FillRect(drawbm,r.left,r.top,r.right-r.left,r.bottom-r.top,bgc,1.0f,LICE_BLIT_MODE_COPY);
  }

  LICE_pixel pencol = GSC(COLOR_3DSHADOW);
  LICE_pixel pencol2 = GSC(COLOR_3DHILIGHT);
  pencol=LICE_RGBA_FROMNATIVE(pencol,255);
  pencol2=LICE_RGBA_FROMNATIVE(pencol2,255);

  LICE_pixel tcol=GSC(COLOR_BTNTEXT);
  if (m_font) 
  {
    m_font->SetBkMode(TRANSPARENT);
    if (m_lsadj != -1000)
      m_font->SetLineSpacingAdjust(m_lsadj);
  }

  float alpha = (m_grayed ? 0.25f : 1.0f);

  int itempos=startpos;
  
  for (int colpos = 0; colpos < (do_wordwise ? 1 : num_cols); colpos ++)
  {
    int col_x = r.left + ((usedw+m_colgap)*colpos) / num_cols;
    int col_w = r.left + ((usedw+m_colgap)*(colpos+1)) / num_cols - col_x - m_colgap;
    int y=r.top, ly = y;
    int cstate=0;
    for (;;)
    {
      const int idx = itempos-startpos;
      int rh,flag;
      if (idx >= 0 && idx < layout.heights->GetSize())
        rh = layout.GetHeight(idx,&flag);
      else
        rh = GetItemHeight(itempos,&flag);
      rh = rh * rscale / WDL_VWND_SCALEBASE;

      if (!do_wordwise)
      {
        ly=y;
        y += rh;
        if (y > r.top+endpos) 
        {
          if (y+ ((flag&ITEMH_FLAG_NOSQUISH) ? 0 : -rh + rh_base) > r.top+endpos) break;
          if (colpos < num_cols-1) break;
          y = r.top+endpos; // size expanded-sized item to fit
        }
      }
      else
      {
        col_x = r.left + ((usedw+m_colgap)*cstate) / num_cols;
        col_w = r.left + ((usedw+m_colgap)*(cstate+1)) / num_cols - col_x - m_colgap;
        if (!cstate)
        {
          ly = y;
          y += rh;
          if (y > r.top+endpos)  break;
        }
        if (++cstate == num_cols) cstate=0;
      }

      WDL_VirtualWnd_BGCfg *bkbm=0;
      if (m_GetItemInfo && ly >= r.top)
      {
        char buf[64];
        buf[0]=0;
        int color=tcol;

        if (m_GetItemInfo(this,itempos++,buf,sizeof(buf),&color,&bkbm))
        {
          color=LICE_RGBA_FROMNATIVE(color,0);
          RECT thisr;
          thisr.left = col_x;
          thisr.right = col_x + col_w;
          thisr.top = ly+1;
          thisr.bottom = y-1;
          int rev=0;
          int bkbmstate=0;
          if (m_cap_state==1 && m_cap_startitem==itempos-1)
          {
            if (bkbm) bkbmstate=1;
            else color = ((color>>1)&0x7f7f7f7f)+LICE_RGBA(0x7f,0x7f,0x7f,0);
          }
          if (m_cap_state>=0x1000 && m_cap_startitem==itempos-1)
          {
            if (bkbm) bkbmstate=2;
            else
            {
              rev=1;
              LICE_FillRect(drawbm,thisr.left,thisr.top,thisr.right-thisr.left,thisr.bottom-thisr.top, color,alpha,LICE_BLIT_MODE_COPY);
            }
          }
          if (bkbm && bkbm->bgimage) //draw image!
          {            
            DrawBkImage(drawbm,bkbm,
                thisr.left,thisr.top-1,thisr.right-thisr.left,thisr.bottom-thisr.top+2,
                cliprect,
                0,bkbm->bgimage->getWidth(),bkbmstate,alpha);

          }
          if (m_CustomDraw)
          {
            m_CustomDraw(this,itempos-1,&thisr,drawbm,rscale);
          }

          if (buf[0])
          {
            thisr.left+=m_margin_l;
            thisr.right-=m_margin_r;
            if (m_font)
            {
              m_font->SetTextColor(rev?bgc:color);
              m_font->SetCombineMode(LICE_BLIT_MODE_COPY, alpha); // maybe gray text only if !bkbm->bgimage
              if (m_align == 0)
              {
                RECT r2={0,};
                m_font->DrawText(drawbm,buf,-1,&r2,DT_CALCRECT|DT_NOPREFIX);
                m_font->DrawText(drawbm,buf,-1,&thisr,DT_VCENTER|((r2.right <= thisr.right-thisr.left) ? DT_CENTER : DT_LEFT)|DT_NOPREFIX);
              }
              else
                m_font->DrawText(drawbm,buf,-1,&thisr,DT_VCENTER|(m_align<0?DT_LEFT:DT_RIGHT)|DT_NOPREFIX);
            }
          }
        }
      }

      if (!bkbm)
      {
        LICE_Line(drawbm,col_x,y,col_x+col_w,y,pencol2,1.0f,LICE_BLIT_MODE_COPY,false);
      }
    }
  }
  if (num_cols>0)
  {
    int offs, height;

    int stype = ScrollbarGetInfo(&offs,&height,num_items,layout);
    if (stype)
    {
      if (rscale != 256)
      {
        offs = offs*rscale / WDL_VWND_SCALEBASE;
        height = height*rscale / WDL_VWND_SCALEBASE;
      }

      int w = stype == 1 ? vscrollsize : (layout.hscrollbar_h * rscale / WDL_VWND_SCALEBASE);
      int drawposx = stype == 1 ? (r.left + usedw) : (r.top + endpos);
      if (m_scrollbar_expanded)
      {
        drawposx -= EXPAND_SCROLLBAR_SIZE(w);
        w += EXPAND_SCROLLBAR_SIZE(w);
      }
      else
      {
        int border = m_scrollbar_border * rscale / WDL_VWND_SCALEBASE;
        if (border>0 && border < w)
        {
          w-=border;
          drawposx+=border;
        }
      }

      const LICE_pixel col = m_scrollbar_color;
      const int mode = m_scrollbar_blendmode;
      const float alpha = m_scrollbar_alpha;
      if (stype == 1)
      {
        LICE_FillRect(drawbm,drawposx,r.top,w,endpos, col,alpha*.5f,mode);
        LICE_FillRect(drawbm,drawposx,r.top + offs,w,height, col,alpha,mode);
      }
      else
      {
        if (drawposx+w > r.bottom) w = r.bottom - drawposx;
        LICE_FillRect(drawbm,r.left,drawposx,usedw,w, col,alpha*.5f,mode);
        LICE_FillRect(drawbm,r.left+offs,drawposx,height,w, col,alpha,mode);
      }
    }
  }

  if (!mainbk)
  {
    LICE_Line(drawbm,r.left,r.bottom-1,r.left,r.top,pencol,1.0f,0,false);
    LICE_Line(drawbm,r.left,r.top,r.right-1,r.top,pencol,1.0f,0,false);
    LICE_Line(drawbm,r.right-1,r.top,r.right-1,r.bottom-1,pencol2,1.0f,0,false);
    LICE_Line(drawbm,r.right-1,r.bottom-1,r.left,r.bottom-1,pencol2,1.0f,0,false);
  }


}
void WDL_VirtualListBox::DoScroll(int dir, const layout_info *layout)
{
  if (dir < 0 && layout->columns>1)
  {
    int y=0;
    if (AreItemsWordWise(*layout))
    {
      int np = wdl_max(0,m_viewoffs-layout->columns);
      if (m_viewoffs != np) { m_viewoffs = np; y=1; }
    }
    else while (m_viewoffs>0)
    {
      y += GetItemHeight(--m_viewoffs);
      if (y >= layout->item_area_h) break;
    }
    if (y) RequestRedraw(NULL);
  }
  else if (dir > 0 && layout->columns>1)
  {
    int y=0;
    if (AreItemsWordWise(*layout))
    {
      m_viewoffs+=layout->columns;
      y++;
    }
    else for (int i = 0;  i < layout->heights->GetSize(); i ++)
    {
      m_viewoffs++;
      y += layout->GetHeight(i);
      if (y >= layout->item_area_h) break;
    }
    if (y)
      RequestRedraw(NULL);
  }
  else if (dir > 0)
  {
    m_viewoffs++; // let painting sort out total visibility (we could easily calculate but meh)
    RequestRedraw(NULL);
  }
  else if (dir < 0)
  {
    if (m_viewoffs>0)
    {
      m_viewoffs--;
      RequestRedraw(NULL);
    }
  }
}

int WDL_VirtualListBox::OnMouseDown(int xpos, int ypos)
{
  if (m_grayed) return 0;
  m_cap_startpos.x = xpos;
  m_cap_startpos.y = ypos;

  if (m__iaccess) m__iaccess->OnFocused();
  const int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  layout_info layout;
  CalcLayout(num_items,&layout);

  if (ScrollbarHit(xpos,ypos,layout))
  {
    int offs,sz;
    switch (ScrollbarGetInfo(&offs,&sz,num_items,layout))
    {
      case 1:
        if (ypos < offs || ypos > offs+sz)
        {
          m_viewoffs = (num_items * (ypos - sz/2)) / wdl_max(1,layout.item_area_h);
          if (m_viewoffs<0) m_viewoffs=0;
          if (m_viewoffs>num_items) m_viewoffs=num_items;
          m_cap_startpos.y = sz/2;
        }
        else
          m_cap_startpos.y = ypos - offs;

        m_scrollbar_expanded = true;
        m_cap_state=2;
        RequestRedraw(NULL);
      return 1;
      case 2:
        if (xpos < offs || xpos > offs+sz)
        {
          m_viewoffs = (num_items * (xpos - sz/2)) / wdl_max(1,layout.item_area_w);
          if (m_viewoffs<0) m_viewoffs=0;
          if (m_viewoffs>num_items) m_viewoffs=num_items;
          m_cap_startpos.x = sz/2;
        }
        else
          m_cap_startpos.x = xpos - offs;

        m_scrollbar_expanded = true;
        m_cap_state=3;
        RequestRedraw(NULL);
      return 1;
    }
  }

  int idx = IndexFromPtInt(xpos,ypos,layout);
  if (idx < 0)
    return 0;

  m_cap_state=0x1000;
  m_cap_startitem=idx;
  RequestRedraw(NULL);
  return 1;
}


bool WDL_VirtualListBox::OnMouseDblClick(int xpos, int ypos)
{
  if (m_grayed) return false;

  const int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  layout_info layout;
  CalcLayout(num_items,&layout);

  int idx = IndexFromPtInt(xpos,ypos,layout);
  if (idx<0)
  {
    if (idx == -2) return false;
    idx = num_items;
  }
  
  RequestRedraw(NULL);
  if (m_clickmsg) SendCommand(m_clickmsg,(INT_PTR)this,idx,this);
  return false;
}

bool WDL_VirtualListBox::OnMouseWheel(int xpos, int ypos, int amt)
{
  if (m_grayed) return false;

  const int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  layout_info layout;
  CalcLayout(num_items,&layout);

  if (xpos >= layout.item_area_w + layout.vscrollbar_w) return false;

  DoScroll(-amt,&layout);
  return true;
}

void WDL_VirtualListBox::OnMouseMove(int xpos, int ypos)
{
  if (m_grayed) return;

  if (m_cap_state>=0x1000)
  {
    m_cap_state++;
    if (m_cap_state < 0x1008)
    {
      int dx = (xpos - m_cap_startpos.x), dy=(ypos-m_cap_startpos.y);
      if (dx*dx + dy*dy > 36) 
        m_cap_state=0x1008;
    }
    if (m_cap_state>=0x1008)
    {
      if (m_dragmsg)
      {
        SendCommand(m_dragmsg,(INT_PTR)this,m_cap_startitem,this);
      }
    }
  }
  else if (m_cap_state >= 0 && m_cap_state <= 3)
  {
    const int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
    layout_info layout;
    CalcLayout(num_items,&layout);
    if (m_cap_state==0)
    {
      int a=IndexFromPtInt(xpos,ypos,layout);
      if (a>=0)
      {
        m_cap_startitem=a;
        m_cap_state=1;
        RequestRedraw(NULL);
      }
    }
    else if (m_cap_state==1)
    {
      int a=IndexFromPtInt(xpos,ypos,layout);
      if (a>=0 && a != m_cap_startitem)
      {
        m_cap_startitem=a;
        m_cap_state=1;
        RequestRedraw(NULL);
      }
      else if (a<0)
      {
        m_cap_state=0;
        RequestRedraw(NULL);
      }
    }
    else if (m_cap_state==2)
    {
      int vwoffs = (num_items * (ypos - m_cap_startpos.y)) / wdl_max(1,layout.item_area_h);
      if (vwoffs<0) vwoffs=0;
      if (vwoffs>num_items) vwoffs=num_items;
      if (vwoffs != m_viewoffs)
      {
        m_viewoffs = vwoffs;
        RequestRedraw(NULL);
      }
    }
    else if (m_cap_state==3)
    {
      int vwoffs = (num_items * (xpos - m_cap_startpos.x)) / wdl_max(1,layout.item_area_w);
      if (vwoffs<0) vwoffs=0;
      if (vwoffs>num_items) vwoffs=num_items;
      if (vwoffs != m_viewoffs)
      {
        m_viewoffs = vwoffs;
        RequestRedraw(NULL);
      }
    }
    bool exp = (m_cap_state>=2 || ScrollbarHit(xpos,ypos,layout));
    if (exp != m_scrollbar_expanded)
    {
      m_scrollbar_expanded = exp;
      RequestRedraw(NULL);
    }
  }
}

bool WDL_VirtualListBox::ScrollbarHit(int xpos, int ypos, const layout_info &layout)
{
  if (layout.vscrollbar_w>0)
  {
    int extra = m_scrollbar_expanded ? EXPAND_SCROLLBAR_SIZE(layout.vscrollbar_w) : 0;
    return ypos>=0 && ypos < layout.item_area_h &&
           xpos >= layout.item_area_w - extra && xpos < layout.item_area_w + layout.vscrollbar_w;
  }
  else if (layout.hscrollbar_h>0)
  {
    int extra = m_scrollbar_expanded ? EXPAND_SCROLLBAR_SIZE(layout.hscrollbar_h) : 0;
    return xpos>=0 && xpos < layout.item_area_w &&
           ypos >= layout.item_area_h - extra && ypos < layout.item_area_h + layout.hscrollbar_h;
  }
  return false;
}

void WDL_VirtualListBox::OnMouseUp(int xpos, int ypos)
{
  if (m_grayed) return;

  const int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  layout_info layout;
  CalcLayout(num_items,&layout);

  int cmd=0;
  INT_PTR p1, p2;
  int hit=IndexFromPtInt(xpos,ypos,layout);
  if (m_cap_state>=0x1000 && m_cap_state<0x1008 && hit==m_cap_startitem) 
  {
    if (m_clickmsg)
    {
      cmd=m_clickmsg;
      p1=(INT_PTR)this;
      p2=hit;
    }
  }
  else if (m_cap_state>=0x1008)
  {
    // send a message saying drag & drop occurred
    if (m_dropmsg)
    {
      cmd=m_dropmsg;
      p1=(INT_PTR)this;
      p2=m_cap_startitem;
    }
  }
  else if (m_cap_state == 2 || m_cap_state == 3)
  {
    m_scrollbar_expanded = ScrollbarHit(xpos,ypos,layout);
  }

  m_cap_state=0;
  RequestRedraw(NULL);
  if (cmd) SendCommand(cmd,p1,p2,this);
}

bool WDL_VirtualListBox::GetItemRect(int item, RECT *r)
{
  const int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  layout_info layout;
  CalcLayout(num_items,&layout);

  item -= layout.startpos;
  if (item < 0) { if (r) memset(r,0,sizeof(RECT)); return false; }
  
  if (r)
  {
    const bool do_wordwise = AreItemsWordWise(layout);
    const int rh_base = GetRowHeight();
    int col = 0,y=0;
    for (int x=0;x<item;x++)
    {
      if (do_wordwise)
      {
        if (++col == layout.columns)
        {
          y += rh_base;
          col = 0;
        }
      }
      else
      {
        int flag = 0;
        const int rh = x < layout.heights->GetSize() ? layout.GetHeight(x,&flag) : rh_base;
        if (y > 0 && y + rh > layout.item_area_h && (col < layout.columns-1 || (flag&ITEMH_FLAG_NOSQUISH) || y+rh_base > layout.item_area_h)) { col++; y = 0; }
        y += rh;
      }
    }

    int flag = 0;
    const int rh = item < layout.heights->GetSize() ? layout.GetHeight(item,&flag) : rh_base;

    if (!do_wordwise)
    {
      if (y > 0 && y + rh > layout.item_area_h && (col < layout.columns-1 || (flag&ITEMH_FLAG_NOSQUISH) || y+rh_base > layout.item_area_h)) { col++; y = 0; }
    }
    if (col >= layout.columns)  { if (r) memset(r,0,sizeof(RECT)); return false; }

    r->top = y;
    r->bottom = y+rh;
    if (r->bottom > layout.item_area_h) r->bottom = layout.item_area_h;
    r->left = (col * layout.item_area_w) / layout.columns;
    r->right = ((col+1) * layout.item_area_w) / layout.columns;
    if (col == layout.columns-1 && layout.vscrollbar_w && m_scrollbar_expanded)
    {
      r->right -= EXPAND_SCROLLBAR_SIZE(layout.vscrollbar_w); // exclude the expanded-scrollbar area from the rect
    }
  }
  return true;
}

int WDL_VirtualListBox::IndexFromPt(int x, int y)
{
  const int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  layout_info layout;
  CalcLayout(num_items,&layout);
  return IndexFromPtInt(x,y,layout);
}

int WDL_VirtualListBox::IndexFromPtInt(int x, int y, const layout_info &layout)
{
  if (x >= layout.item_area_w - (m_scrollbar_expanded ? EXPAND_SCROLLBAR_SIZE(layout.vscrollbar_w) : 0)) return -2;

  if (y < 0 || y >= layout.item_area_h || x < 0)
  {
    return -1;
  }

  // step through visible items
  const int usewid=layout.item_area_w;
  int xpos = 0;
  int col = 0;

  const int do_wordwise = AreItemsWordWise(layout);
  const int rh_base = GetRowHeight();
  int idx = 0;

  if (do_wordwise)
  {
    int ypos = 0;
    for (;;)
    {
      const int nx = (++col * usewid) / layout.columns;
      int flag = 0;
      const int rh = idx < layout.heights->GetSize() ? layout.GetHeight(idx,&flag) : rh_base;
      if (x < nx && y >= ypos && y < ypos+rh) return layout.startpos + idx;
      if (col == layout.columns)
      {
        col = 0;
        ypos += rh;
      }
      idx++;
    }
  }
  else for (;;)
  {
    if (x < xpos) return -1;
    int ypos = 0;
    const int nx = (++col * usewid) / layout.columns;
    for (;;)
    {
      int flag = 0;
      const int rh = idx < layout.heights->GetSize() ? layout.GetHeight(idx,&flag) : rh_base;
      if (ypos > 0 && ypos + rh > layout.item_area_h && (col < layout.columns-1 || (flag & ITEMH_FLAG_NOSQUISH) || ypos+rh_base > layout.item_area_h)) break;
      if (x < nx && y >= ypos && y < ypos+rh) return layout.startpos + idx;
      ypos += rh;
      idx++;
    }
    if (x < nx && y >= ypos) return -1; // empty space at bottom of column
    xpos = nx;
  }
}

void WDL_VirtualListBox::SetViewOffset(int offs)
{
  m_viewoffs = wdl_max(offs,0);
  RequestRedraw(0);
}

int WDL_VirtualListBox::GetViewOffset()
{
  return m_viewoffs;
}

int WDL_VirtualListBox::GetItemHeight(int idx, int *flag)
{
  const int a = m_GetItemHeight ? m_GetItemHeight(this,idx) : -1;
  if (flag) *flag = a<0 ? 0 : (a&~ITEMH_MASK);
  return a>=0 ? (a&ITEMH_MASK) : GetRowHeight();
}

RECT *WDL_VirtualListBox::GetScrollButtonRect(bool isDown)
{
  // used for accessibility!
  static RECT ret;

  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  layout_info layout;
  CalcLayout(num_items,&layout);

  int start,sz;
  int sb = ScrollbarGetInfo(&start,&sz,num_items,layout);

  if (sb == 1 && layout.vscrollbar_w>0)
  {
    if (isDown && start+sz >= layout.item_area_h) return NULL;
    if (!isDown && start <= 0) return NULL;

    ret.left = layout.item_area_w;
    ret.right = ret.left + layout.vscrollbar_w;
    ret.top = isDown ? start+sz : start - layout.vscrollbar_w;
    ret.bottom = ret.top + layout.vscrollbar_w;
    return &ret;
  }
  if (sb ==2 && layout.hscrollbar_h>0)
  {
    if (isDown && start+sz >= layout.item_area_w) return NULL;
    if (!isDown && start <= 0) return NULL;
    ret.top = layout.item_area_h;
    ret.bottom = ret.top + layout.hscrollbar_h;
    ret.left = isDown ? start+sz : start - layout.hscrollbar_h;
    ret.right = ret.left + layout.hscrollbar_h;
    return &ret;
  }

  return NULL;
}
