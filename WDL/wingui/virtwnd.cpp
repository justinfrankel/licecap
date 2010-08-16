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

#include "virtwnd-controls.h"
#include "../lice/lice.h"


#ifdef _WIN32
#define WIN32_NATIVE_GRADIENT
#endif

WDL_VWnd_Painter::WDL_VWnd_Painter()
{
  m_GSC=0;
  m_bm=0;
  m_bgbm=0;

  m_paint_xorig=m_paint_yorig=0;
  m_cur_hwnd=0;
  m_wantg=-1;
  m_gradstart=0.5;
  m_gradslope=0.2;
}

WDL_VWnd_Painter::~WDL_VWnd_Painter()
{
  delete m_bm;
}

void WDL_VWnd_Painter::SetGSC(int (*GSC)(int))
{
  m_GSC=GSC;
}

void WDL_VWnd_Painter::SetBGGradient(int wantGradient, double start, double slope)
{
  m_wantg=wantGradient;
  m_gradstart=start;
  m_gradslope=slope;
}

void WDL_VWnd_Painter::DoPaintBackground(int bgcolor, RECT *clipr, int wnd_w, int wnd_h)
{
  if (!m_bm) return;

  if (m_bgbm&&m_bgbm->bgimage)
  {
    int srcw=m_bgbm->bgimage->getWidth();
    int srch=m_bgbm->bgimage->getHeight();
    if (srcw && srch)
    {
      int bpos=clipr->bottom;
//      if (++bpos>wnd_h)bpos=wnd_h; // for some reason drawing the extra line is necessary for some paints on windows
      int rpos=clipr->right;
  //    if (rpos+=4>wnd_w)rpos=wnd_w; // for some reason drawing the extra line is necessary for some paints on windows
      int lpos=clipr->left;
    //  if (lpos-=4<0) lpos=0;
      WDL_VirtualWnd_ScaledBlitBG(m_bm,m_bgbm,-m_paint_xorig,-m_paint_yorig,wnd_w,wnd_h,
                                  lpos,clipr->top,
                                  rpos-lpos,
                                  bpos-clipr->top,
                                  1.0,LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);

      if (m_bgbmtintcolor>=0)
      {
        float rv=GetRValue(m_bgbmtintcolor)/255.0;
        float gv=GetGValue(m_bgbmtintcolor)/255.0;
        float bv=GetBValue(m_bgbmtintcolor)/255.0;

        float avg=(rv+gv+bv)*0.33333f;
        if (avg<0.05)avg=0.05;

        float sc=0.5f;
        float sc2 = (1.0f-sc)/avg;

        float sc3=32.0f;
        float sc4=64.0f*(avg-0.5);
        // tint
        LICE_MultiplyAddRect(m_bm,lpos,clipr->top,rpos-lpos,bpos-clipr->top,
            sc+rv*sc2,sc+gv*sc2,sc+bv*sc2,1,
            (rv-avg)*sc3+sc4,
            (gv-avg)*sc3+sc4,
            (bv-avg)*sc3+sc4,
            0);
      }

      m_bgbm=0;
      return;
    }
  }

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

void WDL_VWnd_Painter::PaintBegin(HWND hwnd, int bgcolor)
{
  if (!hwnd) return;
  if (!m_cur_hwnd)
  {
    if (BeginPaint(hwnd,&m_ps)) 
    {
      m_cur_hwnd=hwnd;
    }
    if (m_cur_hwnd)
    {
      RECT r;
      GetClientRect(m_cur_hwnd,&r);
      int fwnd_w=r.right-r.left,fwnd_h=r.bottom-r.top;
      if (fwnd_h<0)fwnd_h=-fwnd_h;

      int wnd_w,wnd_h;
      
      if (fwnd_w < 2048 && fwnd_h < 2048)
      {
        m_paint_xorig=m_paint_yorig=0;
        wnd_w=fwnd_w;
        wnd_h=fwnd_h;
      }
      else // alternate large canvas mode 
      {
        m_paint_xorig=m_ps.rcPaint.left;
        m_paint_yorig=m_ps.rcPaint.top;
        wnd_w = m_ps.rcPaint.right-m_ps.rcPaint.left;
        wnd_h = m_ps.rcPaint.bottom - m_ps.rcPaint.top;
      }
      
      if (wnd_h<0)wnd_h=-wnd_h;

      if (!m_bm) m_bm=new LICE_SysBitmap;
      
      if (m_bm->getWidth()<wnd_w || m_bm->getHeight() < wnd_h)
      {
        m_bm->resize(max(m_bm->getWidth(),wnd_w),max(m_bm->getHeight(),wnd_h));
      }

      r=m_ps.rcPaint;
      r.left-=m_paint_xorig;
      r.right-=m_paint_xorig;
      r.top-=m_paint_yorig;
      r.bottom-=m_paint_yorig;
      DoPaintBackground(bgcolor,&r, fwnd_w, fwnd_h);
    }
  }
}


#ifdef _WIN32
typedef struct
{
  HRGN rgn;
  HWND par;
  RECT *sr;
} enumInfo;

static BOOL CALLBACK enumProc(HWND hwnd,LPARAM lParam)
{
  enumInfo *p=(enumInfo*)lParam;
  if (IsWindowVisible(hwnd)) 
  {
    RECT r;
    GetWindowRect(hwnd,&r);
    ScreenToClient(p->par,(LPPOINT)&r);
    ScreenToClient(p->par,((LPPOINT)&r)+1);
    if (!p->rgn) p->rgn=CreateRectRgnIndirect(p->sr);

    HRGN rgn2=CreateRectRgnIndirect(&r);
    CombineRgn(p->rgn,p->rgn,rgn2,RGN_DIFF);
    DeleteObject(rgn2);
  }
  return TRUE;
}
#endif

void WDL_VWnd_Painter::PaintEnd()
{
  if (!m_cur_hwnd) return;
  if (m_bm)
  {
#ifdef _WIN32
    HRGN rgnsave=0;
    if (1)
    {
      enumInfo a={0,m_cur_hwnd,&m_ps.rcPaint};
      EnumChildWindows(m_cur_hwnd,enumProc,(LPARAM)&a);
      if (a.rgn)
      {
        rgnsave=CreateRectRgn(0,0,0,0);
        GetClipRgn(m_ps.hdc,rgnsave);

        ExtSelectClipRgn(m_ps.hdc,a.rgn,RGN_AND);
        DeleteObject(a.rgn);
      }
    }
    BitBlt(m_ps.hdc,m_ps.rcPaint.left,m_ps.rcPaint.top,
                    m_ps.rcPaint.right-m_ps.rcPaint.left,
                    m_ps.rcPaint.bottom-m_ps.rcPaint.top,
                    m_bm->getDC(),m_ps.rcPaint.left-m_paint_xorig,m_ps.rcPaint.top-m_paint_yorig,SRCCOPY);

    if (rgnsave)
    {
      SelectClipRgn(m_ps.hdc,rgnsave);
      DeleteObject(rgnsave);
    }
#else
    SWELL_SyncCtxFrameBuffer(m_bm->getDC());
    BitBlt(m_ps.hdc,m_ps.rcPaint.left,m_ps.rcPaint.top,
           m_ps.rcPaint.right-m_ps.rcPaint.left,
           m_ps.rcPaint.bottom-m_ps.rcPaint.top,
           m_bm->getDC(),m_ps.rcPaint.left-m_paint_xorig,m_ps.rcPaint.top-m_paint_yorig,SRCCOPY);
#endif
  }
  EndPaint(m_cur_hwnd,&m_ps);
  m_cur_hwnd=0;
}

void WDL_VWnd_Painter::PaintVirtWnd(WDL_VWnd *vwnd, int borderflags)
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
    tr.left -= r.left+m_paint_xorig;
    tr.right -= r.left+m_paint_xorig;
    tr.top -= r.top+m_paint_yorig;
    tr.bottom -= r.top+m_paint_yorig;
    vwnd->OnPaint(m_bm,-m_paint_xorig,-m_paint_yorig,&tr);
    if (borderflags)
    {
      PaintBorderForRect(&r,borderflags);
    }
    if (vwnd->WantsPaintOver()) vwnd->OnPaintOver(m_bm,-m_paint_xorig,-m_paint_yorig,&tr);

  }
}

