/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Linux)
   Copyright (C) 2006-2008, Cockos, Inc.

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
  

    This file provides basic windows APIs for handling windows, as well as the stubs to enable swell-dlggen to work.
    GTK version.

  */


#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include <gtk/gtk.h>
#include "../mutex.h"
#include "../ptrlist.h"
#include "../queue.h"

#include "swell-dlggen.h"
#include "swell-internal.h"

HWND SWELL_g_focuswnd; // update from focus-in-event / focus-out-event signals, have to enable the GDK_FOCUS_CHANGE_MASK bits for the gdkwindow

HWND GetParent(HWND hwnd)
{  
  return hwnd ? hwnd->m_parent : NULL;
}

HWND GetDlgItem(HWND hwnd, int idx)
{
  if (!idx) return hwnd;

  if (hwnd) hwnd=hwnd->m_children;
  while (hwnd) 
  {
    if (hwnd->m_id == idx) return hwnd;
    hwnd=hwnd->m_next;
  }
  return 0;
}


LONG SetWindowLong(HWND hwnd, int idx, LONG val)
{
  if (!hwnd) return 0;
  if (idx==GWL_STYLE)
  {
     // todo: special case for buttons
    LONG ret = hwnd->m_style;
    hwnd->m_style=val;
    return ret;
  }
  if (idx==GWL_EXSTYLE)
  {
    LONG ret = hwnd->m_exstyle;
    hwnd->m_exstyle=val;
    return ret;
  }
  if (idx==GWL_USERDATA)
  {
    LONG ret = hwnd->m_userdata;
    hwnd->m_userdata=val;
    return ret;
  }
  if (idx==GWL_ID)
  {
    LONG ret = hwnd->m_id;
    hwnd->m_id=val;
    return ret;
  }
  
  if (idx==GWL_WNDPROC)
  {
    LONG ret = (LONG)hwnd->m_wndproc;
    hwnd->m_wndproc=(WNDPROC)val;
    return ret;
  }
  if (idx==DWL_DLGPROC)
  {
    LONG ret = (LONG)hwnd->m_dlgproc;
    hwnd->m_dlgproc=(DLGPROC)val;
    return ret;
  }
  
  if (idx>=0 && idx < 64*(int)sizeof(INT_PTR))
  {
    INT_PTR ret = hwnd->m_extra[idx/sizeof(INT_PTR)];
    hwnd->m_extra[idx/sizeof(INT_PTR)]=val;
    return (LONG)ret;
  }
  return 0;
}

LONG GetWindowLong(HWND hwnd, int idx)
{
  if (!hwnd) return 0;
  if (idx==GWL_STYLE)
  {
     // todo: special case for buttons
    return hwnd->m_style;
  }
  if (idx==GWL_EXSTYLE)
  {
    return hwnd->m_exstyle;
  }
  if (idx==GWL_USERDATA)
  {
    return hwnd->m_userdata;
  }
  if (idx==GWL_ID)
  {
    return hwnd->m_id;
  }
  
  if (idx==GWL_WNDPROC)
  {
    return (LONG)hwnd->m_wndproc;
  }
  if (idx==DWL_DLGPROC)
  {
    return (LONG)hwnd->m_dlgproc;
  }
  
  if (idx>=0 && idx < 64*(int)sizeof(INT_PTR))
  {
    return (LONG)hwnd->m_extra[idx/sizeof(INT_PTR)];
  }
  return 0;
}


bool IsWindowVisible(HWND hwnd)
{
  if (!hwnd) return false;
  while (hwnd->m_visible)
  {
    hwnd = hwnd->m_parent;
    if (!hwnd) return true;
  }
  return false;
}

int SendMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!hwnd) return 0;
  WNDPROC wp = hwnd->m_wndproc;

  if (msg == WM_DESTROY)
  {
    if (hwnd->m_hashaddestroy) return 0;// todo: allow certain messages to pass?
    hwnd->m_hashaddestroy=true;
    if (GetCapture()==hwnd) ReleaseCapture(); 
    SWELL_MessageQueue_Clear(hwnd);
  }
  else if (msg==WM_CAPTURECHANGED && hwnd->m_hashaddestroy) return 0;
    
  int ret = wp ? wp(hwnd,msg,wParam,lParam) : 0;
 
  if (msg == WM_DESTROY)
  {
    if (GetCapture()==hwnd) ReleaseCapture(); 

    SWELL_MessageQueue_Clear(hwnd);
    // send WM_DESTROY to all children
    HWND tmp=hwnd->m_children;
    while (tmp)
    {
      SendMessage(tmp,WM_DESTROY,0,0);
      tmp=tmp->m_next;
    }
    tmp=hwnd->m_owned;
    while (tmp)
    {
      SendMessage(tmp,WM_DESTROY,0,0);
      tmp=tmp->m_next;
    }
    KillTimer(hwnd,-1);
    if (SWELL_g_focuswnd == hwnd) SWELL_g_focuswnd=0;
  }
  if (wp) return ret;
  
#if 0 // special cases for some controls
    if (msg == BM_SETCHECK && [turd isKindOfClass:[NSButton class]])
    {
      [(NSButton*)turd setState:(wParam&BST_INDETERMINATE)?NSMixedState:((wParam&BST_CHECKED)?NSOnState:NSOffState)];
      return 0;
    }
    if ((msg==BM_GETIMAGE || msg == BM_SETIMAGE) && [turd isKindOfClass:[SWELL_Button class]])
    {
      if (wParam != IMAGE_BITMAP && wParam != IMAGE_ICON) return 0; // ignore unknown types
      LONG ret=(LONG) (void *)[turd getSwellGDIImage];
      if (msg==BM_SETIMAGE)
      {
        NSImage *img=NULL;
        if (lParam) img=(NSImage *)__GetNSImageFromHICON((HICON)lParam);
        [turd setImage:img];
        [turd setSwellGDIImage:(void *)(img?lParam:0)];
      }
      return ret;
    }
    else if (msg >= CB_ADDSTRING && msg <= CB_INITSTORAGE && ([turd isKindOfClass:[NSPopUpButton class]] || [turd isKindOfClass:[NSComboBox class]]))
    {
        switch (msg)
        {
          case CB_ADDSTRING: return SWELL_CB_AddString(hwnd,0,(char*)lParam); 
          case CB_DELETESTRING: SWELL_CB_DeleteString(hwnd,0,wParam); return 1;
          case CB_GETCOUNT: return SWELL_CB_GetNumItems(hwnd,0);
          case CB_GETCURSEL: return SWELL_CB_GetCurSel(hwnd,0);
          case CB_GETLBTEXT: return SWELL_CB_GetItemText(hwnd,0,wParam,(char *)lParam, 1024);  
          case CB_INSERTSTRING: return SWELL_CB_InsertString(hwnd,0,wParam,(char *)lParam);
          case CB_RESETCONTENT: SWELL_CB_Empty(hwnd,0); return 0;
          case CB_SETCURSEL: SWELL_CB_SetCurSel(hwnd,0,wParam); return 0;
          case CB_GETITEMDATA: return SWELL_CB_GetItemData(hwnd,0,wParam);
          case CB_SETITEMDATA: SWELL_CB_SetItemData(hwnd,0,wParam,lParam);
          case CB_FINDSTRING:
          case CB_FINDSTRINGEXACT:
            // todo: implement
          return 0;
          case CB_INITSTORAGE: return 0;                                                      
        }
        return 0;
    }
    else if (msg >= TBM_GETPOS && msg <= TBM_SETRANGE && ([turd isKindOfClass:[NSSlider class]]))
    {
        switch (msg)
        {
          case TBM_GETPOS: return SWELL_TB_GetPos(hwnd,0);
          case TBM_SETTIC: SWELL_TB_SetTic(hwnd,0,lParam); return 1;
          case TBM_SETPOS: SWELL_TB_SetPos(hwnd,0,lParam); return 1;
          case TBM_SETRANGE: SWELL_TB_SetRange(hwnd,0,LOWORD(lParam),HIWORD(lParam)); return 1;
        }
        return 0;
    }
    else
    {
      NSWindow *w;
      NSView *v;
      // if content view gets unhandled message send to window
      if ([turd isKindOfClass:[NSView class]] && (w=[turd window]) && [w contentView] == turd && [w respondsToSelector:@selector(onSwellMessage:p1:p2:)])
      {
        return (int) [(SWELL_hwndChild *)w onSwellMessage:msg p1:wParam p2:lParam];
      }
      // if window gets unhandled message send to content view
      else if ([turd isKindOfClass:[NSWindow class]] && (v=[turd contentView]) && [v respondsToSelector:@selector(onSwellMessage:p1:p2:)])
      {
        return (int) [(SWELL_hwndChild *)v onSwellMessage:msg p1:wParam p2:lParam];
      }
    }
#endif
  return 0;
}

static void RecurseDestroyWindow(HWND hwnd)
{
  HWND tmp=hwnd->m_children;
  while (tmp)
  {
    HWND old = tmp;
    tmp=tmp->m_next;
    RecurseDestroyWindow(old);
  }
  tmp=hwnd->m_owned;
  while (tmp)
  {
    HWND old = tmp;
    tmp=tmp->m_next;
    RecurseDestroyWindow(old);
  }

  delete hwnd;
}

void DestroyWindow(HWND hwnd)
{
  if (!hwnd) return;
  if (hwnd->m_hashaddestroy) return; 
 
  SendMessage(hwnd,WM_DESTROY,0,0);

  if (hwnd->m_next) { hwnd->m_next->m_prev = hwnd->m_prev; } // update next item to point to previous

  // update previous item to point to next
  if (hwnd->m_prev) { hwnd->m_prev->m_next = hwnd->m_next; } 

  // update parent's children pointer if it points here
  if (hwnd->m_parent && hwnd->m_parent->m_children == hwnd) hwnd->m_parent->m_children = hwnd->m_next;

  if (hwnd->m_owner && hwnd->m_owner->m_owned == hwnd) hwnd->m_owner->m_owned = hwnd->m_next;

  // ok window is now out of the list, delete any children etc. they've all had their WM_DESTROYs etc so all we need to do is delete em.
  RecurseDestroyWindow(hwnd);

}


void EnableWindow(HWND hwnd, int enable)
{
  if (!hwnd) return;
  hwnd->m_enabled=!!enable;
  if (hwnd->m_widget) gtk_widget_set_sensitive(hwnd->m_widget, !!enable);
}


void SetForegroundWindow(HWND hwnd)
{
  if (!hwnd) return;
  if (hwnd->m_widget) gtk_widget_grab_focus(hwnd->m_widget);
}

void SetFocus(HWND hwnd)
{
  if (!hwnd) return;
  if (hwnd->m_widget) gtk_widget_grab_focus(hwnd->m_widget);
}

int IsChild(HWND hwndParent, HWND hwndChild)
{
  if (!hwndParent || !hwndChild || hwndParent == hwndChild) return 0;
  while (hwndChild)
  { 
    hwndChild = hwndChild->m_parent;
    if (hwndChild == hwndParent) return 1;
  }
  return false;
}


HWND GetForegroundWindow()
{
  return SWELL_g_focuswnd;
}

HWND GetFocus()
{
  return SWELL_g_focuswnd;
}

void SWELL_GetViewPort(RECT *r, RECT *sourcerect, bool wantWork)
{
  GdkScreen *defscr = gdk_screen_get_default();
  if (!defscr) { r->left=r->top=0; r->right=r->bottom=1024; return; }
  gint idx = sourcerect ? gdk_screen_get_monitor_at_point(defscr,
         (sourcerect->left+sourcerect->right)/2,
         (sourcerect->top+sourcerect->bottom)/2) : 0;
  GdkRectangle rc={0,0,1024,1024};
  gdk_screen_get_monitor_geometry(defscr,idx,&rc);
  r->left=rc.x; r->top = rc.y;
  r->right=rc.x+rc.width;
  r->bottom=rc.y+rc.height;
}


void ScreenToClient(HWND hwnd, POINT *p)
{
  if (!hwnd) return;
  
  gint x=p->x,y=p->y;

  HWND tmp=hwnd, ltmp=0;
  while (tmp) // top level window's m_position left/top should always be 0 anyway
  {
    x -= tmp->m_position.left;
    y -= tmp->m_position.top;
    ltmp=tmp;
    tmp = tmp->m_parent;
  }
#if 1
  // make optional if old gtk2.13- is used?
  if (ltmp && ltmp->m_widget)
  {
    GdkWindow *wnd = gtk_widget_get_window(ltmp->m_widget);
    if (wnd) 
    {
      gint px=0,py=0;
      gdk_window_get_position(wnd,&px,&py); // this is probably unreliable but ugh
      x-=px;
      y-=py;
    }
  }
#endif

  p->x=x;
  p->y=y;
}

void ClientToScreen(HWND hwnd, POINT *p)
{
  if (!hwnd) return;
  
  gint x=p->x,y=p->y;

  HWND tmp=hwnd,ltmp=0;
  while (tmp) // top level window's m_position left/top should always be 0 anyway
  {
    x += tmp->m_position.left;
    y += tmp->m_position.top;
    ltmp=tmp;
    tmp = tmp->m_parent;
  }
#if 1
  // make optional if old gtk2.13- is used?
  if (ltmp && ltmp->m_widget)
  {
    GdkWindow *wnd = gtk_widget_get_window(ltmp->m_widget);
    if (wnd) 
    {
      gint px=0,py=0;
      gdk_window_get_position(wnd,&px,&py); // this is probably unreliable but ugh
      x+=px;
      y+=py;
    }
  }
#endif

  p->x=x;
  p->y=y;
}

bool GetWindowRect(HWND hwnd, RECT *r)
{
  if (!hwnd) return false;
  GetClientRect(hwnd,r);
  ClientToScreen(hwnd,(LPPOINT)r);
  ClientToScreen(hwnd,((LPPOINT)r)+1);
  return true;
}

void GetWindowContentViewRect(HWND hwnd, RECT *r)
{
  GetWindowRect(hwnd,r);
}


void GetClientRect(HWND hwnd, RECT *r)
{
  r->left=r->top=r->right=r->bottom=0;
  if (!hwnd) return;
  
  r->right = hwnd->m_position.right - hwnd->m_position.left;
  r->bottom = hwnd->m_position.bottom - hwnd->m_position.top;
}



