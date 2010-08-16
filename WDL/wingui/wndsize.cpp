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
  if (initr)
    m_orig_rect=*initr;
  else
    GetClientRect(m_hwnd,&m_orig_rect);
  m_list.Resize(0);
}


void WDL_WndSizer::init_itemvirt(WDL_VirtualWnd *vwnd, 
                                 float left_scale, float top_scale, float right_scale, float bottom_scale)
{
  RECT this_r;
  vwnd->GetPosition(&this_r);
  int osize=m_list.GetSize();
  m_list.Resize(osize+sizeof(WDL_WndSizer__rec));

  WDL_WndSizer__rec *rec=(WDL_WndSizer__rec *) ((char *)m_list.Get()+osize);

  rec->hwnd=0;
  rec->vwnd=vwnd;
  rec->scales[0]=left_scale;
  rec->scales[1]=top_scale;
  rec->scales[2]=right_scale;
  rec->scales[3]=bottom_scale;
  rec->real_orig = rec->last = rec->orig = this_r;
}


void WDL_WndSizer::init_itemhwnd(HWND h, float left_scale, float top_scale, float right_scale, float bottom_scale, RECT *srcr)
{
  RECT this_r;
  if (srcr) this_r=*srcr;
  else
  {
    GetWindowRect(h,&this_r);
    ScreenToClient(m_hwnd,(LPPOINT) &this_r);
    ScreenToClient(m_hwnd,((LPPOINT) &this_r)+1);
    
  #ifndef _WIN32
    if (this_r.bottom < this_r.top)
    {
      int oh=this_r.top-this_r.bottom;
      this_r.bottom=this_r.top; 
      this_r.top -= oh;
    }
  #endif
  }
  int osize=m_list.GetSize();
  m_list.Resize(osize+sizeof(WDL_WndSizer__rec));

  WDL_WndSizer__rec *rec=(WDL_WndSizer__rec *) ((char *)m_list.Get()+osize);

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
  init_itemhwnd(GetDlgItem(m_hwnd,dlg_id),left_scale,top_scale,right_scale,bottom_scale,initr);
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
    ScreenToClient(_this->m_hwnd,(LPPOINT)&r);
    ScreenToClient(_this->m_hwnd,((LPPOINT)&r)+1);

    HRGN rgn2=CreateRectRgn(r.left,r.top,r.right,r.bottom);
    CombineRgn(_this->m_enum_rgn,_this->m_enum_rgn,rgn2,RGN_DIFF);
    DeleteObject(rgn2);
  }
  
  return TRUE;
}
#endif

void WDL_WndSizer::remove_item(int dlg_id)
{
  remove_itemhwnd(GetDlgItem(m_hwnd,dlg_id));
}

void WDL_WndSizer::remove_itemhwnd(HWND h)
{
  WDL_WndSizer__rec *rec;
  while ((rec=get_itembywnd(h)))
  {
    WDL_WndSizer__rec *list=(WDL_WndSizer__rec *) ((char *)m_list.Get());
    int list_size=m_list.GetSize()/sizeof(WDL_WndSizer__rec);
    int idx=rec-list;

    if (idx >= 0 && idx < list_size-1)
      memcpy(rec,rec+1,(list_size-(idx+1))*sizeof(WDL_WndSizer__rec));
    m_list.Resize((list_size-1)*sizeof(WDL_WndSizer__rec));
  }
}

