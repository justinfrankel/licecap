/*
    WDL - virtwnd.cpp
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
      

    Implementation for basic virtual window types.

*/

#include "virtwnd.h"
#include "../lice/lice.h"


#ifdef _WIN32
#define WIN32_NATIVE_GRADIENT
#endif

WDL_VirtualWnd_Painter::WDL_VirtualWnd_Painter()
{
  m_GSC=0;
  m_bm=0;

  m_cur_hwnd=0;
  m_wantg=-1;
  m_gradstart=0.5;
  m_gradslope=0.2;
}

WDL_VirtualWnd_Painter::~WDL_VirtualWnd_Painter()
{
  delete m_bm;
}

void WDL_VirtualWnd_Painter::SetGSC(int (*GSC)(int))
{
  m_GSC=GSC;
}

void WDL_VirtualWnd_Painter::SetBGGradient(int wantGradient, double start, double slope)
{
  m_wantg=wantGradient;
  m_gradstart=start;
  m_gradslope=slope;
}

void WDL_VirtualWnd_Painter::DoPaintBackground(int bgcolor, RECT *clipr, int wnd_w, int wnd_h)
{
  if (!m_bm) return;

  if (bgcolor<0) bgcolor=m_GSC?m_GSC(COLOR_3DFACE):GetSysColor(COLOR_3DFACE);

  double gradslope=m_gradslope;
  double gradstart=m_gradstart;
  bool wantGrad=m_wantg>0;
  if (m_wantg<0) wantGrad=WDL_STYLE_GetBackgroundGradient(&gradstart,&gradslope);

  int needfill=1;

  if (wantGrad && gradslope >= 0.01)
  {

#ifdef WIN32_NATIVE_GRADIENT
    static BOOL (WINAPI *__GradientFill)(HDC hdc,CONST PTRIVERTEX pVertex,DWORD dwNumVertex,CONST PVOID pMesh,DWORD dwNumMesh,DWORD dwMode); 
    static bool hasld;

    if (!hasld)
    {
      hasld=true;
      HINSTANCE  lib=LoadLibrary("msimg32.dll");
      if (lib) *((void **)&__GradientFill) = (void *)GetProcAddress(lib,"GradientFill");
    }
#endif

    {
      needfill=0;

      int spos = (int) (gradstart * wnd_h);
      if (spos > 0)
      {
        if (spos > wnd_h) spos=wnd_h;
        if (clipr->top < spos)
        {
          RECT tr=m_ps.rcPaint;
          tr.bottom=spos;
          HBRUSH b=CreateSolidBrush(bgcolor);
          FillRect(m_bm->getDC(),&tr,b);
          DeleteObject(b);
        }
      }
      else spos=0;

      if (spos < wnd_h)
      {
#ifdef WIN32_NATIVE_GRADIENT
        TRIVERTEX        vert[2] = {0,} ;
#else
        struct
        {
          int x,y,Red,Green,Blue;
        }
        vert[2]={0,};

#endif
        double sr=GetRValue(bgcolor);
        double sg=GetGValue(bgcolor);
        double sb=GetBValue(bgcolor);

        vert [0] .x = clipr->left;
        vert [1] .x = clipr->right;


        vert[0].y=clipr->top;
        vert[1].y=clipr->bottom;

        if (vert[0].y < spos) vert[0].y=spos;
        if (vert[1].y>wnd_h) vert[1].y=wnd_h;

        wnd_h-=spos;

        int x;
        for (x =0 ; x < 2; x ++)
        {
          double sc1=(wnd_h-(vert[x].y-spos)*gradslope)/(double)wnd_h * 256.0;

          vert[x].Red = (int) (sr * sc1);
          vert[x].Green = (int) (sg * sc1);
          vert[x].Blue = (int) (sb * sc1);

        }

#ifdef WIN32_NATIVE_GRADIENT
        if (__GradientFill)
        {
          GRADIENT_RECT    gRect;
          gRect.UpperLeft  = 0;
          gRect.LowerRight = 1;
          __GradientFill(m_bm->getDC(),vert,2,&gRect,1,GRADIENT_FILL_RECT_V);
        }
        else
#endif
        {

          int bmh=vert[1].y-vert[0].y;
          int bmw=clipr->right-clipr->left;

          float s=(float) (1.0/(65535.0*bmh));

          LICE_GradRect(m_bm,clipr->left,vert[0].y,bmw,bmh,
            vert[0].Red/65535.0f,vert[0].Green/65535.0f,vert[0].Blue/65535.0f,1.0,0,0,0,0,
            (vert[1].Red-vert[0].Red)*s,
            (vert[1].Green-vert[0].Green)*s,
            (vert[1].Blue-vert[0].Blue)*s,
             0.0,LICE_BLIT_MODE_COPY);
        }
      }
    }
  }



  if (needfill)
  {
    HBRUSH b=CreateSolidBrush(bgcolor);
    FillRect(m_bm->getDC(),clipr,b);
    DeleteObject(b);
  }        

}

