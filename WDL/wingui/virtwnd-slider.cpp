/*
    WDL - virtwnd-slider.cpp
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
      

    Implementation for virtual window sliders.

*/

#include "virtwnd-controls.h"
#include "../lice/lice.h"

#ifdef _WIN32
static void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, short xStart,
                           short yStart, COLORREF cTransparentColor)
   {
   BITMAP     bm;
   COLORREF   cColor;
   HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
   HGDIOBJ    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
   HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
   POINT      ptSize;

   hdcTemp = CreateCompatibleDC(hdc);
   SelectObject(hdcTemp, hBitmap);   // Select the bitmap

   GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
   ptSize.x = bm.bmWidth;            // Get width of bitmap
   ptSize.y = bm.bmHeight;           // Get height of bitmap
   DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device

                                     // to logical points

   // Create some DCs to hold temporary data.
   hdcBack   = CreateCompatibleDC(hdc);
   hdcObject = CreateCompatibleDC(hdc);
   hdcMem    = CreateCompatibleDC(hdc);
   hdcSave   = CreateCompatibleDC(hdc);

   // Create a bitmap for each DC. DCs are required for a number of
   // GDI functions.

   // Monochrome DC
   bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

   // Monochrome DC
   bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

   bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
   bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

   // Each DC must select a bitmap object to store pixel data.
   bmBackOld   = SelectObject(hdcBack, bmAndBack);
   bmObjectOld = SelectObject(hdcObject, bmAndObject);
   bmMemOld    = SelectObject(hdcMem, bmAndMem);
   bmSaveOld   = SelectObject(hdcSave, bmSave);

   // Set proper mapping mode.
   SetMapMode(hdcTemp, GetMapMode(hdc));

   // Save the bitmap sent here, because it will be overwritten.
   BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

   // Set the background color of the source DC to the color.
   // contained in the parts of the bitmap that should be transparent
   cColor = SetBkColor(hdcTemp, cTransparentColor);

   // Create the object mask for the bitmap by performing a BitBlt
   // from the source bitmap to a monochrome bitmap.
   BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0,
          SRCCOPY);

   // Set the background color of the source DC back to the original
   // color.
   SetBkColor(hdcTemp, cColor);

   // Create the inverse of the object mask.
   BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0,
          NOTSRCCOPY);

   // Copy the background of the main DC to the destination.
   BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart,
          SRCCOPY);

   // Mask out the places where the bitmap will be placed.
   BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);

   // Mask out the transparent colored pixels on the bitmap.
   BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

   // XOR the bitmap with the background on the destination DC.
   BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);

   // Copy the destination to the screen.
   BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0,
          SRCCOPY);

   // Place the original bitmap back into the bitmap sent here.
   BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

   // Delete the memory bitmaps.
   DeleteObject(SelectObject(hdcBack, bmBackOld));
   DeleteObject(SelectObject(hdcObject, bmObjectOld));
   DeleteObject(SelectObject(hdcMem, bmMemOld));
   DeleteObject(SelectObject(hdcSave, bmSaveOld));

   // Delete the memory DCs.
   DeleteDC(hdcMem);
   DeleteDC(hdcBack);
   DeleteDC(hdcObject);
   DeleteDC(hdcSave);
   DeleteDC(hdcTemp);
   }

#endif

WDL_VirtualSlider::WDL_VirtualSlider()
{
  m_tl_extra=m_br_extra=0;
  m_skininfo=0;
  m_bgcol1_msg=0;
  m_minr=0;
  m_maxr=1000;
  m_needflush=0;
  m_pos=m_center=500;
  m_captured=false;
}

WDL_VirtualSlider::~WDL_VirtualSlider()
{
}

bool WDL_VirtualSlider::GetIsVert()
{
  return m_position.right-m_position.left < m_position.bottom-m_position.top;
}

static void AdjustThumbImageSize(WDL_VirtualSlider_SkinConfig *a, bool vert, int *bmw, int *bmh, int *startoffs=NULL)
{
  if (!a) return;
  int ret=a->thumbimage_rb[vert] - a->thumbimage_lt[vert];
  if (ret<1) return;
  if (startoffs) *startoffs = a->thumbimage_lt[vert];
  if (ret>0)
  {
    if (vert)
    {
      if (*bmh > ret) (*bmw)--;
      *bmh=ret;
    }
    else 
    {
      if (*bmw > ret) (*bmh)--;
      *bmw=ret;
    }
  }
}

