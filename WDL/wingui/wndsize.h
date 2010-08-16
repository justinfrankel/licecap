/*
    WDL - wndsize.h
    Copyright (C) 2004 and later Cockos Incorporated

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

  This file provides the interface for a simple class that allows one to easily 
  make resizeable dialogs and have controls that move according to ratios of the 
  new size.

  Usually, one does a 

  static WDL_WndSizer resize;

  in their DlgProc, and in WM_INITDIALOG:

        resize.init(hwndDlg);

        // add a list of items
        resize.init_item(IDC_MASTERVOL,  // dialog id
                              0.0,  // left position, 0=left anchor, 1=right anchor, etc
                              0.0, // top position, 0=anchored to its initial top position, 1=anchored to distance from bottom, etc
                              0.7f,  // right position
                              0.0);  // bottom position
        ...


  then, in WM_SIZE,
      if (wParam != SIZE_MINIMIZED) 
      {
        resize.onResize(); // don't actually resize, just compute the rects
      }


  is about all that's needed.

   
*/

#ifndef _WNDSIZE_H_
#define _WNDSIZE_H_


#include "virtwnd.h"
#include "../heapbuf.h"


typedef struct
{
  HWND hwnd;
  RECT orig;
  RECT real_orig;
  RECT last;
  float scales[4];
  WDL_VirtualWnd *vwnd;

} WDL_WndSizer__rec;

class WDL_WndSizer
{
public:
  WDL_WndSizer() { }
  ~WDL_WndSizer() { }

  void init(HWND hwndDlg, RECT *initr=NULL);

  // 1.0 means it moves completely with the point, 0.0 = not at all
  void init_item(int dlg_id, float left_scale=0.0, float top_scale=0.0, float right_scale=1.0, float bottom_scale=1.0, RECT *initr=NULL);
  void init_itemvirt(WDL_VirtualWnd *vwnd, float left_scale=0.0, float top_scale=0.0, float right_scale=1.0, float bottom_scale=1.0);
  void init_itemhwnd(HWND h, float left_scale=0.0, float top_scale=0.0, float right_scale=1.0, float bottom_scale=1.0, RECT *srcr=NULL);
  void remove_item(int dlg_id);
  void remove_itemvirt(WDL_VirtualWnd *vwnd);
  void remove_itemhwnd(HWND h);

  WDL_WndSizer__rec *get_item(int dlg_id);
  WDL_WndSizer__rec *get_itembyindex(int id);
  WDL_WndSizer__rec *get_itembywnd(HWND h);
  WDL_WndSizer__rec *get_itembyvirt(WDL_VirtualWnd *vwnd);
  
  RECT get_orig_rect() { return m_orig_rect; }

  void onResize(HWND only=0, int notouch=0);

private:
#ifdef _WIN32
  static BOOL CALLBACK enum_RegionRemove(HWND hwnd,LPARAM lParam);
  HRGN m_enum_rgn;
#else
  static HWND NSView_GetSubViewByTag(HWND hwnd, int tag);
  static void NSView_GetBounds(HWND hwnd, RECT *r);
  static void NSView_GetFrame(HWND hwnd, RECT *r);
  static void NSView_SetFrame(HWND hwnd, RECT *r);
#endif
  HWND m_hwnd;
  RECT m_orig_rect;

  // treat as WDL_WndSizer__rec[]
  WDL_HeapBuf m_list;

};

#endif//_WNDSIZE_H_