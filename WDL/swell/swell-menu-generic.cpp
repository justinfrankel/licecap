/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Linux)
   Copyright (C) 2006-2007, Cockos, Inc.

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
  

    This file provides basic windows menu API

  */

#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-menugen.h"

#include "swell-internal.h"

#include "../ptrlist.h"
#include "../wdlcstring.h"

HMENU__ *HMENU__::Duplicate()
{
  HMENU__ *p = new HMENU__;
  int x;
  for (x = 0; x < items.GetSize(); x ++)
  {
    MENUITEMINFO *s = items.Get(x);
    MENUITEMINFO *inf = (MENUITEMINFO*)calloc(sizeof(MENUITEMINFO),1);

    *inf = *s;
    if (inf->dwTypeData && inf->fType == MFT_STRING) inf->dwTypeData=strdup(inf->dwTypeData);
    if (inf->hSubMenu) inf->hSubMenu = inf->hSubMenu->Duplicate();

    p->items.Add(inf);
  }
  return p;
}

void HMENU__::freeMenuItem(void *p)
{
  MENUITEMINFO *inf = (MENUITEMINFO *)p;
  if (!inf) return;
  delete inf->hSubMenu;
  if (inf->fType == MFT_STRING) free(inf->dwTypeData);
  free(inf);
}

static MENUITEMINFO *GetMenuItemByID(HMENU menu, int id, bool searchChildren=true)
{
  if (!menu) return 0;
  int x;
  for (x = 0; x < menu->items.GetSize(); x ++)
    if (menu->items.Get(x)->wID == id) return menu->items.Get(x);

  if (searchChildren) for (x = 0; x < menu->items.GetSize(); x ++)
  { 
    if (menu->items.Get(x)->hSubMenu)
    {
      MENUITEMINFO *ret = GetMenuItemByID(menu->items.Get(x)->hSubMenu,id,true);
      if (ret) return ret;
    }
  }

  return 0;
}

bool SetMenuItemText(HMENU hMenu, int idx, int flag, const char *text)
{
  MENUITEMINFO *item = hMenu ? ((flag & MF_BYPOSITION) ? hMenu->items.Get(idx) : GetMenuItemByID(hMenu,idx)) : NULL;
  if (!item) return false;

  if (item->fType == MFT_STRING) free(item->dwTypeData);
  item->fType = MFT_STRING;
  item->dwTypeData=strdup(text?text:"");
  
  return true;
}

bool EnableMenuItem(HMENU hMenu, int idx, int en)
{
  MENUITEMINFO *item = hMenu ? ((en & MF_BYPOSITION) ? hMenu->items.Get(idx) : GetMenuItemByID(hMenu,idx)) : NULL;
  if (!item) return false;
 
  int mask = MF_ENABLED|MF_DISABLED|MF_GRAYED;
  item->fState &= ~mask;
  item->fState |= en&mask;

  return true;
}

bool CheckMenuItem(HMENU hMenu, int idx, int chk)
{
  MENUITEMINFO *item = hMenu ? ((chk & MF_BYPOSITION) ? hMenu->items.Get(idx) : GetMenuItemByID(hMenu,idx)) : NULL;
  if (!item) return false;
  
  int mask = MF_CHECKED;
  item->fState &= ~mask;
  item->fState |= chk&mask;
  
  return true;
}
HMENU SWELL_GetCurrentMenu()
{
  return NULL; // not osx
}
void SWELL_SetCurrentMenu(HMENU hmenu)
{
}

HMENU GetSubMenu(HMENU hMenu, int pos)
{
  MENUITEMINFO *item = hMenu ? hMenu->items.Get(pos) : NULL;
  if (item) return item->hSubMenu;
  return 0;
}

int GetMenuItemCount(HMENU hMenu)
{
  if (hMenu) return hMenu->items.GetSize();
  return 0;
}

int GetMenuItemID(HMENU hMenu, int pos)
{
  MENUITEMINFO *item = hMenu ? hMenu->items.Get(pos) : NULL;
  if (!item || item->hSubMenu) return -1;
  return item->wID;
}