void SetWindowPos(HWND hwnd, HWND unused, int x, int y, int cx, int cy, int flags)
{
  if (!hwnd) return;
 
 // todo: handle SWP_SHOWWINDOW
  RECT f = hwnd->m_position;
  bool repos=false;
  if (!(flags&SWP_NOMOVE))
  {
    int oldw = f.right-f.left;
    int oldh = f.bottom-f.top; 
    f.left=x; 
    f.right=x+oldw;
    f.top=y; 
    f.bottom=y+oldh;
    repos=true;
  }
  if (!(flags&SWP_NOSIZE))
  {
    f.right = f.left + cx;
    f.bottom = f.top + cy;
    repos=true;
  }
  if (repos)
  {
    if (!hwnd->m_parent)
    { 
      hwnd->m_position.left = hwnd->m_position.top = 0;
      hwnd->m_position.right = f.right-f.left;
      hwnd->m_position.bottom = f.bottom-f.top;
      if (hwnd->m_widget)
      {
        // todo: gdkwindow move etc?
      }
    }
    else 
    {
      hwnd->m_position = f;
      if (hwnd->m_widget) 
      {
        // todo: notify parent container to move?
      }
      SendMessage(hwnd,WM_SIZE,0,0);
      InvalidateRect(hwnd,NULL,FALSE);
    }
  }  
  
}



HWND GetWindow(HWND hwnd, int what)
{
  if (!hwnd) return 0;
  
  if (what == GW_CHILD) return hwnd->m_children;
  if (what == GW_OWNER) return hwnd->m_owner;
  if (what == GW_HWNDNEXT) return hwnd->m_next;
  if (what == GW_HWNDPREV) return hwnd->m_prev;
  if (what == GW_HWNDFIRST) 
  { 
    while (hwnd->m_prev) hwnd = hwnd->m_prev;
    return hwnd;
  }
  if (what == GW_HWNDLAST) 
  { 
    while (hwnd->m_next) hwnd = hwnd->m_next;
    return hwnd;
  }
  return 0;
}

#if 0



HWND SetParent(HWND hwnd, HWND newPar)
{
  NSView *v=(NSView *)hwnd;
  if (!v || ![(id)v isKindOfClass:[NSView class]]) return 0;
  v=NavigateUpScrollClipViews(v);
  
  if ([(id)hwnd isKindOfClass:[NSView class]])
  {
    NSView *tv=(NSView *)hwnd;
    if ([[tv window] contentView] == tv) // if we're reparenting a contentview (aka top level window)
    {
      if (!newPar) return NULL;
    
      NSView *npv = (NSView *)newPar;
      if ([npv isKindOfClass:[NSWindow class]]) npv=[(NSWindow *)npv contentView];
      if (!npv || ![npv isKindOfClass:[NSView class]])
        return NULL;
    
      char oldtitle[2048];
      oldtitle[0]=0;
      GetWindowText(hwnd,oldtitle,sizeof(oldtitle));
    
      NSWindow *oldwnd = [tv window];
    
      [tv retain];
      SWELL_hwndChild *tmpview = [[SWELL_hwndChild alloc] initChild:nil Parent:(NSView *)oldwnd dlgProc:nil Param:0];          
      [tmpview release];
    
      [npv addSubview:tv];  
      [tv release];
    
      DestroyWindow((HWND)oldwnd); // close old window since its no longer used
      if (oldtitle[0]) SetWindowText(hwnd,oldtitle);
      return (HWND)npv;
    }
    else if (!newPar) // not content view, not parent (so making it a top level modeless dialog)
    {
      NSWindow *oldw = [tv window];
      char oldtitle[2048];
      oldtitle[0]=0;
      GetWindowText(hwnd,oldtitle,sizeof(oldtitle));
      
      [tv retain];
      [tv removeFromSuperview];

    
      unsigned int wf=(NSTitledWindowMask|NSMiniaturizableWindowMask|NSClosableWindowMask|NSResizableWindowMask);
      if ([tv respondsToSelector:@selector(swellCreateWindowFlags)])
        wf=(unsigned int)[(SWELL_hwndChild *)tv swellCreateWindowFlags];
      HWND SWELL_CreateModelessFrameForWindow(HWND childW, HWND ownerW, unsigned int);
      HWND bla=SWELL_CreateModelessFrameForWindow((HWND)tv,(HWND)oldw,wf);
      // create a new modeless frame 
      
      [(NSWindow *)bla display];
      
      [tv release];
      
      if (oldtitle[0]) SetWindowText(hwnd,oldtitle);
      
      return NULL;
      
    }
  }
  HWND ret=(HWND) [v superview];
  if (ret) 
  {
    [v retain];
    [v removeFromSuperview];
  }
  NSView *np=(NSView *)newPar;
  if (np && [np isKindOfClass:[NSWindow class]]) np=[(NSWindow *)np contentView];
  
  if (np && [np isKindOfClass:[NSView class]])
  {
    [np addSubview:v];
    [v release];
  }
  return ret;
}




#endif

// timer stuff
typedef struct
{
  int timerid;
  HWND hwnd;
  guint glib_timerid;
} TimerInfoRec;
static WDL_PtrList<TimerInfoRec> m_timerlist;
static WDL_Mutex m_timermutex;
static pthread_t m_pmq_mainthread;
static void SWELL_pmq_settimer(HWND h, int timerid, int rate);

static gboolean settimer_callback(gpointer data)
{
  TimerInfoRec *rec=(TimerInfoRec*)data;
  if (rec)
  {
    SendMessage(rec->hwnd,WM_TIMER,rec->timerid,0);
  }
  return TRUE;
}

int SetTimer(HWND hwnd, int timerid, int rate, unsigned long *notUsed)
{
  if (!hwnd) return 0;
  
  if (timerid != -1 && m_pmq_mainthread && pthread_self()!=m_pmq_mainthread)
  {
    SWELL_pmq_settimer(hwnd,timerid,rate);
    return timerid;
  }

  WDL_MutexLock lock(&m_timermutex);
  KillTimer(hwnd,timerid);
  TimerInfoRec *rec=(TimerInfoRec*)malloc(sizeof(TimerInfoRec));
  rec->timerid=timerid;
  rec->hwnd=hwnd;
  rec->glib_timerid = g_timeout_add(max(rate,1),settimer_callback,rec);
  
  m_timerlist.Add(rec);
  
  return timerid;
}

void KillTimer(HWND hwnd, int timerid)
{
  WDL_MutexLock lock(&m_timermutex);
  if (timerid != -1 && m_pmq_mainthread && pthread_self()!=m_pmq_mainthread)
  {
    SWELL_pmq_settimer(hwnd,timerid,-1);
    return;
  }
  int x;
  for (x = 0; x < m_timerlist.GetSize(); x ++)
  {
    if ((timerid==-1 || m_timerlist.Get(x)->timerid == timerid) && m_timerlist.Get(x)->hwnd == hwnd)
    {
      g_source_remove(m_timerlist.Get(x)->glib_timerid);
      m_timerlist.Delete(x--,true,free);
      if (timerid!=-1) break;
    }
  }
}


#if 0

BOOL SetDlgItemText(HWND hwnd, int idx, const char *text)
{
  NSView *poo=(NSView *)(idx ? GetDlgItem(hwnd,idx) : hwnd);
  if (!poo) return false;
  
  NSWindow *nswnd;
  if ([(id)poo isKindOfClass:[NSView class]] && (nswnd=[(NSView *)poo window]) && [nswnd contentView]==(id)poo)
    SetDlgItemText((HWND)nswnd,0,text); // also set window if setting content view
    
  if ([poo respondsToSelector:@selector(onSwellSetText:)])
  {
    [(SWELL_hwndChild*)poo onSwellSetText:text];
    return TRUE;
  }
  
  BOOL rv=TRUE;  
  NSString *lbl=(NSString *)SWELL_CStringToCFString(text);
  if ([poo isKindOfClass:[NSWindow class]] || [poo isKindOfClass:[NSButton class]]) [(NSButton*)poo setTitle:lbl];
  else if ([poo isKindOfClass:[NSControl class]]) 
  {
    [(NSControl*)poo setStringValue:lbl];
    if ([poo isKindOfClass:[NSTextField class]] && [(NSTextField *)poo isEditable])
    {
      SendMessage(GetParent((HWND)poo),WM_COMMAND,[(NSControl *)poo tag]|(EN_CHANGE<<16),(LPARAM)poo);
    }
  }
  else if ([poo isKindOfClass:[NSText class]])  [(NSText*)poo setString:lbl];
  else rv=FALSE;
  
  [lbl release];
  return rv;
}

BOOL GetDlgItemText(HWND hwnd, int idx, char *text, int textlen)
{
  *text=0;
  NSView *poo=(NSView *)(idx?GetDlgItem(hwnd,idx) : hwnd);
  if (!poo) return false;
  
  if ([(id)poo isKindOfClass:[NSView class]] && [[(id)poo window] contentView] == poo)
  {
    poo=(NSView *)[(id)poo window];
  }
  
  if ([(id)poo respondsToSelector:@selector(onSwellGetText)])
  {  
    const char *p=(const char *)[(SWELL_hwndChild*)poo onSwellGetText];
    lstrcpyn(text,p?p:"",textlen);
    return TRUE;
  }
  
  NSString *s;
  
  if ([poo isKindOfClass:[NSButton class]]||[poo isKindOfClass:[NSWindow class]]) s=[((NSButton *)poo) title];
  else if ([poo isKindOfClass:[NSControl class]]) s=[((NSControl *)poo) stringValue];
  else if ([poo isKindOfClass:[NSText class]])  s=[(NSText*)poo string];
  else return FALSE;
  
  if (s)
    [s getCString:text maxLength:textlen];
    
  return !!s;
}

void CheckDlgButton(HWND hwnd, int idx, int check)
{
  NSView *poo=(NSView *)GetDlgItem(hwnd,idx);
  if (!poo) return;
  if ([poo isKindOfClass:[NSButton class]]) [(NSButton*)poo setState:(check&BST_INDETERMINATE)?NSMixedState:((check&BST_CHECKED)?NSOnState:NSOffState)];
}


int IsDlgButtonChecked(HWND hwnd, int idx)
{
  NSView *poo=(NSView *)GetDlgItem(hwnd,idx);
  if (poo && [poo isKindOfClass:[NSButton class]])
  {
    int a=[(NSButton*)poo state];
    if (a==NSMixedState) return BST_INDETERMINATE;
    return a!=NSOffState;
  }
  return 0;
}

void SWELL_TB_SetPos(HWND hwnd, int idx, int pos)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (p  && [p isKindOfClass:[NSSlider class]]) 
  {
    [p setDoubleValue:(double)pos];
  }
  else if (p && [p respondsToSelector:@selector(onSwellMessage:p1:p2:)])
  {
    [(SWELL_hwndChild *)p onSwellMessage:TBM_SETPOS p1:1 p2:pos];
  }
}

void SWELL_TB_SetRange(HWND hwnd, int idx, int low, int hi)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (p && [p isKindOfClass:[NSSlider class]])
  {
    [p setMinValue:low];
    [p setMaxValue:hi];
  }
  else if (p && [p respondsToSelector:@selector(onSwellMessage:p1:p2:)])
  {
    [(SWELL_hwndChild *)p onSwellMessage:TBM_SETRANGE p1:1 p2:(low|(hi<<16))];
  }
  
}

int SWELL_TB_GetPos(HWND hwnd, int idx)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (p && [p isKindOfClass:[NSSlider class]]) 
  {
    return (int) ([p doubleValue]+0.5);
  }
  else if (p && [p respondsToSelector:@selector(onSwellMessage:p1:p2:)])
  {
    return (int) [(SWELL_hwndChild *)p onSwellMessage:TBM_GETPOS p1:0 p2:0];
  }
  return 0;
}

void SWELL_TB_SetTic(HWND hwnd, int idx, int pos)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (p && [p respondsToSelector:@selector(onSwellMessage:p1:p2:)])
  {
    [(SWELL_hwndChild *)p onSwellMessage:TBM_SETTIC p1:0 p2:pos];
  }
}

void SWELL_CB_DeleteString(HWND hwnd, int idx, int wh)
{
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  if (!p) return;
  if ([p isKindOfClass:[SWELL_ComboBox class]])
  {
    if (wh>=0 && wh<[p numberOfItems])
    {
      [p removeItemAtIndex:wh];
      if (((SWELL_ComboBox*)p)->m_ids) ((SWELL_ComboBox*)p)->m_ids->Delete(wh);
    }
  }
  else if ( [p isKindOfClass:[NSPopUpButton class]])
  {
    NSMenu *menu = [p menu];
    if (menu)
    {
      if (wh >= 0 && wh < [menu numberOfItems])
        [menu removeItemAtIndex:wh];
    }
  }
}


int SWELL_CB_GetItemText(HWND hwnd, int idx, int item, char *buf, int bufsz)
{
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);

  *buf=0;
  if (!p) return 0;
  int ni=[p numberOfItems];
  if (item < 0 || item >= ni) return 0;
  
  if ([p isKindOfClass:[NSComboBox class]])
  {
    NSString *s=[p itemObjectValueAtIndex:item];
    if (s)
    {
      [s getCString:buf maxLength:bufsz];
      return 1;
    }
  }
  else 
  {
    NSMenuItem *i=[(NSPopUpButton *)p itemAtIndex:item];
    if (i)
    {
      NSString *s=[i title];
      if (s)
      {
        [s getCString:buf maxLength:bufsz];
        return 1;
      }
    }
  }
  return 0;
}


