/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice_bmp.cpp (BMP loading for LICE)
  See lice.h for license and other information
*/

#include "lice.h"

static LICE_IBitmap *hbmToBit(HBITMAP hbm, LICE_IBitmap *bmp)
{
  BITMAP bm;
  GetObject(hbm, sizeof(BITMAP), (LPSTR)&bm);

  LICE_SysBitmap sysbitmap(bm.bmWidth,bm.bmHeight);
  
  HDC hdc=CreateCompatibleDC(NULL);
  HGDIOBJ oldBM=SelectObject(hdc,hbm);

  BitBlt(sysbitmap.getDC(),0,0,bm.bmWidth,bm.bmHeight,hdc,0,0,SRCCOPY);
  GdiFlush();

  if (!bmp) bmp=new LICE_MemBitmap(bm.bmWidth,bm.bmHeight);
  LICE_Copy(bmp,&sysbitmap);

  SelectObject(hdc,oldBM);
  DeleteDC(hdc);

  return bmp;
}


LICE_IBitmap *LICE_LoadBMP(const char *filename, LICE_IBitmap *bmp) // returns a bitmap (bmp if nonzero) on success
{
  HBITMAP bm=(HBITMAP) LoadImage(NULL,filename,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_LOADFROMFILE);
  if (!bm) return 0;

  LICE_IBitmap *ret=hbmToBit(bm,bmp);

  DeleteObject(bm);
  return ret;
}

LICE_IBitmap *LICE_LoadBMPFromResource(HINSTANCE hInst, int resid, LICE_IBitmap *bmp) // returns a bitmap (bmp if nonzero) on success
{
  HBITMAP bm=(HBITMAP) LoadImage(hInst,MAKEINTRESOURCE(resid),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
  if (!bm) return 0;

  LICE_IBitmap *ret=hbmToBit(bm,bmp);

  DeleteObject(bm);
  return ret;
}