bool SetMenuItemModifier(HMENU hMenu, int idx, int flag, int code, unsigned int mask)
{
  return false;
}

HMENU CreatePopupMenu()
{
  return new HMENU__;
}
HMENU CreatePopupMenuEx(const char *title)
{
  return CreatePopupMenu();
}

void DestroyMenu(HMENU hMenu)
{
  delete hMenu;
}

int AddMenuItem(HMENU hMenu, int pos, const char *name, int tagid)
{
  if (!hMenu) return -1;
  MENUITEMINFO *inf = (MENUITEMINFO*)calloc(1,sizeof(MENUITEMINFO));
  inf->wID = tagid;
  inf->fType = MFT_STRING;
  inf->dwTypeData = strdup(name?name:"");
  hMenu->items.Insert(pos,inf);
  return 0;
}

bool DeleteMenu(HMENU hMenu, int idx, int flag)
{
  if (!hMenu) return false;
  if (flag&MF_BYPOSITION)
  {
    if (hMenu->items.Get(idx))
    {
      hMenu->items.Delete(idx,true,HMENU__::freeMenuItem);
      return true;
    }
    return false;
  }
  else
  {
    int x;
    int cnt=0;
    for (x=0;x<hMenu->items.GetSize(); x ++)
    {
      if (!hMenu->items.Get(x)->hSubMenu && hMenu->items.Get(x)->wID == idx)
      {
        hMenu->items.Delete(x--,true,HMENU__::freeMenuItem);
        cnt++;
      }
    }
    if (!cnt)
    {
      for (x=0;x<hMenu->items.GetSize(); x ++)
      {
        if (hMenu->items.Get(x)->hSubMenu) cnt += DeleteMenu(hMenu->items.Get(x)->hSubMenu,idx,flag)?1:0;
      }
    }
    return !!cnt;
  }
}


BOOL SetMenuItemInfo(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (!hMenu) return 0;
  MENUITEMINFO *item = byPos ? hMenu->items.Get(pos) : GetMenuItemByID(hMenu, pos, true);
  if (!item) return 0;
  
  if ((mi->fMask & MIIM_SUBMENU) && mi->hSubMenu != item->hSubMenu)
  {  
    delete item->hSubMenu;
    item->hSubMenu = mi->hSubMenu;
  } 
  if (mi->fMask & MIIM_TYPE)
  {
    if (item->fType == MFT_STRING) free(item->dwTypeData);
    item->dwTypeData=0;

    if (mi->fType == MFT_STRING && mi->dwTypeData) item->dwTypeData = strdup( mi->dwTypeData );
    else if (mi->fType == MFT_BITMAP) item->dwTypeData = mi->dwTypeData;
    item->fType = mi->fType;
  }

  if (mi->fMask & MIIM_STATE) item->fState = mi->fState;
  if (mi->fMask & MIIM_ID) item->wID = mi->wID;
  if (mi->fMask & MIIM_DATA) item->dwItemData = mi->dwItemData;
  
  return true;
}

BOOL GetMenuItemInfo(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (!hMenu) return 0;
  MENUITEMINFO *item = byPos ? hMenu->items.Get(pos) : GetMenuItemByID(hMenu, pos, true);
  if (!item) return 0;
  
  if (mi->fMask & MIIM_TYPE)
  {
    mi->fType = item->fType;
    if (item->fType == MFT_STRING && mi->dwTypeData && mi->cch)
    {
      lstrcpyn_safe(mi->dwTypeData,item->dwTypeData?item->dwTypeData:"",mi->cch);
    }
    else if (item->fType == MFT_BITMAP) mi->dwTypeData = item->dwTypeData;
  }
  
  if (mi->fMask & MIIM_DATA) mi->dwItemData = item->dwItemData;
  if (mi->fMask & MIIM_STATE) mi->fState = item->fState;
  if (mi->fMask & MIIM_ID) mi->wID = item->wID;
  if (mi->fMask & MIIM_SUBMENU) mi->hSubMenu = item->hSubMenu;
  
  return 1;
  
}