int SWELL_CB_InsertString(HWND hwnd, int idx, int pos, const char *str)
{
  NSString *label=(NSString *)SWELL_CStringToCFString(str);
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  if (!p) return 0;
  
  bool isAppend=false;
  int ni=[p numberOfItems];
  if (pos == -1000) 
  {
    isAppend=true;
    pos=ni;
  }
  else if (pos < 0) pos=0;
  else if (pos > ni) pos=ni;
  
   
  if ([p isKindOfClass:[SWELL_ComboBox class]])
  {
    if (isAppend && (((int)[(SWELL_ComboBox*)p getSwellStyle]) & CBS_SORT))
    {
      pos=arr_bsearch_mod(label,[p objectValues],_nsStringSearchProc);
    }
    
    if (pos==ni)
      [p addItemWithObjectValue:label];
    else
      [p insertItemWithObjectValue:label atIndex:pos];
  
    if (((SWELL_ComboBox*)p)->m_ids) ((SWELL_ComboBox*)p)->m_ids->Insert(pos,(char*)0);
    [p setNumberOfVisibleItems:(ni+1)];
  }
  else
  {
    NSMenu *menu = [(NSPopUpButton *)p menu];
    if (menu)
    {
      if (isAppend && [p respondsToSelector:@selector(getSwellStyle)] && (((int)[(SWELL_PopUpButton*)p getSwellStyle]) & CBS_SORT))
      {
        pos=arr_bsearch_mod(label,[menu itemArray],_nsMenuSearchProc);
      }
      NSMenuItem *item=[menu insertItemWithTitle:label action:NULL keyEquivalent:@"" atIndex:pos];
      [item setEnabled:YES];      
    }
  }
  [label release];
  return pos;
  
}

int SWELL_CB_AddString(HWND hwnd, int idx, const char *str)
{
  return SWELL_CB_InsertString(hwnd,idx,-1000,str);
}

int SWELL_CB_GetCurSel(HWND hwnd, int idx)
{
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  if (!p) return -1;
  return [p indexOfSelectedItem];
}

void SWELL_CB_SetCurSel(HWND hwnd, int idx, int item)
{
  if (item>=0 && item<SWELL_CB_GetNumItems(hwnd,idx))
    [(NSComboBox *)GetDlgItem(hwnd,idx) selectItemAtIndex:item];
}

int SWELL_CB_GetNumItems(HWND hwnd, int idx)
{
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  if (!p) return 0;
  return [p numberOfItems];
}



void SWELL_CB_SetItemData(HWND hwnd, int idx, int item, LONG data)
{
  id cb=(id)GetDlgItem(hwnd,idx);
  if (!cb) return;

  if ([cb isKindOfClass:[NSPopUpButton class]])
  {
    if (item < 0 || item >= [cb numberOfItems]) return;
    NSMenuItem *it=[cb itemAtIndex:item];
    if (!it) return;
  
    SWELL_DataHold *p=[[SWELL_DataHold alloc] initWithVal:(void *)data];  
    [it setRepresentedObject:p];
    [p release];
  }
  else if ([cb isKindOfClass:[SWELL_ComboBox class]])
  {
    if (item < 0 || item >= [cb numberOfItems]) return;
    if (((SWELL_ComboBox*)cb)->m_ids) ((SWELL_ComboBox*)cb)->m_ids->Set(item,(char*)data);
  }
}

LONG SWELL_CB_GetItemData(HWND hwnd, int idx, int item)
{
  id cb=(id)GetDlgItem(hwnd,idx);
  if (!cb) return 0;
  if ([cb isKindOfClass:[NSPopUpButton class]])
  {
    if (item < 0 || item >= [cb numberOfItems]) return 0;
    NSMenuItem *it=[cb itemAtIndex:item];
    if (!it) return 0;
    id p= [it representedObject];
    if (!p || ![p isKindOfClass:[SWELL_DataHold class]]) return 0;
    return (LONG) (void *)[p getValue];
  }
  else if ([cb isKindOfClass:[SWELL_ComboBox class]])
  {
    if (item < 0 || item >= [cb numberOfItems]) return 0;
    if (((SWELL_ComboBox*)cb)->m_ids) return (LONG) ((SWELL_ComboBox*)cb)->m_ids->Get(item);    
  }
  return 0;
}

void SWELL_CB_Empty(HWND hwnd, int idx)
{
  id cb=(id)GetDlgItem(hwnd,idx);
  if (!cb) return;  
  if ([cb isKindOfClass:[NSPopUpButton class]] ||
      [cb isKindOfClass:[NSComboBox class]]) [cb removeAllItems];
  
  if ([cb isKindOfClass:[SWELL_ComboBox class]])
  {
    if (((SWELL_ComboBox*)cb)->m_ids) ((SWELL_ComboBox*)cb)->m_ids->Empty(); 
  }
}


BOOL SetDlgItemInt(HWND hwnd, int idx, int val, int issigned)
{
  char buf[128];
  sprintf(buf,issigned?"%d":"%u",val);
  return SetDlgItemText(hwnd,idx,buf);
}

int GetDlgItemInt(HWND hwnd, int idx, BOOL *translated, int issigned)
{
  char buf[128];
  if (!GetDlgItemText(hwnd,idx,buf,sizeof(buf)))
  {
    if (translated) *translated=0;
    return 0;
  }
  char *p=buf;
  while (*p == ' ' || *p == '\t') p++;
  int a=atoi(p);
  if ((a<0 && !issigned) || (!a && p[0] != '0')) { if (translated) *translated=0; return 0; }
  if (translated) *translated=1;
  return a;
}


void ShowWindow(HWND hwnd, int cmd)
{
  id pid=(id)hwnd;
  
  if (pid && [pid isKindOfClass:[NSWindow class]])
  {
    if (cmd==SW_SHOW)
    {
      [pid makeKeyAndOrderFront:pid];
    }
    else if (cmd==SW_SHOWNA)
    {
      [pid orderFront:pid];
    }
    else if (cmd==SW_HIDE)
    {
      [pid orderOut:pid];
    }
    return;
  }
  if (!pid || ![pid isKindOfClass:[NSView class]]) return;
  
  pid=NavigateUpScrollClipViews(pid);
  
  switch (cmd)
  {
    case SW_SHOW:
    case SW_SHOWNA:
      [((NSView *)pid) setHidden:NO];
    break;
    case SW_HIDE:
      {
        NSWindow *pw=[pid window];
        if (pw && [NSApp keyWindow] == pw)
        {
          id foc=[pw firstResponder];
          if (foc && (foc == pid || IsChild((HWND)pid,(HWND)foc)))
          {
            HWND h=GetParent((HWND)pid);
            if (h) SetFocus(h);
          }
        }
        [((NSView *)pid) setHidden:YES];
      }
    break;
  }
  
  NSWindow *nswnd;
  if ((nswnd=[(NSView *)pid window]) && [nswnd contentView]==(id)pid)
    ShowWindow((HWND)nswnd,cmd);
}

void *SWELL_ModalWindowStart(HWND hwnd)
{
  if (hwnd && [(id)hwnd isKindOfClass:[NSView class]]) hwnd=(HWND)[(NSView *)hwnd window];
  if (!hwnd) return 0;
  return (void *)[NSApp beginModalSessionForWindow:(NSWindow *)hwnd];
}

bool SWELL_ModalWindowRun(void *ctx, int *ret) // returns false and puts retval in *ret when done
{
  if (!ctx) return false;
  int r=[NSApp runModalSession:(NSModalSession)ctx];
  if (r==NSRunContinuesResponse) return true;
  if (ret) *ret=r;
  return false;
}

void SWELL_ModalWindowEnd(void *ctx)
{
  if (ctx) 
  {
    if ([NSApp runModalSession:(NSModalSession)ctx] == NSRunContinuesResponse)
    {    
      [NSApp stopModal];
      while ([NSApp runModalSession:(NSModalSession)ctx]==NSRunContinuesResponse) Sleep(10);
    }
    [NSApp endModalSession:(NSModalSession)ctx];
  }
}

void SWELL_CloseWindow(HWND hwnd)
{
  if (hwnd && [(id)hwnd isKindOfClass:[NSWindow class]])
  {
    [((NSWindow*)hwnd) close];
  }
  else if (hwnd && [(id)hwnd isKindOfClass:[NSView class]])
  {
    [[(NSView*)hwnd window] close];
  }
}


#include "swell-dlggen.h"

static id m_make_owner;
static NSRect m_transform;
static float m_parent_h;
static bool m_doautoright;
static NSRect m_lastdoauto;
static bool m_sizetofits;

#define ACTIONTARGET (m_make_owner)

void SWELL_MakeSetCurParms(float xscale, float yscale, float xtrans, float ytrans, HWND parent, bool doauto, bool dosizetofit)
{
  m_sizetofits=dosizetofit;
  m_lastdoauto.origin.x = 0;
  m_lastdoauto.origin.y = -100;
  m_lastdoauto.size.width = 0;
  m_doautoright=doauto;
  m_transform.origin.x=xtrans;
  m_transform.origin.y=ytrans;
  m_transform.size.width=xscale;
  m_transform.size.height=yscale;
  m_make_owner=(id)parent;
  if ([m_make_owner isKindOfClass:[NSWindow class]]) m_make_owner=[(NSWindow *)m_make_owner contentView];
  m_parent_h=100.0;
  if ([(id)m_make_owner isKindOfClass:[NSView class]])
  {
    m_parent_h=[(NSView *)m_make_owner bounds].size.height;
    if (m_transform.size.height > 0 && [(id)parent isFlipped])
      m_transform.size.height*=-1;
  }
}

static void UpdateAutoCoords(NSRect r)
{
  m_lastdoauto.size.width=r.origin.x + r.size.width - m_lastdoauto.origin.x;
}

static NSRect MakeCoords(int x, int y, int w, int h, bool wantauto)
{
  if (w<0&&h<0)
  {
    return NSMakeRect(-x,-y,-w,-h);
  }
  float ysc=m_transform.size.height;
  int newx=(int)((x+m_transform.origin.x)*m_transform.size.width + 0.5);
  int newy=(int)((ysc >= 0.0 ? m_parent_h - ((y+m_transform.origin.y) + h)*ysc : 
                         ((y+m_transform.origin.y) )*-ysc) + 0.5);
                         
  NSRect ret= NSMakeRect(newx,  
                         newy,                  
                        (int) (w*m_transform.size.width+0.5),
                        (int) (h*fabs(ysc)+0.5));
                        
  NSRect oret=ret;
  if (wantauto && m_doautoright)
  {
    float dx = ret.origin.x - m_lastdoauto.origin.x;
    if (fabs(dx)<32 && m_lastdoauto.origin.y >= ret.origin.y && m_lastdoauto.origin.y < ret.origin.y + ret.size.height)
    {
      ret.origin.x += (int) m_lastdoauto.size.width;
    }
    
    m_lastdoauto.origin.x = oret.origin.x + oret.size.width;
    m_lastdoauto.origin.y = ret.origin.y + ret.size.height*0.5;
    m_lastdoauto.size.width=0;
  }
  return ret;
}

static const double minwidfontadjust=1.81;
#define TRANSFORMFONTSIZE ((m_transform.size.width+1.0)*3.7)
/// these are for swell-dlggen.h
HWND SWELL_MakeButton(int def, const char *label, int idx, int x, int y, int w, int h, int flags)
{  
  unsigned int a=(unsigned int)label;
  if (a < 65536) label = "ICONTEMP";
  SWELL_Button *button=[[SWELL_Button alloc] init];
  
  if (m_transform.size.width < minwidfontadjust)
  {
    [button setFont:[NSFont systemFontOfSize:TRANSFORMFONTSIZE]];
  }
  
  [button setTag:idx];
  [button setBezelStyle:NSRoundedBezelStyle ];
  NSRect tr=MakeCoords(x,y,w,h,true);
  
  // todo: some way to better calculate these!
  if (tr.size.width < 30)
  {
    tr.size.width += 12;
    tr.origin.x -= 6;
  }
  if (tr.size.height >= 18 && tr.size.height<24)
  {
    tr.size.height=24;
  }
  
  [button setFrame:tr];
  NSString *labelstr=(NSString *)SWELL_CStringToCFString_FilterPrefix(label);
  [button setTitle:labelstr];
  [button setTarget:ACTIONTARGET];
  [button setAction:@selector(onSwellCommand:)];
  if (flags&SWELL_NOT_WS_VISIBLE) [button setHidden:YES];
  [m_make_owner addSubview:button];
  if (m_doautoright) UpdateAutoCoords([button frame]);
  if (def) [[m_make_owner window] setDefaultButtonCell:(NSButtonCell*)button];
  [labelstr release];
  [button release];
  return (HWND) button;
}


@implementation SWELL_TextView
-(int) tag
{
  return m_tag;
}
-(void) setTag:(int)tag
{
  m_tag=tag;
}
@end

HWND SWELL_MakeEditField(int idx, int x, int y, int w, int h, int flags)
{  
  if ((flags & WS_VSCROLL)) // || (flags & ES_READONLY))
  {
    SWELL_TextView *obj=[[SWELL_TextView alloc] init];
    [obj setEditable:(flags & ES_READONLY)?NO:YES];
    if (m_transform.size.width < minwidfontadjust)
      [obj setFont:[NSFont systemFontOfSize:TRANSFORMFONTSIZE]];
    [obj setTag:idx];
    [obj setDelegate:ACTIONTARGET];
  
    [obj setHorizontallyResizable:NO];
    
    if (flags & WS_VSCROLL)
    {
      NSRect fr=MakeCoords(x,y,w,h,true);
      
      [obj setVerticallyResizable:YES];
      NSScrollView *obj2=[[NSScrollView alloc] init];
      [obj2 setFrame:fr];
      [obj2 setHasVerticalScroller:YES];
      [obj2 setAutohidesScrollers:YES];
      [obj2 setDrawsBackground:NO];
      [obj2 setDocumentView:obj];
      [m_make_owner addSubview:obj2];
      if (m_doautoright) UpdateAutoCoords([obj2 frame]);
      if (flags&SWELL_NOT_WS_VISIBLE) [obj2 setHidden:YES];
      [obj2 release];
      
      NSRect tr={0,};
      tr.size = [obj2 contentSize];
      [obj setFrame:tr];
      [obj release];
      
      return (HWND)obj2;
    }
    else
    {
      [obj setFrame:MakeCoords(x,y,w,h,true)];
      [obj setVerticallyResizable:NO];
      if (flags&SWELL_NOT_WS_VISIBLE) [obj setHidden:YES];
      [m_make_owner addSubview:obj];
      if (m_doautoright) UpdateAutoCoords([obj frame]);
      [obj release];
      return (HWND)obj;
    }  
  }  
  
  
  NSTextField *obj;
  
  if (flags & ES_PASSWORD) obj=[[NSSecureTextField alloc] init];
  else obj=[[NSTextField alloc] init];
  [obj setEditable:(flags & ES_READONLY)?NO:YES];
  if (flags & ES_READONLY) [obj setSelectable:YES];
  if (m_transform.size.width < minwidfontadjust)
    [obj setFont:[NSFont systemFontOfSize:TRANSFORMFONTSIZE]];
  if (abs(h) < 20)
  {
    [[obj cell] setWraps:NO];
    [[obj cell] setScrollable:YES];
  }
  [obj setTag:idx];
  [obj setTarget:ACTIONTARGET];
  [obj setAction:@selector(onSwellCommand:)];
  [obj setDelegate:ACTIONTARGET];
  
  [obj setFrame:MakeCoords(x,y,w,h,true)];
  if (flags&SWELL_NOT_WS_VISIBLE) [obj setHidden:YES];
  [m_make_owner addSubview:obj];
  if (m_doautoright) UpdateAutoCoords([obj frame]);
  [obj release];

  return (HWND)obj;
}

