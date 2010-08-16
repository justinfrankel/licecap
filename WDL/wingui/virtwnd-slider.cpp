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

#include "virtwnd.h"

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

void WDL_VirtualSlider::OnPaint(HDC hdc, int origin_x, int origin_y, RECT *cliprect)
{
  bool isVert = GetIsVert();

  int rsize=m_maxr-m_minr;
  if (rsize<1)rsize=1;

  int viewh=m_position.bottom-m_position.top;
  int vieww=m_position.right-m_position.left;

  int bm_w=16,bm_h=16;
  HBITMAP bm=WDL_STYLE_GetSliderBitmap(isVert,&bm_w,&bm_h);

  if (isVert)
  {
    int pos = viewh - bm_h - ((m_pos-m_minr) * (viewh - bm_h))/rsize;

    int center=m_center;
    if (center < 0) center=WDL_STYLE_GetSliderDynamicCenterPos();

    int y=((m_maxr-center)*(viewh-bm_h))/rsize + bm_h/2;

    HPEN pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_BTNTEXT));
    HGDIOBJ oldPen=SelectObject(hdc,pen);

    MoveToEx(hdc,origin_x+2,origin_y+y,NULL);
    LineTo(hdc,origin_x+vieww-2,origin_y+y);

    SelectObject(hdc,oldPen);
    DeleteObject(pen);

    pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_3DHILIGHT));
    oldPen=SelectObject(hdc,pen);

    int brcol=WDL_STYLE_GetSysColor(COLOR_3DSHADOW);
    if (m_parent && m_bgcol1_msg)
      m_parent->SendCommand(m_bgcol1_msg,(int)&brcol,GetID(),this);

    HBRUSH br=CreateSolidBrush(brcol);
    HGDIOBJ oldBr=SelectObject(hdc,br);

    int offs= (vieww - 4)/2;
    // white with black border, mmm
    RoundRect(hdc,origin_x + offs,origin_y + bm_h/3, origin_x + offs + 5,origin_y + viewh - bm_h/3,2,2);

    SelectObject(hdc,oldPen);
    SelectObject(hdc,oldBr);
    DeleteObject(pen);
    DeleteObject(br);

#ifdef _WIN32
    if (bm) DrawTransparentBitmap(hdc,bm,origin_x+(vieww-bm_w)/2,origin_y+pos,RGB(255,0,255));
#else
    RECT r={origin_x+(vieww-bm_w)/2,origin_y+pos,};
    r.right=r.left+bm_w;
    r.bottom=r.top+bm_h;
    if (bm) DrawImageInRect(hdc,bm,&r);
    
#endif
  }
  else
  {
    int pos = ((m_pos-m_minr) * (vieww - bm_w))/rsize;

    int center=m_center;
    if (center < 0) center=WDL_STYLE_GetSliderDynamicCenterPos();

    int x=((center-m_minr)*(vieww-bm_w))/rsize + bm_w/2;

    HPEN pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_BTNTEXT));
    HGDIOBJ oldPen=SelectObject(hdc,pen);

    MoveToEx(hdc,origin_x+x,origin_y+2,NULL);
    LineTo(hdc,origin_x+x,origin_y+viewh-2);

    SelectObject(hdc,oldPen);
    DeleteObject(pen);

    pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_3DHILIGHT));
    oldPen=SelectObject(hdc,pen);
    int brcol=WDL_STYLE_GetSysColor(COLOR_3DSHADOW);
    if (m_parent && m_bgcol1_msg)
      m_parent->SendCommand(m_bgcol1_msg,(int)&brcol,GetID(),this);

    HBRUSH br=CreateSolidBrush(brcol);
    HGDIOBJ oldBr=SelectObject(hdc,br);

    int offs= (viewh - 4)/2;
    // white with black border, mmm
    RoundRect(hdc,origin_x + bm_w/3,origin_y + offs, origin_x + vieww - bm_w/3,origin_y + offs + 5,2,2);

    SelectObject(hdc,oldPen);
    SelectObject(hdc,oldBr);
    DeleteObject(pen);
    DeleteObject(br);

#ifdef _WIN32
    if (bm) DrawTransparentBitmap(hdc,bm,origin_x+pos,origin_y+(viewh-bm_h)/2+1,RGB(255,0,255));
#else
    RECT r={origin_x+pos,origin_y+(viewh-bm_h)/2+1};
    r.right=r.left+bm_w;
    r.bottom=r.top+bm_h;
    if (bm) DrawImageInRect(hdc,bm,&r);
#endif
  }

}