#ifdef _WIN32
void WDL_VirtualWnd_Painter::PaintBegin(HWND hwnd, int bgcolor)
#else
void WDL_VirtualWnd_Painter::PaintBegin(void *ctx, int bgcolor, RECT *clipr, int wnd_w, int wnd_h)
#endif
{
#ifdef _WIN32
  if (!hwnd) return;
  if (!m_cur_hwnd)
  {
    if (BeginPaint(hwnd,&m_ps)) m_cur_hwnd=hwnd;
    if (m_cur_hwnd)
    {
      RECT r;
      GetWindowRect(m_cur_hwnd,&r);
      int wnd_w=r.right-r.left,wnd_h=r.bottom-r.top;
#else
      if (!ctx) return;
      if (!m_cur_hwnd)
      {
        m_cur_hwnd=ctx;
        m_ps.rcPaint=*clipr;
        
#endif

      if (!m_bm)
        m_bm=new LICE_SysBitmap;
      
      if (m_bm->getWidth()<wnd_w || m_bm->getHeight() < wnd_h)
        m_bm->resize(max(m_bm->getWidth(),wnd_w),max(m_bm->getHeight(),wnd_h));

      DoPaintBackground(bgcolor,&m_ps.rcPaint, wnd_w, wnd_h);


      }
#ifdef _WIN32
  }
#endif
}

void WDL_VirtualWnd_Painter::PaintEnd()
{
  if (!m_cur_hwnd) return;
#ifdef _WIN32
  if (m_bm)
  {
    BitBlt(m_ps.hdc,m_ps.rcPaint.left,m_ps.rcPaint.top,
                    m_ps.rcPaint.right-m_ps.rcPaint.left,
                    m_ps.rcPaint.bottom-m_ps.rcPaint.top,
                    m_bm->getDC(),m_ps.rcPaint.left,m_ps.rcPaint.top,SRCCOPY);
  }
  EndPaint(m_cur_hwnd,&m_ps);
#else
  if (m_bm)
  {
    HDC hdc=WDL_GDP_CreateContext(m_cur_hwnd);
    BitBlt(hdc,m_ps.rcPaint.left,m_ps.rcPaint.top,
                    m_ps.rcPaint.right-m_ps.rcPaint.left,
                    m_ps.rcPaint.bottom-m_ps.rcPaint.top,
                    m_bm->getDC(),m_ps.rcPaint.left,m_ps.rcPaint.top,SRCCOPY);

    WDL_GDP_DeleteContext(hdc);
  }
#endif
  m_cur_hwnd=0;
}

void WDL_VirtualWnd_Painter::PaintVirtWnd(WDL_VirtualWnd *vwnd, int borderflags)
{
  RECT tr=m_ps.rcPaint;
  if (!m_bm||!m_cur_hwnd|| !vwnd->IsVisible()) return;

  RECT r;
  vwnd->GetPosition(&r);

  if (tr.left<r.left) tr.left=r.left;
  if (tr.right>r.right) tr.right=r.right;
  if (tr.top<r.top) tr.top=r.top;
  if (tr.bottom>r.bottom)tr.bottom=r.bottom;

  if (tr.bottom > tr.top && tr.right > tr.left)
  {
    vwnd->OnPaint(m_bm,r.left,r.top,&tr);

    if (borderflags)
    {
      PaintBorderForRect(&r,borderflags);
    }
  }
}

void WDL_VirtualWnd_Painter::PaintBorderForHWND(HWND hwnd, int borderflags)
{
#ifdef _WIN32
  if (m_cur_hwnd)
  {
    RECT r;
    GetWindowRect(hwnd,&r);
    ScreenToClient(m_cur_hwnd,(LPPOINT)&r);
    ScreenToClient(m_cur_hwnd,((LPPOINT)&r)+1);
    PaintBorderForRect(&r,borderflags);
  }
#endif
}

void WDL_VirtualWnd_Painter::PaintBorderForRect(const RECT *r, int borderflags)
{
  if (!m_bm|| !m_cur_hwnd||!borderflags) return;

  HPEN pen=CreatePen(PS_SOLID,0,m_GSC?m_GSC(COLOR_3DHILIGHT):GetSysColor(COLOR_3DHILIGHT));
  HPEN pen2=CreatePen(PS_SOLID,0,m_GSC?m_GSC(COLOR_3DSHADOW):GetSysColor(COLOR_3DSHADOW));

  HDC hdc=m_bm->getDC();
  if (borderflags== WDL_VWP_SUNKENBORDER || borderflags == WDL_VWP_SUNKENBORDER_NOTOP)
  {
    MoveToEx(hdc,r->left-1,r->bottom,NULL);
    HGDIOBJ o=SelectObject(hdc,pen);
    LineTo(hdc,r->right,r->bottom);
    LineTo(hdc,r->right,r->top-1);
    SelectObject(hdc,pen2);
    if (borderflags == WDL_VWP_SUNKENBORDER_NOTOP) MoveToEx(hdc,r->left-1,r->top-1,NULL);
    else LineTo(hdc,r->left-1,r->top-1);

    LineTo(hdc,r->left-1,r->bottom);
    SelectObject(hdc,o);
  }
  else if (borderflags == WDL_VWP_DIVIDER_VERT) // vertical
  {
    int left=r->left;
    HGDIOBJ o=SelectObject(hdc,pen2);
    MoveToEx(hdc,left,r->top,NULL);
    LineTo(hdc,left,r->bottom+1);
    SelectObject(hdc,pen);
    MoveToEx(hdc,left+1,r->top,NULL);
    LineTo(hdc,left+1,r->bottom+1);
    SelectObject(hdc,o);
  }
  else if (borderflags == WDL_VWP_DIVIDER_HORZ) 
  {
    int top=r->top+1;
    HGDIOBJ o=SelectObject(hdc,pen2);
    MoveToEx(hdc,r->left,top,NULL);
    LineTo(hdc,r->right+1,top);
    SelectObject(hdc,pen);
    MoveToEx(hdc,r->left,top+1,NULL);
    LineTo(hdc,r->right+1,top+1);
    SelectObject(hdc,o);
  }

  DeleteObject(pen);
  DeleteObject(pen2);


}



WDL_VirtualWnd_ChildList::WDL_VirtualWnd_ChildList()
{
  m_captureidx=-1;
  m_lastmouseidx=-1;
}

WDL_VirtualWnd_ChildList::~WDL_VirtualWnd_ChildList()
{
  m_children.Empty(true);
}

void WDL_VirtualWnd_ChildList::AddChild(WDL_VirtualWnd *wnd)
{
  wnd->SetParent(this);
  m_children.Add(wnd);
}

WDL_VirtualWnd *WDL_VirtualWnd_ChildList::GetChildByID(int id)
{
  int x;
  for (x = 0; x < m_children.GetSize(); x ++)
    if (m_children.Get(x)->GetID()==id) return m_children.Get(x);
  return 0;
}

void WDL_VirtualWnd_ChildList::RemoveChild(WDL_VirtualWnd *wnd, bool dodel)
{
  int idx=m_children.Find(wnd);
  if (idx>=0) m_children.Delete(idx,dodel);
}


void WDL_VirtualWnd_ChildList::OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
  int x;
  for (x = 0; x < m_children.GetSize(); x ++)
  {
    WDL_VirtualWnd *ch=m_children.Get(x);
    if (ch->IsVisible())
    {
      RECT r;
      ch->GetPosition(&r);
      RECT cr=*cliprect;
      r.left += origin_x;
      r.right += origin_x;
      r.top += origin_y;
      r.bottom += origin_y;

      if (cr.left < r.left) cr.left=r.left;
      if (cr.right > r.right) cr.right=r.right;
      if (cr.top < r.top) cr.top=r.top;
      if (cr.bottom > r.bottom) cr.bottom=r.bottom;

      if (cr.left < cr.right && cr.top < cr.bottom)
        ch->OnPaint(drawbm,r.left,r.top,&cr);
    }
  }
}