HWND SWELL_MakeLabel( int align, const char *label, int idx, int x, int y, int w, int h, int flags)
{
  NSTextField *obj=[[NSTextField alloc] init];
  [obj setEditable:NO];
  [obj setSelectable:NO];
  [obj setBordered:NO];
  [obj setBezeled:NO];
  [obj setDrawsBackground:NO];
  if (m_transform.size.width < minwidfontadjust)
    [obj setFont:[NSFont systemFontOfSize:TRANSFORMFONTSIZE]];

  if (flags & SS_NOTIFY)
  {
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onSwellCommand:)];
  }
  
  NSString *labelstr=(NSString *)SWELL_CStringToCFString_FilterPrefix(label);
  [obj setStringValue:labelstr];
  [obj setAlignment:(align<0?NSLeftTextAlignment:align>0?NSRightTextAlignment:NSCenterTextAlignment)];
  
  [[obj cell] setWraps:(h>12 ? YES : NO)];
  
  [obj setTag:idx];
  [obj setFrame:MakeCoords(x,y,w,h,true)];
  if (flags&SWELL_NOT_WS_VISIBLE) [obj setHidden:YES];
  [m_make_owner addSubview:obj];
  if (m_sizetofits && strlen(label)>1)[obj sizeToFit];
  if (m_doautoright) UpdateAutoCoords([obj frame]);
  [obj release];
  [labelstr release];
  return (HWND)obj;
}


HWND SWELL_MakeCheckBox(const char *name, int idx, int x, int y, int w, int h)
{
  return SWELL_MakeControl(name,idx,"Button",BS_AUTOCHECKBOX,x,y,w,h,0);
}

HWND SWELL_MakeListBox(int idx, int x, int y, int w, int h, int styles)
{
  HWND hw=SWELL_MakeControl("",idx,"SysListView32_LB",styles,x,y,w,h,0);
/*  if (hw)
  {
    LVCOLUMN lvc={0,};
    RECT r;
    GetClientRect(hw,&r);
    lvc.cx=300;//yer.right-r.left;
    lvc.pszText="";
    ListView_InsertColumn(hw,0,&lvc);
  }
  */
  return hw;
}


typedef struct ccprocrec
{
  SWELL_ControlCreatorProc proc;
  int cnt;
  struct ccprocrec *next;
} ccprocrec;

static ccprocrec *m_ccprocs;

void SWELL_RegisterCustomControlCreator(SWELL_ControlCreatorProc proc)
{
  if (!proc) return;
  
  ccprocrec *p=m_ccprocs;
  while (p && p->next)
  {
    if (p->proc == proc)
    {
      p->cnt++;
      return;
    }
    p=p->next;
  }
  ccprocrec *ent = (ccprocrec*)malloc(sizeof(ccprocrec));
  ent->proc=proc;
  ent->cnt=1;
  ent->next=0;
  
  if (p) p->next=ent;
  else m_ccprocs=ent;
}

void SWELL_UnregisterCustomControlCreator(SWELL_ControlCreatorProc proc)
{
  if (!proc) return;
  
  ccprocrec *lp=NULL;
  ccprocrec *p=m_ccprocs;
  while (p)
  {
    if (p->proc == proc)
    {
      if (--p->cnt <= 0)
      {
        if (lp) lp->next=p->next;
        else m_ccprocs=p->next;
        free(p);
      }
      return;
    }
    lp=p;
    p=p->next;
  }
}


HWND SWELL_MakeControl(const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h, int exstyle)
{
  if (m_ccprocs)
  {
    NSRect poo=MakeCoords(x,y,w,h,false);
    ccprocrec *p=m_ccprocs;
    while (p)
    {
      HWND h=p->proc((HWND)m_make_owner,cname,idx,classname,style,(int)(poo.origin.x+0.5),(int)(poo.origin.y+0.5),(int)(poo.size.width+0.5),(int)(poo.size.height+0.5));
      if (h) 
      {
        if (exstyle) SetWindowLong(h,GWL_EXSTYLE,exstyle);
        return h;
      }
      p=p->next;
    }
  }
  if (!stricmp(classname,"SysTabControl32"))
  {
    SWELL_TabView *obj=[[SWELL_TabView alloc] init];
    [obj setTag:idx];
    [obj setDelegate:obj];
    [obj setAllowsTruncatedLabels:YES];
    [obj setNotificationWindow:ACTIONTARGET];
    [obj setHidden:NO];
    [obj setFrame:MakeCoords(x,y,w,h,false)];
    if (style&SWELL_NOT_WS_VISIBLE) [obj setHidden:YES];
    [m_make_owner addSubview:obj];
    [obj release];
    return (HWND)obj;
  }
  else if (!stricmp(classname, "SysListView32")||!stricmp(classname, "SysListView32_LB"))
  {
    SWELL_ListView *obj = [[SWELL_ListView alloc] init];
    [obj setDataSource:obj];
    obj->style=style;

    BOOL isLB=!stricmp(classname, "SysListView32_LB");
    [obj setSwellNotificationMode:isLB];
    
    if (isLB)
    {
      [obj setHeaderView:nil];
      [obj setAllowsMultipleSelection:!!(style & LBS_EXTENDEDSEL)];
    }
    else
    {
      if ((style & LVS_NOCOLUMNHEADER) || !(style & LVS_REPORT))  [obj setHeaderView:nil];
      [obj setAllowsMultipleSelection:!(style & LVS_SINGLESEL)];
    }
    [obj setAllowsColumnReordering:NO];
    [obj setAllowsEmptySelection:YES];
    [obj setTag:idx];
    [obj setHidden:NO];
    id target=ACTIONTARGET;
    [obj setDelegate:target];
    [obj setTarget:target];
    [obj setAction:@selector(onSwellCommand:)];
    if ([target respondsToSelector:@selector(swellOnControlDoubleClick:)])
      [obj setDoubleAction:@selector(swellOnControlDoubleClick:)];
    else
      [obj setDoubleAction:@selector(onSwellCommand:)];
    NSScrollView *obj2=[[NSScrollView alloc] init];
    NSRect tr=MakeCoords(x,y,w,h,false);
    [obj2 setFrame:tr];
    [obj2 setDocumentView:obj];
    [obj2 setHasVerticalScroller:YES];
    [obj2 setAutohidesScrollers:YES];
    [obj2 setDrawsBackground:NO];
    [obj release];
    if (style&SWELL_NOT_WS_VISIBLE) [obj2 setHidden:YES];
    [m_make_owner addSubview:obj2];
    [obj2 release];
    
    if (isLB || !(style & LVS_REPORT))
    {
      LVCOLUMN lvc={0,};
      lvc.cx=(int)ceil(max(tr.size.width,300.0));
      lvc.pszText="";
      ListView_InsertColumn((HWND)obj,0,&lvc);
      if (isLB && (style & LBS_OWNERDRAWFIXED))
      {
        NSArray *ar=[obj tableColumns];
        NSTableColumn *c;
        if (ar && [ar count] && (c=[ar objectAtIndex:0]))
        {
          SWELL_ODListViewCell *t=[[SWELL_ODListViewCell alloc] init];
          [c setDataCell:t];
          [t setOwnerControl:obj];
          [t release];
        }
      }
    }
    
    return (HWND)obj;
  }
  else if (!stricmp(classname, "SysTreeView32"))
  {
    SWELL_TreeView *obj = [[SWELL_TreeView alloc] init];
    [obj setDataSource:obj];
    obj->style=style;
    id target=ACTIONTARGET;
    [obj setHeaderView:nil];    
    [obj setDelegate:target];
    [obj setAllowsColumnReordering:NO];
    [obj setAllowsMultipleSelection:NO];
    [obj setAllowsEmptySelection:YES];
    [obj setTag:idx];
    [obj setHidden:NO];
    [obj setTarget:target];
    [obj setAction:@selector(onSwellCommand:)];
    if ([target respondsToSelector:@selector(swellOnControlDoubleClick:)])
      [obj setDoubleAction:@selector(swellOnControlDoubleClick:)];
    else
      [obj setDoubleAction:@selector(onSwellCommand:)];
    NSScrollView *obj2=[[NSScrollView alloc] init];
    NSRect tr=MakeCoords(x,y,w,h,false);
    [obj2 setFrame:tr];
    [obj2 setDocumentView:obj];
    [obj2 setHasVerticalScroller:YES];
    [obj2 setAutohidesScrollers:YES];
    [obj2 setDrawsBackground:NO];
    [obj release];
    if (style&SWELL_NOT_WS_VISIBLE) [obj2 setHidden:YES];
    [m_make_owner addSubview:obj2];
    [obj2 release];

    {
      NSTableColumn *col=[[NSTableColumn alloc] init];
      [col setWidth:(int)ceil(max(tr.size.width,300.0))];
      [col setEditable:NO];
      [[col dataCell] setWraps:NO];     
      [obj addTableColumn:col];
      [obj setOutlineTableColumn:col];

      [col release];
    }
///    [obj setIndentationPerLevel:10.0];
    
    return (HWND)obj;
  }
  else if (!stricmp(classname, "msctls_progress32"))
  {
    SWELL_ProgressView *obj=[[SWELL_ProgressView alloc] init];
    [obj setStyle:NSProgressIndicatorBarStyle];
    [obj setIndeterminate:NO];
    [obj setTag:idx];
    [obj setFrame:MakeCoords(x,y,w,h,false)];
    if (style&SWELL_NOT_WS_VISIBLE) [obj setHidden:YES];
    [m_make_owner addSubview:obj];
    [obj release];
    return (HWND)obj;
  }
  else if (!stricmp(classname,"Edit"))
  {
    return SWELL_MakeEditField(idx,x,y,w,h,style);
  }
  else if (!stricmp(classname, "static"))
  {
    NSTextField *obj=[[NSTextField alloc] init];
    [obj setEditable:NO];
    [obj setSelectable:NO];
    [obj setBordered:NO];
    [obj setBezeled:NO];
    [obj setDrawsBackground:NO];
    if (m_transform.size.width < minwidfontadjust)
      [obj setFont:[NSFont systemFontOfSize:TRANSFORMFONTSIZE]];
    if (cname && *cname)
    {
      NSString *labelstr=(NSString *)SWELL_CStringToCFString_FilterPrefix(cname);
      [obj setStringValue:labelstr];
      [labelstr release];
    }
    
    if ((style&SS_TYPEMASK) == SS_LEFTNOWORDWRAP) [[obj cell] setWraps:NO];
    [obj setTag:idx];
    [obj setFrame:MakeCoords(x,y,w,h,true)];
    if (style&SWELL_NOT_WS_VISIBLE) [obj setHidden:YES];
    [m_make_owner addSubview:obj];
    if ((style & SS_TYPEMASK) == SS_BLACKRECT)
    {
      [obj setHidden:YES];
    }
    [obj release];
    return (HWND)obj;
  }
  else if (!stricmp(classname,"Button"))
  {
    SWELL_Button *button=[[SWELL_Button alloc] init];
    [button setTag:idx];
    if ((style & 0xf) == BS_AUTO3STATE)
    {
      [button setButtonType:NSSwitchButton];
      [button setAllowsMixedState:YES];
    }    
    else if ((style & 0xf) == BS_AUTOCHECKBOX)
    {
      [button setButtonType:NSSwitchButton];
      [button setAllowsMixedState:NO];
    }
    else if ((style & 0xf) == BS_AUTORADIOBUTTON)
    {
      [button setButtonType:NSRadioButton];
      [button swellSetRadioFlags:(style & WS_GROUP)?3:1];
    }
    else if ((style & 0xf) == BS_OWNERDRAW)
    {
      SWELL_ODButtonCell *cell = [[SWELL_ODButtonCell alloc] init];
      [button setCell:cell];
      [cell release];
      //NSButtonCell
    }
    if (m_transform.size.width < minwidfontadjust)
      [button setFont:[NSFont systemFontOfSize:TRANSFORMFONTSIZE]];
    [button setFrame:MakeCoords(x,y,w,h,true)];
    NSString *labelstr=(NSString *)SWELL_CStringToCFString_FilterPrefix(cname);
    [button setTitle:labelstr];
    [button setTarget:ACTIONTARGET];
    [button setAction:@selector(onSwellCommand:)];
    if (style&SWELL_NOT_WS_VISIBLE) [button setHidden:YES];
    [m_make_owner addSubview:button];
    if (m_sizetofits && (style & 0xf) != BS_OWNERDRAW) [button sizeToFit];
    if (m_doautoright) UpdateAutoCoords([button frame]);
    [labelstr release];
    [button release];
    return (HWND)button;
  }
  else if (!stricmp(classname,"REAPERhfader")||!stricmp(classname,"msctls_trackbar32"))
  {
    NSSlider *obj=[[NSSlider alloc] init];
    [obj setTag:idx];
    [obj setMinValue:0.0];
    [obj setMaxValue:1000.0];
    [obj setFrame:MakeCoords(x,y,w,h,false)];
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onSwellCommand:)];
    if (style&SWELL_NOT_WS_VISIBLE) [obj setHidden:YES];
    [m_make_owner addSubview:obj];
    [obj release];
    return (HWND)obj;
  }
  return 0;
}

