/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: main.cpp (example use of LICE)
  See lice.h for license and other information
*/

#include "../lice.h"
#include <math.h>

#include "resource.h"

#define NUM_EFFECTS 14

LICE_IBitmap *bmp;
LICE_IBitmap *icon;
LICE_SysBitmap *framebuffer;
static int m_effect = 0;
static int m_doeff = 0;

BOOL WINAPI dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    SetTimer(hwndDlg,1,10,NULL);
    {
      int x;
      for (x = 0; x < NUM_EFFECTS; x ++)
      {
        char buf[512];
        wsprintf(buf,"Effect %d",x+1);
        SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_ADDSTRING,0,(LPARAM)buf);
      }
      SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_SETCURSEL,0,0);

    }
  return 0;

  case WM_TIMER:
    InvalidateRect(hwndDlg,NULL,FALSE);
  return 0;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;

      HDC dc = BeginPaint(hwndDlg, &ps);
      RECT r;
      GetClientRect(hwndDlg, &r);
      r.top+=40;
      if (r.top >= r.bottom) r.top=r.bottom-1;
 
      if (framebuffer->resize(r.right-r.left,r.bottom-r.top))
      {
        m_doeff=1;
        memset(framebuffer->getBits(),0,framebuffer->getWidth()*framebuffer->getHeight()*4);
      }

      int x=rand()%(r.right+300)-150;
      int y=rand()%(r.bottom+300)-150;

      switch(m_effect)
      {
      case 0:
        {
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
        }
        break;
      case 1:
        if (rand()%6==0)
          LICE_Blit(framebuffer,bmp,x,y,NULL,-1.4,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA);
        else
          LICE_Blit(framebuffer,bmp,x,y,NULL,0.6,LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);
        break;
      case 2:
        {
          LICE_Clear(framebuffer,0);
          double a=GetTickCount()/1000.0;
          
          double scale=(1.1+sin(a)*0.3);
          LICE_RotatedBlit(framebuffer,bmp,r.right*scale,r.bottom*scale,r.right*(1.0-scale*2.0),r.bottom*(1.0-scale*2.0),0,0,bmp->getWidth(),bmp->getHeight(),cos(a*0.3)*13.0,false,1.0,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA|LICE_BLIT_FILTER_BILINEAR,0.0,-bmp->getHeight()/2);
        }
        break;
      case 3:
        {
          //LICE_Clear(framebuffer,0);
          static double a;
          a+=0.04;
          int xsize=sin(a*3.0)*r.right*1.5;
          int ysize=sin(a*1.7)*r.bottom*1.5;
          
          if (rand()%3==0)
            LICE_ScaledBlit(framebuffer,bmp,r.right/2-xsize/2,r.bottom/2-ysize/2,xsize,ysize,0.0,0.0,bmp->getWidth(),bmp->getHeight(),-0.7,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_ADD|LICE_BLIT_FILTER_BILINEAR);
          else
            LICE_ScaledBlit(framebuffer,bmp,r.right/2-xsize/2,r.bottom/2-ysize/2,xsize,ysize,0.0,0.0,bmp->getWidth(),bmp->getHeight(),0.25,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
        }
        break;
      case 4:
      case 9:

        {
          static double a;
          a+=0.003;

          LICE_GradRect(framebuffer,0,0,framebuffer->getWidth(),framebuffer->getHeight(),
            0.5*sin(a*14.0),0.5*cos(a*2.0+1.3),0.5*sin(a*4.0),1.0,
            (cos(a*37.0))/framebuffer->getWidth()*0.5,(sin(a*17.0))/framebuffer->getWidth()*0.5,(cos(a*7.0))/framebuffer->getWidth()*0.5,0,
            (sin(a*12.0))/framebuffer->getHeight()*0.5,(cos(a*4.0))/framebuffer->getHeight()*0.5,(cos(a*3.0))/framebuffer->getHeight()*0.5,0,
            LICE_BLIT_MODE_COPY);


          if (m_effect==9)
          {
/*            LOGFONT lf={
      140,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,
      "Times New Roman"
            };
            HFONT font=CreateFontIndirect(&lf);

  */


            LICE_SysBitmap bm(60,60);
            LICE_Clear(&bm,LICE_RGBA(0,0,0,0));
            SetTextColor(bm.getDC(),RGB(255,255,255));
            SetBkMode(bm.getDC(),TRANSPARENT);
//            HGDIOBJ of=SelectObject(bm.getDC(),font);
            RECT r={0,0,bm.getWidth(),bm.getHeight()};
            DrawText(bm.getDC(),"LICE",-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    //        SelectObject(bm.getDC(),of);
  //          DeleteObject(font);

            LICE_Blit(&bm,&bm,0,0,NULL,1.0,LICE_BLIT_MODE_CHANCOPY|LICE_PIXEL_R|(LICE_PIXEL_A<<2));

            LICE_RotatedBlit(framebuffer,&bm,0,0,framebuffer->getWidth(),framebuffer->getHeight(),0,0,bm.getWidth(),bm.getHeight(),a*10.0,false,.4,LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA|LICE_BLIT_FILTER_BILINEAR,cos(a*30.1)*10.0,sin(a*21.13)*10.0);
          }

          break;
        }
      case 5:
        if(m_doeff)
        {
          LICE_TexGen_Marble(framebuffer, NULL, 1, 1, 1, 1);
        }
        break;
      case 6:
        if(m_doeff)
        {
          LICE_TexGen_Noise(framebuffer, NULL, 0.9, 0.3, 0.6, 6.0f, NOISE_MODE_WOOD, 2);
        }
        break;
      case 7:
        if(m_doeff)
        {
          LICE_TexGen_Noise(framebuffer, NULL, 1,1,1, 8.0f, NOISE_MODE_NORMAL, 8);
        }
        break;
      case 8:
        if(m_doeff)
        {
          LICE_TexGen_CircNoise(framebuffer, NULL, 0.5f,0.5f,0.5f, 12.0f, 0.1f, 32);
        }
        break;
      case 10:
        {
          int x;
          static double a;
          double sc=sin(a)*0.024;
          a+=0.03;
          for (x = 0; x < 10000; x ++)
            LICE_PutPixel(framebuffer,rand()%framebuffer->getWidth(),rand()%framebuffer->getHeight(),LICE_RGBA(255,255,255,255),sc,LICE_BLIT_MODE_ADD);
        }
        break;
      case 11:
        //line test
        {
          LICE_Line(framebuffer, rand()%framebuffer->getWidth(), rand()%framebuffer->getHeight(), rand()%framebuffer->getWidth(), rand()%framebuffer->getHeight(), LICE_RGBA(rand()%255,rand()%255,rand()%255,255));
        }
        break;
      case 12:
        //lice draw text test
        {
          static double a;
          a+=0.001;
          LICE_DrawText(framebuffer,0.5*(1+sin(a))*(framebuffer->getWidth()-30),0.5*(1+sin(a*7.0+1.3))*(framebuffer->getHeight()-16),"LICE RULEZ",LICE_RGBA(255,0,0,0),sin(a*0.7),LICE_BLIT_MODE_ADD);
        }
        break;
      case 13:
        //icon loading test
        {
          LICE_Clear(framebuffer, LICE_RGBA(255,255,255,255));
          LICE_Blit(framebuffer,icon,0,0,NULL,1.0f,LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);
        }
        break;
      }

      m_doeff = 0;
  
      BitBlt(dc,r.left,r.top,framebuffer->getWidth(),framebuffer->getHeight(),framebuffer->getDC(),0,0,SRCCOPY);
//      bmp->blitToDC(dc, NULL, 0, 0);

      EndPaint(hwndDlg, &ps);
    }
    break;
  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
      case IDC_COMBO1:
        m_effect = SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_GETCURSEL,0,0);
        m_doeff=1;
      break;
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

  icon = LICE_LoadIconFromResource(hInstance, IDI_MAIN, 0);
  //icon = LICE_LoadIcon("main.ico", 0);
  if(!icon) return 0;

  DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, dlgProc);

  delete icon;
  delete bmp;
  delete framebuffer;
  return 0;
}