void WDL_VWnd_Painter::PaintBorderForHWND(HWND hwnd, int borderflags)
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

void WDL_VWnd_Painter::PaintBorderForRect(const RECT *r, int borderflags)
{
  if (!m_bm|| !m_cur_hwnd||!borderflags) return;
  RECT rrr = *r;
  rrr.left-=m_paint_xorig;
  rrr.right-=m_paint_xorig;
  rrr.top-=m_paint_yorig;
  rrr.bottom-=m_paint_yorig;
  r=&rrr;


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

WDL_VWnd::WDL_VWnd() 
{ 
  m_visible=true; m_id=0; 
  m_position.left=0; m_position.top=0; m_position.right=0; m_position.bottom=0; 
  m_parent=0;
  m_children=0;
  m_realparent=0;
  m_captureidx=-1;
  m_lastmouseidx=-1;
  m_userdata=0;
}

WDL_VWnd::~WDL_VWnd() 
{ 
  if (m_children) 
  {
    m_children->Empty(true); 
    delete m_children;
  }
}


INT_PTR WDL_VWnd::SendCommand(int command, INT_PTR parm1, INT_PTR parm2, WDL_VWnd *src)
{
  if (m_realparent)
  {
    return SendMessage(m_realparent,command,parm1,parm2);
  }
  else if (m_parent) return m_parent->SendCommand(command,parm1,parm2,src);
  return 0;
}

void WDL_VWnd::RequestRedraw(RECT *r)
{ 
  RECT r2;
  
  if (r)
  {
    r2=*r; 
    r2.left+=m_position.left; r2.right += m_position.left; 
    r2.top += m_position.top; r2.bottom += m_position.top;
  }
  else 
  {
    GetPositionPaintExtent(&r2);
    RECT r3;
    if (WantsPaintOver())
    {
      GetPositionPaintOverExtent(&r3);
      if (r3.left<r2.left)r2.left=r3.left;
      if (r3.top<r2.top)r2.top=r3.top;
      if (r3.right>r2.right)r2.right=r3.right;
      if (r3.bottom>r2.bottom)r2.bottom=r3.bottom;
    }
  }

  if (m_realparent)
  {
#ifdef _WIN32
    HWND hCh;
    if ((hCh=GetWindow(m_realparent,GW_CHILD)))
    {
      HRGN rgn=CreateRectRgnIndirect(&r2);
      int n=30; // limit to 30 children
      while (n-- && hCh)
      {
        if (IsWindowVisible(hCh))
        {
          RECT r;
          GetWindowRect(hCh,&r);
          ScreenToClient(m_realparent,(LPPOINT)&r);
          ScreenToClient(m_realparent,((LPPOINT)&r)+1);
          HRGN tmprgn=CreateRectRgn(r.left,r.top,r.right,r.bottom);
          CombineRgn(rgn,rgn,tmprgn,RGN_DIFF);
          DeleteObject(tmprgn);
        }
        hCh=GetWindow(hCh,GW_HWNDNEXT);
      }
      InvalidateRgn(m_realparent,rgn,FALSE);
      DeleteObject(rgn);

    }
    else
#else
  // OS X, expand region up slightly
  r2.top--;
#endif
      InvalidateRect(m_realparent,&r2,FALSE);
  }
  else if (m_parent) m_parent->RequestRedraw(&r2); 
}



void WDL_VWnd::AddChild(WDL_VWnd *wnd, int pos)
{
  if (!wnd) return;

  wnd->SetParent(this);
  if (!m_children) m_children=new WDL_PtrList<WDL_VWnd>;
  if (pos<0||pos>=m_children->GetSize()) m_children->Add(wnd);
  else m_children->Insert(pos,wnd);
}

WDL_VWnd *WDL_VWnd::GetChildByID(int id)
{
  if (m_children) 
  {
    int x;
    for (x = 0; x < m_children->GetSize(); x ++)
      if (m_children->Get(x)->GetID()==id) return m_children->Get(x);
    WDL_VWnd *r;
    for (x = 0; x < m_children->GetSize(); x ++) if ((r=m_children->Get(x)->GetChildByID(id))) return r;
  }

  return 0;
}

void WDL_VWnd::RemoveChild(WDL_VWnd *wnd, bool dodel)
{
  int idx=m_children ? m_children->Find(wnd) : -1;
  if (idx>=0) m_children->Delete(idx,dodel);
}


void WDL_VWnd::OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
  int x;
  if (m_children) for (x = m_children->GetSize()-1; x >=0; x --)
  {
    WDL_VWnd *ch=m_children->Get(x);
    if (ch->IsVisible())
    {
      RECT re;
      ch->GetPositionPaintExtent(&re);
      re.left += origin_x;
      re.right += origin_x;
      re.top += origin_y;
      re.bottom += origin_y;

      RECT cr=*cliprect;
      if (cr.left < re.left) cr.left=re.left;
      if (cr.right > re.right) cr.right=re.right;
      if (cr.top < re.top) cr.top=re.top;
      if (cr.bottom > re.bottom) cr.bottom=re.bottom;

      if (cr.left < cr.right && cr.top < cr.bottom)
      {
        ch->OnPaint(drawbm,m_position.left+origin_x,m_position.top+origin_y,&cr);
      }
    }
  }
}