void WDL_WndSizer::remove_itemvirt(WDL_VirtualWnd *vwnd)
{
  WDL_WndSizer__rec *rec;
  while ((rec=get_itembyvirt(vwnd)))
  {
    WDL_WndSizer__rec *list=(WDL_WndSizer__rec *) ((char *)m_list.Get());
    int list_size=m_list.GetSize()/sizeof(WDL_WndSizer__rec);
    int idx=rec-list;

    if (idx >= 0 && idx < list_size-1)
      memcpy(rec,rec+1,(list_size-(idx+1))*sizeof(WDL_WndSizer__rec));
    m_list.Resize((list_size-1)*sizeof(WDL_WndSizer__rec));
  }
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
  WDL_WndSizer__rec *rec=(WDL_WndSizer__rec *) ((char *)m_list.Get());
  int cnt=m_list.GetSize() / sizeof(WDL_WndSizer__rec);

  int x;
  for (x = 0; x < cnt; x ++)
  {

    if ((rec->vwnd && !only) || (rec->hwnd && (!only || only == rec->hwnd)))
    {
      RECT r;
      if (rec->scales[0] <= 0.0) r.left = rec->orig.left;
      else if (rec->scales[0] >= 1.0) r.left = rec->orig.left + new_rect.right - m_orig_rect.right;
      else r.left = rec->orig.left + (int) ((new_rect.right - m_orig_rect.right)*rec->scales[0]);

      if (rec->scales[1] <= 0.0) r.top = rec->orig.top;
      else if (rec->scales[1] >= 1.0) r.top = rec->orig.top + new_rect.bottom - m_orig_rect.bottom;
      else r.top = rec->orig.top + (int) ((new_rect.bottom - m_orig_rect.bottom)*rec->scales[1]);

      if (rec->scales[2] <= 0.0) r.right = rec->orig.right;
      else if (rec->scales[2] >= 1.0) r.right = rec->orig.right + new_rect.right - m_orig_rect.right;
      else r.right = rec->orig.right + (int) ((new_rect.right - m_orig_rect.right)*rec->scales[2]);

      if (rec->scales[3] <= 0.0) r.bottom = rec->orig.bottom;
      else if (rec->scales[3] >= 1.0) r.bottom = rec->orig.bottom + new_rect.bottom - m_orig_rect.bottom;
      else r.bottom = rec->orig.bottom + (int) ((new_rect.bottom - m_orig_rect.bottom)*rec->scales[3]);

      if (r.bottom < r.top) r.bottom=r.top;
      if (r.right < r.left) r.right=r.left;
    
      rec->last = r;

      if (!notouch)
      {
        if (rec->hwnd)
        {
#ifdef _WIN32
          if (!has_dfwp)
          {
            has_dfwp=1;
            if (!only && GetVersion() < 0x80000000) hdwndpos=BeginDeferWindowPos(cnt);
          }


          if (!hdwndpos) 
#endif
            SetWindowPos(rec->hwnd, NULL, r.left+xtranslate,r.top+ytranslate,r.right-r.left,r.bottom-r.top, SWP_NOZORDER|SWP_NOACTIVATE);
          
#ifdef _WIN32
          else 
          {
            hdwndpos=DeferWindowPos(hdwndpos, rec->hwnd, NULL, r.left+xtranslate,r.top+ytranslate,r.right-r.left,r.bottom-r.top, SWP_NOZORDER|SWP_NOACTIVATE);
          }
#endif
        }
        if (rec->vwnd)
        {
          rec->vwnd->SetPosition(&r);
        }
      }
    }
    rec++;
  }
#ifdef _WIN32
  if (hdwndpos) EndDeferWindowPos(hdwndpos);

  EnumChildWindows(m_hwnd,enum_RegionRemove,(LPARAM)this);
  InvalidateRgn(m_hwnd,m_enum_rgn,FALSE);
  DeleteObject(m_enum_rgn);
#endif
}

WDL_WndSizer__rec *WDL_WndSizer::get_itembyindex(int id)
{
  if (id >= 0 && id < (m_list.GetSize() / (int)sizeof(WDL_WndSizer__rec)))
  {
    return ((WDL_WndSizer__rec *) m_list.Get())+id;
  }
  return NULL;
}

WDL_WndSizer__rec *WDL_WndSizer::get_itembywnd(HWND h)
{
  if (h)
  {
    WDL_WndSizer__rec *rec=(WDL_WndSizer__rec *) ((char *)m_list.Get());
    int cnt=m_list.GetSize() / sizeof(WDL_WndSizer__rec);
    while (cnt--)
    {
      if (rec->hwnd == h) return rec;
      rec++;
    }
  }
  return 0;
}


WDL_WndSizer__rec *WDL_WndSizer::get_itembyvirt(WDL_VirtualWnd *vwnd)
{
  if (!vwnd) return 0;
  WDL_WndSizer__rec *rec=(WDL_WndSizer__rec *) ((char *)m_list.Get());
  int cnt=m_list.GetSize() / sizeof(WDL_WndSizer__rec);
  while (cnt--)
  {
    if (rec->vwnd == vwnd) return rec;
    rec++;
  }
  return 0;
}

WDL_WndSizer__rec *WDL_WndSizer::get_item(int dlg_id)
{
  WDL_WndSizer__rec *rec=(WDL_WndSizer__rec *) ((char *)m_list.Get());
  int cnt=m_list.GetSize() / sizeof(WDL_WndSizer__rec);
  while (cnt--)
  {
    if (rec->vwnd && rec->vwnd->GetID() == dlg_id) return rec;
    rec++;
  }

  return get_itembywnd(GetDlgItem(m_hwnd,dlg_id));
}
