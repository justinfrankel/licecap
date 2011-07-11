/*
    LICEcap
    Copyright (C) 2010 Cockos Incorporated

    LICEcap is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    LICEcap is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LICEcap; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <windows.h>
#include <stdio.h>
#include <multimon.h>
#include "../WDL/lice/lice_lcf.h"
#include "../WDL/wdltypes.h"
#include "../WDL/wingui/wndsize.h"
#include "../WDL/filebrowse.h"


#include "licecap_version.h"

#include "resource.h"

HINSTANCE g_hInst;


#define MIN_SIZE_X 160
#define MIN_SIZE_Y 120


int g_prefs; // &1=title frame, &2=giant font, &4=record mousedown, &8=timeline, &16=shift+space pause


int g_max_fps=8;  

char g_last_fn[2048];
char g_ini_file[1024];


typedef struct {
  DWORD   cbSize;
  DWORD   flags;
  HCURSOR hCursor;
  POINT   ptScreenPos;
} pCURSORINFO, *pPCURSORINFO, *pLPCURSORINFO;

void DoMouseCursor(LICE_SysBitmap* sbm, HWND h, int xoffs, int yoffs)
{
  // XP+ only

  static BOOL (WINAPI *pGetCursorInfo)(pLPCURSORINFO);
  static bool tr;
  if (!tr)
  {
    tr=true;
    HINSTANCE hUser=LoadLibrary("USER32.dll");
    if (hUser)
      *(void **)&pGetCursorInfo = (void*)GetProcAddress(hUser,"GetCursorInfo");
  }

  if (pGetCursorInfo)
  {
    pCURSORINFO ci={sizeof(ci)};
    pGetCursorInfo(&ci);
    if (ci.flags && ci.hCursor)
    {
      ICONINFO inf={0,};
      GetIconInfo(ci.hCursor,&inf);

      int mousex = ci.ptScreenPos.x+xoffs;
      int mousey = ci.ptScreenPos.y+yoffs;

      if ((g_prefs&4) && ((GetAsyncKeyState(VK_LBUTTON)&0x8000) || (GetAsyncKeyState(VK_RBUTTON)&0x8000)))
      {
        LICE_Circle(sbm, mousex+1, mousey+1, 10.0f, LICE_RGBA(0,0,0,255), 1.0f, LICE_BLIT_MODE_COPY, true);
        LICE_Circle(sbm, mousex+1, mousey+1, 9.0f, LICE_RGBA(255,255,255,255), 1.0f, LICE_BLIT_MODE_COPY, true);
      }

      DrawIconEx(sbm->getDC(),mousex-inf.xHotspot,mousey-inf.yHotspot,ci.hCursor,0,0,0,NULL,DI_NORMAL);
      if (inf.hbmColor) DeleteObject(inf.hbmColor);
      if (inf.hbmMask) DeleteObject(inf.hbmMask);
    }
  }
}


void MakeTimeStr(int sec, char* buf, int w, int h, int* timepos)
{
  if (sec < 0) sec=0;
  sprintf(buf, "%d:%02d", sec/60, sec%60);
  LICE_MeasureText(buf, &timepos[2], &timepos[3]);
  timepos[2] += 8;
  timepos[3] += 8;
  timepos[0] = w-timepos[2];
  timepos[1] = h-timepos[3];
}


#undef GetSystemMetrics

void my_getViewport(RECT *r, RECT *sr, bool wantWork) {
  if (sr) 
  {
	  static HINSTANCE hlib;
    static bool haschk;
    
    if (!haschk && !hlib) { hlib=LoadLibrary("user32.dll");haschk=true; }

	  if (hlib) 
    {

      static HMONITOR (WINAPI *Mfr)(LPCRECT lpcr, DWORD dwFlags);
      static BOOL (WINAPI *Gmi)(HMONITOR mon, MONITORINFOEX* lpmi);

      if (!Mfr) Mfr = (HMONITOR (WINAPI *)(LPCRECT, DWORD)) GetProcAddress(hlib, "MonitorFromRect");
      if (!Gmi) Gmi = (BOOL (WINAPI *)(HMONITOR,MONITORINFOEX*)) GetProcAddress(hlib,"GetMonitorInfoA");    

			if (Mfr && Gmi) {
			  HMONITOR hm;
			  hm=Mfr(sr,MONITOR_DEFAULTTONULL);
        if (hm) {
          MONITORINFOEX mi;
          memset(&mi,0,sizeof(mi));
          mi.cbSize=sizeof(mi);

          if (Gmi(hm,&mi)) {
            if (wantWork)
              *r=mi.rcWork;
            else *r=mi.rcMonitor;
            return;
          }          
        }
			}
		}
	}
  if (wantWork)
    SystemParametersInfo(SPI_GETWORKAREA,0,r,0);
  else
    GetWindowRect(GetDesktopWindow(), r);
}

void EnsureNotCompletelyOffscreen(RECT *r)
{
  RECT scr;
  my_getViewport(&scr, r,true);
  if (r->right < scr.left+20 || r->left >= scr.right-20 ||
      r->bottom < scr.top+20 || r->top >= scr.bottom-20)
  {
    r->right -= r->left;
    r->bottom -= r->top;
    r->left = 0;
    r->top = 0;
  }
}

void FixRectForScreen(RECT *r, int minw, int minh)
{
 
  RECT scr;
  my_getViewport(&scr, r,true);
  
 
  if (r->right > scr.right) 
  {
    r->left += scr.right-r->right;
    r->right=scr.right;
  }
  if (r->bottom > scr.bottom) 
  {
    r->top += scr.bottom-r->bottom;
    r->bottom=scr.bottom;
  }
  if (r->left < scr.left)
  {
    r->right += scr.left-r->left;
    r->left=scr.left;
    if (r->right > scr.right)  r->right=scr.right;
  }
  if (r->top < scr.top)
  {
    r->bottom += scr.top-r->top;
    r->top=scr.top;
    if (r->bottom > scr.bottom)  r->bottom=scr.bottom;
  }
  
  if (r->right-r->left<minw) r->right=r->left+minw;
  if (r->bottom-r->top<minh) r->bottom=r->top+minh;
  
  EnsureNotCompletelyOffscreen(r);
}


bool g_frate_valid;
double g_frate_avg;
DWORD g_ms_written;
int g_cap_state; // 1=rec, 2=pause

#define PREROLL_AMT 3000
DWORD g_cap_prerolluntil;
DWORD g_skip_capture_until;
DWORD g_last_frame_capture_time;

LICECaptureCompressor *g_cap_lcf;
void *g_cap_gif;
LICE_MemBitmap *g_cap_gif_lastbm; // used for gif, so we can know time until next frame, etc
int g_cap_gif_lastbm_coords[4];
int g_cap_gif_lastbm_accumdelay;

int g_titlems=1750;
char g_title[4096];
bool g_dotitle;
int g_last_sec_written;

LICE_SysBitmap *g_cap_bm;

DWORD g_last_wndstyle;

int g_reent=0;


static WDL_WndSizer g_wndsize;

void UpdateDimBoxes(HWND hwndDlg)
{
  ShowWindow(GetDlgItem(hwndDlg, IDC_STATUS), (g_cap_state ? SW_SHOWNA : SW_HIDE));
  {
    WDL_WndSizer__rec* rec=g_wndsize.get_item(IDC_REC);
    if (rec && rec->last.left > 0)
    {
      int xmin=rec->last.left-4;     
      static const unsigned short ids[] = { IDC_MAXFPS_LBL, IDC_MAXFPS, IDC_DIMLBL_1, IDC_XSZ, IDC_YSZ, IDC_DIMLBL };
      int i;
      for (i=0; i < sizeof(ids)/sizeof(ids[0]); ++i)
      {
        WDL_WndSizer__rec* rec=g_wndsize.get_item(ids[i]);
        if (rec) 
        {
          int show = (rec->last.right > xmin || (i >= 2 && g_cap_state)) ? SW_HIDE : SW_SHOWNA;
          HWND h = GetDlgItem(hwndDlg, ids[i]);
          if (h) ShowWindow(h, show);
        }
      }
    }
  }

  if (!g_cap_state)
  {
    int x, y;
    RECT r;
    GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);
    x=r.right-r.left-2;
    y=r.bottom-r.top-2;

    char buf[2048];
    char obuf[2048];

    ++g_reent;
    sprintf(buf, "%d", x);
    GetDlgItemText(hwndDlg, IDC_XSZ, obuf, sizeof(obuf));
    if (strcmp(buf, obuf)) SetDlgItemText(hwndDlg, IDC_XSZ, buf);
    sprintf(buf, "%d", y);
    GetDlgItemText(hwndDlg, IDC_YSZ, obuf, sizeof(obuf));
    if (strcmp(buf, obuf)) SetDlgItemText(hwndDlg, IDC_YSZ, buf);   
    --g_reent;  
  }
}


DWORD g_pause_time; // time of last pause
DWORD g_insert_cnt=0; // number of frames inserted, purely for display
int g_insert_ms=g_titlems;
double g_insert_alpha=0.5f;
LICE_SysBitmap *g_cap_bm_txt; 

void UpdateStatusText(HWND hwndDlg)
{
  if (!g_cap_state) return;

  int x, y;
  if (g_cap_bm)
  {
    x=g_cap_bm->getWidth();
    y=g_cap_bm->getHeight();
  }
  else
  {
    RECT r;
    GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);
    x=r.right-r.left-2;
    y=r.bottom-r.top-2;
  }

  char buf[2048];
  char oldtext[2048];

  char dims[128];
  sprintf(dims,"%dx%d", x, y);

  char pbuf[64];
  pbuf[0]=0;
  DWORD now=timeGetTime();
  if (g_cap_state==1 && g_cap_prerolluntil)
  {
    if (now < g_cap_prerolluntil) sprintf(pbuf,"PREROLL: %d - ",(g_cap_prerolluntil-now+999)/1000);
  }
  else if (g_cap_state == 2) 
  {
    strcpy(pbuf,"Paused - ");
  }
  sprintf(buf,"%s%s",pbuf,dims);

  if (g_cap_lcf) strcat(buf, " LCF");
  if (g_cap_gif) strcat(buf, " GIF");
  
  if (g_cap_state)
  {
    sprintf(buf+strlen(buf), " %d:%02d", g_ms_written/60000, (g_ms_written/1000)%60);
  }
  if (g_cap_state && g_frate_valid)
  {   
    sprintf(buf+strlen(buf)," @ %.1ffps" ,g_frate_avg);
  }

  GetDlgItemText(hwndDlg,IDC_STATUS,oldtext,sizeof(oldtext));
  if (strcmp(buf,oldtext))
  {
    SetDlgItemText(hwndDlg,IDC_STATUS,buf);
  }
}


void UpdateCaption(HWND hwndDlg)
{
  if (!g_cap_state) SetWindowText(hwndDlg,"LICEcap " LICECAP_VERSION " [stopped]");
  else
  {
    const char *p=g_last_fn;
    while (*p) p++;
    while (p >= g_last_fn && *p != '\\' && *p != '/') p--;
    p++;
    char buf[256];
    char pbuf[64];
    pbuf[0]=0;
    if (g_cap_state==1 && g_cap_prerolluntil)
    {
      DWORD now=timeGetTime();
      if (now < g_cap_prerolluntil)
      {
        sprintf(pbuf,"PREROLL: %d - ",(g_cap_prerolluntil-now+999)/1000);
      }
      else g_cap_prerolluntil=0;
    }
    sprintf(buf,"%s%.100s - LICEcap%s",pbuf,p,pbuf[0]?"":g_cap_state==1?" [recording]":" [paused]");
    SetWindowText(hwndDlg,buf);
  }
}

void SaveRestoreRecRect(HWND hwndDlg, bool restore)
{
  static RECT r;
  if (!restore) GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);
  else
  {
    RECT r2;
    GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r2);

    int xdiff = r.left-r2.left;
    int ydiff = r.top - r2.top;

    int wdiff = (r.right-r.left) - (r2.right-r2.left);
    int hdiff = (r.bottom-r.top) - (r2.bottom-r2.top);

    GetWindowRect(hwndDlg,&r2);
    SetWindowPos(hwndDlg,NULL,r2.left+xdiff,r2.top+ydiff,r2.right-r2.left + wdiff, r2.bottom-r2.top + hdiff,SWP_NOZORDER|SWP_NOACTIVATE);

  }
}

void Capture_Finish(HWND hwndDlg)
{
  SetDlgItemText(hwndDlg,IDC_REC,"Record...");
  EnableWindow(GetDlgItem(hwndDlg,IDC_STOP),0);

  if (g_cap_state)
  {
    SaveRestoreRecRect(hwndDlg,false);
    SetWindowLong(hwndDlg,GWL_STYLE,g_last_wndstyle);
    SetWindowPos(hwndDlg,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_DRAWFRAME|SWP_NOACTIVATE);
    SaveRestoreRecRect(hwndDlg,true);
    
    g_cap_state=0;
  }

  UpdateDimBoxes(hwndDlg);

  delete g_cap_lcf;
  g_cap_lcf=0;

  if (g_cap_gif)
  {
    if (g_cap_gif_lastbm)
    {
      LICE_SubBitmap bm(g_cap_gif_lastbm, g_cap_gif_lastbm_coords[0],g_cap_gif_lastbm_coords[1],
        g_cap_gif_lastbm_coords[2],g_cap_gif_lastbm_coords[3]);

      int del = (timeGetTime()-g_last_frame_capture_time+g_cap_gif_lastbm_accumdelay);
      if (del<1) del=1;
      LICE_WriteGIFFrame(g_cap_gif,&bm,g_cap_gif_lastbm_coords[0],g_cap_gif_lastbm_coords[1],true,del);      
    }

    LICE_WriteGIFEnd(g_cap_gif);
    g_cap_gif=0;
  }

  delete g_cap_gif_lastbm;
  g_cap_gif_lastbm=0;

  g_cap_gif_lastbm_accumdelay=0;

  delete g_cap_bm;
  g_cap_bm=0;
}

static UINT_PTR CALLBACK SaveOptsProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      //ShowWindow(GetDlgItem(hwndDlg, IDC_TIMELINE), SW_HIDE);
      if (g_prefs&1) CheckDlgButton(hwndDlg, IDC_TITLEUSE, BST_CHECKED);
      if (g_prefs&2) CheckDlgButton(hwndDlg, IDC_BIGFONT, BST_CHECKED);
      if (g_prefs&4) CheckDlgButton(hwndDlg, IDC_MOUSECAP, BST_CHECKED);
      if (g_prefs&8) CheckDlgButton(hwndDlg, IDC_TIMELINE, BST_CHECKED);
      if (g_prefs&16) CheckDlgButton(hwndDlg, IDC_SSPAUSE, BST_CHECKED);
      char buf[256];
      sprintf(buf, "%.1f", (double)g_titlems/1000.0);
      SetDlgItemText(hwndDlg, IDC_MS, buf);
      SetDlgItemText(hwndDlg, IDC_TITLE, (g_title[0] ? g_title : "Title"));
      EnableWindow(GetDlgItem(hwndDlg, IDC_MS), (g_prefs&1));
      EnableWindow(GetDlgItem(hwndDlg, IDC_BIGFONT), (g_prefs&1));
      EnableWindow(GetDlgItem(hwndDlg, IDC_TITLE), (g_prefs&1));
    }
    return 0;
    case WM_DESTROY:
    {
      g_prefs=0;
      if (IsDlgButtonChecked(hwndDlg, IDC_TITLEUSE)) g_prefs |= 1;
      if (IsDlgButtonChecked(hwndDlg, IDC_BIGFONT)) g_prefs |= 2;
      if (IsDlgButtonChecked(hwndDlg, IDC_MOUSECAP)) g_prefs |= 4;
      if (IsDlgButtonChecked(hwndDlg, IDC_TIMELINE)) g_prefs |= 8;
      if (IsDlgButtonChecked(hwndDlg, IDC_SSPAUSE)) g_prefs |= 16;
      char buf[256];
      buf[0]=0;
      GetDlgItemText(hwndDlg, IDC_MS, buf, sizeof(buf)-1);
      double titlesec = atof(buf);
      g_titlems = (int)(titlesec*1000.0);
      g_title[0]=0;
      GetDlgItemText(hwndDlg, IDC_TITLE, g_title, sizeof(g_title)-1);
      if (!strcmp(g_title, "Title")) g_title[0]=0;
    }
    return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_TITLEUSE:
        {
          bool use = !!IsDlgButtonChecked(hwndDlg, IDC_TITLEUSE);
          EnableWindow(GetDlgItem(hwndDlg, IDC_MS), use);
          EnableWindow(GetDlgItem(hwndDlg, IDC_BIGFONT), use);
          EnableWindow(GetDlgItem(hwndDlg, IDC_TITLE), use);
          if (use) SetFocus(GetDlgItem(hwndDlg, IDC_TITLE));
        }
        return 0;
        case IDC_TITLE:
          if (HIWORD(wParam) == EN_SETFOCUS)
          {
            SendDlgItemMessage(hwndDlg, IDC_TITLE, EM_SETSEL, 0, -1);
          }          
        return 0;
      }
    return 0;
  }
  return 0;
}

void WriteTextFrame(const char* str, int ms, bool isTitle, int w, int h, double bgAlpha=1.0f)
{
  if (isTitle)
    LICE_Clear(g_cap_bm, 0);

  if (str)
  {
    int tw=w;
    int th=h;   

	  LICE_IBitmap* tbm = g_cap_bm;
    if (isTitle)
  	{
		  if (g_prefs&2) 
		  {
		    tw /= 2;
		    th /= 2;
		  }
		  tbm = new LICE_MemBitmap(tw, th); 
		  LICE_Clear(tbm, 0);                      
	  }
	  else
	  {
		  tbm = new LICE_MemBitmap(tw, th); 
		  LICE_Copy(tbm, g_cap_bm_txt);
      LICE_FillRect(tbm, 0,0,w,h, LICE_RGBA(0,0,0,255), bgAlpha, LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);
	  }
    /*
	    LICE_DrawRect(tbm,30,30,150,150,LICE_RGBA(255,0,0,255), 1.0f, LICE_BLIT_MODE_COPY);
	    LICE_DrawRect(tbm,31,31,148,148,LICE_RGBA(255,0,0,255), 1.0f, LICE_BLIT_MODE_COPY);
	    LICE_DrawRect(tbm,32,32,146,146,LICE_RGBA(255,0,0,255), 1.0f, LICE_BLIT_MODE_COPY);
    */
	  char buf[4096];
    lstrcpyn(buf, str,sizeof(buf));                    

    int numlines=1;
    char* p = strstr(buf, "\n");
    while (p)
    {
      *p=0;
      p = strstr(p+1, "\n");
      ++numlines;
    }

    p=buf;
    int i;
    for (i = 0; i < numlines; ++i)
    {                   
      int txtw, txth;
      LICE_MeasureText(p, &txtw, &txth);
      LICE_DrawText(tbm, (tw-txtw)/2, (th-txth*numlines*4)/2+txth*i*4, p, LICE_RGBA(255,255,255,255), 1.0f, LICE_BLIT_MODE_COPY);
      p += strlen(p)+1;
    }

	  if (tbm != g_cap_bm)
    {
      LICE_ScaledBlit(g_cap_bm, tbm, 0, 0, w, h, 0, 0, tw, th, 1.0f, LICE_BLIT_MODE_COPY);
      delete tbm;
    }
  }

  if (g_cap_gif) 
  {
	  LICE_WriteGIFFrame(g_cap_gif, g_cap_bm, 0, 0, true, ms);                 
	  g_cap_gif_lastbm=NULL; // force bm refresh
  }

  if (g_cap_lcf) 
  {
	  int del=0;
	  g_cap_lcf->OnFrame(g_cap_bm, del); 
	  if (!isTitle)
	  {
      del = g_pause_time-g_last_frame_capture_time;
      del += ms;
	  }
	  g_cap_lcf->OnFrame(g_cap_bm, del); 
  }
}

