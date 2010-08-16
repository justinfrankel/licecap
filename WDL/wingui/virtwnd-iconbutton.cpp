/*
    WDL - virtwnd-iconbutton.cpp
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
      

    Implementation for virtual window icon buttons, icons, combo boxes, and static text controls.

*/

#include "virtwnd.h"
#include "../lice/lice.h"

WDL_VirtualIconButton::WDL_VirtualIconButton()
{
  m_pressed=0;
  m_iconPtr=0;
  m_en=true;
}

WDL_VirtualIconButton::~WDL_VirtualIconButton()
{
}


void WDL_VirtualIconButton::OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect) 
{ 
  HDC hdc=drawbm->getDC();
  int col;
  RECT r={origin_x,origin_y,origin_x+m_position.right-m_position.left,origin_y+m_position.bottom-m_position.top};
  if (WDL_STYLE_WantGlobalButtonBackground(&col))
  {
    HBRUSH br=CreateSolidBrush(col);
    FillRect(hdc,&r,br);
    DeleteObject(br);
  }
  if ((m_pressed&2) || WDL_STYLE_WantGlobalButtonBorders())
  {
    int cidx=(m_pressed&1)?COLOR_3DSHADOW:COLOR_3DHILIGHT;

    HPEN pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(cidx));
    HGDIOBJ open=SelectObject(hdc,pen);

    MoveToEx(hdc,r.left,r.bottom-1,NULL);
    LineTo(hdc,r.left,r.top);
    LineTo(hdc,r.right-1,r.top);
    SelectObject(hdc,open);
    DeleteObject(pen);
    cidx=(m_pressed&1)?COLOR_3DHILIGHT:COLOR_3DSHADOW;
    pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(cidx));
    open=SelectObject(hdc,pen);

    LineTo(hdc,r.right-1,r.bottom-1);
    LineTo(hdc,r.left,r.bottom-1);
    SelectObject(hdc,open);
    DeleteObject(pen);

  }
  if (m_iconPtr && *m_iconPtr)
  {
    int sz=16,sz2=16;
    WDL_STYLE_ScaleImageCoords(&sz,&sz2);

    //if (m_position.right-m_position.left > 24) sz=m_position.right-m_position.left-8;
    
    int x=origin_x+((m_position.right-m_position.left)-sz)/2;
    int y=origin_y+((m_position.bottom-m_position.top)-sz2)/2;
    if ((m_pressed&3)==3) { x++; y++; }
#ifdef _WIN32
    DrawIconEx(hdc,x,y,*m_iconPtr,sz,sz2,0,NULL,DI_NORMAL);
#else
    RECT r={x,y,x+sz,y+sz2};
    DrawImageInRect(hdc,*m_iconPtr,&r);
#endif
  }
} 


void WDL_VirtualIconButton::OnMouseMove(int xpos, int ypos)
{
  if (m_en)
  {
    int wp=m_pressed;
    if (xpos >= 0&& xpos < m_position.right-m_position.left && ypos >= 0 && ypos < m_position.bottom-m_position.top)
      m_pressed|=2;
    else m_pressed&=~2;

    if ((m_pressed&3)!=(wp&3))
      RequestRedraw(NULL);
  }
}

bool WDL_VirtualIconButton::OnMouseDown(int xpos, int ypos)
{
  if (m_en)
  {
    m_pressed=3;
    RequestRedraw(NULL);
    return true;
  }
  return false;
}

bool WDL_VirtualIconButton::OnMouseDblClick(int xpos, int ypos)
{
  if (m_parent)
  {
    if (m_en)
      m_parent->SendCommand(WM_COMMAND,GetID(),0,this);
  }
  return true;
}

void WDL_VirtualIconButton::OnMouseUp(int xpos, int ypos)
{
  int waspress=!!m_pressed;
  m_pressed&=~1;
  if (m_parent) 
  {
    RequestRedraw(NULL);
    if (waspress&&xpos >= 0&& xpos < m_position.right-m_position.left && ypos >= 0 && ypos < m_position.bottom-m_position.top)
    {
      if (m_en)
        m_parent->SendCommand(WM_COMMAND,GetID(),0,this);
    }
  }
}


WDL_VirtualIcon::WDL_VirtualIcon()
{
  m_iconPtr=0;
}

WDL_VirtualIcon::~WDL_VirtualIcon()
{
}


void WDL_VirtualIcon::OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect) 
{ 
  HDC hdc=drawbm->getDC();
  if (m_iconPtr && *m_iconPtr)
  {
    int sz=16,sz2=16;
    WDL_STYLE_ScaleImageCoords(&sz,&sz2);

    int x=origin_x+((m_position.right-m_position.left)-sz)/2;
    int y=origin_y+((m_position.bottom-m_position.top)-sz2)/2;
#ifdef _WIN32
    DrawIconEx(hdc,x,y,*m_iconPtr,sz,sz2,0,NULL,DI_NORMAL);
#else
    RECT r={x,y,x+sz,y+sz2};
    DrawImageInRect(hdc,*m_iconPtr,&r);
#endif
  }
} 

WDL_VirtualComboBox::WDL_VirtualComboBox()
{
  m_font=0;
  m_align=-1;
  m_curitem=-1;
}

WDL_VirtualComboBox::~WDL_VirtualComboBox()
{
  m_items.Empty(true,free);
}


