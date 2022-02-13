#ifndef _SWELL_WIN32_H_
#define _SWELL_WIN32_H_

/*************
** helpers to give swell-like functionality on win32
*/

#ifdef _WIN32
void SWELL_DisableContextMenu(HWND h, bool dis);



#ifndef SWELL_WIN32_DECLARE_ONLY

#include <windows.h>
#include "../wdltypes.h"

#define SWELL_DISABLE_CTX_OLDPROC_NAME "SWELLDisableCtxOldProc"

static LRESULT WINAPI swellDisableCtxNewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  HANDLE oldProc = uMsg != WM_CONTEXTMENU ? GetProp(hwnd,SWELL_DISABLE_CTX_OLDPROC_NAME) : NULL;
  if (uMsg == WM_DESTROY)
  {
    RemoveProp(hwnd,SWELL_DISABLE_CTX_OLDPROC_NAME);
    if (oldProc) SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LPARAM)oldProc);
  }
  return oldProc ? CallWindowProc((WNDPROC)oldProc,hwnd,uMsg,wParam,lParam) : DefWindowProc(hwnd,uMsg,wParam,lParam);
}

void SWELL_DisableContextMenu(HWND h, bool dis)
{
  char classname[512];
  if (WDL_NOT_NORMALLY(!h)) return;
  if (GetClassName(h,classname,sizeof(classname)) && !strcmp(classname,"ComboBox"))
  {
    HWND h2 = FindWindowEx(h,NULL,"Edit",NULL);
    if (h2) h = h2;
  }

  if (dis)
  {
    if (!GetProp(h,SWELL_DISABLE_CTX_OLDPROC_NAME))
      SetProp(h,SWELL_DISABLE_CTX_OLDPROC_NAME,(HANDLE)SetWindowLongPtr(h,GWLP_WNDPROC,(LPARAM)swellDisableCtxNewWndProc));
  }
  else
  {
    LPARAM op = (LPARAM)GetProp(h,SWELL_DISABLE_CTX_OLDPROC_NAME);
    if (op) SetWindowLongPtr(h,GWLP_WNDPROC,op);
    RemoveProp(h,SWELL_DISABLE_CTX_OLDPROC_NAME);
  }
}

#endif // !SWELL_WIN32_DECLARE_ONLY

#endif // _WIN32

#endif // _SWELL_WIN32_H_
