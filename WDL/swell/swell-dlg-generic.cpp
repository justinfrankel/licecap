#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-dlggen.h"

#include "../ptrlist.h"

static HMENU g_swell_defaultmenu,g_swell_defaultmenumodal;

void (*SWELL_DDrop_onDragLeave)();
void (*SWELL_DDrop_onDragOver)(POINT pt);
void (*SWELL_DDrop_onDragEnter)(void *hGlobal, POINT pt);
const char* (*SWELL_DDrop_getDroppedFileTargetPath)(const char* extension);

bool SWELL_owned_windows_levelincrease=false;

#include "swell-internal.h"

static SWELL_DialogResourceIndex *resById(SWELL_DialogResourceIndex *reshead, const char *resid)
{
  SWELL_DialogResourceIndex *p=reshead;
  while (p)
  {
    if (p->resid == resid) return p;
    p=p->_next;
  }
  return 0;
}

// keep list of modal dialogs
struct modalDlgRet { 
  HWND hwnd; 
  bool has_ret;
  int ret;
};


static WDL_PtrList<modalDlgRet> s_modalDialogs;

HWND DialogBoxIsActive()
{
  int a = s_modalDialogs.GetSize();
  while (a-- > 0)
  {
    modalDlgRet *r = s_modalDialogs.Get(a);
    if (r && !r->has_ret && r->hwnd) return r->hwnd; 
  }
  return NULL;
}

void EndDialog(HWND wnd, int ret)
{   
  if (!wnd) return;
  
  int a = s_modalDialogs.GetSize();
  while (a-->0)
  {
    modalDlgRet *r = s_modalDialogs.Get(a);
    if (r && r->hwnd == wnd)  
    {
      r->ret = ret;
      if (r->has_ret) return;

      r->has_ret=true;
    }
  }
  DestroyWindow(wnd);
}

int SWELL_DialogBox(SWELL_DialogResourceIndex *reshead, const char *resid, HWND parent,  DLGPROC dlgproc, LPARAM param)
{
  SWELL_DialogResourceIndex *p=resById(reshead,resid);
  if (resid) // allow modal dialogs to be created without template
  {
    if (!p||(p->windowTypeFlags&SWELL_DLG_WS_CHILD)) return -1;
  }
  else if (parent)
  {
    resid = (const char *)(INT_PTR)(0x400002); // force non-child, force no minimize box
  }


  int ret=-1;
  HWND hwnd = SWELL_CreateDialog(reshead,resid,parent,dlgproc,param);
  // create dialog
  if (hwnd)
  {
    ReleaseCapture(); // force end of any captures

    WDL_PtrList<HWND__> enwnds;
    extern HWND__ *SWELL_topwindows;
    HWND a = SWELL_topwindows;
    while (a)
    {
      if (a->m_enabled && a != hwnd) { EnableWindow(a,FALSE); enwnds.Add(a); }
      a = a->m_next;
    }

    modalDlgRet r = { hwnd,false, -1 };
    s_modalDialogs.Add(&r);
    ShowWindow(hwnd,SW_SHOW);
    while (s_modalDialogs.Find(&r)>=0 && !r.has_ret)
    {
      void SWELL_RunMessageLoop();
      SWELL_RunMessageLoop();
      Sleep(10);
    }
    ret=r.ret;
    s_modalDialogs.DeletePtr(&r);

    a = SWELL_topwindows;
    while (a)
    {
      if (!a->m_enabled && a != hwnd && enwnds.Find(a)>=0) EnableWindow(a,TRUE);
      a = a->m_next;
    }
  }
  // while in list, do something
  return ret;
}

HWND SWELL_CreateDialog(SWELL_DialogResourceIndex *reshead, const char *resid, HWND parent, DLGPROC dlgproc, LPARAM param)
{
  int forceStyles=0; // 1=resizable, 2=no minimize, 4=no close
  bool forceNonChild=false;
  if ((((INT_PTR)resid)&~0xf)==0x400000)
  {
    forceStyles = (int) (((INT_PTR)resid)&0xf);
    if (forceStyles) forceNonChild=true;
    resid=0;
  }
  SWELL_DialogResourceIndex *p=resById(reshead,resid);
  if (!p&&resid) return 0;
  
  RECT r={0,0,p?p->width : 300, p?p->height : 200};
  HWND owner=NULL;

  if (!forceNonChild && parent && (!p || (p->windowTypeFlags&SWELL_DLG_WS_CHILD)))
  {
  } 
  else 
  {
    owner = parent;
    parent = NULL; // top level window
  }

  HWND__ *h = new HWND__(parent,0,&r,NULL,false,NULL,NULL, owner);
  if (forceNonChild || (p && !(p->windowTypeFlags&SWELL_DLG_WS_CHILD)))
  {
    if ((forceStyles&1) || (p && (p->windowTypeFlags&SWELL_DLG_WS_RESIZABLE))) 
      h->m_style |= WS_THICKFRAME|WS_CAPTION;
    else h->m_style |= WS_CAPTION;
  }
  else if (!p && !parent) h->m_style |= WS_CAPTION;
  else if (parent && (!p || (p->windowTypeFlags&SWELL_DLG_WS_CHILD))) h->m_style |= WS_CHILD;

  if (p)
  {
    p->createFunc(h,p->windowTypeFlags);
    if (p->title) SetWindowText(h,p->title);

    h->m_dlgproc = dlgproc;
    h->m_wndproc = SwellDialogDefaultWindowProc;

    //HWND hFoc=m_children;
//    while (hFoc && !hFoc->m_wantfocus) hFoc=hFoc->m_next;
 //   if (!hFoc) hFoc=this;
  //  if (dlgproc(this,WM_INITDIALOG,(WPARAM)hFoc,0)&&hFoc) SetFocus(hFoc);

    h->m_dlgproc(h,WM_INITDIALOG,0,param);
  } 
  else
  {
    h->m_wndproc = (WNDPROC)dlgproc;
    h->m_wndproc(h,WM_CREATE,0,param);
  }
    
  return h;
}


HMENU SWELL_GetDefaultWindowMenu() { return g_swell_defaultmenu; }
void SWELL_SetDefaultWindowMenu(HMENU menu)
{
  g_swell_defaultmenu=menu;
}
HMENU SWELL_GetDefaultModalWindowMenu() 
{ 
  return g_swell_defaultmenumodal; 
}
void SWELL_SetDefaultModalWindowMenu(HMENU menu)
{
  g_swell_defaultmenumodal=menu;
}



SWELL_DialogResourceIndex *SWELL_curmodule_dialogresource_head; // this eventually will go into a per-module stub file


static char* s_dragdropsrcfn = 0;
static void (*s_dragdropsrccallback)(const char*) = 0;

void SWELL_InitiateDragDrop(HWND hwnd, RECT* srcrect, const char* srcfn, void (*callback)(const char* dropfn))
{
  SWELL_FinishDragDrop();

  if (1) return;

  s_dragdropsrcfn = strdup(srcfn);
  s_dragdropsrccallback = callback;
  
  char* p = s_dragdropsrcfn+strlen(s_dragdropsrcfn)-1;
  while (p >= s_dragdropsrcfn && *p != '.') --p;
  ++p;
  
} 

// owner owns srclist, make copies here etc
void SWELL_InitiateDragDropOfFileList(HWND hwnd, RECT *srcrect, const char **srclist, int srccount, HICON icon)
{
  SWELL_FinishDragDrop();

  if (1) return;
  
}

void SWELL_FinishDragDrop()
{
  free(s_dragdropsrcfn);
  s_dragdropsrcfn = 0;
  s_dragdropsrccallback = 0;  
}

#endif