void WDL_VirtualSlider_PreprocessSkinConfig(WDL_VirtualSlider_SkinConfig *a)
{
  if (a&&a->thumbimage[0])
  {
    int w=a->thumbimage[0]->getWidth();
    int h=a->thumbimage[0]->getHeight();
    int x;
    for (x = 0; x < w && LICE_GetPixel(a->thumbimage[0],x,h-1)==LICE_RGBA(255,0,255,255); x ++);
    a->thumbimage_lt[0] = x;
    for (x = w-1; x > a->thumbimage_lt[0]+1 && LICE_GetPixel(a->thumbimage[0],x,h-1)==LICE_RGBA(255,0,255,255); x --);
    a->thumbimage_rb[0] = x;
  }
  if (a&&a->thumbimage[1])
  {
    int w=a->thumbimage[1]->getWidth();
    int h=a->thumbimage[1]->getHeight();
    int y;
    for (y = 0; y < h-4 && LICE_GetPixel(a->thumbimage[1],w-1,y)==LICE_RGBA(255,0,255,255); y++);
    a->thumbimage_lt[1] = y;
    for (y = h-1; y > a->thumbimage_lt[1]+1 && LICE_GetPixel(a->thumbimage[1],w-1,y)==LICE_RGBA(255,0,255,255); y --);
    a->thumbimage_rb[1] = y;
  }
  WDL_VirtualWnd_PreprocessBGConfig(&a->bgimagecfg[0]);
  WDL_VirtualWnd_PreprocessBGConfig(&a->bgimagecfg[1]);
}