HWND SWELL_MakeCombo(int idx, int x, int y, int w, int h, int flags)
{
  if ((flags & 0x3) == CBS_DROPDOWNLIST)
  {
    SWELL_PopUpButton *obj=[[SWELL_PopUpButton alloc] init];
    [obj setTag:idx];
    if (m_transform.size.width < minwidfontadjust)
      [obj setFont:[NSFont systemFontOfSize:TRANSFORMFONTSIZE]];
    NSRect rc=MakeCoords(x,y-1,w,14,true);
    if (rc.size.height<14*1.7) rc.size.height=14*1.7;
    [obj setSwellStyle:flags];
    [obj setFrame:rc];
    [obj setAutoenablesItems:NO];
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onSwellCommand:)];
    if (flags&SWELL_NOT_WS_VISIBLE) [obj setHidden:YES];
    [m_make_owner addSubview:obj];
    if (m_doautoright) UpdateAutoCoords([obj frame]);
    [obj release];
    return (HWND)obj;
  }
  else
  {
    SWELL_ComboBox *obj=[[SWELL_ComboBox alloc] init];
    if (m_transform.size.width < minwidfontadjust)
      [obj setFont:[NSFont systemFontOfSize:TRANSFORMFONTSIZE]];
    [obj setEditable:(flags & 0x3) == CBS_DROPDOWNLIST?NO:YES];
    [obj setSwellStyle:flags];
    [obj setTag:idx];
    [obj setFrame:MakeCoords(x,y-1,w,14,true)];
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onSwellCommand:)];
    [obj setDelegate:ACTIONTARGET];
    if (flags&SWELL_NOT_WS_VISIBLE) [obj setHidden:YES];
    [m_make_owner addSubview:obj];
    if (m_doautoright) UpdateAutoCoords([obj frame]);
    [obj release];
    return (HWND)obj;
  }
}

@implementation SWELL_BoxView
-(int) tag
{
  return m_tag;
}
-(void) setTag:(int)tag
{
  m_tag=tag;
}
@end

HWND SWELL_MakeGroupBox(const char *name, int idx, int x, int y, int w, int h, int style)
{
  SWELL_BoxView *obj=[[SWELL_BoxView alloc] init];
//  [obj setTag:idx];
  NSString *labelstr=(NSString *)SWELL_CStringToCFString_FilterPrefix(name);
  [obj setTitle:labelstr];
  [obj setTag:idx];
  [labelstr release];
  if (style & BS_CENTER)
  {
    [[obj titleCell] setAlignment:NSCenterTextAlignment];
  }
  [obj setFrame:MakeCoords(x,y,w,h,false)];
  [m_make_owner addSubview:obj positioned:NSWindowBelow relativeTo:nil];
  [obj release];
  return (HWND)obj;
}


int TabCtrl_GetItemCount(HWND hwnd)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return 0;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  return [tv numberOfTabViewItems];
}

BOOL TabCtrl_AdjustRect(HWND hwnd, BOOL fLarger, RECT *r)
{
  if (!r || !hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return FALSE;
  
  int sign=fLarger?-1:1;
  r->left+=sign*7; // todo: correct this?
  r->right-=sign*7;
  r->top+=sign*26;
  r->bottom-=sign*3;
  return TRUE;
}


BOOL TabCtrl_DeleteItem(HWND hwnd, int idx)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return 0;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  if (idx<0 || idx>= [tv numberOfTabViewItems]) return 0;
  [tv removeTabViewItem:[tv tabViewItemAtIndex:idx]];
  return TRUE;
}

int TabCtrl_InsertItem(HWND hwnd, int idx, TCITEM *item)
{
  if (!item || !hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return -1;
  if (!(item->mask & TCIF_TEXT) || !item->pszText) return -1;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  if (idx<0) idx=0;
  else if (idx>[tv numberOfTabViewItems]) idx=[tv numberOfTabViewItems];
  
  NSTabViewItem *tabitem=[[NSTabViewItem alloc] init];
  NSString *str=(NSString *)SWELL_CStringToCFString(item->pszText);  
  [tabitem setLabel:str];
  [str release];
  id turd=[tv getNotificationWindow];
  [tv setNotificationWindow:nil];
  [tv insertTabViewItem:tabitem atIndex:idx];
  [tv setNotificationWindow:turd];
  [tabitem release];
  return idx;
}

int TabCtrl_SetCurSel(HWND hwnd, int idx)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return -1;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  int ret=TabCtrl_GetCurSel(hwnd);
  if (idx>=0 && idx < [tv numberOfTabViewItems])
  {
    [tv selectTabViewItemAtIndex:idx];
  }
  return ret;
}

int TabCtrl_GetCurSel(HWND hwnd)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return 0;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  NSTabViewItem *item=[tv selectedTabViewItem];
  if (!item) return 0;
  return [tv indexOfTabViewItem:item];
}

void ListView_SetExtendedListViewStyleEx(HWND h, int flag, int mask)
{
}

void ListView_SetImageList(HWND h, HIMAGELIST imagelist, int which)
{
  if (!h || !imagelist || which != LVSIL_STATE) return;
  
  SWELL_ListView *v=(SWELL_ListView *)h;
  
  v->m_status_imagelist=(WDL_PtrList<char> *)imagelist;
  if (v->m_cols && v->m_cols->GetSize()>0)
  {
    NSTableColumn *col=(NSTableColumn*)v->m_cols->Get(0);
    if (![col isKindOfClass:[SWELL_StatusCell class]])
    {
      SWELL_StatusCell *cell=[[SWELL_StatusCell alloc] initNewCell];
      [cell setWraps:NO];
      [col setDataCell:cell];
      [cell release];
    }
  }  
}

int ListView_GetColumnWidth(HWND h, int pos)
{
  if (!h || ![(id)h isKindOfClass:[SWELL_ListView class]]) return 0;
  SWELL_ListView *v=(SWELL_ListView *)h;
  if (!v->m_cols || pos < 0 || pos >= v->m_cols->GetSize()) return 0;
  
  NSTableColumn *col=v->m_cols->Get(pos);
  if (!col) return 0;
  return (int) floor(0.5+[col width]);
}

void ListView_InsertColumn(HWND h, int pos, const LVCOLUMN *lvc)
{
  if (!h || !lvc) return;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return;
  SWELL_ListView *v=(SWELL_ListView *)h;
  NSTableColumn *col=[[NSTableColumn alloc] init];
  [col setWidth:lvc->cx];
  [col setEditable:NO];
  if (!v->m_lbMode && !(v->style & LVS_NOCOLUMNHEADER))
  {
    NSString *lbl=(NSString *)SWELL_CStringToCFString(lvc->pszText);  
    [[col headerCell] setStringValue:lbl];
    [lbl release];
  }
  
  if (!pos && v->m_status_imagelist) 
  {
    SWELL_StatusCell *cell=[[SWELL_StatusCell alloc] initNewCell];
    [cell setWraps:NO];
    [col setDataCell:cell];
    [cell release];
  }
  else
  {  
    [[col dataCell] setWraps:NO];
  }

  [v addTableColumn:col];
  v->m_cols->Add(col);
  [col release];
}


void ListView_GetItemText(HWND hwnd, int item, int subitem, char *text, int textmax)
{
  LVITEM it={LVIF_TEXT,item,subitem,0,0,text,textmax,};
  ListView_GetItem(hwnd,&it);
}

int ListView_InsertItem(HWND h, const LVITEM *item)
{
  if (!h || !item || item->iSubItem) return 0;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return 0;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  if (!tv->m_lbMode && (tv->style & LVS_OWNERDATA)) return -1;
  if (!tv->m_items) return -1;
    
  int a=item->iItem;
  if (a<0)a=0;
  else if (a > tv->m_items->GetSize()) a=tv->m_items->GetSize();
  
  if (!tv->m_lbMode && (item->mask & LVIF_TEXT))
  {
    if (tv->style & LVS_SORTASCENDING)
    {
       a=ptrlist_bsearch_mod((char *)item->pszText,tv->m_items,_listviewrowSearchFunc,NULL);
    }
    else if (tv->style & LVS_SORTDESCENDING)
    {
       a=ptrlist_bsearch_mod((char *)item->pszText,tv->m_items,_listviewrowSearchFunc2,NULL);
    }
  }
  
  SWELL_ListView_Row *nr=new SWELL_ListView_Row;
  nr->m_vals.Add(strdup((item->mask & LVIF_TEXT) ? item->pszText : ""));
  if (item->mask & LVIF_PARAM) nr->m_param = item->lParam;
  tv->m_items->Insert(a,nr);
  

  
  if ((item->mask&LVIF_STATE) && (item->stateMask & (0xff<<16)))
  {
    nr->m_imageidx=(item->state>>16)&0xff;
  }
  
  [tv reloadData];
  
  if (a < tv->m_items->GetSize()-1)
  {
    NSIndexSet *sel=[tv selectedRowIndexes];
    if (sel && [sel count])
    {
      NSMutableIndexSet *ms = [[NSMutableIndexSet alloc] initWithIndexSet:sel];
      [ms shiftIndexesStartingAtIndex:a by:1];
      [tv selectRowIndexes:ms byExtendingSelection:NO];
      [ms release];
    }
  }
  
  if (item->mask & LVIF_STATE)
  {
    if (item->stateMask & LVIS_SELECTED)
    {
      if (item->state&LVIS_SELECTED)
      {
        bool isSingle = tv->m_lbMode ? !(tv->style & LBS_EXTENDEDSEL) : !!(tv->style&LVS_SINGLESEL);
        [tv selectRow:a byExtendingSelection:isSingle?NO:YES];        
      }
    }
  }
  
  return a;
}

void ListView_SetItemText(HWND h, int ipos, int cpos, const char *txt)
{
  if (!h || cpos < 0 || cpos >= 32) return;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  if (!tv->m_lbMode && (tv->style & LVS_OWNERDATA)) return;
  if (!tv->m_items) return;
  
  SWELL_ListView_Row *p=tv->m_items->Get(ipos);
  if (!p) return;
  int x;
  for (x = p->m_vals.GetSize(); x < cpos; x ++)
  {
    p->m_vals.Add(strdup(""));
  }
  if (cpos < p->m_vals.GetSize())
  {
    free(p->m_vals.Get(cpos));
    p->m_vals.Set(cpos,strdup(txt));
  }
  else p->m_vals.Add(strdup(txt));
    
  [tv reloadData];
}

int ListView_GetNextItem(HWND h, int istart, int flags)
{
  if (flags==LVNI_FOCUSED||flags==LVNI_SELECTED)
  {
    if (!h) return 0;
    if (![(id)h isKindOfClass:[SWELL_ListView class]]) return 0;
    
    SWELL_ListView *tv=(SWELL_ListView*)h;
    
    if (flags==LVNI_SELECTED)
    {
      int orig_start=istart;
      if (istart++<0)istart=0;
      int n = [tv numberOfRows];
      while (istart < n)
      {
        if ([tv isRowSelected:istart]) return istart;
        istart++;
      }
      istart=0;
      while (istart <= orig_start && istart < n)
      {
        if ([tv isRowSelected:istart]) return istart;
        istart++;        
      }
      return -1;
    }
    
    return [tv selectedRow];
  }
  return -1;
}

