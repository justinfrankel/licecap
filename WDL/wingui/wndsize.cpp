/*
    WDL - wndsize.cpp
    Copyright (C) 2004 and later, Cockos Incorporated
  
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
      
*/

/*
For information on how to use this class, see wndsize.h :)
*/

#ifndef _WIN32
#include "../swell/swell.h"
#else
#include <windows.h>
#endif

#include "wndsize.h"
#include "virtwnd.h"

void WDL_WndSizer::init(HWND hwndDlg, RECT *initr)
{
  m_hwnd=hwndDlg;
  RECT r={0,};
  if (initr)
    r=*initr;
  else if (m_hwnd) 
    GetClientRect(m_hwnd,&r);
  set_orig_rect(&r);

  m_list.Resize(0);

  memset(&m_margins,0,sizeof(m_margins));
}


void WDL_WndSizer::init_item(int dlg_id, float *scales, RECT *initr)
{
  init_item(dlg_id,scales[0],scales[1],scales[2],scales[3],initr);
}
void WDL_WndSizer::init_itemvirt(WDL_VWnd *vwnd, float *scales)
{
  init_itemvirt(vwnd,scales[0],scales[1],scales[2],scales[3]);
}

void WDL_WndSizer::init_itemvirt(WDL_VirtualWnd *vwnd, 
                                 float left_scale, float top_scale, float right_scale, float bottom_scale)
{
  RECT this_r={0,};
  if (vwnd) vwnd->GetPosition(&this_r);
  
  const int osize=m_list.GetSize();
  m_list.Resize(osize+1);
  if (m_list.GetSize() != osize + 1) return;

  WDL_WndSizer__rec *rec=m_list.Get()+osize;

  rec->hwnd=0;
  rec->vwnd=vwnd;
  rec->scales[0]=left_scale;
  rec->scales[1]=top_scale;
  rec->scales[2]=right_scale;
  rec->scales[3]=bottom_scale;
  rec->real_orig = rec->last = rec->orig = this_r;
}

POINT WDL_WndSizer::get_min_size(bool applyMargins) const
{ 
  POINT p = m_min_size;
  if (applyMargins)
  {
    p.x += m_margins.left+m_margins.right;
    p.y += m_margins.top+m_margins.bottom;
  }
  return p; 
}


void WDL_WndSizer::init_itemhwnd(HWND h, float left_scale, float top_scale, float right_scale, float bottom_scale, RECT *srcr)
{
  RECT this_r={0,};
  if (srcr) this_r=*srcr;
  else if (h)
  {
    GetWindowRect(h,&this_r);
    if (m_hwnd)
    {
      ScreenToClient(m_hwnd,(LPPOINT) &this_r);
      ScreenToClient(m_hwnd,((LPPOINT) &this_r)+1);
    }    
  #ifndef _WIN32
    if (this_r.bottom < this_r.top)
    {
      int oh=this_r.top-this_r.bottom;
      this_r.bottom=this_r.top; 
      this_r.top -= oh;
    }
  #endif
  }
  const int osize=m_list.GetSize();
  m_list.Resize(osize+1);
  if (m_list.GetSize() != osize + 1) return;

  WDL_WndSizer__rec *rec=m_list.Get()+osize;

  rec->hwnd=h;
  rec->vwnd=0;
  rec->scales[0]=left_scale;
  rec->scales[1]=top_scale;
  rec->scales[2]=right_scale;
  rec->scales[3]=bottom_scale;

  rec->real_orig = rec->last = rec->orig = this_r;
}

void WDL_WndSizer::init_item(int dlg_id, float left_scale, float top_scale, float right_scale, float bottom_scale, RECT *initr)
{
  if (m_hwnd)
  {
    HWND h = GetDlgItem(m_hwnd, dlg_id);
    if (h) init_itemhwnd(h, left_scale, top_scale, right_scale, bottom_scale, initr);
  }
}

#ifdef _WIN32
BOOL CALLBACK WDL_WndSizer::enum_RegionRemove(HWND hwnd,LPARAM lParam)
{
  WDL_WndSizer *_this=(WDL_WndSizer *)lParam;
//  if(GetParent(hwnd)!=_this->m_hwnd) return TRUE;
    
  if (IsWindowVisible(hwnd)) 
  {
    RECT r;
    GetWindowRect(hwnd,&r);
    if (_this->m_hwnd)
    {
      ScreenToClient(_this->m_hwnd,(LPPOINT)&r);
      ScreenToClient(_this->m_hwnd,((LPPOINT)&r)+1);
    }
    HRGN rgn2=CreateRectRgn(r.left,r.top,r.right,r.bottom);
    CombineRgn(_this->m_enum_rgn,_this->m_enum_rgn,rgn2,RGN_DIFF);
    DeleteObject(rgn2);
  }
  
  return TRUE;
}
#endif

void WDL_WndSizer::remove_item(int dlg_id)
{
  int x = m_list.GetSize();
  while (--x >= 0)
  {
    const WDL_WndSizer__rec *rec = m_list.Get() + x;
    if ((rec->hwnd && GetWindowLong(rec->hwnd,GWL_ID) == dlg_id) ||
        (rec->vwnd && rec->vwnd->GetID() == dlg_id)) m_list.Delete(x);
  }
}

void WDL_WndSizer::remove_itemhwnd(HWND h)
{
  if (h)
  {
    int x = m_list.GetSize();
    while (--x >= 0)
    {
      if (m_list.Get()[x].hwnd == h) m_list.Delete(x);
    }
  }
}