WDL_DLGRET InsertProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
    case WM_INITDIALOG :
		  {
        char buf[256];
        sprintf(buf,"Insert (%d)",g_insert_cnt);
        SetDlgItemText(hwnd,IDOK,buf);
        sprintf(buf, "%.1f", (double)g_insert_ms/1000.0);
        SetDlgItemText(hwnd, IDC_MS, buf);
        sprintf(buf, "%.1f", g_insert_alpha);
        SetDlgItemText(hwnd, IDC_ALPHA, buf);
        SetFocus(GetDlgItem(hwnd,IDC_EDIT));
		  }
		return 0;
    case WM_CLOSE:
	    EndDialog(hwnd,0);
		break;
		case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDC_ALPHA:
        {
				  char buf[256];
				  buf[0]=0;
				  GetDlgItemText(hwnd, IDC_ALPHA, buf, sizeof(buf)-1);
				  g_insert_alpha = min(1.0f, atof(buf));
        }
        break;
        case IDC_MS:
				{
				  char buf[256];
				  buf[0]=0;
				  GetDlgItemText(hwnd, IDC_MS, buf, sizeof(buf)-1);
				  g_insert_ms = (int)(atof(buf)*1000.0);
        }
        break;
        case IDOK:
			    if (g_cap_bm_txt && g_insert_ms)
          {
            char buf[4096];
            GetDlgItemText(hwnd,IDC_EDIT,buf,4096);
				    WriteTextFrame(buf,g_insert_ms,g_insert_alpha<0,g_cap_bm_txt->getWidth(),g_cap_bm_txt->getHeight(), g_insert_alpha >=0 ? g_insert_alpha : 1.0f);
				    g_insert_cnt++;
				    sprintf(buf,"Insert (%d)",g_insert_cnt);
            SetDlgItemText(hwnd,IDOK,buf);
            SetDlgItemText(hwnd,IDC_EDIT,"");
            SetFocus(GetDlgItem(hwnd,IDC_EDIT));
          }
        break;
        case IDCANCEL:
          EndDialog(hwnd,0);
        break;
			}
		break;
	}
	return 0;
}