bool ListView_SetItem(HWND h, LVITEM *item)
{
  if (!item) return false;
  if (!h) return false;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return false;
    
  SWELL_ListView *tv=(SWELL_ListView*)h;
  if (tv->m_lbMode || !(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return false;
    SWELL_ListView_Row *row=tv->m_items->Get(item->iItem);
    if (!row) return false;  
  
    if (item->mask & LVIF_PARAM) row->m_param=item->lParam;
    if (item->mask & LVIF_TEXT) if (item->pszText) ListView_SetItemText(h,item->iItem,item->iSubItem,item->pszText);
  }
  if (item->mask & LVIF_STATE) if (item->stateMask)
    ListView_SetItemState(h,item->iItem,item->state,item->stateMask); 

  return true;
}

bool ListView_GetItem(HWND h, LVITEM *item)
{
  if (!item) return false;
  if ((item->mask&LVIF_TEXT)&&item->pszText && item->cchTextMax > 0) item->pszText[0]=0;
  item->state=0;
  if (!h) return false;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return false;
  
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  if (tv->m_lbMode || !(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return false;
    
    SWELL_ListView_Row *row=tv->m_items->Get(item->iItem);
    if (!row) return false;  
  
    if (item->mask & LVIF_PARAM) item->lParam=row->m_param;
    if (item->mask & LVIF_TEXT) if (item->pszText && item->cchTextMax>0)
    {
      char *p=row->m_vals.Get(item->iSubItem);
      lstrcpyn(item->pszText,p?p:"",item->cchTextMax);
    }
      if (item->mask & LVIF_STATE)
      {
        if (item->stateMask & (0xff<<16))
        {
          item->state|=row->m_imageidx<<16;
        }
      }
  }
  else
  {
    if (item->iItem <0 || item->iItem >= tv->ownermode_cnt) return false;
  }
  if (item->mask & LVIF_STATE)
  {
     if ((item->stateMask&LVIS_SELECTED) && [tv isRowSelected:item->iItem]) item->state|=LVIS_SELECTED;
     if ((item->stateMask&LVIS_FOCUSED) && [tv selectedRow] == item->iItem) item->state|=LVIS_FOCUSED;
  }

  return true;
}
int ListView_GetItemState(HWND h, int ipos, int mask)
{
  if (!h || ![(id)h isKindOfClass:[SWELL_ListView class]]) return 0;
  SWELL_ListView *tv=(SWELL_ListView*)h;
  int flag=0;
  if (tv->m_lbMode || !(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return 0;
    SWELL_ListView_Row *row=tv->m_items->Get(ipos);
    if (!row) return 0;  
    if (mask & (0xff<<16))
    {
      flag|=row->m_imageidx<<16;
    }
  }
  else
  {
    if (ipos<0 || ipos >= tv->ownermode_cnt) return 0;
  }
  
  if ((mask&LVIS_SELECTED) && [tv isRowSelected:ipos]) flag|=LVIS_SELECTED;
  if ((mask&LVIS_FOCUSED) && [tv selectedRow]==ipos) flag|=LVIS_FOCUSED;
  return flag;  
}

bool ListView_SetItemState(HWND h, int ipos, int state, int statemask)
{
  int doref=0;
  if (!h || ![(id)h isKindOfClass:[SWELL_ListView class]]) return false;
  SWELL_ListView *tv=(SWELL_ListView*)h;
  static int _is_doing_all;
  
  if (ipos == -1)
  {
    int x;
    int n=ListView_GetItemCount(h);
    _is_doing_all++;
    for (x = 0; x < n; x ++)
      ListView_SetItemState(h,x,state,statemask);
    _is_doing_all--;
    ListView_RedrawItems(h,0,n-1);
    return true;
  }

  if (tv->m_lbMode || !(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return false;
    SWELL_ListView_Row *row=tv->m_items->Get(ipos);
    if (!row) return false;  
    if (statemask & (0xff<<16))
    {
      if (row->m_imageidx!=((state>>16)&0xff))
      {
        row->m_imageidx=(state>>16)&0xff;
        doref=1;
      }
    }
  }
  else
  {
    if (ipos<0 || ipos >= tv->ownermode_cnt) return 0;
  }
  bool didsel=false;
  if (statemask & LVIS_SELECTED)
  {
    if (state & LVIS_SELECTED)
    {      
      bool isSingle = tv->m_lbMode ? !(tv->style & LBS_EXTENDEDSEL) : !!(tv->style&LVS_SINGLESEL);
      if (![tv isRowSelected:ipos]) { didsel=true;  [tv selectRow:ipos byExtendingSelection:isSingle?NO:YES]; }
    }
    else
    {
      if ([tv isRowSelected:ipos]) { didsel=true; [tv deselectRow:ipos];  }
    }
  }
  if (statemask & LVIS_FOCUSED)
  {
    if (state&LVIS_FOCUSED)
    {
    }
    else
    {
      
    }
  }
  
  if (!_is_doing_all)
  {
    if (didsel)
    {
      static int __rent;
      if (!__rent)
      {
        __rent=1;
        NMLISTVIEW nm={{(HWND)h,[tv tag],LVN_ITEMCHANGED},ipos,0,state,};
        SendMessage(GetParent(h),WM_NOTIFY,[tv tag],(LPARAM)&nm);      
        __rent=0;
      }
    }
    if (doref)
      ListView_RedrawItems(h,ipos,ipos);
  }
  return true;
}
void ListView_RedrawItems(HWND h, int startitem, int enditem)
{
  if (!h || ![(id)h isKindOfClass:[SWELL_ListView class]]) return;
  SWELL_ListView *tv=(SWELL_ListView*)h;
  if (!tv->m_items) return;
  [tv reloadData];
}

void ListView_DeleteItem(HWND h, int ipos)
{
  if (!h) return;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  if (!tv->m_items) return;
  
  if (ipos >=0 && ipos < tv->m_items->GetSize())
  {
    if (ipos != tv->m_items->GetSize()-1)
    {
      NSIndexSet *sel=[tv selectedRowIndexes];
      if (sel && [sel count])
      {
        NSMutableIndexSet *ms = [[NSMutableIndexSet alloc] initWithIndexSet:sel];
        [ms shiftIndexesStartingAtIndex:ipos+1 by:-1];
        [tv selectRowIndexes:ms byExtendingSelection:NO];
        [ms release];
      }
    }
    tv->m_items->Delete(ipos,true);
    
    [tv reloadData];
    
  }
}

void ListView_DeleteAllItems(HWND h)
{
  if (!h) return;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  tv->ownermode_cnt=0;
  if (tv->m_items) tv->m_items->Empty(true);
  
  [tv reloadData];
}

int ListView_GetSelectedCount(HWND h)
{
  if (!h) return 0;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return 0;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  return [tv numberOfSelectedRows];
}

int ListView_GetItemCount(HWND h)
{
  if (!h) return 0;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return 0;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  if (tv->m_lbMode || !(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return 0;
  
    return tv->m_items->GetSize();
  }
  return tv->ownermode_cnt;
}

int ListView_GetSelectionMark(HWND h)
{
  if (!h) return 0;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return 0;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  return [tv selectedRow];
}

void ListView_SetColumnWidth(HWND h, int colpos, int wid)
{
}

int ListView_HitTest(HWND h, LVHITTESTINFO *pinf)
{
  if (!h) return -1;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return -1;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  // return index
  pinf->flags=0;
  pinf->iItem=-1;
  
  NSPoint pt={pinf->pt.x,pinf->pt.y};
    
  pinf->iItem=[(NSTableView *)h rowAtPoint:pt];
  if (pinf->iItem >= 0 && tv->m_status_imagelist)
  {
    pinf->flags=LVHT_ONITEMLABEL;
    float rh = [tv rowHeight];
    if (pinf->pt.x <= rh)
    {      
      pinf->flags=LVHT_ONITEMSTATEICON;
    }
  }
  else
  {
    pinf->flags=LVHT_NOWHERE;
    
    NSRect fr=[(NSTableView *)h bounds];
    if (pt.y < 0) pinf->flags=LVHT_ABOVE;
    else pinf->flags=LVHT_BELOW;
    
  }
  
  return pinf->iItem;
}

void ListView_SetItemCount(HWND h, int cnt)
{
  if (!h) return;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  if (!tv->m_lbMode && (tv->style & LVS_OWNERDATA))
  {
    tv->ownermode_cnt=cnt;
  }
}

void ListView_EnsureVisible(HWND h, int i, BOOL pok)
{
  if (!h) return;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return;
  
  SWELL_ListView *tv=(SWELL_ListView*)h;
  
  if (i<0)i=0;
  if (!tv->m_lbMode && (tv->style & LVS_OWNERDATA))
  {
    if (i >=tv->ownermode_cnt-1) i=tv->ownermode_cnt-1;
  }
  else
  {
    if (tv->m_items && i >= tv->m_items->GetSize()) i=tv->m_items->GetSize()-1;
  }
  if (i>=0)
  {
    [tv scrollRowToVisible:i];
  }  
}
bool ListView_GetSubItemRect(HWND h, int item, int subitem, int code, RECT *r)
{
  if (!h) return false;
  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return false;
  if (item < 0 || item > ListView_GetItemCount(h)) return false;
  SWELL_ListView *tv=(SWELL_ListView*)h;
  
  if (!tv->m_cols || subitem<0 || subitem >= tv->m_cols->GetSize()) return false;
  
  NSRect ar=[tv frameOfCellAtColumn:subitem row:item];
  r->left=(int)ar.origin.x;
  r->top=(int)ar.origin.y;
  r->right=r->left + (int)ar.size.width;
  r->bottom=r->top+(int)ar.size.height;
  return true;
}

static int __listview_sortfunc(void *p1, void *p2, int (*compar)(LPARAM val1, LPARAM val2, LPARAM userval), LPARAM userval)
{
  SWELL_ListView_Row *a = *(SWELL_ListView_Row **)p1;
  SWELL_ListView_Row *b = *(SWELL_ListView_Row **)p2;
  return compar(a->m_param,b->m_param,userval);
}

static void __listview_mergesort_internal(void *base, size_t nmemb, size_t size, 
                                 int (*compar)(LPARAM val1, LPARAM val2, LPARAM userval), 
                                 LPARAM parm,
                                 char *tmpspace)
{
  char *b1,*b2;
  size_t n1, n2;

  if (nmemb < 2) return;

  n1 = nmemb / 2;
  b1 = (char *) base;
  n2 = nmemb - n1;
  b2 = b1 + (n1 * size);

  if (nmemb>2)
  {
    __listview_mergesort_internal(b1, n1, size, compar, parm, tmpspace);
    __listview_mergesort_internal(b2, n2, size, compar, parm, tmpspace);
  }

  char *p = tmpspace;

  do
  {
	  if (__listview_sortfunc(b1, b2, compar,parm) > 0)
	  {
	    memcpy(p, b2, size);
	    b2 += size;
	    n2--;
	  }
	  else
	  {
	    memcpy(p, b1, size);
	    b1 += size;
	    n1--;
	  }
  	p += size;
  }
  while (n1 > 0 && n2 > 0);

  if (n1 > 0) memcpy(p, b1, n1 * size);
  memcpy(base, tmpspace, (nmemb - n2) * size);
}


void ListView_SortItems(HWND hwnd, PFNLVCOMPARE compf, LPARAM parm)
{
  if (!hwnd) return;
  if (![(id)hwnd isKindOfClass:[SWELL_ListView class]]) return;
  SWELL_ListView *tv=(SWELL_ListView*)hwnd;
  if (tv->m_lbMode || (tv->style & LVS_OWNERDATA) || !tv->m_items) return;
    
  WDL_HeapBuf tmp;
  tmp.Resize(tv->m_items->GetSize()*sizeof(void *));
  __listview_mergesort_internal(tv->m_items->GetList(),tv->m_items->GetSize(),sizeof(void *),compf,parm,(char*)tmp.Get());
  
   [tv reloadData];
}


HWND WindowFromPoint(POINT p)
{
  NSArray *windows=[NSApp orderedWindows];
  if (!windows) return 0;
  
  NSWindow *bestwnd=0;
  int x;
  for (x = 0; x < [windows count]; x ++)
  {
    NSWindow *wnd=[windows objectAtIndex:x];
    if (wnd)
    {
      NSRect fr=[wnd frame];
      if (p.x >= fr.origin.x && p.x < fr.origin.x + fr.size.width &&
          p.y >= fr.origin.y && p.y < fr.origin.y + fr.size.height)
      {
        bestwnd=wnd;
        break;
      }    
    }
  }
  
  if (!bestwnd) return 0;
  NSPoint pt={p.x,p.y};
  NSPoint lpt=[bestwnd convertScreenToBase:pt];
  NSView *v=[[bestwnd contentView] hitTest:lpt];
  if (v) return (HWND)v;
  return (HWND)[bestwnd contentView]; // todo: implement
}

void UpdateWindow(HWND hwnd)
{
  if (hwnd && [(id)hwnd isKindOfClass:[NSView class]])
  {
    NSWindow *wnd = [(id)hwnd window];
    [wnd displayIfNeeded];
  }
}
#endif

void InvalidateRect(HWND hwnd, RECT *r, int eraseBk)
{ 
  if (!hwnd) return;
  // todo: implement
}


HWND GetCapture()
{
  return (HWND) gtk_grab_get_current();
}

HWND SetCapture(HWND hwnd)
{
  GtkWidget *oc = gtk_grab_get_current();
  if (oc) gtk_grab_remove(oc);

  if (hwnd) gtk_grab_add(GTK_WIDGET(hwnd));

  if (oc) SendMessage((HWND)oc,WM_CAPTURECHANGED,0,(LPARAM)hwnd);

  return (HWND)oc;
}

void ReleaseCapture()
{
  GtkWidget *oc = gtk_grab_get_current();
  if (oc) gtk_grab_remove(oc);
  if (oc)
  {
    SendMessage((HWND)oc,WM_CAPTURECHANGED,0,0);
  }
}


#if 0

HDC BeginPaint(HWND hwnd, PAINTSTRUCT *ps)
{
  if (!ps) return 0;
  memset(ps,0,sizeof(PAINTSTRUCT));
  if (!hwnd) return 0;
  id turd = (id)hwnd;
  if (![turd respondsToSelector:@selector(getSwellPaintInfo:)]) return 0;

  [(SWELL_hwndChild*)turd getSwellPaintInfo:(PAINTSTRUCT *)ps];
  return ps->hdc;
}

BOOL EndPaint(HWND hwnd, PAINTSTRUCT *ps)
{
  return TRUE;
}

long DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg==WM_RBUTTONUP||msg==WM_NCRBUTTONUP)
  {  
    POINT p={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
    HWND hwndDest=hwnd;
    if (msg==WM_RBUTTONUP)
    {
      ClientToScreen(hwnd,&p);
      HWND h=WindowFromPoint(p);
      if (h && IsChild(hwnd,h)) hwndDest=h;
    }
    SendMessage(hwnd,WM_CONTEXTMENU,(WPARAM)hwndDest,((short)p.x)|(p.y<<16));
    return 1;
  }
  else if (msg==WM_CONTEXTMENU || msg == WM_MOUSEWHEEL)
  {
    if ([(id)hwnd isKindOfClass:[NSView class]])
    {
      NSView *h=(NSView *)hwnd;
      while (h && [[h window] contentView] != h)
      {
        h=[h superview];
        if (h && [h respondsToSelector:@selector(onSwellMessage:p1:p2:)]) 
        {
           return SendMessage((HWND)h,msg,wParam,lParam);    
        }
      }
    }
  }
  else if (msg==WM_NCHITTEST) 
  {
    return HTCLIENT;
  }
  else if (msg==WM_KEYDOWN || msg==WM_KEYUP) return 69;
  return 0;
}

















///////////////// clipboard compatability (NOT THREAD SAFE CURRENTLY)


BOOL DragQueryPoint(HDROP hDrop,LPPOINT pt)
{
  if (!hDrop) return 0;
  DROPFILES *df=(DROPFILES*)GlobalLock(hDrop);
  BOOL rv=!df->fNC;
  *pt=df->pt;
  GlobalUnlock(hDrop);
  return rv;
}

void DragFinish(HDROP hDrop)
{
//do nothing for now (caller will free hdrops)
}

UINT DragQueryFile(HDROP hDrop, UINT wf, char *buf, UINT bufsz)
{
  if (!hDrop) return 0;
  DROPFILES *df=(DROPFILES*)GlobalLock(hDrop);

  UINT rv=0;
  char *p=(char*)df + df->pFiles;
  if (wf == 0xFFFFFFFF)
  {
    while (*p)
    {
      rv++;
      p+=strlen(p)+1;
    }
  }
  else
  {
    while (*p)
    {
      if (!wf--)
      {
        if (buf)
        {
          lstrcpyn(buf,p,bufsz);
          rv=strlen(buf);
        }
        else rv=strlen(p);
          
        break;
      }
      p+=strlen(p)+1;
    }
  }
  GlobalUnlock(hDrop);
  return rv;
}











static WDL_PtrList<void> m_clip_recs;
static WDL_PtrList<NSString> m_clip_fmts;
static WDL_PtrList<char> m_clip_curfmts;
bool OpenClipboard(HWND hwndDlg)
{
  NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:@"SWELL_APP"];
  m_clip_curfmts.Empty();
  NSArray *ar=[pasteboard types];

  
  if (ar && [ar count])
  {
    int x;
    
    for (x = 0; x < [ar count]; x ++)
    {
      NSString *s=[ar objectAtIndex:x];
      if (!s) continue;
      int y;
      for (y = 0; y < m_clip_fmts.GetSize(); y ++)
      {
        if ([s compare:(NSString *)m_clip_fmts.Get(y)]==NSOrderedSame)
        {
          if (m_clip_curfmts.Find((char*)(y+1))<0)
            m_clip_curfmts.Add((char*)(y+1));
          break;
        }
      }
      
    }
  }
  return true;
}

void CloseClipboard() // frees any remaining items in clipboard
{
  m_clip_recs.Empty(true,GlobalFree);
}

UINT EnumClipboardFormats(UINT lastfmt)
{
  if (!m_clip_curfmts.GetSize()) return 0;
  if (lastfmt == 0) return (UINT)m_clip_curfmts.Get(0);
  int x;
  for (x = m_clip_curfmts.GetSize()-2; x >= 0; x--) // scan backwards to avoid dupes causing infinite loops
  {
    if (m_clip_curfmts.Get(x) == (char*)lastfmt)
      return (UINT)m_clip_curfmts.Get(x+1);
  }
  return 0;
}

HANDLE GetClipboardData(UINT type)
{
  NSString *fmt=m_clip_fmts.Get(type-1);
  if (!fmt) return 0;
  NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:@"SWELL_APP"];

	NSData *data=[pasteboard dataForType:fmt];
	if (!data) return 0; 
  int l=[data length];
  HANDLE h=GlobalAlloc(0,l);  
	memcpy(GlobalLock(h),[data bytes],l);
  GlobalUnlock(h);
  m_clip_recs.Add(h);
  
	return h;
}


void EmptyClipboard()
{
}

void SetClipboardData(UINT type, HANDLE h)
{
  NSString *fmt=m_clip_fmts.Get(type-1);
  if (fmt)
  {
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:@"SWELL_APP"];
    [pasteboard declareTypes:[NSArray arrayWithObject:fmt] owner:nil];
    void *buf=GlobalLock(h);
    int bufsz=GlobalSize(h);
    NSData *data=[NSData dataWithBytes:buf length:bufsz];
    [pasteboard setData:data forType:fmt];
    GlobalUnlock(h);
  }
  GlobalFree(h);
  
}

UINT RegisterClipboardFormat(const char *desc)
{
  NSString *s=(NSString*)SWELL_CStringToCFString(desc);
  int x;
  for (x = 0; x < m_clip_fmts.GetSize(); x ++)
  {
    NSString *ts=m_clip_fmts.Get(x);
    if ([ts compare:s]==NSOrderedSame)
    {
      [s release];
      return x+1;
    }
  }
  m_clip_fmts.Add(s);
  return m_clip_fmts.GetSize();
}



HIMAGELIST ImageList_CreateEx()
{
  return new WDL_PtrList<char>;
}

void ImageList_Destroy(HIMAGELIST list)
{
  if (!list) return;
  WDL_PtrList<char> *p=(WDL_PtrList<char>*)list;
  p->Empty(true,DeleteObject);
  delete p;
}

void ImageList_ReplaceIcon(HIMAGELIST list, int offset, HICON image)
{
  if (!image || !list) return;
  WDL_PtrList<char> *l=(WDL_PtrList<char> *)list;
  if (offset<0||offset>=l->GetSize()) l->Add((char*)image);
  else
  {
    HICON old=l->Get(offset);
    l->Set(offset,(char*)image);
    if (old) DestroyIcon(old);
  }
}



#endif


///////// PostMessage emulation

BOOL PostMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  return SWELL_Internal_PostMessage(hwnd,message,wParam,lParam);
}

