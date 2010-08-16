/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: main.cpp (example use of LICE)
  See lice.h for license and other information
*/

#include "../lice.h"
#include <math.h>
#include <stdio.h>
#include "../../wingui/membitmap.h"

#include "resource.h"

WDL_WinMemBitmap fbmem;
LICE_SysBitmap *framebuffer;

DWORD start_t;
int framecnt;

BOOL WINAPI dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
  return 0;

  case WM_PAINT:
    {
      PAINTSTRUCT ps;

      HDC dc = BeginPaint(hwndDlg, &ps);
      RECT r;
      GetClientRect(hwndDlg, &r);
      r.top+=40;
      if (r.top >= r.bottom) r.top=r.bottom-1;
 

      int mode=0;


      HDC hdc;
      if (mode)
      {
        fbmem.DoSize(ps.hdc,r.right-r.left,r.bottom-r.top);
        hdc=fbmem.GetDC();
      }
      else
      {
        if (framebuffer->resize(r.right-r.left,r.bottom-r.top))
        {
          memset(framebuffer->getBits(),0,framebuffer->getWidth()*framebuffer->getHeight()*4);
        }
        hdc=framebuffer->getDC();
      }


      framecnt++;
      if (!(framecnt&15))
      {
        char buf[512];
        sprintf(buf,"%.5ffps\n",framecnt*1000.0/(GetTickCount()-start_t));
        OutputDebugString(buf);
      }
      int n;
      SelectObject(hdc,GetStockObject(rand()&1?WHITE_PEN:BLACK_PEN));
      for (n = 0; n < 800; n ++)
      {
        Rectangle(hdc,rand()%(r.right-r.left),rand()%(r.bottom-r.top),
          64,64);
//        LineTo(hdc,rand()%(r.right-r.left),rand()%(r.bottom-r.top));
      }
  
      BitBlt(dc,r.left,r.top,r.right-r.left,r.bottom-r.top,hdc,0,0,SRCCOPY);
//      bmp->blitToDC(dc, NULL, 0, 0);

      EndPaint(hwndDlg, &ps);
    }
    break;
  case WM_CLOSE:
        DestroyWindow(hwndDlg);
      break;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
      case IDCANCEL:
        DestroyWindow(hwndDlg);
      break;
    }
    break;
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nShowCmd)
{
  framebuffer = new LICE_SysBitmap(0,0);

  HWND h=CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, dlgProc);
  ShowWindow(h,SW_SHOW);
  start_t=GetTickCount();
  while (IsWindow(h))
  {
    MSG msg;
    if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    InvalidateRect(h,NULL,FALSE);
    UpdateWindow(h);
  }

  delete framebuffer;
  return 0;
}