void SWELL_InsertMenu(HMENU menu, int pos, unsigned int flag, UINT_PTR idx, const char *str)
{
  MENUITEMINFO mi={sizeof(mi),MIIM_ID|MIIM_STATE|MIIM_TYPE,MFT_STRING,
    (flag & ~MF_BYPOSITION),(flag&MF_POPUP) ? 0 : (int)idx,NULL,NULL,NULL,0,(char *)str};
  
  if (flag&MF_POPUP) 
  {
    mi.hSubMenu = (HMENU)idx;
    mi.fMask |= MIIM_SUBMENU;
    mi.fState &= ~MF_POPUP;
  }
  
  if (flag&MF_SEPARATOR)
  {
    mi.fMask=MIIM_TYPE;
    mi.fType=MFT_SEPARATOR;
    mi.fState &= ~MF_SEPARATOR;
  }

  if (flag&MF_BITMAP)
  {
    mi.fType=MFT_BITMAP;
    mi.fState &= ~MF_BITMAP;
  }
    
  InsertMenuItem(menu,pos,(flag&MF_BYPOSITION) ?  TRUE : FALSE, &mi);
}

void InsertMenuItem(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (!hMenu) return;
  int ni=hMenu->items.GetSize();
  
  if (!byPos) 
  {
    int x;
    for (x=0;x<ni && hMenu->items.Get(x)->wID != pos; x++);
    pos = x;
  }
  if (pos < 0 || pos > ni) pos=ni; 
  
  MENUITEMINFO *inf = (MENUITEMINFO*)calloc(sizeof(MENUITEMINFO),1);
  inf->fType = mi->fType;
  if (mi->fType == MFT_STRING)
  {
    inf->dwTypeData = strdup(mi->dwTypeData?mi->dwTypeData:"");
  }
  else if (mi->fType == MFT_BITMAP)
  {
    inf->dwTypeData = mi->dwTypeData;
  }
  else if (mi->fType == MFT_SEPARATOR)
  {
  }
  if (mi->fMask&MIIM_SUBMENU) inf->hSubMenu = mi->hSubMenu;
  if (mi->fMask & MIIM_STATE) inf->fState = mi->fState;
  if (mi->fMask & MIIM_DATA) inf->dwItemData = mi->dwItemData;
  if (mi->fMask & MIIM_ID) inf->wID = mi->wID;

  hMenu->items.Insert(pos,inf);
}


void SWELL_SetMenuDestination(HMENU menu, HWND hwnd)
{
  // only needed for Cocoa
}

static POINT m_trackingPt;
static int m_trackingMouseFlag;
static int m_trackingFlags,m_trackingRet;
static HWND m_trackingPar;
static WDL_PtrList<HWND__> m_trackingMenus; // each HWND as userdata = HMENU