void WDL_VWnd::OnPaintOver(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
  int x;
  if (m_children) for (x = m_children->GetSize()-1; x >=0; x --)
  {
    WDL_VWnd *ch=m_children->Get(x);
    if (ch->IsVisible() && ch->WantsPaintOver())
    {
      RECT re;
      ch->GetPositionPaintOverExtent(&re);
      re.left += origin_x;
      re.right += origin_x;
      re.top += origin_y;
      re.bottom += origin_y;

      RECT cr=*cliprect;

      if (cr.left < re.left) cr.left=re.left;
      if (cr.right > re.right) cr.right=re.right;
      if (cr.top < re.top) cr.top=re.top;
      if (cr.bottom > re.bottom) cr.bottom=re.bottom;

      if (cr.left < cr.right && cr.top < cr.bottom)
      {
        ch->OnPaintOver(drawbm,m_position.left+origin_x,m_position.top+origin_y,&cr);
      }
    }
  }
}

int WDL_VWnd::GetNumChildren()
{
  return m_children ? m_children->GetSize() : 0;
}
WDL_VWnd *WDL_VWnd::EnumChildren(int x)
{
  return m_children ? m_children->Get(x) : 0;
}

void WDL_VWnd::RemoveAllChildren(bool dodel)
{
  if (m_children) m_children->Empty(dodel);
}