void WDL_VirtualSlider::OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
  origin_x += m_position.left; // convert drawing origin to local coords
  origin_y += m_position.top;

  HDC hdc=drawbm->getDC();
  bool isVert = GetIsVert();

  int rsize=m_maxr-m_minr;
  if (rsize<1)rsize=1;

  int viewh=m_position.bottom-m_position.top;
  int vieww=m_position.right-m_position.left;

  WDL_VirtualWnd_BGCfg *back_image=m_skininfo && m_skininfo->bgimagecfg[isVert].bgimage ? &m_skininfo->bgimagecfg[isVert] : 0;
  LICE_IBitmap *bm_image=m_skininfo ? m_skininfo->thumbimage[isVert] : 0;
  int bm_w=16,bm_h=16,bm_w2,bm_h2;
  int imgoffset=0;
  HBITMAP bm=0;
  if (!bm_image) 
  {
    bm=WDL_STYLE_GetSliderBitmap(isVert,&bm_w,&bm_h);
    bm_w2=bm_w;
    bm_h2=bm_h;
  }
  else
  {
    bm_w=bm_image->getWidth();
    bm_h=bm_image->getHeight();

    bm_w2=bm_w;
    bm_h2=bm_h;
    AdjustThumbImageSize(m_skininfo,isVert,&bm_w2,&bm_h2,&imgoffset);
  }

  if (isVert)
  {
    int pos = ((m_maxr-m_pos)*(viewh-bm_h2))/rsize; //viewh - bm_h2 - ((m_pos-m_minr) * (viewh - bm_h2))/rsize;

    if (back_image)
    {
      WDL_VirtualWnd_ScaledBlitBG(drawbm,back_image,
        origin_x,origin_y,vieww,viewh,
        origin_x,origin_y,vieww,viewh,
        1.0,LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR|LICE_BLIT_USE_ALPHA);
                                            
      if (m_bgcol1_msg)
      {
        int brcol=-100;
        SendCommand(m_bgcol1_msg,(INT_PTR)&brcol,GetID(),this);
        if (brcol != -100)
        {
          LICE_MemBitmap tmpbm;
          tmpbm.resize(vieww,viewh);

          WDL_VirtualWnd_ScaledBlitBG(&tmpbm,back_image,0,0,vieww,viewh,
              0,0,vieww,viewh,1.0,LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);

          LICE_ClearRect(&tmpbm,0,0,vieww,viewh,LICE_RGBA(0,0,0,255),LICE_RGBA(GetRValue(brcol),GetGValue(brcol),GetBValue(brcol),0));

          RECT r={0,0,vieww,viewh};
          LICE_Blit(drawbm,&tmpbm,origin_x,origin_y,&r,0.5,LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);
        }
      }
    }

    if (!back_image || (m_skininfo&&m_skininfo->zeroline_color))
    {
      int center=m_center;
      if (center < 0) center=WDL_STYLE_GetSliderDynamicCenterPos();

      int y=((m_maxr-center)*(viewh-bm_h2))/rsize + ((bm_h-1)/2-imgoffset);

      if (m_skininfo && m_skininfo->zeroline_color)
      {
        LICE_Line(drawbm,origin_x+2,origin_y+y,origin_x+vieww-2,origin_y+y,
          m_skininfo->zeroline_color,
          LICE_GETA(m_skininfo->zeroline_color)/255.0,
          LICE_BLIT_MODE_COPY,false);
      }
      else
      {
        HPEN pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_BTNTEXT));
        HGDIOBJ oldPen=SelectObject(hdc,pen);

        MoveToEx(hdc,origin_x+2,origin_y+y,NULL);
        LineTo(hdc,origin_x+vieww-2,origin_y+y);

        SelectObject(hdc,oldPen);
        DeleteObject(pen);
      }
    }


    if (!back_image)
    {

      HPEN pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_3DHILIGHT));
      HGDIOBJ oldPen=SelectObject(hdc,pen);

      int brcol=WDL_STYLE_GetSysColor(COLOR_3DSHADOW);
      if (m_bgcol1_msg)
        SendCommand(m_bgcol1_msg,(INT_PTR)&brcol,GetID(),this);

      HBRUSH br=CreateSolidBrush(brcol);
      HGDIOBJ oldBr=SelectObject(hdc,br);

      int offs= (vieww - 4)/2;
      // white with black border, mmm
      RoundRect(hdc,origin_x + offs,origin_y + bm_h2/3, origin_x + offs + 5,origin_y + viewh - bm_h2/3,2,2);

      SelectObject(hdc,oldPen);
      SelectObject(hdc,oldBr);
      DeleteObject(pen);
      DeleteObject(br);
    }

    if (bm_image)
    {
      int ypos=origin_y+pos-imgoffset;
      int xpos=origin_x;

      RECT r={0,0,bm_w2,bm_h};
/*      if (vieww<bm_w)
      {
        r.left=(bm_w-vieww)/2;
        r.right=r.left+vieww;
      }
      else 
      */
      xpos+=(vieww-bm_w2)/2;

      m_tl_extra=origin_y-ypos;
      if (m_tl_extra<0)m_tl_extra=0;

      m_br_extra=ypos+(r.bottom-r.top) - (origin_y+m_position.bottom-m_position.top);
      if (m_br_extra<0)m_br_extra=0;


      LICE_Blit(drawbm,bm_image,xpos,ypos,&r,1.0,LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);    
    }
    else if (bm)
    {
#ifdef _WIN32
      DrawTransparentBitmap(hdc,bm,origin_x+(vieww-bm_w)/2,origin_y+pos,RGB(255,0,255));
#else
      RECT r={origin_x+(vieww-bm_w)/2,origin_y+pos,};
      r.right=r.left+bm_w;
      r.bottom=r.top+bm_h;
      DrawImageInRect(hdc,bm,&r);
#endif
    }
  }
  else
  {
    int pos = ((m_pos-m_minr) * (vieww - bm_w2))/rsize;

    if (back_image)
    {
      WDL_VirtualWnd_ScaledBlitBG(drawbm,back_image,
        origin_x,origin_y,vieww,viewh,
        origin_x,origin_y,vieww,viewh,
        1.0,LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR|LICE_BLIT_USE_ALPHA);
      // blit, tint color too?

      if (m_bgcol1_msg)
      {
        int brcol=-100;
        SendCommand(m_bgcol1_msg,(INT_PTR)&brcol,GetID(),this);
        if (brcol != -100)
        {
          LICE_MemBitmap tmpbm;
          tmpbm.resize(vieww,viewh);

          WDL_VirtualWnd_ScaledBlitBG(&tmpbm,back_image,0,0,vieww,viewh,
              0,0,vieww,viewh,1.0,LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);

          LICE_ClearRect(&tmpbm,0,0,vieww,viewh,LICE_RGBA(0,0,0,255),LICE_RGBA(GetRValue(brcol),GetGValue(brcol),GetBValue(brcol),0));

          RECT r={0,0,vieww,viewh};
          LICE_Blit(drawbm,&tmpbm,origin_x,origin_y,&r,0.5,LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);
        }
      }

    }

    if (!back_image || (m_skininfo && m_skininfo->zeroline_color))
    {
      int center=m_center;
      if (center < 0) center=WDL_STYLE_GetSliderDynamicCenterPos();

      int x=((center-m_minr)*(vieww-bm_w2))/rsize + bm_w/2 - imgoffset;
      if (m_skininfo && m_skininfo->zeroline_color)
      {
        LICE_Line(drawbm,origin_x+x,origin_y+2,origin_x+x,origin_y+viewh-2,
          m_skininfo->zeroline_color,
          LICE_GETA(m_skininfo->zeroline_color)/255.0,
          LICE_BLIT_MODE_COPY,false);
      }
      else
      {
        HPEN pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_BTNTEXT));
        HGDIOBJ oldPen=SelectObject(hdc,pen);

        MoveToEx(hdc,origin_x+x,origin_y+2,NULL);
        LineTo(hdc,origin_x+x,origin_y+viewh-2);

        SelectObject(hdc,oldPen);
        DeleteObject(pen);
      }
    }

    if (!back_image)
    {

      HPEN pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_3DHILIGHT));
      HGDIOBJ oldPen=SelectObject(hdc,pen);
      int brcol=WDL_STYLE_GetSysColor(COLOR_3DSHADOW);
      if (m_bgcol1_msg)
        SendCommand(m_bgcol1_msg,(INT_PTR)&brcol,GetID(),this);

      HBRUSH br=CreateSolidBrush(brcol);
      HGDIOBJ oldBr=SelectObject(hdc,br);

      int offs= (viewh - 4)/2;
      // white with black border, mmm
      RoundRect(hdc,origin_x + bm_w2/3,origin_y + offs, origin_x + vieww - bm_w2/3,origin_y + offs + 5,2,2);

      SelectObject(hdc,oldPen);
      SelectObject(hdc,oldBr);
      DeleteObject(pen);
      DeleteObject(br);
    }

    if (bm_image)
    {
      int xpos=origin_x+pos-imgoffset;
      int ypos=origin_y;

      RECT r={0,0,bm_w,bm_h2};
      /*if (viewh<bm_h)
      {
        r.top=(bm_h-viewh)/2;
        r.bottom=r.top+viewh;
      }
      else 
      */
      ypos+=(viewh-bm_h2)/2;


      m_tl_extra=origin_x-xpos;
      if (m_tl_extra<0)m_tl_extra=0;

      m_br_extra=xpos+(r.right-r.left) - (origin_x+m_position.right-m_position.left);
      if (m_br_extra<0)m_br_extra=0;

      /*      if (xpos < origin_x)
      {
        r.left += (origin_x-xpos);
        xpos=origin_x;
      }
      if (xpos+(r.right-r.left) > origin_x+m_position.right-m_position.left)
        r.right = origin_x+m_position.right-m_position.left - (xpos-r.left);
*/

      LICE_Blit(drawbm,bm_image,xpos,ypos,&r,1.0,LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);    
    }
    else if (bm)
    {
#ifdef _WIN32
      DrawTransparentBitmap(hdc,bm,origin_x+pos,origin_y+(viewh-bm_h)/2+1,RGB(255,0,255));
#else
      RECT r={origin_x+pos,origin_y+(viewh-bm_h)/2+1};
      r.right=r.left+bm_w;
      r.bottom=r.top+bm_h;
      DrawImageInRect(hdc,bm,&r);
#endif
    }
  }

}