static LRESULT WINAPI submenuWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  const int lcol=24, rcol=12, mcol=10, top_margin=4;
  const int separator_ht = 8, text_ht_pad = 4, bitmap_ht_pad = 4;
  switch (uMsg)
  {
    case WM_CREATE:
      hwnd->m_classname = "__SWELL_MENU";
      m_trackingMenus.Add(hwnd);
      SetWindowLongPtr(hwnd,GWLP_USERDATA,lParam);

      if (m_trackingPar && !(m_trackingFlags&TPM_NONOTIFY))
        SendMessage(m_trackingPar,WM_INITMENUPOPUP,(WPARAM)lParam,0);

      {
        HDC hdc = GetDC(hwnd);
        HMENU__ *menu = (HMENU__*)lParam;
        int ht = 0, wid=100,wid2=0;
        int xpos=m_trackingPt.x;
        int ypos=m_trackingPt.y;
        int x;
        for (x=0; x < menu->items.GetSize(); x++)
        {
          MENUITEMINFO *inf = menu->items.Get(x);
          if (inf->fType == MFT_STRING)
          {
            RECT r={0,};
            const char *str = inf->dwTypeData;
            if (!str || !*str) str="XXXXX";
            const char *pt2 = strstr(str,"\t");
            DrawText(hdc,str,pt2 ? (int)(pt2-str) : -1,&r,DT_CALCRECT|DT_SINGLELINE);
            if (r.right > wid) wid=r.right;
            ht += r.bottom + text_ht_pad;

            if (pt2)
            { 
              r.right=r.left;
              DrawText(hdc,pt2+1,-1,&r,DT_CALCRECT|DT_SINGLELINE);
              if (r.right > wid2) wid2=r.right;
            }
          }
          else if (inf->fType == MFT_BITMAP)
          {
            BITMAP bm={16,16};
            if (inf->dwTypeData) GetObject((HBITMAP)inf->dwTypeData,sizeof(bm),&bm);
            if (bm.bmWidth > wid) wid = bm.bmWidth;

            ht += bm.bmHeight + bitmap_ht_pad;
          }
          else
          {
            // treat as separator
            ht += separator_ht;
          }
        }
        wid+=lcol+rcol + (wid2?wid2+mcol:0);
        ReleaseDC(hwnd,hdc);
        RECT tr={xpos,ypos,xpos+wid+4,ypos+ht+top_margin * 2},vp;
        SWELL_GetViewPort(&vp,&tr,true);
        if (tr.bottom > vp.bottom) { tr.top += vp.bottom-tr.bottom; tr.bottom=vp.bottom; }
        if (tr.right > vp.right) { tr.left += vp.right-tr.right; tr.right=vp.right; }
        if (tr.left < vp.left) { tr.right += vp.left-tr.left; tr.left=vp.left; }
        if (tr.top < vp.top) { tr.bottom += vp.top-tr.top; tr.top=vp.top; }
        SetWindowPos(hwnd,NULL,tr.left,tr.top,tr.right-tr.left,tr.bottom-tr.top,SWP_NOZORDER);
      }
      SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)&~WS_CAPTION);
      ShowWindow(hwnd,SW_SHOW);
      SetFocus(hwnd);
      SetTimer(hwnd,1,250,NULL);
    break;
    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT cr;
          GetClientRect(hwnd,&cr);
          HBRUSH br=CreateSolidBrush(GetSysColor(COLOR_3DFACE));
          HPEN pen=CreatePen(PS_SOLID,0,GetSysColor(COLOR_3DSHADOW));
          HPEN pen2=CreatePen(PS_SOLID,0,GetSysColor(COLOR_3DHILIGHT));
          HGDIOBJ oldbr = SelectObject(ps.hdc,br);
          HGDIOBJ oldpen = SelectObject(ps.hdc,pen2);
          Rectangle(ps.hdc,cr.left,cr.top,cr.right-1,cr.bottom-1);
          SetBkMode(ps.hdc,TRANSPARENT);
          int cols[2]={ GetSysColor(COLOR_BTNTEXT),GetSysColor(COLOR_3DHILIGHT)};
          HMENU__ *menu = (HMENU__*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
          int x;
          int ypos = top_margin;
          extern HWND GetFocusIncludeMenus();

          MoveToEx(ps.hdc,cr.left+lcol-4,cr.top,NULL);
          LineTo(ps.hdc,cr.left+lcol-4,cr.bottom);
          SelectObject(ps.hdc,pen);
          MoveToEx(ps.hdc,cr.left+lcol-5,cr.top,NULL);
          LineTo(ps.hdc,cr.left+lcol-5,cr.bottom);

          for (x=0; x < menu->items.GetSize(); x++)
          {
            MENUITEMINFO *inf = menu->items.Get(x);
            RECT r={lcol,ypos,cr.right, };
            bool dis = !!(inf->fState & MF_GRAYED);
            BITMAP bm={16,16};

            if (inf->fType == MFT_STRING)
            {
              const char *str = inf->dwTypeData;
              if (!str || !*str) str="XXXXX";
              RECT mr={0,};
              DrawText(ps.hdc,str,-1,&mr,DT_CALCRECT|DT_SINGLELINE);

              ypos += mr.bottom + text_ht_pad;
              r.bottom = ypos;
            }
            else if (inf->fType == MFT_BITMAP)
            {
              if (inf->dwTypeData) GetObject((HBITMAP)inf->dwTypeData,sizeof(bm),&bm);

              ypos += bm.bmHeight + bitmap_ht_pad;
              r.bottom = ypos;

            }
            else
            {
              dis=true;
              ypos += separator_ht;
              r.bottom = ypos;
            }

            if (x == menu->sel_vis && !dis)
            {
              HBRUSH br=CreateSolidBrush(cols[dis]);
              RECT r2=r;
              FillRect(ps.hdc,&r2,br);
              DeleteObject(br);
              SetTextColor(ps.hdc,GetSysColor(COLOR_3DFACE));
            }
            else SetTextColor(ps.hdc,cols[dis]);

            if (inf->fType == MFT_STRING)
            {
              const char *str = inf->dwTypeData;
              if (!str) str="";
              const char *pt2 = strstr(str,"\t");

              if (*str) 
              {
                DrawText(ps.hdc,str,pt2 ? (int)(pt2-str) : -1,&r,DT_VCENTER|DT_SINGLELINE);
                if (pt2)
                {
                  RECT tr=r; tr.right-=rcol;
                  DrawText(ps.hdc,pt2+1,-1,&tr,DT_VCENTER|DT_SINGLELINE|DT_RIGHT);
                }
              }
            }
            else if (inf->fType == MFT_BITMAP)
            {
              if (inf->dwTypeData)
              {
                RECT tr = r;
                tr.top += bitmap_ht_pad/2;
                tr.right = tr.left + bm.bmWidth;
                tr.bottom = tr.top + bm.bmHeight;
                DrawImageInRect(ps.hdc,(HBITMAP)inf->dwTypeData,&tr);
              }
            }
            else 
            {
              SelectObject(ps.hdc,pen2);
              int y = r.top/2+r.bottom/2, right = r.right-rcol*3/2;
              MoveToEx(ps.hdc,r.left,y,NULL);
              LineTo(ps.hdc,right,y);
              SelectObject(ps.hdc,pen);

              y++;
              MoveToEx(ps.hdc,r.left,y,NULL);
              LineTo(ps.hdc,right,y);
            }
            if (inf->hSubMenu) 
            {
               RECT r2=r; r2.left = r2.right - rcol; r2.right -= 4;
               DrawText(ps.hdc,">",-1,&r2,DT_VCENTER|DT_RIGHT|DT_SINGLELINE);
            }
            if (inf->fState&MF_CHECKED)
            {
               SetTextColor(ps.hdc,cols[dis]);
               RECT r2=r; r2.left = 0; r2.right=lcol;
               DrawText(ps.hdc,"X",-1,&r2,DT_VCENTER|DT_CENTER|DT_SINGLELINE);
            }
          }
          SelectObject(ps.hdc,oldbr);
          SelectObject(ps.hdc,oldpen);
          DeleteObject(br);
          DeleteObject(pen);
          DeleteObject(pen2);
          EndPaint(hwnd,&ps); 
        }       
      }
    break;
    case WM_TIMER:
      if (wParam==1)
      {
        HWND GetFocusIncludeMenus();
        HWND h = GetFocusIncludeMenus();
        if (h!=hwnd)
        {
          int a = h ? m_trackingMenus.Find(h) : -1;
          if (a<0 || a < m_trackingMenus.Find(hwnd)) 
          {
            if (m_trackingMouseFlag && m_trackingMenus.Get(0))
            {
              SetFocus(m_trackingMenus.Get(0));
              m_trackingMouseFlag=0;
            }
            else DestroyWindow(hwnd); 
          }
        }
      }
    break;
    case WM_KEYUP:
    return 1;
    case WM_KEYDOWN:
      if (wParam == VK_ESCAPE || (wParam == VK_LEFT && m_trackingMenus.GetSize()>1))
      {
        HWND l = m_trackingMenus.Get(m_trackingMenus.Find(hwnd)-1);
        if (l) SetFocus(l);
        else DestroyWindow(hwnd);
      }
      else if (wParam == VK_RETURN || wParam == VK_RIGHT)
      {
        HMENU__ *menu = (HMENU__*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (wParam == VK_RIGHT)
        {
          MENUITEMINFO *inf = menu->items.Get(menu->sel_vis);
          if (!inf || !inf->hSubMenu) return 1;
        }
        SendMessage(hwnd,WM_USER+100,1,menu->sel_vis);
      }
      else if (wParam == VK_UP)
      {
        HMENU__ *menu = (HMENU__*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (menu->sel_vis > 0)
        {
          menu->sel_vis--;
          InvalidateRect(hwnd,NULL,FALSE);
        }
      }
      else if (wParam == VK_DOWN)
      {
        HMENU__ *menu = (HMENU__*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (menu->sel_vis < menu->items.GetSize()-1)
        {
          menu->sel_vis++;
          InvalidateRect(hwnd,NULL,FALSE);
        }
      }
    return 1;
    case WM_DESTROY:
      {
        int a = m_trackingMenus.Find(hwnd);
        m_trackingMenus.Delete(a);
        if (m_trackingMenus.Get(a)) DestroyWindow(m_trackingMenus.Get(a));
        RemoveProp(hwnd,"SWELL_MenuOwner");
      }
    break;
    case WM_USER+100:
      if (wParam == 1 || wParam == 2 || wParam == 3 || wParam == 4)
      {
        int which = (int) lParam;
        int item_ypos = which;

        HMENU__ *menu = (HMENU__*)GetWindowLongPtr(hwnd,GWLP_USERDATA);

        int ht = 0;
        int x;
        HDC hdc=GetDC(hwnd);
        if (wParam > 1) which = -1;
        else item_ypos = 0;
        for (x=0; x < menu->items.GetSize(); x++)
        {
          if (wParam == 1 && which == x) { item_ypos = ht; break; }
          MENUITEMINFO *inf = menu->items.Get(x);
          int lastht = ht;
          if (inf->fType == MFT_STRING)
          {
            RECT r={0,};
            const char *str = inf->dwTypeData;
            if (!str || !*str) str="XXXXX";
            const char *pt2 = strstr(str,"\t");
            DrawText(hdc,str,pt2 ? (int)(pt2-str) : -1,&r,DT_CALCRECT|DT_SINGLELINE);
            ht += r.bottom + text_ht_pad;
          }
          else if (inf->fType == MFT_BITMAP)
          {
            BITMAP bm={16,16};
            if (inf->dwTypeData) GetObject((HBITMAP)inf->dwTypeData,sizeof(bm),&bm);
            ht += bm.bmHeight + bitmap_ht_pad;
          }
          else
          {
            ht += separator_ht;
          }
          if (wParam > 1 && item_ypos < ht) 
          { 
            item_ypos = lastht; 
            which = x; 
            if (wParam == 4 && inf->hSubMenu) 
            {
              HWND nextmenu = m_trackingMenus.Get(m_trackingMenus.Find(hwnd)+1);
              if (!nextmenu || GetWindowLongPtr(nextmenu,GWLP_USERDATA) != (LPARAM)inf->hSubMenu) wParam = 1; // activate if not already visible
            }
            break; 
          }
        }
        ReleaseDC(hwnd,hdc);
        if (wParam == 3 || wParam == 4)
        {
          MENUITEMINFO *inf = menu->items.Get(which);
          HWND next = m_trackingMenus.Get(m_trackingMenus.Find(hwnd)+1);
          if (next && inf && (!inf->hSubMenu || (LPARAM)inf->hSubMenu != GetWindowLongPtr(next,GWLP_USERDATA))) DestroyWindow(next); 
          menu->sel_vis = which;
          return 0;
        }

        MENUITEMINFO *inf = menu->items.Get(which);

        if (inf) 
        {
          if (inf->fState&MF_GRAYED){ }
          else if (inf->hSubMenu)
          {
            HWND next = m_trackingMenus.Get(m_trackingMenus.Find(hwnd)+1);
            if (next) DestroyWindow(next); 

            RECT r;
            GetClientRect(hwnd,&r);
            m_trackingPt.x=r.right;
            m_trackingPt.y=item_ypos;
            ClientToScreen(hwnd,&m_trackingPt);
            HWND hh;
            inf->hSubMenu->sel_vis=-1;
            submenuWndProc(hh=new HWND__(NULL,0,NULL,"menu",false,submenuWndProc,NULL, hwnd),WM_CREATE,0,(LPARAM)inf->hSubMenu);
            SetProp(hh,"SWELL_MenuOwner",GetProp(hwnd,"SWELL_MenuOwner"));
            InvalidateRect(hwnd,NULL,FALSE);
          }
          else if (inf->wID) m_trackingRet = inf->wID;
        }
      }
    return 0;
    case WM_MOUSEMOVE:
      {
        RECT r;
        GetClientRect(hwnd,&r);
        HMENU__ *menu = (HMENU__*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        const int oldsel = menu->sel_vis;
        if (GET_X_LPARAM(lParam)>=r.left && GET_X_LPARAM(lParam)<r.right)
        {
          int mode = 4;//GET_X_LPARAM(lParam) >= r.right - rcol*2 ? 4 : 3;
          SendMessage(hwnd,WM_USER+100,mode,GET_Y_LPARAM(lParam));
        }
        else menu->sel_vis = -1;
        if (oldsel != menu->sel_vis) InvalidateRect(hwnd,NULL,FALSE);
      }
    return 0;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
      {
        RECT r;
        GetClientRect(hwnd,&r);
        if (GET_X_LPARAM(lParam)>=r.left && GET_X_LPARAM(lParam)<r.right)
        {
          SendMessage(hwnd,WM_USER+100,2,GET_Y_LPARAM(lParam));
          return 0;
        }
        else DestroyWindow(hwnd);
      }
    return 0;
  }
  return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

int TrackPopupMenu(HMENU hMenu, int flags, int xpos, int ypos, int resvd, HWND hwnd, const RECT *r)
{
  if (!hMenu || m_trackingMenus.GetSize()) return 0;

  ReleaseCapture();
  m_trackingPar=hwnd;
  m_trackingFlags=flags;
  m_trackingRet=-1;
  m_trackingPt.x=xpos;
  m_trackingPt.y=ypos;
  m_trackingMouseFlag = 0;
  if (GetAsyncKeyState(VK_LBUTTON)) m_trackingMouseFlag |= 1;
  if (GetAsyncKeyState(VK_RBUTTON)) m_trackingMouseFlag |= 2;
  if (GetAsyncKeyState(VK_MBUTTON)) m_trackingMouseFlag |= 4;

//  HWND oldFoc = GetFocus();
 // bool oldFoc_child = oldFoc && (IsChild(hwnd,oldFoc) || oldFoc == hwnd || oldFoc==GetParent(hwnd));

  if (hwnd) hwnd->Retain();

  hMenu->sel_vis=-1;
  HWND hh=new HWND__(NULL,0,NULL,"menu",false,submenuWndProc,NULL, hwnd);

  submenuWndProc(hh,WM_CREATE,0,(LPARAM)hMenu);

  SetProp(hh,"SWELL_MenuOwner",(HANDLE)hwnd);

  while (m_trackingRet<0 && m_trackingMenus.GetSize())
  {
    void SWELL_RunMessageLoop();
    SWELL_RunMessageLoop();
    Sleep(10);
  }

  int x=m_trackingMenus.GetSize()-1;
  while (x>=0)
  {
    HWND h = m_trackingMenus.Get(x);
    m_trackingMenus.Delete(x);
    if (h) DestroyWindow(h);
    x--;
  }

//  if (oldFoc_child) SetFocus(oldFoc);

  if (!(flags&TPM_NONOTIFY) && m_trackingRet>0) 
    SendMessage(hwnd,WM_COMMAND,m_trackingRet,0);
  
  if (hwnd) hwnd->Release();

  return m_trackingRet>0?m_trackingRet:0;
}




void SWELL_Menu_AddMenuItem(HMENU hMenu, const char *name, int idx, unsigned int flags)
{
  MENUITEMINFO mi={sizeof(mi),MIIM_ID|MIIM_STATE|MIIM_TYPE,MFT_STRING,
    (flags)?MFS_GRAYED:0,idx,NULL,NULL,NULL,0,(char *)name};
  if (!name)
  {
    mi.fType = MFT_SEPARATOR;
    mi.fMask&=~(MIIM_STATE|MIIM_ID);
  }
  InsertMenuItem(hMenu,GetMenuItemCount(hMenu),TRUE,&mi);
}


SWELL_MenuResourceIndex *SWELL_curmodule_menuresource_head; // todo: move to per-module thingy

static SWELL_MenuResourceIndex *resById(SWELL_MenuResourceIndex *head, const char *resid)
{
  SWELL_MenuResourceIndex *p=head;
  while (p)
  {
    if (p->resid == resid) return p;
    p=p->_next;
  }
  return 0;
}

HMENU SWELL_LoadMenu(SWELL_MenuResourceIndex *head, const char *resid)
{
  SWELL_MenuResourceIndex *p;
  
  if (!(p=resById(head,resid))) return 0;
  HMENU hMenu=CreatePopupMenu();
  if (hMenu) p->createFunc(hMenu);
  return hMenu;
}

HMENU SWELL_DuplicateMenu(HMENU menu)
{
  if (!menu) return 0;
  return menu->Duplicate();
}

BOOL  SetMenu(HWND hwnd, HMENU menu)
{
  if (!hwnd) return 0;
  HMENU oldmenu = hwnd->m_menu;

  hwnd->m_menu = menu;
  
  if (!hwnd->m_parent && !!hwnd->m_menu != !!oldmenu)
  {
    WNDPROC oldwc = hwnd->m_wndproc;
    hwnd->m_wndproc = DefWindowProc;
    RECT r;
    GetWindowRect(hwnd,&r);

    if (oldmenu) r.bottom -= SWELL_INTERNAL_MENUBAR_SIZE; // hack: we should WM_NCCALCSIZE before and after, really
    else r.bottom += SWELL_INTERNAL_MENUBAR_SIZE;

    SetWindowPos(hwnd,NULL,0,0,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);
    hwnd->m_wndproc = oldwc;
    // resize
  }

  return TRUE;
}

HMENU GetMenu(HWND hwnd)
{
  if (!hwnd) return 0;
  return hwnd->m_menu;
}

void DrawMenuBar(HWND hwnd)
{
  InvalidateRect(hwnd,NULL,FALSE);
}


// copied from swell-menu.mm, can have a common impl someday
int SWELL_GenerateMenuFromList(HMENU hMenu, const void *_list, int listsz)
{
  SWELL_MenuGen_Entry *list = (SWELL_MenuGen_Entry *)_list;
  const int l1=strlen(SWELL_MENUGEN_POPUP_PREFIX);
  while (listsz>0)
  {
    int cnt=1;
    if (!list->name) SWELL_Menu_AddMenuItem(hMenu,NULL,-1,0);
    else if (!strcmp(list->name,SWELL_MENUGEN_ENDPOPUP)) return list + 1 - (SWELL_MenuGen_Entry *)_list;
    else if (!strncmp(list->name,SWELL_MENUGEN_POPUP_PREFIX,l1)) 
    { 
      MENUITEMINFO mi={sizeof(mi),MIIM_SUBMENU|MIIM_STATE|MIIM_TYPE,MFT_STRING,0,0,CreatePopupMenuEx(list->name+l1),NULL,NULL,0,(char *)list->name+l1};
      cnt += SWELL_GenerateMenuFromList(mi.hSubMenu,list+1,listsz-1);
      InsertMenuItem(hMenu,GetMenuItemCount(hMenu),TRUE,&mi);
    }
    else SWELL_Menu_AddMenuItem(hMenu,list->name,list->idx,list->flags);

    list+=cnt;
    listsz -= cnt;
  }
  return list + 1 - (SWELL_MenuGen_Entry *)_list;
}
#endif