WDL_VWnd *WDL_VWnd::VirtWndFromPoint(int xpos, int ypos, int maxdepth)
{
  int x;
  if (m_children) for (x = 0; x < m_children->GetSize(); x++)
  {
    WDL_VWnd *wnd=m_children->Get(x);
    if (wnd->IsVisible())
    {
      RECT r;
      wnd->GetPosition(&r);
      if (xpos >= r.left && xpos < r.right && ypos >= r.top && ypos < r.bottom) 
      {
        if (maxdepth!=0)
        {
          WDL_VWnd *cwnd=wnd->VirtWndFromPoint(xpos-r.left,ypos-r.top,maxdepth > 0 ? (maxdepth-1) : -1);
          if (cwnd) return cwnd;
        }
        return wnd;
      }
    }
  }
  return 0;

}


int WDL_VWnd::OnMouseDown(int xpos, int ypos) // returns TRUE if handled
{
  if (!m_children) return false;

  WDL_VWnd *wnd=VirtWndFromPoint(xpos,ypos,0);
  if (!wnd) 
  {
    m_captureidx=-1;
    return 0;
  }  
  RECT r;
  wnd->GetPosition(&r);
  int a;
  if ((a=wnd->OnMouseDown(xpos-r.left,ypos-r.top)))
  {
    if (a<0) return -1;
    m_captureidx=m_children->Find(wnd);
    return 1;
  }
  return false;
}