static int m_move_offset;
static int m_click_pos,m_last_y,m_last_x, m_last_precmode;


int WDL_VirtualSlider::OnMouseDown(int xpos, int ypos)
{
  m_needflush=0;

  bool isVert = GetIsVert();
  int rsize=m_maxr-m_minr;
  if (rsize<1)rsize=1;

  int viewh=m_position.bottom-m_position.top;
  int vieww=m_position.right-m_position.left;
  if (vieww<1) vieww=1;
  if (viewh<1) viewh=1;

  LICE_IBitmap *bm_image=m_skininfo ? m_skininfo->thumbimage[isVert] : 0;
  int bm_w=16,bm_h=16;
  if (!bm_image) WDL_STYLE_GetSliderBitmap(isVert,&bm_w,&bm_h);
  else
  {
    bm_w=bm_image->getWidth();
    bm_h=bm_image->getHeight();
    AdjustThumbImageSize(m_skininfo,isVert,&bm_w,&bm_h);
  }

  m_last_y=ypos;    
  m_last_x=xpos;
  m_last_precmode=0;

  if (isVert)
  {
    m_move_offset=ypos-( viewh - bm_h - (((m_pos-m_minr) * (viewh-bm_h))/rsize));
    m_click_pos=m_pos;
    if (m_move_offset < 0 || m_move_offset >= bm_h)
    {
      int xcent=xpos - vieww/2;
      bool hit;

      if (m_skininfo && m_skininfo->bgimagecfg[1].bgimage)
      {
        LICE_pixel pix=WDL_VirtualWnd_ScaledBG_GetPix(&m_skininfo->bgimagecfg[1],
          vieww,viewh,xpos,ypos);

        hit = LICE_GETA(pix)>=64;
      }
      else hit= (xcent >= -2 && xcent < 3 && ypos >= bm_h/3 && ypos <= viewh-bm_h/3);

      if (hit)
      {
        m_move_offset=bm_h/2;
        int pos=m_minr+((viewh-bm_h - (ypos-m_move_offset))*rsize)/(viewh-bm_h);
        if (pos < m_minr)pos=m_minr;
        else if (pos > m_maxr)pos=m_maxr;
        m_pos=pos;

        SendCommand(m_scrollmsg?m_scrollmsg:WM_VSCROLL,SB_THUMBTRACK,GetID(),this);
        RequestRedraw(NULL);
      }
      else
        return false;
    }
  }
  else
  {
    m_move_offset=xpos-( (((m_pos-m_minr) * (vieww-bm_w))/rsize));
    m_click_pos=m_pos;
    if (m_move_offset < 0 || m_move_offset >= bm_w)
    {
      int ycent=ypos - viewh/2;

      bool hit;
      if (m_skininfo && m_skininfo->bgimagecfg[0].bgimage)
      {
        LICE_pixel pix=WDL_VirtualWnd_ScaledBG_GetPix(&m_skininfo->bgimagecfg[0],
          vieww,viewh,xpos,ypos);

        hit = LICE_GETA(pix)>=64;
      }
      else hit = (ycent >= -2 && ycent < 3 && xpos >= bm_w/3 && xpos <= vieww-bm_w/3);

      if (hit)
      {
        m_move_offset=bm_w/2;
        int pos=m_minr+((xpos-m_move_offset)*rsize)/(vieww-bm_w);
        if (pos < m_minr)pos=m_minr;
        else if (pos > m_maxr)pos=m_maxr;
        m_pos=pos;

        SendCommand(m_scrollmsg?m_scrollmsg:WM_HSCROLL,SB_THUMBTRACK,GetID(),this);
        RequestRedraw(NULL);
      }
      else
        return false;
    }
  }

  m_captured=true;

  return 1;
}