void SWELL_MessageQueue_Clear(HWND h)
{
  SWELL_Internal_PMQ_ClearAllMessages(h);
}



// implementation of postmessage stuff




typedef struct PMQ_rec
{
  HWND hwnd;
  UINT msg;
  WPARAM wParam;
  LPARAM lParam;

  struct PMQ_rec *next;
  bool istimerpoo;
} PMQ_rec;

static WDL_Mutex *m_pmq_mutex;
static PMQ_rec *m_pmq, *m_pmq_empty, *m_pmq_tail;
static int m_pmq_size;
static guint m_pmq_timer;
#define MAX_POSTMESSAGE_SIZE 1024

static gboolean postmessagetimer_callback(gpointer data)
{
  SWELL_MessageQueue_Flush();
  return TRUE;
}

void SWELL_Internal_PostMessage_Init()
{
  if (m_pmq_mutex) return;
  
  m_pmq_mainthread=pthread_self();
  m_pmq_mutex = new WDL_Mutex;
  
  m_pmq_timer = g_timeout_add(50,postmessagetimer_callback,NULL);

}

void SWELL_MessageQueue_Flush()
{
  if (!m_pmq_mutex) return;
  
  m_pmq_mutex->Enter();
  PMQ_rec *p=m_pmq, *startofchain=m_pmq;
  m_pmq=m_pmq_tail=0;
  m_pmq_mutex->Leave();
  
  int cnt=0;
  // process out queue
  while (p)
  {
    // process this message
    if (p->istimerpoo)
    {
      if (p->wParam == (unsigned int)-1)  KillTimer(p->hwnd,p->msg);
      else SetTimer(p->hwnd,p->msg,p->wParam,0);
    }
    else
      SendMessage(p->hwnd,p->msg,p->wParam,p->lParam); 

    cnt ++;
    if (!p->next) // add the chain back to empties
    {
      m_pmq_mutex->Enter();
      m_pmq_size-=cnt;
      p->next=m_pmq_empty;
      m_pmq_empty=startofchain;
      m_pmq_mutex->Leave();
      break;
    }
    p=p->next;
  }
}

void SWELL_Internal_PMQ_ClearAllMessages(HWND hwnd)
{
  if (!m_pmq_mutex) return;
  
  m_pmq_mutex->Enter();
  PMQ_rec *p=m_pmq;
  PMQ_rec *lastrec=NULL;
  while (p)
  {
    if (hwnd && p->hwnd != hwnd) { lastrec=p; p=p->next; }
    else
    {
      PMQ_rec *next=p->next; 
      
      p->next=m_pmq_empty; // add p to empty list
      m_pmq_empty=p;
      m_pmq_size--;
      
      
      if (p==m_pmq_tail) m_pmq_tail=lastrec; // update tail
      
      if (lastrec)  p = lastrec->next = next;
      else p = m_pmq = next;
    }
  }
  m_pmq_mutex->Leave();
}

static void SWELL_pmq_settimer(HWND h, int timerid, int rate)
{
  if (!h||!m_pmq_mutex) return;
  WDL_MutexLock lock(m_pmq_mutex);
  
  PMQ_rec *rec=m_pmq;
  while (rec)
  {
    if (rec->istimerpoo && rec->hwnd == h && rec->msg == (unsigned int)timerid)
    {
      rec->wParam = rate; // adjust to new rate
      return;
    }
    rec=rec->next;
  }  
  
  rec=m_pmq_empty;
  if (rec) m_pmq_empty=rec->next;
  else rec=(PMQ_rec*)malloc(sizeof(PMQ_rec));
  rec->next=0;
  rec->hwnd=h;
  rec->msg=timerid;
  rec->wParam=rate;
  rec->lParam=0;
  rec->istimerpoo=true;

  if (m_pmq_tail) m_pmq_tail->next=rec;
  else 
  {
    PMQ_rec *p=m_pmq;
    while (p && p->next) p=p->next; // shouldnt happen unless m_pmq is NULL As well but why not for safety
    if (p) p->next=rec;
    else m_pmq=rec;
  }
  m_pmq_tail=rec;
  m_pmq_size++;
}

BOOL SWELL_Internal_PostMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!hwnd||!m_pmq_mutex) return FALSE;

  BOOL ret=FALSE;
  m_pmq_mutex->Enter();

  if (m_pmq_empty||m_pmq_size<MAX_POSTMESSAGE_SIZE)
  {
    PMQ_rec *rec=m_pmq_empty;
    if (rec) m_pmq_empty=rec->next;
    else rec=(PMQ_rec*)malloc(sizeof(PMQ_rec));
    rec->next=0;
    rec->hwnd=hwnd;
    rec->msg=msg;
    rec->wParam=wParam;
    rec->lParam=lParam;
    rec->istimerpoo=false;

    if (m_pmq_tail) m_pmq_tail->next=rec;
    else 
    {
      PMQ_rec *p=m_pmq;
      while (p && p->next) p=p->next; // shouldnt happen unless m_pmq is NULL As well but why not for safety
      if (p) p->next=rec;
      else m_pmq=rec;
    }
    m_pmq_tail=rec;
    m_pmq_size++;

    ret=TRUE;
  }

  m_pmq_mutex->Leave();

  return ret;
}



#if 0


int EnumPropsEx(HWND hwnd, PROPENUMPROCEX proc, LPARAM lParam)
{
  if (!hwnd || ![(id)hwnd respondsToSelector:@selector(swellEnumProps:lp:)]) return -1;
  return (int)[(SWELL_hwndChild *)hwnd swellEnumProps:proc lp:lParam];
}

HANDLE GetProp(HWND hwnd, const char *name)
{
  if (!hwnd || ![(id)hwnd respondsToSelector:@selector(swellGetProp:wantRemove:)]) return NULL;
  return (HANDLE)[(SWELL_hwndChild *)hwnd swellGetProp:name wantRemove:NO];
}

BOOL SetProp(HWND hwnd, const char *name, HANDLE val)
{
  if (!hwnd || ![(id)hwnd respondsToSelector:@selector(swellSetProp:value:)]) return FALSE;
  return (BOOL)!![(SWELL_hwndChild *)hwnd swellSetProp:name value:val];
}

HANDLE RemoveProp(HWND hwnd, const char *name)
{
  if (!hwnd || ![(id)hwnd respondsToSelector:@selector(swellGetProp:wantRemove:)]) return NULL;
  return (HANDLE)[(SWELL_hwndChild *)hwnd swellGetProp:name wantRemove:YES];
}


int GetSystemMetrics(int p)
{
switch (p)
{
case SM_CXSCREEN:
case SM_CYSCREEN:
{
  NSScreen *s=[NSScreen mainScreen];
  if (!s) return 1024;
  return p==SM_CXSCREEN ? [s frame].size.width : [s frame].size.height;
}
case SM_CXHSCROLL: return 16;
case SM_CYHSCROLL: return 16;
case SM_CXVSCROLL: return 16;
case SM_CYVSCROLL: return 16;
}
return 0;
}

BOOL ScrollWindow(HWND hwnd, int xamt, int yamt, const RECT *lpRect, const RECT *lpClipRect)
{
  if (hwnd && [(id)hwnd isKindOfClass:[NSWindow class]]) hwnd=(HWND)[(id)hwnd contentView];
  if (!hwnd || ![(id)hwnd isKindOfClass:[NSView class]]) return FALSE;

  if (!xamt && !yamt) return FALSE;
  
  // move child windows only
  if (1)
  {
    if (xamt || yamt)
    {
      NSArray *ar=[(NSView*)hwnd subviews];
      int i,c=[ar count];
      for(i=0;i<c;i++)
      {
        NSView *v=(NSView *)[ar objectAtIndex:i];
        NSRect r=[v frame];
        r.origin.x+=xamt;
        r.origin.y+=yamt;
        [v setFrame:r];
      }
    }
  }
  else
  {
    NSRect r=[(NSView*)hwnd bounds];
    r.origin.x -= xamt;
    r.origin.y -= yamt;
    [(id)hwnd setBoundsOrigin:r.origin];
    [(id)hwnd setNeedsDisplay:YES];
  }
  return TRUE;
}

HWND FindWindowEx(HWND par, HWND lastw, const char *classname, const char *title)
{
  if (!par&&!lastw) return NULL; // need to implement this modes
  HWND h=lastw?GetWindow(lastw,GW_HWNDNEXT):GetWindow(par,GW_CHILD);
  while (h)
  {
    bool isOk=true;
    if (title)
    {
      char buf[512];
      buf[0]=0;
      GetWindowText(h,buf,sizeof(buf));
      if (strcmp(title,buf)) isOk=false;
    }
    if (classname)
    {
      if (!stricmp(classname,"Static") && ![(id)h isKindOfClass:[NSTextField class]]) isOk=false;
      // todo: other classname translations
    }
    
    if (isOk) return h;
    h=GetWindow(h,GW_HWNDNEXT);
  }
  return h;
}

HTREEITEM TreeView_InsertItem(HWND hwnd, TV_INSERTSTRUCT *ins)
{
  if (!hwnd || !ins) return 0;
  if (![(id)hwnd isKindOfClass:[SWELL_TreeView class]]) return 0;
  
  SWELL_TreeView *tv=(SWELL_TreeView*)hwnd;

  SWELL_TreeView_Item *par=NULL;
  int inspos=0;
  
  if (ins->hParent && ins->hParent != TVI_ROOT && ins->hParent != TVI_FIRST && ins->hParent != TVI_LAST && ins->hParent != TVI_SORT)
  {
    if ([tv findItem:ins->hParent parOut:&par idxOut:&inspos])
    {
      par = (SWELL_TreeView_Item *)ins->hParent; 
    }
    else return 0;
  }
  
  if (ins->hInsertAfter == TVI_FIRST) inspos=0;
  else if (ins->hInsertAfter == TVI_LAST || ins->hInsertAfter == TVI_SORT || !ins->hInsertAfter) inspos=par ? par->m_children.GetSize() : tv->m_items ? tv->m_items->GetSize() : 0;
  else inspos = par ? par->m_children.Find((SWELL_TreeView_Item*)ins->hInsertAfter)+1 : tv->m_items ? tv->m_items->Find((SWELL_TreeView_Item*)ins->hInsertAfter)+1 : 0;      
  
  SWELL_TreeView_Item *item=new SWELL_TreeView_Item;
  if (ins->item.mask & TVIF_CHILDREN)
    item->m_haschildren = !!ins->item.cChildren;
  if (ins->item.mask & TVIF_PARAM) item->m_param = ins->item.lParam;
  if (ins->item.mask & TVIF_TEXT) item->m_value = strdup(ins->item.pszText);
  if (!par)
  {
    if (!tv->m_items) tv->m_items = new WDL_PtrList<SWELL_TreeView_Item>;
    tv->m_items->Insert(inspos,item);
  }
  else par->m_children.Insert(inspos,item);
  
  [tv reloadData];
  return (HTREEITEM) item;
}