WDL_VirtualWnd *WDL_VirtualWnd_ChildList::EnumChildren(int x)
{
  return m_children.Get(x);
}

void WDL_VirtualWnd_ChildList::RemoveAllChildren(bool dodel)
{
  m_children.Empty(dodel);
}

WDL_VirtualWnd *WDL_VirtualWnd_ChildList::VirtWndFromPoint(int xpos, int ypos)
{
  int x;
  for (x = 0; x < m_children.GetSize(); x++)
  {
    WDL_VirtualWnd *wnd=m_children.Get(x);
    if (wnd->IsVisible())
    {
      RECT r;
      wnd->GetPosition(&r);
      if (xpos >= r.left && xpos < r.right && ypos >= r.top && ypos < r.bottom) return wnd;
    }
  }
  return 0;

}


bool WDL_VirtualWnd_ChildList::OnMouseDown(int xpos, int ypos) // returns TRUE if handled
{
  WDL_VirtualWnd *wnd=VirtWndFromPoint(xpos,ypos);
  if (!wnd) 
  {
    m_captureidx=-1;
    return false;
  }  
  RECT r;
  wnd->GetPosition(&r);
  if (wnd->OnMouseDown(xpos-r.left,ypos-r.top))
  {
    m_captureidx=m_children.Find(wnd);
    return true;
  }
  return false;
}