void WDL_VirtualSlider::OnMoveOrUp(int xpos, int ypos, int isup)
{
  int pos;
  bool isVert = GetIsVert();
  int rsize=m_maxr-m_minr;
  if (rsize<1)rsize=1;

  int viewh=m_position.bottom-m_position.top;
  int vieww=m_position.right-m_position.left;

  LICE_IBitmap *bm_image=m_skininfo ? m_skininfo->thumbimage[isVert] : 0;
  int bm_w=16,bm_h=16;
  if (!bm_image) WDL_STYLE_GetSliderBitmap(isVert,&bm_w,&bm_h);
  else
  {
    bm_w=bm_image->getWidth();
    bm_h=bm_image->getHeight();
    AdjustThumbImageSize(m_skininfo,isVert,&bm_w,&bm_h);
  }

  int precmode=0;

  if (isVert)
  {
#ifndef _WIN32
    if (isup) pos=m_pos;
    else 
#endif
      if ((GetAsyncKeyState(VK_CONTROL)&0x8000) || viewh <= bm_h)
    {
      pos = m_pos- (ypos-m_last_y);
      precmode=1;
    }
    else 
    {
      pos=m_minr+ (((viewh-bm_h - ypos + m_move_offset)*rsize)/(viewh-bm_h));
    }
    if (pos < m_minr)pos=m_minr;
    else if (pos > m_maxr)pos=m_maxr;
    

    if (pos != m_pos || isup)
    {
      if (ypos == m_last_y)
        pos=m_pos;

      if ((GetAsyncKeyState(VK_MENU)&0x8000) && isup)
        pos=m_click_pos;

      m_pos=pos;

      SendCommand(m_scrollmsg?m_scrollmsg:WM_VSCROLL,isup?SB_ENDSCROLL:SB_THUMBTRACK,GetID(),this);

      RequestRedraw(NULL);
    }
  }
  else
  {
#ifndef _WIN32
    if (isup) pos=m_pos;
    else 
#endif
      if ((GetAsyncKeyState(VK_CONTROL)&0x8000) || vieww <= bm_w)
    {
      pos = m_pos+ (xpos-m_last_x);
      precmode=1;
    }
    else 
    {
      pos=m_minr + (((xpos - m_move_offset)*rsize)/(vieww-bm_w));
    }
    if (pos < m_minr)pos=m_minr;
    else if (pos > m_maxr)pos=m_maxr;

    if (pos != m_pos || isup)
    {
      if (xpos == m_last_x)
        pos=m_pos;

      if ((GetAsyncKeyState(VK_MENU)&0x8000) && isup)
        pos=m_click_pos;

      m_pos=pos;

      SendCommand(m_scrollmsg?m_scrollmsg:WM_HSCROLL,isup?SB_ENDSCROLL:SB_THUMBTRACK,GetID(),this);

      RequestRedraw(NULL);
    }
  }
  if (precmode&&GetRealParent())
  {
    if (xpos != m_last_x || ypos != m_last_y)
    {
      POINT p;
      GetCursorPos(&p);
      p.x-=(xpos-m_last_x);
      
    #ifdef _WIN32
      p.y-=(ypos-m_last_y);
      POINT pt={0,0};
      ClientToScreen(GetRealParent(),&pt);
      WDL_VWnd *wnd=this;
      while (wnd)
      {
        RECT r;
        wnd->GetPosition(&r);
        pt.x+=r.left;
        pt.y+=r.top;
        wnd=wnd->GetParent();
      }

      if (isVert) 
      {
        m_last_y=( viewh - bm_h - (((m_pos-m_minr) * (viewh-bm_h))/rsize))+m_move_offset;
        p.y=m_last_y+pt.y;
      }
      else 
      {
        m_last_x=( (((m_pos-m_minr) * (vieww-bm_w))/rsize))+m_move_offset;
        p.x=m_last_x+pt.x;
      }
    #else
      p.y+=(ypos-m_last_y);
    #endif
      SetCursorPos(p.x,p.y);
    }
    if (!m_last_precmode)
    {
      m_last_precmode=1;
      //ShowCursor(FALSE);
      ShowCursor(FALSE);
    }
  }
  else
  {
    m_last_y=ypos;
    m_last_x=xpos;
    if (m_last_precmode)
    {
      m_last_precmode=0;
      //ShowCursor(TRUE);
      ShowCursor(TRUE);
    }
  }
  m_needflush=0;
}