BOOL TreeView_Expand(HWND hwnd, HTREEITEM item, UINT flag)
{
  if (!hwnd || !item) return false;
  
  if (![(id)hwnd isKindOfClass:[SWELL_TreeView class]]) return false;
  
  SWELL_TreeView *tv=(SWELL_TreeView*)hwnd;
  
  id itemid=((SWELL_TreeView_Item*)item)->m_dh;
  bool isExp=!![tv isItemExpanded:itemid];
  
  if (flag == TVE_EXPAND && !isExp) [tv expandItem:itemid];
  else if (flag == TVE_COLLAPSE && isExp) [tv collapseItem:itemid];
  else if (flag==TVE_TOGGLE) 
  {
    if (isExp) [tv collapseItem:itemid];
    else [tv expandItem:itemid];
  }
  else return FALSE;

  return TRUE;
  
}

HTREEITEM TreeView_GetSelection(HWND hwnd)
{ 
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TreeView class]]) return NULL;
  
  SWELL_TreeView *tv=(SWELL_TreeView*)hwnd;
  int idx=[tv selectedRow];
  if (idx<0) return NULL;
  
  SWELL_DataHold *t=[tv itemAtRow:idx];
  if (t) return (HTREEITEM)[t getValue];
  return NULL;
  
}

void TreeView_DeleteItem(HWND hwnd, HTREEITEM item)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TreeView class]]) return;
  SWELL_TreeView *tv=(SWELL_TreeView*)hwnd;
  
  SWELL_TreeView_Item *par=NULL;
  int idx=0;
  
  if ([tv findItem:item parOut:&par idxOut:&idx])
  {
    if (par)
    {
      par->m_children.Delete(idx,true);
    }
    else if (tv->m_items)
    {
      tv->m_items->Delete(idx,true);
    }
    [tv reloadData];
  }
}

void TreeView_SelectItem(HWND hwnd, HTREEITEM item)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TreeView class]]) return;
  
  int row=[(SWELL_TreeView*)hwnd rowForItem:((SWELL_TreeView_Item*)item)->m_dh];
  if (row>=0)
         [(SWELL_TreeView*)hwnd selectRow:row byExtendingSelection:NO];            
  static int __rent;
  if (!__rent)
  {
    __rent=1;
    NMTREEVIEW nm={{(HWND)hwnd,[(SWELL_TreeView*)hwnd tag],TVN_SELCHANGED},};
    SendMessage(GetParent(hwnd),WM_NOTIFY,[(SWELL_TreeView*)hwnd tag],(LPARAM)&nm);      
    __rent=0;
  }
}

BOOL TreeView_GetItem(HWND hwnd, LPTVITEM pitem)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TreeView class]] || !pitem || !(pitem->mask & TVIF_HANDLE) || !(pitem->hItem)) return FALSE;
  
  SWELL_TreeView_Item *ti = (SWELL_TreeView_Item*)pitem->hItem;
  pitem->cChildren = ti->m_haschildren ? 1:0;
  pitem->lParam = ti->m_param;
  if ((pitem->mask&TVIF_TEXT)&&pitem->pszText&&pitem->cchTextMax>0)
  {
    lstrcpyn(pitem->pszText,ti->m_value?ti->m_value:"",pitem->cchTextMax);
  }
  pitem->state=0;
  
  
  int itemRow = [(SWELL_TreeView*)hwnd rowForItem:ti->m_dh];
  if (itemRow >= 0 && [(SWELL_TreeView*)hwnd isRowSelected:itemRow])
    pitem->state |= TVIS_SELECTED;   
  if ([(SWELL_TreeView*)hwnd isItemExpanded:ti->m_dh])
    pitem->state |= TVIS_EXPANDED;   
  
  return TRUE;
}

BOOL TreeView_SetItem(HWND hwnd, LPTVITEM pitem)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TreeView class]] || !pitem || !(pitem->mask & TVIF_HANDLE) || !(pitem->hItem)) return FALSE;
  
  SWELL_TreeView_Item *par=NULL;
  int idx=0;
  
  if (![(SWELL_TreeView*)hwnd findItem:pitem->hItem parOut:&par idxOut:&idx]) return FALSE;
  
  SWELL_TreeView_Item *ti = (SWELL_TreeView_Item*)pitem->hItem;
  
  if (pitem->mask & TVIF_CHILDREN) ti->m_haschildren = pitem->cChildren?1:0;
  if (pitem->mask & TVIF_PARAM)  ti->m_param =  pitem->lParam;
  
  if ((pitem->mask&TVIF_TEXT)&&pitem->pszText)
  {
    free(ti->m_value);
    ti->m_value=strdup(pitem->pszText);
  }

  if (pitem->stateMask & TVIS_SELECTED)
  {
    int itemRow = [(SWELL_TreeView*)hwnd rowForItem:ti->m_dh];
    if (itemRow >= 0)
    {
      if (pitem->state&TVIS_SELECTED)
      {
        [(SWELL_TreeView*)hwnd selectRow:itemRow byExtendingSelection:NO];
        
        static int __rent;
        if (!__rent)
        {
          __rent=1;
          NMTREEVIEW nm={{(HWND)hwnd,[(SWELL_TreeView*)hwnd tag],TVN_SELCHANGED},};
          SendMessage(GetParent(hwnd),WM_NOTIFY,[(SWELL_TreeView*)hwnd tag],(LPARAM)&nm);      
          __rent=0;
        }
        
      }
      else
      {
        // todo figure out unselect?!
//         [(SWELL_TreeView*)hwnd selectRow:itemRow byExtendingSelection:NO];
      }
    }
  }
  
  if (pitem->stateMask & TVIS_EXPANDED)
    TreeView_Expand(hwnd,pitem->hItem,(pitem->state&TVIS_EXPANDED)?TVE_EXPAND:TVE_COLLAPSE);
    
  
  return TRUE;
}

HTREEITEM TreeView_HitTest(HWND hwnd, TVHITTESTINFO *hti)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TreeView class]] || !hti) return NULL;
  
  return NULL; // todo implement
}

HTREEITEM TreeView_GetRoot(HWND hwnd)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TreeView class]]) return NULL;
  SWELL_TreeView *tv=(SWELL_TreeView*)hwnd;
  
  if (!tv->m_items) return 0;
  return (HTREEITEM) tv->m_items->Get(0);
}

HTREEITEM TreeView_GetChild(HWND hwnd, HTREEITEM item)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TreeView class]]) return NULL;
  SWELL_TreeView *tv=(SWELL_TreeView*)hwnd;

  SWELL_TreeView_Item *titem=(SWELL_TreeView_Item *)item;
  if (!titem) return TreeView_GetRoot(hwnd);
  
  return (HTREEITEM) titem->m_children.Get(0);
}
HTREEITEM TreeView_GetNextSibling(HWND hwnd, HTREEITEM item)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TreeView class]]) return NULL;
  SWELL_TreeView *tv=(SWELL_TreeView*)hwnd;

  if (!item) return TreeView_GetRoot(hwnd);
  
  SWELL_TreeView_Item *par=NULL;
  int idx=0;  
  if ([tv findItem:item parOut:&par idxOut:&idx])
  {
    if (par)
    {
      return par->m_children.Get(idx+1);
    }    
  }
  return 0;
}


BOOL ShellExecute(HWND hwndDlg, const char *action,  const char *content1, const char *content2, const char *content3, int blah)
{
  if (content1 && !strnicmp(content1,"http://",7))
  {
     NSWorkspace *wk = [NSWorkspace sharedWorkspace];
     if (!wk) return FALSE;
     NSString *fnstr=(NSString *)SWELL_CStringToCFString(content1);
     BOOL ret=[wk openURL:[NSURL URLWithString:fnstr]];
     [fnstr release];
     return ret;
  }
  
  if (content1 && !stricmp(content1,"explorer.exe")) content1="";
  else if (content1 && (!stricmp(content1,"notepad.exe")||!stricmp(content1,"notepad"))) content1="TextEdit.app";
  
  if (content2 && !stricmp(content2,"explorer.exe")) content2="";

  if (content1 && content2 && *content1 && *content2)
  {
      NSWorkspace *wk = [NSWorkspace sharedWorkspace];
      if (!wk) return FALSE;
      NSString *appstr=(NSString *)SWELL_CStringToCFString(content1);
      NSString *fnstr=(NSString *)SWELL_CStringToCFString(content2);
      BOOL ret=[wk openFile:fnstr withApplication:appstr andDeactivate:YES];
      [fnstr release];
      [appstr release];
      return ret;
  }
  else if ((content1&&*content1) || (content2&&*content2))
  {
      const char *fn = (content1 && *content1) ? content1 : content2;
      NSWorkspace *wk = [NSWorkspace sharedWorkspace];
      if (!wk) return FALSE;
      NSString *fnstr=(NSString *)SWELL_CStringToCFString(fn);
      BOOL ret;
      
      if (strlen(fn)>4 && !stricmp(fn+strlen(fn)-4,".app")) ret=[wk launchApplication:fnstr];
      else ret=[wk openFile:fnstr];
      
      [fnstr release];
      return ret;
  }
  return FALSE;
}




@implementation SWELL_FocusRectWnd

-(BOOL)isOpaque { return YES; }
-(void) drawRect:(NSRect)rect
{
  NSColor *col=[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0];
  [col set];
  
  CGRect r = CGRectMake(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height);
  
  CGContextRef ctx = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
  
  CGContextFillRect(ctx,r);	         
  
}
@end

// r=NULL to "free" handle
// otherwise r is in hwndPar coordinates
void SWELL_DrawFocusRect(HWND hwndPar, RECT *rct, void **handle)
{
  if (!handle) return;
  NSWindow *wnd = (NSWindow *)*handle;
  
  if (!rct)
  {
    if (wnd)
    {
      NSWindow *ow=[wnd parentWindow];
      if (ow) [ow removeChildWindow:wnd];
//      [wnd setParentWindow:nil];
      [wnd close];
      *handle=0;
    }
  }
  else if (hwndPar)
  {
    RECT r=*rct;
    ClientToScreen(hwndPar,((LPPOINT)&r));
    ClientToScreen(hwndPar,((LPPOINT)&r)+1);
    if (r.top>r.bottom) { int a=r.top; r.top=r.bottom;r.bottom=a; }
    NSRect rr=NSMakeRect(r.left,r.top,r.right-r.left,r.bottom-r.top);
    
    if (!wnd)
    {
      NSWindow *par=nil;
      if ([(id)hwndPar isKindOfClass:[NSWindow class]]) par=(NSWindow *)hwndPar;
      else if ([(id)hwndPar isKindOfClass:[NSView class]]) par=[(NSView *)hwndPar window];
      else return;
      
      *handle  = wnd = [[NSWindow alloc] initWithContentRect:rr styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES];
      [wnd setOpaque:YES];
      [wnd setAlphaValue:0.5];
      [wnd setExcludedFromWindowsMenu:YES];
      [wnd setIgnoresMouseEvents:YES];
      [wnd setContentView:[[SWELL_FocusRectWnd alloc] init]];
      
      [par addChildWindow:wnd ordered:NSWindowAbove];
      //    [wnd setParentWindow:par];
//      [wnd orderWindow:NSWindowAbove relativeTo:[par windowNumber]];
    }
    
    [wnd setFrame:rr display:YES];    
  }
}

@implementation SWELL_ThreadTmp
-(void)bla:(id)obj
{
  [NSThread exit];
}
@end

void SWELL_EnsureMultithreadedCocoa()
{
  static int a;
  if (!a)
  {
    a++;
    if (![NSThread isMultiThreaded]) // force cocoa into multithreaded mode
    {
      SWELL_ThreadTmp *t=[[SWELL_ThreadTmp alloc] init]; 
      [NSThread detachNewThreadSelector:@selector(bla:) toTarget:t withObject:t];
///      [t release];
    }
  }
}

// used by swell.cpp (lazy these should go elsewhere)
void *SWELL_InitAutoRelease()
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  return (void *)pool;
}
void SWELL_QuitAutoRelease(void *p)
{
  if (p)
    [(NSAutoreleasePool*)p release];
}



@implementation SWELL_PopUpButton
-(void)setSwellStyle:(LONG)style { m_style=style; }
-(LONG)getSwellStyle { return m_style; }
@end

@implementation SWELL_ComboBox
-(void)setSwellStyle:(LONG)style { m_style=style; }
-(LONG)getSwellStyle { return m_style; }
-(id)init { self = [super init]; if (self) { m_ids=new WDL_PtrList<char>; }  return self; }
-(void)dealloc { delete m_ids; [super dealloc];  }
@end




bool SWELL_HandleMouseEvent(NSEvent *evt)
{
  int etype = [evt type];
  if (GetCapture()) return false;
  if (etype >= NSLeftMouseDown && etype <= NSRightMouseDragged)
  {
  }
  else return false;
  
  NSWindow *w = [evt window];
  if (w)
  {
    NSView *cview = [w contentView];
    NSView *besthit=NULL;
    if (cview)
    {
      NSPoint lpt = [evt locationInWindow];    
      NSView *hitv=[cview hitTest:lpt];
      lpt = [w convertBaseToScreen:lpt];
      
      int xpos=(int)(lpt.x+0.5);
      int ypos=(int)(lpt.y+0.5);
      
      while (hitv)
      {
        if ([hitv respondsToSelector:@selector(onSwellMessage:p1:p2:)])
        {
          int ht=(int) [(SWELL_hwndChild*)hitv onSwellMessage:WM_NCHITTEST p1:0 p2:MAKELPARAM(xpos,ypos)];
          if (ht && ht != HTCLIENT) besthit=hitv;
        }
        if (hitv==cview) break;
        hitv = [hitv superview];
      }
    }
    if (besthit)
    {
      if (etype == NSLeftMouseDown) [besthit mouseDown:evt];
      else if (etype == NSLeftMouseUp) [besthit mouseUp:evt];
      else if (etype == NSLeftMouseDragged) [besthit mouseDragged:evt];
      else if (etype == NSRightMouseDown) [besthit rightMouseDown:evt];
      else if (etype == NSRightMouseUp) [besthit rightMouseUp:evt];
      else if (etype == NSRightMouseDragged) [besthit rightMouseDragged:evt];
      else if (etype == NSMouseMoved) [besthit mouseMoved:evt];
      else return false;
      
      return true;
    }
  }
  return false;
}

#endif//if 0

#endif