static WDL_DLGRET liceCapMainProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static POINT s_last_mouse;
  switch (uMsg)
  {
    case WM_INITDIALOG:

      SetClassLong(hwndDlg,GCL_HICON,(LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1)));
      g_wndsize.init(hwndDlg);
      g_wndsize.init_item(IDC_VIEWRECT,0,0,1,1);
      g_wndsize.init_item(IDC_MAXFPS_LBL,0,1,0,1);
      g_wndsize.init_item(IDC_MAXFPS,0,1,0,1);
      g_wndsize.init_item(IDC_XSZ,0,1,0,1);
      g_wndsize.init_item(IDC_YSZ,0,1,0,1);
      g_wndsize.init_item(IDC_DIMLBL_1,0,1,0,1);
      g_wndsize.init_item(IDC_DIMLBL,0,1,0,1);
      g_wndsize.init_item(IDC_STATUS,0,1,1,1);
      g_wndsize.init_item(IDC_REC,1,1,1,1);
      g_wndsize.init_item(IDC_STOP,1,1,1,1);
      g_wndsize.init_item(IDC_INSERT,1,1,1,1);

      ShowWindow(GetDlgItem(hwndDlg, IDC_INSERT), SW_HIDE);
      SendMessage(hwndDlg,WM_SIZE,0,0);

      ++g_reent;

      g_max_fps = GetPrivateProfileInt("licecap", "maxfps", g_max_fps, g_ini_file);
      SetDlgItemInt(hwndDlg,IDC_MAXFPS,g_max_fps,FALSE);
      --g_reent;

      Capture_Finish(hwndDlg);

      UpdateCaption(hwndDlg);
      UpdateStatusText(hwndDlg);
      UpdateDimBoxes(hwndDlg);

      SetTimer(hwndDlg,1,30,NULL);

      {
        char buf[1024];
        GetPrivateProfileString("licecap","wnd_r","",buf,sizeof(buf),g_ini_file);
        if (buf[0])
        {
          int a[4]={0,};
          const char *p=buf;
          int x;
          for (x=0;x<4;x++)
          {
            a[x] = atoi(p);
            if (x==3) break;
            while (*p && *p != ' ') p++;
            while (*p == ' ') p++;
          }
          if (*p && a[2]>a[0] && a[3]>a[1])
          {
            SetWindowPos(hwndDlg,NULL,a[0],a[1],a[2]-a[0],a[3]-a[1],SWP_NOZORDER|SWP_NOACTIVATE);
          }
        }
      }

      g_prefs = GetPrivateProfileInt("licecap", "prefs", g_prefs, g_ini_file);
      g_titlems = GetPrivateProfileInt("licecap", "titlems", g_titlems, g_ini_file);
      g_title[0]=0;

    return 1;
    case WM_DESTROY:

      Capture_Finish(hwndDlg);
      {
        RECT r;
        GetWindowRect(hwndDlg,&r);
        char buf[1024];
        sprintf(buf,"%d %d %d %d\n",r.left,r.top,r.right,r.bottom);
        WritePrivateProfileString("licecap","wnd_r",buf,g_ini_file);
        sprintf(buf, "%d", g_max_fps);
        WritePrivateProfileString("licecap","maxfps",buf,g_ini_file);
        sprintf(buf, "%d", g_prefs);
        WritePrivateProfileString("licecap","prefs",buf,g_ini_file);
        sprintf(buf, "%d", g_titlems);
        WritePrivateProfileString("licecap","titlems",buf,g_ini_file);
      }

    break;
    case WM_TIMER:
      if (wParam==1)
      {     
        DWORD now=timeGetTime();

        if (g_cap_state==1 && g_cap_bm && now >= g_cap_prerolluntil && now >= g_skip_capture_until)
        {
          if (now >= g_last_frame_capture_time + (1000/(max(g_max_fps,1))))
          {
            g_ms_written += now-g_last_frame_capture_time;

            HWND h = GetDesktopWindow();
            RECT r;
            GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);

            HDC hdc = GetDC(h);
            if (hdc)
            {
              int bw = g_cap_bm->getWidth();
              int bh = g_cap_bm->getHeight();

              LICE_Clear(g_cap_bm,0);
              BitBlt(g_cap_bm->getDC(),0,0,bw,bh,hdc,r.left+1,r.top+1,SRCCOPY);
              ReleaseDC(h,hdc);

              DoMouseCursor(g_cap_bm,h,-(r.left+1),-(r.top+1));                        

              bool dotime = !!(g_prefs&8);
              bool newtime = false;
              char timestr[256];
              int timepos[4]; // x,y,w,h
              if (dotime)
              {
                newtime = (g_ms_written/1000 != g_last_sec_written);
                if (newtime) g_last_sec_written = g_ms_written/1000;
                MakeTimeStr(g_last_sec_written, timestr, bw, bh, timepos);
                LICE_FillRect(g_cap_bm, timepos[0], timepos[1], timepos[2], timepos[3], LICE_RGBA(0,0,0,255), 1.0f, LICE_BLIT_MODE_COPY);
              }

              if (g_cap_lcf)
              {
                if (dotime)
                {
                  LICE_DrawText(g_cap_bm, timepos[0]+4, timepos[1]+4, timestr, LICE_RGBA(255,255,255,255), 1.0f, LICE_BLIT_MODE_COPY);
                }

                int del = now-g_last_frame_capture_time;
                if (g_dotitle)
                {
                  del += g_titlems;
                  g_dotitle=false;
                }
                g_cap_lcf->OnFrame(g_cap_bm,del);
              }

              if (g_cap_gif)
              {
                int diffs[4] = { 0,0,g_cap_bm->getWidth(),g_cap_bm->getHeight() };  // x,y,w,h
            
                bool wantframe = !g_cap_gif_lastbm || LICE_BitmapCmp(g_cap_gif_lastbm, g_cap_bm, diffs);

                if (!wantframe && !newtime)
                {
                  g_cap_gif_lastbm_accumdelay+=now-g_last_frame_capture_time;
                }
                else 
                {
                  if (!g_cap_gif_lastbm) 
                  {
                    g_cap_gif_lastbm = new LICE_MemBitmap(bw, bh);
                  }
                  else 
                  {
                    int del = now-g_last_frame_capture_time+g_cap_gif_lastbm_accumdelay;
                    if (del<1) del=1;
                              
                    if (dotime &&
                      g_cap_gif_lastbm_coords[0]+g_cap_gif_lastbm_coords[2] > timepos[0] &&
                      g_cap_gif_lastbm_coords[1]+g_cap_gif_lastbm_coords[3] > timepos[1])                   
                    {
                      char prevtimestr[256];
                      int prevtimepos[4]; 
                      int sec = (g_ms_written-del)/1000;
                      MakeTimeStr(sec, prevtimestr, bw, bh, prevtimepos);
                      LICE_DrawText(g_cap_gif_lastbm, prevtimepos[0]+4, prevtimepos[1]+4, prevtimestr, LICE_RGBA(255,255,255,255), 1.0f, LICE_BLIT_MODE_COPY);                    
                    }

                    LICE_SubBitmap bm(g_cap_gif_lastbm, g_cap_gif_lastbm_coords[0], g_cap_gif_lastbm_coords[1], g_cap_gif_lastbm_coords[2], g_cap_gif_lastbm_coords[3]);
                    LICE_WriteGIFFrame(g_cap_gif,&bm,g_cap_gif_lastbm_coords[0],g_cap_gif_lastbm_coords[1],true,del);

                    if (newtime)
                    {
                      if (!wantframe)
                      {
                        memcpy(diffs, timepos, sizeof(diffs));
                      }
                      else if (diffs[0] > timepos[0] ||
                        diffs[1] > timepos[1] ||
                        diffs[0]+diffs[2] < timepos[0]+timepos[2] ||
                        diffs[1]+diffs[3] < timepos[1]+timepos[3])
                      {
                        LICE_DrawText(g_cap_bm, timepos[0]+4, timepos[1]+4, timestr, LICE_RGBA(255,255,255,255), 1.0f, LICE_BLIT_MODE_COPY);
                        LICE_SubBitmap tbm(g_cap_bm, timepos[0], timepos[1], timepos[2], timepos[3]);
                        LICE_WriteGIFFrame(g_cap_gif, &tbm, timepos[0], timepos[1], true, 1);
                        LICE_FillRect(g_cap_bm, timepos[0], timepos[1], timepos[2], timepos[3], LICE_RGBA(0,0,0,255), 1.0f, LICE_BLIT_MODE_COPY);
                      }
                    }

                    g_cap_gif_lastbm_accumdelay=0;
                  }

                  memcpy(g_cap_gif_lastbm_coords,diffs,sizeof(diffs));
                  LICE_Blit(g_cap_gif_lastbm, g_cap_bm, diffs[0], diffs[1], diffs[0], diffs[1], diffs[2], diffs[3], 1.0f, LICE_BLIT_MODE_COPY);
                }
              }

              double fr = 1000.0 / (double) (now - g_last_frame_capture_time);
              if (fr>100.0) fr=100.0;

              if (g_frate_valid) 
              {
                g_frate_avg = g_frate_avg*0.9 + fr*0.1;
              }
              else 
              {
                g_frate_avg=fr;
                g_frate_valid=true;
              }

              g_last_frame_capture_time = now;
            }
          }
        }

        bool force_status=false;
        if (g_cap_prerolluntil && g_cap_state==1)
        {
          static DWORD lproll;
          static int lcnt;
          if (lproll != g_cap_prerolluntil || (g_cap_prerolluntil-now+999)/1000 != lcnt)
          {
            lcnt=(g_cap_prerolluntil-now+999)/1000;
            lproll=g_cap_prerolluntil;
            UpdateCaption(hwndDlg);
            force_status=true;
          }
        }
        static DWORD last_status_t;
        if (force_status || now > last_status_t+500)
        {
          last_status_t=now;
          UpdateStatusText(hwndDlg);
        }

      }
    break;

    case WM_HOTKEY:
      if (lParam == MAKELPARAM(MOD_CONTROL|MOD_ALT, 'P') && (g_prefs&16)) // prefs check not necessary
      {
        SendMessage(hwndDlg, WM_COMMAND, IDC_REC, 0);
      }
    break;    

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_MAXFPS:
          if (HIWORD(wParam) == EN_CHANGE && !g_reent)
          {          
            BOOL t;
            int a = GetDlgItemInt(hwndDlg,IDC_MAXFPS,&t,FALSE);
            if (t && a) g_max_fps = a;
          }
        break;

        case IDC_XSZ:
        case IDC_YSZ:
          if (HIWORD(wParam) == EN_CHANGE && !g_cap_state && !g_reent)
          {
            int ox, oy;
            RECT r;
            GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);
            ox=r.right-r.left-2;
            oy=r.bottom-r.top-2;

            int nx=ox;
            int ny=oy;

            BOOL t;
            int a=GetDlgItemInt(hwndDlg, IDC_XSZ, &t, FALSE);
            if (t && a > MIN_SIZE_X) nx=a;
            a=GetDlgItemInt(hwndDlg, IDC_YSZ, &t, FALSE);
            if (t && a > MIN_SIZE_Y) ny=a;

            if (nx != ox || ny != oy)
            {
              GetWindowRect(hwndDlg, &r);
              int w=(r.right-r.left)+(nx-ox);
              int h=(r.bottom-r.top)+(ny-oy);
              SetWindowPos(hwndDlg, 0, 0, 0, w, h, SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);
            }
          }
        break;

        case IDC_STOP:
          ShowWindow(GetDlgItem(hwndDlg, IDC_INSERT), SW_HIDE);
          g_insert_cnt=0;
          Capture_Finish(hwndDlg);
          UpdateCaption(hwndDlg);
          UpdateStatusText(hwndDlg);
              
          if (g_prefs&16)
          {          
            UnregisterHotKey(hwndDlg, IDC_REC);
          }
        break;

        case IDC_INSERT:
          if (!g_cap_bm_txt)
          {
            RECT r;
            GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);
            g_cap_bm_txt = new LICE_SysBitmap(r.right-r.left-2,r.bottom-r.top-2);
            LICE_Copy(g_cap_bm_txt, g_cap_bm);
          }
          DialogBox(g_hInst,MAKEINTRESOURCE(IDD_INSERT),hwndDlg,InsertProc);
        break;

        case IDC_REC:

          if (!g_cap_state)
          {
            //g_title[0]=0;

            const char *extlist="LiceCap files (*.lcf)\0*.lcf\0GIF files (*.gif)\0*.gif\0";
            const char *extlist_giflast = "GIF files (*.gif)\0*.gif\0LiceCap files (*.lcf)\0*.lcf\0\0";
            bool last_gif = strlen(g_last_fn)<4 || !stricmp(g_last_fn+strlen(g_last_fn)-4,".gif");
            const char* useextlist = (last_gif ? extlist_giflast : extlist);
            const char* useext = (last_gif ? "gif" : "lcf");          

            if (WDL_ChooseFileForSave(hwndDlg, "Choose file for recording", NULL, g_last_fn,
              useextlist, useext, false, g_last_fn, sizeof(g_last_fn),
              MAKEINTRESOURCE(IDD_SAVEOPTS),(void*)SaveOptsProc, g_hInst))
            {
              WritePrivateProfileString("licecap","lastfn",g_last_fn,g_ini_file);

              if (g_prefs&16)
              {          
                RegisterHotKey(hwndDlg, IDC_REC, MOD_CONTROL|MOD_ALT, 'P');               
              }

              RECT r;
              GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);
              int w = r.right-r.left-2, h=r.bottom-r.top-2;
              delete g_cap_bm;
              g_cap_bm = new LICE_SysBitmap(w,h);

              g_dotitle = ((g_prefs&1) && g_titlems);

              if (strlen(g_last_fn)>4 && !stricmp(g_last_fn+strlen(g_last_fn)-4,".gif"))
              {
                g_cap_gif = LICE_WriteGIFBeginNoFrame(g_last_fn,w,h,0,true);
              }
              if (strlen(g_last_fn)>4 && !stricmp(g_last_fn+strlen(g_last_fn)-4,".lcf"))
              {
                g_cap_lcf = new LICECaptureCompressor(g_last_fn,w,h);
              }

              if (g_cap_gif || g_cap_lcf)
              {

                if (g_dotitle)
                  WriteTextFrame(g_title,g_titlems,true,w,h);

                SaveRestoreRecRect(hwndDlg,false);

                g_last_wndstyle = SetWindowLong(hwndDlg,GWL_STYLE,GetWindowLong(hwndDlg,GWL_STYLE)&~WS_THICKFRAME);
                SetWindowPos(hwndDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_DRAWFRAME|SWP_NOACTIVATE);

                SaveRestoreRecRect(hwndDlg,true);

                SetDlgItemText(hwndDlg,IDC_REC,"[pause]");
                EnableWindow(GetDlgItem(hwndDlg,IDC_STOP),1);

                g_frate_valid=false;
                g_frate_avg=0.0;
                g_cap_gif_lastbm_accumdelay=0;
                g_ms_written = 0;
                g_last_sec_written=-1;

                g_last_frame_capture_time = g_cap_prerolluntil=timeGetTime()+PREROLL_AMT;
                g_cap_state=1;
                UpdateCaption(hwndDlg);
                UpdateStatusText(hwndDlg);
                UpdateDimBoxes(hwndDlg);
              }
            }
          }
          else if (g_cap_state==1)
          {
            g_pause_time = timeGetTime();
            ShowWindow(GetDlgItem(hwndDlg, IDC_INSERT), SW_SHOWNA);
            ShowWindow(GetDlgItem(hwndDlg,IDC_STATUS),SW_HIDE);
            SetDlgItemText(hwndDlg,IDC_REC,"[unpause]");
            g_cap_state=2;
            UpdateCaption(hwndDlg);
            UpdateStatusText(hwndDlg);
          }
          else // unpause!
          {
            delete g_cap_bm_txt;
            g_cap_bm_txt = NULL;
            g_insert_cnt=0;
            ShowWindow(GetDlgItem(hwndDlg,IDC_STATUS),SW_SHOWNA);
            ShowWindow(GetDlgItem(hwndDlg, IDC_INSERT), SW_HIDE);
            SetDlgItemText(hwndDlg,IDC_REC,"[pause]");
            g_last_frame_capture_time = g_cap_prerolluntil=timeGetTime()+PREROLL_AMT;
            g_cap_state=1;
            UpdateCaption(hwndDlg);
            UpdateStatusText(hwndDlg);
          }

        break;
      }
    break;
    case WM_CLOSE:
      if (!g_cap_state)
        EndDialog(hwndDlg,0);
    break;
    case WM_GETMINMAXINFO:
      if (lParam)
      {
        LPMINMAXINFO l = (LPMINMAXINFO)lParam;
        l->ptMinTrackSize.x = MIN_SIZE_X;
        l->ptMinTrackSize.y = MIN_SIZE_Y;

      }
    break;

    case WM_LBUTTONDOWN:
      SetCapture(hwndDlg);
      GetCursorPos(&s_last_mouse);
    break;
    case WM_MOUSEMOVE:
      if (GetCapture()==hwndDlg)
      {
        POINT p;
        GetCursorPos(&p);
        int dx= p.x - s_last_mouse.x;
        int dy= p.y - s_last_mouse.y;
        if (dx||dy)
        {
          s_last_mouse=p;
          RECT r;
          GetWindowRect(hwndDlg,&r);
          SetWindowPos(hwndDlg,NULL,r.left+dx,r.top+dy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        }
      }
    break;
    case WM_LBUTTONUP:
      ReleaseCapture();
    break;

    case WM_MOVE:
      //g_skip_capture_until = timeGetTime()+30;
    break;
    case WM_SIZE:
     // g_skip_capture_until = timeGetTime()+30;

      if (wParam != SIZE_MINIMIZED)
      {
        g_wndsize.onResize();
       

				RECT r,r2;
				GetWindowRect(hwndDlg,&r);
        GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r2);

        r.right-=r.left;
        r.bottom-=r.top;
        r2.right-=r.left;
        r2.bottom-=r.top;
        r2.left-=r.left;
        r2.top-=r.top;

        r2.left++;
        r2.top++;
        r2.bottom--;
        r2.right--;

        r.left=r.top=0;
        HRGN rgn=CreateRectRgnIndirect(&r);
        HRGN rgn2=CreateRectRgnIndirect(&r2);
        HRGN rgn3=CreateRectRgn(0,0,0,0);
        CombineRgn(rgn3,rgn,rgn2,RGN_DIFF);

			  DeleteObject(rgn);
		    DeleteObject(rgn2);
        SetWindowRgn(hwndDlg,rgn3,TRUE);

        UpdateDimBoxes(hwndDlg);

        InvalidateRect(hwndDlg,NULL,TRUE);
      }
    break;
  }
  return 0;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  strcpy(g_ini_file,"licecap.ini");

  HKEY k;
  if (RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",0,KEY_READ,&k) == ERROR_SUCCESS)
  {
    char buf[1024];
    DWORD b=sizeof(buf);
    DWORD t=REG_SZ;
    if (RegQueryValueEx(k,"AppData",0,&t,(unsigned char *)buf,&b) == ERROR_SUCCESS && t == REG_SZ)
    {
 
      lstrcpyn(g_ini_file,buf,sizeof(g_ini_file)-32);
      strcat(g_ini_file,"\\licecap.ini");
    }
    RegCloseKey(k);
  }

  GetPrivateProfileString("licecap","lastfn","",g_last_fn,sizeof(g_last_fn),g_ini_file);

  g_hInst = hInstance;
  DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),GetDesktopWindow(),liceCapMainProc);
  return 0;
}