void WDL_VirtualSlider::OnMouseMove(int xpos, int ypos)
{
  if (m_captured) OnMoveOrUp(xpos,ypos,0);
  else if (m_needflush && (xpos <0  || xpos > m_position.right- m_position.left || ypos < 0|| ypos > m_position.bottom-m_position.top ))
  {
    bool isVert = GetIsVert();
    m_needflush=0;
    SendCommand(m_scrollmsg?m_scrollmsg:(isVert?WM_VSCROLL:WM_HSCROLL),SB_ENDSCROLL,GetID(),this);
  }
}

void WDL_VirtualSlider::OnMouseUp(int xpos, int ypos)
{
  if (m_captured) 
  {
    OnMoveOrUp(xpos,ypos,1);
    if (m_last_precmode)
    {
      m_last_precmode=0;
      ShowCursor(TRUE);
    }
  }
  m_captured=false;
}

bool WDL_VirtualSlider::OnMouseDblClick(int xpos, int ypos)
{
  bool isVert = GetIsVert();
  int pos=m_center;
  if (pos < 0) pos=WDL_STYLE_GetSliderDynamicCenterPos();
  m_pos=pos;
  SendCommand(m_scrollmsg?m_scrollmsg:(isVert?WM_VSCROLL:WM_HSCROLL),SB_ENDSCROLL,GetID(),this);

  RequestRedraw(NULL);
  
  m_captured=false;
  return true;
}

