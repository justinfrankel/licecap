/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: main.cpp (example use of LICE)
  See lice.h for license and other information
*/

#include "../lice.h"
#include <math.h>

#include "resource.h"

LICE_IBitmap *bmp;
LICE_SysBitmap *framebuffer;

BOOL WINAPI dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SetTimer(hwndDlg,1,10,NULL);
  return 0;

  case WM_TIMER:
    InvalidateRect(hwndDlg,NULL,FALSE);
  return 0;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;

      HDC dc = BeginPaint(hwndDlg, &ps);
      RECT r;
      GetClientRect(hwndDlg,&r);

      if (framebuffer->resize(r.right,r.bottom))
      {
        memset(framebuffer->getBits(),0,framebuffer->getWidth()*framebuffer->getHeight()*4);
      }

      int x=rand()%(r.right+300)-150;
      int y=rand()%(r.bottom+300)-150;

#if 0
      if (rand()%6==0)
        LICE_Blit(framebuffer,bmp,x,y,NULL,-1.4,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA);
      else
        LICE_Blit(framebuffer,bmp,x,y,NULL,0.6,LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);
#elif 0
      LICE_Clear(framebuffer,0);
      double a=GetTickCount()/1000.0;

      double scale=(1.1+sin(a)*0.3);
      LICE_RotatedBlit(framebuffer,bmp,r.right*scale,r.bottom*scale,r.right*(1.0-scale*2.0),r.bottom*(1.0-scale*2.0),0,0,bmp->getWidth(),bmp->getHeight(),cos(a*0.3)*13.0,false,1.0,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA|LICE_BLIT_FILTER_BILINEAR,0.0,-bmp->getHeight()/2);

#elif 1
      double a=GetTickCount()/1000.0;

      double scale=(1.1+sin(a)*0.3);

      if (1)  // weirdness
      {
        LICE_RotatedBlit(framebuffer,framebuffer,0,0,r.right,r.bottom,0+sin(a*0.3)*16.0,0+sin(a*0.21)*16.0,r.right,r.bottom,cos(a*0.5)*0.13,false,254/255.0,LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
      }
      else // artifact-free mode
      {
        LICE_MemBitmap framebuffer_back;

        LICE_Copy(&framebuffer_back,framebuffer);
        LICE_RotatedBlit(framebuffer,&framebuffer_back,0,0,r.right,r.bottom,0+sin(a*0.3)*16.0,0+sin(a*0.21)*16.0,r.right,r.bottom,cos(a*0.5)*0.13,false,1.0,LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
      }
      //LICE_Clear(framebuffer,0);
      LICE_RotatedBlit(framebuffer,bmp,r.right*scale,r.bottom*scale,r.right*(1.0-scale*2.0),r.bottom*(1.0-scale*2.0),0,0,bmp->getWidth(),bmp->getHeight(),cos(a*0.3)*13.0,false,rand()%16==0 ? -0.5: 0.1,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA|LICE_BLIT_FILTER_BILINEAR);

#else
      //LICE_Clear(framebuffer,0);
      static double a;
      a+=0.04;
      int xsize=sin(a*3.0)*r.right*1.5;
      int ysize=sin(a*1.7)*r.bottom*1.5;

      if (rand()%3==0)
        LICE_ScaledBlit(framebuffer,bmp,r.right/2-xsize/2,r.bottom/2-ysize/2,xsize,ysize,0.0,0.0,bmp->getWidth(),bmp->getHeight(),-0.7,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_ADD|LICE_BLIT_FILTER_BILINEAR);
      else
        LICE_ScaledBlit(framebuffer,bmp,r.right/2-xsize/2,r.bottom/2-ysize/2,xsize,ysize,0.0,0.0,bmp->getWidth(),bmp->getHeight(),0.25,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);

#endif

      BitBlt(dc,0,0,framebuffer->getWidth(),framebuffer->getHeight(),framebuffer->getDC(),0,0,SRCCOPY);
//      bmp->blitToDC(dc, NULL, 0, 0);

      EndPaint(hwndDlg, &ps);
    }
    break;
  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDCANCEL:
      EndDialog(hwndDlg, 0);
      break;
    }
    break;
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nShowCmd)
{
  framebuffer = new LICE_SysBitmap(0,0);

  //char buf[512];
  //GetModuleFileName(hInstance,buf,sizeof(buf)-32);
  //strcat(buf,".png");
  //bmp = LICE_LoadPNG(buf);
  bmp = LICE_LoadPNGFromResource(hInstance, IDC_PNG1);
  if(!bmp) return 0;

  DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, dlgProc);

  delete bmp;
  delete framebuffer;
  return 0;
}