bool WDL_VirtualWnd_ChildList::OnMouseDblClick(int xpos, int ypos) // returns TRUE if handled
{
  WDL_VirtualWnd *wnd=VirtWndFromPoint(xpos,ypos);
  if (!wnd) return false;
  RECT r;
  wnd->GetPosition(&r);
  return wnd->OnMouseDblClick(xpos-r.left,ypos-r.top);
}


bool WDL_VirtualWnd_ChildList::OnMouseWheel(int xpos, int ypos, int amt)
{
  WDL_VirtualWnd *wnd=VirtWndFromPoint(xpos,ypos);
  if (!wnd) return false;
  RECT r;
  wnd->GetPosition(&r);
  return wnd->OnMouseWheel(xpos-r.left,ypos-r.top,amt);
}

void WDL_VirtualWnd_ChildList::OnMouseMove(int xpos, int ypos)
{
  WDL_VirtualWnd *wnd=m_children.Get(m_captureidx);
  
  if (!wnd) 
  {
    wnd=VirtWndFromPoint(xpos,ypos);
    if (wnd) // todo: stuff so if the mouse goes out of the window completely, the virtualwnd gets notified
    {
      int idx=m_children.Find(wnd);
      if (idx != m_lastmouseidx)
      {
        WDL_VirtualWnd *t=m_children.Get(m_lastmouseidx);
        if (t)
        {
          RECT r;
          t->GetPosition(&r);
          t->OnMouseMove(xpos-r.left,ypos-r.top);
        }
        m_lastmouseidx=idx;
      }
    }
    else
    {
      WDL_VirtualWnd *t=m_children.Get(m_lastmouseidx);
      if (t)
      {
        RECT r;
        t->GetPosition(&r);
        t->OnMouseMove(xpos-r.left,ypos-r.top);
      }
      m_lastmouseidx=-1;
    }
  }

  if (wnd) 
  {
    RECT r;
    wnd->GetPosition(&r);
    wnd->OnMouseMove(xpos-r.left,ypos-r.top);
  }
}

void WDL_VirtualWnd_ChildList::OnMouseUp(int xpos, int ypos)
{
  WDL_VirtualWnd *wnd=m_children.Get(m_captureidx);
  
  if (!wnd) 
  {
    wnd=VirtWndFromPoint(xpos,ypos);
  }

  if (wnd) 
  {
    RECT r;
    wnd->GetPosition(&r);
    wnd->OnMouseUp(xpos-r.left,ypos-r.top);
  }

  m_captureidx=-1;
}



void WDL_VirtualWnd_ChildList::RequestRedraw(RECT *r)
{
  if (m_realparent)
  {
    RECT cr=*r;
    cr.top += m_position.top;
    cr.bottom += m_position.top;
    cr.left += m_position.left;
    cr.right += m_position.left;
#ifdef _WIN32
    InvalidateRect(m_realparent,&cr,FALSE);
#else
    Mac_Invalidate(m_realparent,&cr);
#endif
  }
  else WDL_VirtualWnd::RequestRedraw(r);
}

int WDL_VirtualWnd_ChildList::SendCommand(int command, int parm1, int parm2, WDL_VirtualWnd *src)
{
  if (m_realparent)
  {
#ifdef _WIN32
    return SendMessage(m_realparent,command,parm1,parm2);
#else
    return Mac_SendCommand(m_realparent,command,parm1,parm2,src);
#endif
  }
  return WDL_VirtualWnd::SendCommand(command,parm1,parm2,this);
}