bool WDL_VirtualSlider::OnMouseWheel(int xpos, int ypos, int amt)
{
  if (!WDL_STYLE_AllowSliderMouseWheel()) return false;

  bool isVert = GetIsVert();
	int l=amt;
  if (!(GetAsyncKeyState(VK_CONTROL)&0x8000)) l *= 16;
  l *= (m_maxr-m_minr);
  l/=120000;
  if (!l) { if (amt<0)l=-1; else if (amt>0) l=1; }

  int pos=m_pos+l;
  if (pos < m_minr)pos=m_minr;
  else if (pos > m_maxr)pos=m_maxr;

  m_pos=pos;

  m_needflush=1;
  SendCommand(m_scrollmsg?m_scrollmsg:(isVert?WM_VSCROLL:WM_HSCROLL),SB_THUMBTRACK,GetID(),this);

  RequestRedraw(NULL);

  return true;
}




int WDL_VirtualSlider::GetSliderPosition() 
{
  if (m_pos < m_minr) return m_minr; 
  if (m_pos > m_maxr) return m_maxr; 
  return m_pos; 
}

void WDL_VirtualSlider::SetSliderPosition(int pos) 
{ 
  if (!m_captured)
  {
    if (pos < m_minr) pos=m_minr; 
    else if (pos>m_maxr) pos=m_maxr; 

    if (pos != m_pos)
    {
      m_pos=pos;
      RequestRedraw(NULL); 
    }
  }
}

void WDL_VirtualSlider::GetPositionPaintExtent(RECT *r)
{
  *r=m_position;
  bool isVert=GetIsVert();
  LICE_IBitmap *bm_image=m_skininfo ? m_skininfo->thumbimage[isVert] : 0;
  if (bm_image)
  {
    int bm_w=bm_image->getWidth();
    int bm_h=bm_image->getHeight();
    int s=0;
    int bm_w2=bm_w;
    int bm_h2=bm_h;
    AdjustThumbImageSize(m_skininfo,isVert,&bm_w,&bm_h,&s);
    int rsize=m_maxr-m_minr;
    int viewh=m_position.bottom-m_position.top;
    int vieww=m_position.right-m_position.left;

    if (isVert)
    {
      if (bm_w > vieww)
      {
        r->left-=(bm_w-vieww)/2+1;
        r->right+=(bm_w-vieww)/2+1;
      }

      int tadj=m_tl_extra;
      int badj=m_br_extra;

      int pos = viewh - bm_h - ((m_pos-m_minr) * (viewh - bm_h))/rsize-s;

      if (-pos > tadj) tadj=-pos;
      if (pos+bm_h2 > viewh+badj) badj=pos+bm_h2-viewh;

      //m_tl_extra=m_br_extra=
      r->top-=tadj; //s;
      r->bottom += badj; //(bm_h2-bm_h)-s;
    }
    else
    {
      if (bm_h > viewh)
      {
        r->top-=(bm_h-viewh)/2+1;
        r->bottom+=(bm_h-viewh)/2+1;
      }

      int ladj=m_tl_extra;
      int radj=m_br_extra;

      int pos = ((m_pos-m_minr) * (vieww - bm_w))/rsize - s;

      if (-pos > ladj) ladj=-pos;
      if (pos+bm_w2 > vieww+radj) radj=pos+bm_w2-vieww;

      r->left-=ladj; //s;
      r->right += radj; // (bm_w2-bm_w)-s;
    }

  }
}