bool WDL_VirtualComboBox::OnMouseDown(int xpos, int ypos)
{
  if (m_items.GetSize() && m_parent)
  {
    HMENU menu=CreatePopupMenu();
    int x;
    for (x = 0 ; x < m_items.GetSize(); x ++)
    {
      MENUITEMINFO mi={sizeof(mi),MIIM_ID|MIIM_STATE|MIIM_TYPE,MFT_STRING,
        0,1000+x,NULL,NULL,NULL,0};
      mi.dwTypeData = (char *)m_items.Get(x);
      mi.fState = m_curitem == x ?MFS_CHECKED:0;
      InsertMenuItem(menu,x,TRUE,&mi);
    }
    HWND h=GetRealParent();
    POINT p={0,};
    WDL_VirtualWnd *w=this;
    while (w)
    {
      RECT r;
      w->GetPosition(&r);
      p.x+=r.left;
      p.y+=w==this?r.bottom:r.top;
      w=w->GetParent();
    }
    if (h) 
    {
      ClientToScreen(h,&p);
      //SetFocus(h);
    }
    int ret=TrackPopupMenu(menu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY,p.x,p.y,0,h,NULL);

    if (ret>=1000)
    {
      m_curitem=ret-1000;
      RequestRedraw(NULL);
    // track menu
      m_parent->SendCommand(WM_COMMAND,GetID() | (CBN_SELCHANGE<<16),0,this);
    }
  }
  return true;
}

void WDL_VirtualComboBox::OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
  HDC hdc=drawbm->getDC();
  {
    int tcol=WDL_STYLE_GetSysColor(COLOR_BTNTEXT);
    SetTextColor(hdc,tcol);
    SetBkMode(hdc,TRANSPARENT);
    HGDIOBJ of=0;
    if (m_font) of=SelectObject(hdc,m_font);

    RECT r={origin_x,origin_y,origin_x+m_position.right-m_position.left,origin_y+m_position.bottom-m_position.top};
    int col=WDL_STYLE_GetSysColor(COLOR_WINDOW);
    HBRUSH br=CreateSolidBrush(col);
    FillRect(hdc,&r,br);
    DeleteObject(br);

    {
      RECT tr=r;
      tr.left=tr.right-(tr.bottom-tr.top);
      br=CreateSolidBrush(WDL_STYLE_GetSysColor(COLOR_BTNFACE));
      FillRect(hdc,&tr,br);
      DeleteObject(br);
    }
    

    if (m_items.Get(m_curitem)&&m_items.Get(m_curitem)[0])
    {
      RECT tr=r;
      tr.left+=2;
      tr.right-=16;
      DrawText(hdc,m_items.Get(m_curitem),-1,&tr,DT_SINGLELINE|DT_VCENTER|(m_align<0?DT_LEFT:m_align>0?DT_RIGHT:DT_CENTER|DT_NOPREFIX));
    }

    if (m_font) SelectObject(hdc,of);

    HPEN pen=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_3DSHADOW));
    HPEN pen3=CreatePen(PS_SOLID,2,tcol);
    HPEN pen2=CreatePen(PS_SOLID,0,WDL_STYLE_GetSysColor(COLOR_3DHILIGHT));
    of=SelectObject(hdc,pen);

    // draw the down arrow button
    {
      int bs=(r.bottom-r.top);
      int l=r.right-bs;
      MoveToEx(hdc,l,r.top,NULL);
      LineTo(hdc,l,r.bottom);

      SelectObject(hdc,pen2);
      MoveToEx(hdc,l-1,r.top,NULL);
      LineTo(hdc,l-1,r.bottom);

      SelectObject(hdc,pen3);
      int a=bs/4;
      MoveToEx(hdc,l+bs/2 - a,r.top+bs/2 - a/2,NULL);
        LineTo(hdc,l+bs/2, r.top+bs/2+a/2);
        LineTo(hdc,l+bs/2 + a,r.top+bs/2 - a/2);

    }

    SelectObject(hdc,pen);
    

    // draw the border
    MoveToEx(hdc,r.left,r.bottom-1,NULL);
    LineTo(hdc,r.left,r.top);
    LineTo(hdc,r.right-1,r.top);
    SelectObject(hdc,pen2);
    LineTo(hdc,r.right-1,r.bottom-1);
    LineTo(hdc,r.left,r.bottom-1);

    SelectObject(hdc,of);
    DeleteObject(pen);
    DeleteObject(pen2);
    DeleteObject(pen3);


  }
}



WDL_VirtualStaticText::WDL_VirtualStaticText()
{
  m_font=0;
  m_align=-1;
  m_wantsingle=false;
}

WDL_VirtualStaticText::~WDL_VirtualStaticText()
{
}

bool WDL_VirtualStaticText::OnMouseDown(int xpos, int ypos)
{
  if (m_wantsingle)
  {
    if (m_parent) m_parent->SendCommand(WM_COMMAND,GetID() | (STN_CLICKED<<16),0,this);
    return true;
  }
  return false;
}

void WDL_VirtualStaticText::OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
  HDC hdc=drawbm->getDC();
  if (m_text.Get()[0])
  {
    SetTextColor(hdc,WDL_STYLE_GetSysColor(COLOR_BTNTEXT));
    SetBkMode(hdc,TRANSPARENT);
    HGDIOBJ of=0;
    if (m_font) of=SelectObject(hdc,m_font);

    RECT r={origin_x,origin_y,origin_x+m_position.right-m_position.left,origin_y+m_position.bottom-m_position.top};
    

    DrawText(hdc,m_text.Get(),-1,&r,DT_SINGLELINE|DT_VCENTER|(m_align<0?DT_LEFT:m_align>0?DT_RIGHT:DT_CENTER|DT_NOPREFIX));

    if (m_font) SelectObject(hdc,of);


  }
}


bool WDL_VirtualStaticText::OnMouseDblClick(int xpos, int ypos)
{
  if (m_parent) m_parent->SendCommand(WM_COMMAND,GetID() | (STN_DBLCLK<<16),0,this);
  return true;

}