void WDL_WndSizer::remove_itemvirt(WDL_VirtualWnd *vwnd)
{
  if (vwnd)
  {
    int x = m_list.GetSize();
    while (--x >= 0)
    {
      if (m_list.Get()[x].vwnd == vwnd) m_list.Delete(x);
    }
  }
}

void WDL_WndSizer::transformRect(RECT *r, const float *scales, const RECT *wndSize) const
{
  POINT sz = { wndSize->right, wndSize->bottom };

  sz.x -= m_margins.left+m_margins.right;
  sz.y -= m_margins.top+m_margins.bottom;

  if (sz.x < m_min_size.x) sz.x=m_min_size.x;
  if (sz.y < m_min_size.y) sz.y=m_min_size.y;

  sz.x -= m_orig_size.x;
  sz.y -= m_orig_size.y;

  if (scales[0] >= 1.0) r->left += sz.x;
  else if (scales[0]>0.0) r->left += (int) (sz.x*scales[0]);

  if (scales[1] >= 1.0) r->top += sz.y;
  else if (scales[1]>0.0) r->top += (int) (sz.y*scales[1]);

  if (scales[2] >= 1.0) r->right += sz.x;
  else if (scales[2]>0.0) r->right += (int) (sz.x*scales[2]);

  if (scales[3] >= 1.0) r->bottom += sz.y;
  else if (scales[3]>0.0) r->bottom += (int) (sz.y*scales[3]);

  r->left += m_margins.left;
  r->right += m_margins.left;
  r->top += m_margins.top;
  r->bottom += m_margins.top;

  if (r->bottom < r->top) r->bottom=r->top;
  if (r->right < r->left) r->right=r->left;
}


void WDL_WndSizer::onResize(HWND only, int notouch, int xtranslate, int ytranslate)
{
  if (!m_hwnd) return;

  RECT new_rect;
  
  GetClientRect(m_hwnd,&new_rect);
#ifdef _WIN32

  m_enum_rgn=CreateRectRgn(new_rect.left,new_rect.top,new_rect.right,new_rect.bottom);
 // EnumChildWindows(m_hwnd,enum_RegionRemove,(LPARAM)this);
  
  HDWP hdwndpos=NULL;
  int has_dfwp=0;
#endif
  int x;
  for (x = 0; x < m_list.GetSize(); x ++)
  {
    WDL_WndSizer__rec *rec=m_list.Get() + x;

    if ((rec->vwnd && !only) || (rec->hwnd && (!only || only == rec->hwnd)))
    {
      RECT r=rec->orig;
      transformRect(&r,rec->scales,&new_rect);
    
      rec->last = r;

      if (!notouch)
      {
        if (rec->hwnd)
        {
#ifdef _WIN32
          if (!has_dfwp)
          {
            has_dfwp=1;
            if (!only 
#ifdef WDL_SUPPORT_WIN9X
            && GetVersion() < 0x80000000
#endif
            ) hdwndpos=BeginDeferWindowPos(m_list.GetSize() - x);
          }


          if (!hdwndpos) 
#endif
            SetWindowPos(rec->hwnd, NULL, r.left+xtranslate,r.top+ytranslate,r.right-r.left,r.bottom-r.top, SWP_NOZORDER|SWP_NOACTIVATE);
          
#ifdef _WIN32
          else 
            hdwndpos=DeferWindowPos(hdwndpos, rec->hwnd, NULL, r.left+xtranslate,r.top+ytranslate,r.right-r.left,r.bottom-r.top, SWP_NOZORDER|SWP_NOACTIVATE);
#endif
        }
        if (rec->vwnd)
        {
          rec->vwnd->SetPosition(&r);
        }
      }
    }
  }
#ifdef _WIN32
  if (hdwndpos) EndDeferWindowPos(hdwndpos);

  EnumChildWindows(m_hwnd,enum_RegionRemove,(LPARAM)this);
  InvalidateRgn(m_hwnd,m_enum_rgn,FALSE);
  DeleteObject(m_enum_rgn);
#endif
}

WDL_WndSizer__rec *WDL_WndSizer::get_itembyindex(int idx) const
{
  if (idx >= 0 && idx < m_list.GetSize()) return m_list.Get()+idx;

  return NULL;
}

WDL_WndSizer__rec *WDL_WndSizer::get_itembywnd(HWND h) const
{
  if (h)
  {
    WDL_WndSizer__rec *rec=m_list.Get();
    int cnt=m_list.GetSize();
    while (cnt--)
    {
      if (rec->hwnd == h) return rec;
      rec++;
    }
  }
  return 0;
}


WDL_WndSizer__rec *WDL_WndSizer::get_itembyvirt(WDL_VirtualWnd *vwnd) const
{
  if (!vwnd) return 0;
  WDL_WndSizer__rec *rec=m_list.Get();
  int cnt=m_list.GetSize();
  while (cnt--)
  {
    if (rec->vwnd == vwnd) return rec;
    rec++;
  }
  return 0;
}

WDL_WndSizer__rec *WDL_WndSizer::get_item(int dlg_id) const
{
  WDL_WndSizer__rec *rec=m_list.Get();
  int cnt=m_list.GetSize();
  while (cnt--)
  {
    if (rec->vwnd && rec->vwnd->GetID() == dlg_id) return rec;
    if (rec->hwnd && GetWindowLong(rec->hwnd, GWL_ID) == dlg_id) return rec;
    rec++;
  }
  return NULL;
}