bool WDL_VWnd::OnMouseDblClick(int xpos, int ypos) // returns TRUE if handled
{
  WDL_VWnd *wnd=VirtWndFromPoint(xpos,ypos,0);
  if (!wnd) return false;
  RECT r;
  wnd->GetPosition(&r);
  return wnd->OnMouseDblClick(xpos-r.left,ypos-r.top);
}


bool WDL_VWnd::OnMouseWheel(int xpos, int ypos, int amt)
{
  WDL_VWnd *wnd=VirtWndFromPoint(xpos,ypos,0);
  if (!wnd) return false;
  RECT r;
  wnd->GetPosition(&r);
  return wnd->OnMouseWheel(xpos-r.left,ypos-r.top,amt);
}

void WDL_VWnd::OnMouseMove(int xpos, int ypos)
{
  if (!m_children) return;

  WDL_VWnd *wnd=m_children->Get(m_captureidx);
  
  if (!wnd) 
  {
    wnd=VirtWndFromPoint(xpos,ypos,0);
    if (wnd) // todo: stuff so if the mouse goes out of the window completely, the virtualwnd gets notified
    {
      int idx=m_children->Find(wnd);
      if (idx != m_lastmouseidx)
      {
        WDL_VWnd *t=m_children->Get(m_lastmouseidx);
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
      WDL_VWnd *t=m_children->Get(m_lastmouseidx);
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

void WDL_VWnd::OnMouseUp(int xpos, int ypos)
{
  if (m_children)
  {
    WDL_VWnd *wnd=m_children->Get(m_captureidx);
  
    if (!wnd) 
    {
      wnd=VirtWndFromPoint(xpos,ypos,0);
    }

    if (wnd) 
    {
      RECT r;
      wnd->GetPosition(&r);
      wnd->OnMouseUp(xpos-r.left,ypos-r.top);
    }
  }

  m_captureidx=-1;
}

void WDL_VirtualWnd_PreprocessBGConfig(WDL_VirtualWnd_BGCfg *a)
{
  if (!a || !a->bgimage) return;
  a->bgimage_lt[0]=a->bgimage_lt[1]=a->bgimage_rb[0]=a->bgimage_rb[1]=0;

  int w=a->bgimage->getWidth();
  int h=a->bgimage->getHeight();
  if (w>1&&h>1 && LICE_GetPixel(a->bgimage,0,0)==LICE_RGBA(255,0,255,255) &&
      LICE_GetPixel(a->bgimage,w-1,h-1)==LICE_RGBA(255,0,255,255))
  {
    int x;
    for (x = 1; x < w && LICE_GetPixel(a->bgimage,x,0)==LICE_RGBA(255,0,255,255); x ++);
    a->bgimage_lt[0] = x;
    for (x = w-2; x > a->bgimage_lt[0]+1 && LICE_GetPixel(a->bgimage,x,h-1)==LICE_RGBA(255,0,255,255); x --);
    a->bgimage_rb[0] = w-1-x;

    for (x = 1; x < h && LICE_GetPixel(a->bgimage,0,x)==LICE_RGBA(255,0,255,255); x ++);
    a->bgimage_lt[1] = x;
    for (x = h-2; x > a->bgimage_lt[1]+1 && LICE_GetPixel(a->bgimage,w-1,x)==LICE_RGBA(255,0,255,255); x --);
    a->bgimage_rb[1] = h-1-x;
  }

}

static void __VirtClipBlit(int clipx, int clipy, int clipright, int clipbottom,
                           LICE_IBitmap *dest, LICE_IBitmap *src, int dstx, int dsty, int dstw, int dsth, 
                           float srcx, float srcy, float srcw, float srch, float alpha, int mode)
{
  if (dstw<1||dsth<1 || dstx+dstw < clipx || dstx > clipright ||
      dsty+dsth < clipy || dsty > clipbottom) 
  {
    return; // dont draw if fully outside
  }

  if (dstx < clipx || dsty < clipy || 
      dstx+dstw > clipright || dsty+dsth > clipbottom) 
  {
    float xsc=(float)srcw/dstw;
    float ysc=(float)srch/dsth;

    if (dstx<clipx)
    {
      int diff=clipx-dstx;
      srcx += xsc*diff;
      srcw -= xsc*diff;
      dstw -= diff;
      dstx += diff;
    }
    if (dsty<clipy)
    {
      int diff=clipy-dsty;
      srcy += ysc*diff;
      srch -= ysc*diff;
      dsth -= diff;
      dsty += diff;
    }
    if (dstx+dstw > clipright)
    {
      int diff=clipright-dstx-dstw;
      dstw -= diff;
      srcw -= diff*xsc;
    }
    if (dsty+dsth > clipbottom)
    {
      int diff=clipbottom-dsty-dsth;
      dsth -= diff;
      srch -= diff*ysc;
    }

  }

  if (dstw>0&&dsth>0)
    LICE_ScaledBlit(dest,src,dstx,dsty,dstw,dsth,srcx,srcy,srcw,srch,alpha,mode);
}

void WDL_VirtualWnd_ScaledBlitBG(LICE_IBitmap *dest, 
                                 WDL_VirtualWnd_BGCfg *src,
                                 int destx, int desty, int destw, int desth,
                                 int clipx, int clipy, int clipw, int cliph,
                                 float alpha, int mode)
{
  // todo: blit clipping optimizations
  if (!src || !src->bgimage) return;

  int left_margin=src->bgimage_lt[0];
  int top_margin=src->bgimage_lt[1];
  int right_margin=src->bgimage_rb[0];
  int bottom_margin=src->bgimage_rb[1];

  int sw=src->bgimage->getWidth();
  int sh=src->bgimage->getHeight();

  int clipright=clipx+clipw;
  int clipbottom=clipy+cliph;

  if (clipx<destx) clipx=destx;
  if (clipy<desty) clipy=desty;
  if (clipright>destx+destw) clipright=clipx+destw;
  if (clipbottom>desty+desth) clipbottom=clipy+desth;
  
  if (left_margin<1||top_margin<1||right_margin<1||bottom_margin<1) 
  {
    float xsc=(float)sw/destw;
    float ysc=(float)sh/desth;


    LICE_ScaledBlit(dest,src->bgimage,
      clipx,clipy,clipright-clipx,clipbottom-clipy,
      (clipx-destx)*xsc,
      (clipy-desty)*ysc,
      (clipright-clipx)*xsc,
      (clipbottom-clipy)*ysc,
      alpha,mode);

    return;
  }

  // remove 1px additional margins from calculations
  left_margin--; top_margin--; right_margin--; bottom_margin--;

  if (left_margin+right_margin>destw) 
  { 
    int w=left_margin+right_margin;
    left_margin = destw*left_margin/max(w,1);
    right_margin=destw-left_margin; 
  }
  if (top_margin+bottom_margin>desth) 
  { 
    int h=(top_margin+bottom_margin);
    top_margin=desth*top_margin/max(h,1);
    bottom_margin=desth-top_margin; 
  }

  
  int pass;
  for (pass=0;pass<3; pass++)
  {
    int outy,outh,iny;
    int inh;    
    switch (pass)
    {
      case 0:
        outy=desty;
        outh=top_margin;
        iny=1;
        inh=src->bgimage_lt[1]-1;
      break;
      case 1:
        outy=desty+top_margin;
        outh=desth-top_margin-bottom_margin;
        iny=src->bgimage_lt[1];
        inh=sh-src->bgimage_lt[1]-src->bgimage_rb[1];
      break;
      case 2:
        outy=desty+desth-bottom_margin;
        outh=bottom_margin;
        iny=sh - src->bgimage_rb[1];
        inh=src->bgimage_rb[1]-1;
      break;
    }
    
    if (outh > 0 && inh > 0)
    {

      // left 
      if (left_margin > 0)
        __VirtClipBlit(clipx,clipy,clipright,clipbottom,dest,src->bgimage,destx,outy,left_margin,outh,
                             1,iny,src->bgimage_lt[0]-1,inh,alpha,mode);
      // center
      __VirtClipBlit(clipx,clipy,clipright,clipbottom,dest,src->bgimage,destx+left_margin,outy,
                              destw-right_margin-left_margin,outh,
                           src->bgimage_lt[0],iny,
                           sw-src->bgimage_lt[0]-src->bgimage_rb[0],
                           inh,alpha,mode);
      // right
      if (right_margin > 0)
        __VirtClipBlit(clipx,clipy,clipright,clipbottom,dest,src->bgimage,destx+destw-right_margin,outy, right_margin,outh,
                             sw-src->bgimage_rb[0],iny,
                             src->bgimage_rb[0]-1,inh,alpha,mode); 
    }
  }
}

int WDL_VirtualWnd_ScaledBG_GetPix(WDL_VirtualWnd_BGCfg *src,
                                   int ww, int wh,
                                   int x, int y)
{
  if (!src->bgimage) return 0;
  int imgw=src->bgimage->getWidth();
  int imgh=src->bgimage->getHeight();

  int left_margin=src->bgimage_lt[0];
  int top_margin=src->bgimage_lt[1];
  int right_margin=src->bgimage_rb[0];
  int bottom_margin=src->bgimage_rb[1];

  if (left_margin<1||top_margin<1||right_margin<1||bottom_margin<1) 
  {
    if (ww<1)ww=1;
    x=(x * imgw)/ww;
    if (wh<1)wh=1;
    y=(y * imgh)/wh;
  }
  else
  {
    // remove 1px additional margins from calculations
    left_margin--; top_margin--; right_margin--; bottom_margin--;
    int destw=ww,desth=wh;
    if (left_margin+right_margin>destw) 
    { 
      int w=left_margin+right_margin;
      left_margin = destw*left_margin/max(w,1);
      right_margin=destw-left_margin; 
    }
    if (top_margin+bottom_margin>desth) 
    { 
      int h=(top_margin+bottom_margin);
      top_margin=desth*top_margin/max(h,1);
      bottom_margin=desth-top_margin; 
    }

    if (x >= ww-right_margin) x=imgw-1- (ww-x);
    else if (x >= left_margin)
    {
      int xd=ww-left_margin-right_margin;
      if (xd<1)xd=1;
      x=src->bgimage_lt[0] + 
        (x-left_margin) * (imgw-src->bgimage_lt[0]-src->bgimage_rb[0])/xd;
    }
    else x++; 

    if (y >= wh-bottom_margin) y=imgh -1- (wh-y);
    else if (y >= top_margin)
    {
      int yd=wh-top_margin-bottom_margin;
      if (yd<1)yd=1;
      y=src->bgimage_lt[1] + 
        (y-top_margin) * (imgh-src->bgimage_lt[1]-src->bgimage_rb[1])/yd;
    }
    else y++; 
  }

  return LICE_GetPixel(src->bgimage,x,y);
}