static int m_move_offset;
static int m_click_pos,m_last_y,m_last_x, m_last_precmode;


bool WDL_VirtualSlider::OnMouseDown(int xpos, int ypos)
{
  m_needflush=0;

  bool isVert = GetIsVert();
  int rsize=m_maxr-m_minr;
  if (rsize<1)rsize=1;

  int viewh=m_position.bottom-m_position.top;
  int vieww=m_position.right-m_position.left;

  int bm_w=16,bm_h=16;
  HBITMAP bm=WDL_STYLE_GetSliderBitmap(isVert,&bm_w,&bm_h);
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
      if (xcent >= -2 && xcent < 3 && ypos >= bm_h/3 && ypos <= viewh-bm_h/3)
      {
        m_move_offset=bm_h/2;
        int pos=m_minr+((viewh-bm_h - (ypos-m_move_offset))*rsize)/(viewh-bm_h);
        if (pos < m_minr)pos=m_minr;
        else if (pos > m_maxr)pos=m_maxr;
        m_pos=pos;

        if (m_parent) m_parent->SendCommand(m_scrollmsg?m_scrollmsg:WM_VSCROLL,SB_THUMBTRACK,GetID(),this);
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
      if (ycent >= -2 && ycent < 3 && xpos >= bm_w/3 && xpos <= vieww-bm_w/3)
      {
        m_move_offset=bm_w/2;
        int pos=m_minr+((xpos-m_move_offset)*rsize)/(vieww-bm_w);
        if (pos < m_minr)pos=m_minr;
        else if (pos > m_maxr)pos=m_maxr;
        m_pos=pos;

        if (m_parent) m_parent->SendCommand(m_scrollmsg?m_scrollmsg:WM_HSCROLL,SB_THUMBTRACK,GetID(),this);
        RequestRedraw(NULL);
      }
      else
        return false;
    }
  }

  m_captured=true;

  return true;
}


void WDL_VirtualSlider::OnMoveOrUp(int xpos, int ypos, int isup)
{
  int pos;
  bool isVert = GetIsVert();
  int rsize=m_maxr-m_minr;
  if (rsize<1)rsize=1;

  int viewh=m_position.bottom-m_position.top;
  int vieww=m_position.right-m_position.left;

  int bm_w=16,bm_h=16;
  HBITMAP bm=WDL_STYLE_GetSliderBitmap(isVert,&bm_w,&bm_h);

  int precmode=0;

  if (isVert)
  {
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

      if (m_parent) m_parent->SendCommand(m_scrollmsg?m_scrollmsg:WM_VSCROLL,isup?SB_ENDSCROLL:SB_THUMBTRACK,GetID(),this);

      RequestRedraw(NULL);
    }
  }
  else
  {
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

      if (m_parent) m_parent->SendCommand(m_scrollmsg?m_scrollmsg:WM_HSCROLL,isup?SB_ENDSCROLL:SB_THUMBTRACK,GetID(),this);

      RequestRedraw(NULL);
    }
  }
#ifdef _WIN32
  if (precmode&&0)
  {
    POINT p;
    GetCursorPos(&p);
    p.x-=(xpos-m_last_x);
    p.y-=(ypos-m_last_y);
    if (xpos != m_last_x || ypos != m_last_y)
      SetCursorPos(p.x,p.y);
    if (!m_last_precmode)
    {
      m_last_precmode=1;
      ShowCursor(FALSE);
    }
  }
  else
#endif
  {
    m_last_y=ypos;
    m_last_x=xpos;
    if (m_last_precmode)
    {
      m_last_precmode=0;
#ifdef _WIN32
      ShowCursor(TRUE);
#endif
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
    m_parent->SendCommand(m_scrollmsg?m_scrollmsg:(isVert?WM_VSCROLL:WM_HSCROLL),SB_ENDSCROLL,GetID(),this);
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
#ifdef _WIN32
      ShowCursor(TRUE);
#endif
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
  if (m_parent) m_parent->SendCommand(m_scrollmsg?m_scrollmsg:(isVert?WM_VSCROLL:WM_HSCROLL),SB_ENDSCROLL,GetID(),this);

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

  int pos=m_pos+l;
  if (pos < m_minr)pos=m_minr;
  else if (pos > m_maxr)pos=m_maxr;

  m_pos=pos;

  if (m_parent) 
  {
    m_needflush=1;
    m_parent->SendCommand(m_scrollmsg?m_scrollmsg:(isVert?WM_VSCROLL:WM_HSCROLL),SB_THUMBTRACK,GetID(),this);
  }

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
