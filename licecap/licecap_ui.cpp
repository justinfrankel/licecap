#include <windows.h>
#include <stdio.h>
#include "../WDL/lice/lice_lcf.h"
#include "../WDL/wdltypes.h"
#include "../WDL/wingui/wndsize.h"
#include "../WDL/filebrowse.h"


#include "resource.h"

int g_max_fps=5;

char g_last_fn[2048];


bool g_frate_valid;
double g_frate_avg;
int g_cap_state; // 1=rec, 2=pause

#define PREROLL_AMT 3000
DWORD g_cap_prerolluntil;
DWORD g_skip_capture_until;
DWORD g_last_frame_capture_time;

LICECaptureCompressor *g_cap_lcf;
void *g_cap_gif;

LICE_MemBitmap *g_cap_lastbm; // used for gif, so we can know time until next frame, etc
LICE_SysBitmap *g_cap_bm;
DWORD g_cap_lastt;

DWORD g_last_wndstyle;

void UpdateStatusText(HWND hwndDlg)
{
  char dims[128];
  if (g_cap_bm)
    sprintf(dims,"%dx%d", g_cap_bm->getWidth(),g_cap_bm->getHeight());
  else
  {
    RECT r;
    GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);
    sprintf(dims,"%dx%d",r.right-r.left-2,r.bottom-r.top-2);
  }

  char buf[2048],oldtext[2048];
  char pbuf[64];
  pbuf[0]=0;
  if (g_cap_state==1 && g_cap_prerolluntil)
  {
    DWORD now=GetTickCount();
    if (now < g_cap_prerolluntil) sprintf(pbuf,"PREROLL: %d",(g_cap_prerolluntil-now+999)/1000);
  }
  else if (g_cap_state == 1) strcpy(pbuf,"Recording");
  else if (g_cap_state == 2) strcpy(pbuf,"Paused");
  else strcpy(pbuf,"Stopped");
  sprintf(buf,"%s - %s",pbuf,dims);

  if (g_cap_state && g_frate_valid)
    sprintf(buf+strlen(buf)," @ %.1fps",g_frate_avg);

  if (g_cap_lcf) strcat(buf, " (LCF)");
  if (g_cap_gif) strcat(buf, " (GIF)");


  GetDlgItemText(hwndDlg,IDC_STATUS,oldtext,sizeof(oldtext));
  if (strcmp(buf,oldtext))
    SetDlgItemText(hwndDlg,IDC_STATUS,buf);
}


void UpdateCaption(HWND hwndDlg)
{
  if (!g_cap_state) SetWindowText(hwndDlg,"LICEcap [stopped]");
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
      DWORD now=GetTickCount();
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

  delete g_cap_lcf;
  g_cap_lcf=0;

  if (g_cap_gif)
  {
    if (g_cap_lastbm)
      LICE_WriteGIFFrame(g_cap_gif,g_cap_lastbm,0,0,true,GetTickCount()-g_cap_lastt);

    LICE_WriteGIFEnd(g_cap_gif);
    g_cap_gif=0;
  }
  delete g_cap_lastbm;
  g_cap_lastbm=0;
  delete g_cap_bm;
  g_cap_bm=0;
}

static WDL_DLGRET liceCapMainProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static POINT s_last_mouse;
  static WDL_WndSizer wndsize;
  switch (uMsg)
  {
    case WM_INITDIALOG:
      wndsize.init(hwndDlg);
      wndsize.init_item(IDC_VIEWRECT,0,0,1,1);
      wndsize.init_item(IDC_MAXFPS_LBL,0,1,0,1);
      wndsize.init_item(IDC_MAXFPS,0,1,0,1);
      wndsize.init_item(IDC_STATUS,0,1,1,1);
      wndsize.init_item(IDC_REC,1,1,1,1);
      wndsize.init_item(IDC_STOP,1,1,1,1);

      SendMessage(hwndDlg,WM_SIZE,0,0);
      SetDlgItemInt(hwndDlg,IDC_MAXFPS,g_max_fps,FALSE);

      Capture_Finish(hwndDlg);

      UpdateCaption(hwndDlg);
      UpdateStatusText(hwndDlg);

      SetTimer(hwndDlg,1,30,NULL);
    return 1;
    case WM_DESTROY:

      Capture_Finish(hwndDlg);

    break;
    case WM_TIMER:
      if (wParam==1)
      {     
        DWORD now=GetTickCount();

        if (g_cap_state==1 && g_cap_bm && now >= g_cap_prerolluntil && now >= g_skip_capture_until)
        {
          if (now >= g_last_frame_capture_time + (1000/(max(g_max_fps,1))))
          {
            HWND h = GetDesktopWindow();
            RECT r;
            GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);

            HDC hdc = GetDC(h);
            if (hdc)
            {
              LICE_Clear(g_cap_bm,0);
              BitBlt(g_cap_bm->getDC(),0,0,g_cap_bm->getWidth(),g_cap_bm->getHeight(),hdc,r.left+1,r.top+1,SRCCOPY);
              ReleaseDC(h,hdc);
            
              double fr = 1000.0 / (double) (now - g_last_frame_capture_time);
              if (fr>100.0) fr=100.0;

              if (g_frate_valid) g_frate_avg = g_frate_avg*0.9 + fr*0.1;
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
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_MAXFPS:
          {
            BOOL t;
            int a = GetDlgItemInt(hwndDlg,IDC_MAXFPS,&t,FALSE);
            if (t && a)
              g_max_fps = a;
          }
        break;
        case IDC_STOP:

          Capture_Finish(hwndDlg);

          UpdateCaption(hwndDlg);
          UpdateStatusText(hwndDlg);

        break;
        case IDC_REC:

          if (!g_cap_state)
          {
            const char *extlist="LiceCap files (*.lcf)\0*.lcf\0GIF files (*.gif)\0*.gif\0";
            const char *extlist_giflast = "GIF files (*.gif)\0*.gif\0LiceCap files (*.lcf)\0*.lcf\0\0";
            bool last_gif = strlen(g_last_fn)>4 && !stricmp(g_last_fn+strlen(g_last_fn)-4,".gif");
            if (WDL_ChooseFileForSave(hwndDlg,"Choose file for recording",NULL,g_last_fn,last_gif?extlist_giflast:extlist,last_gif ? "gif" : "lcf",false,g_last_fn,sizeof(g_last_fn)))
            {

              if (1)
              {
                SaveRestoreRecRect(hwndDlg,false);

                g_last_wndstyle = SetWindowLong(hwndDlg,GWL_STYLE,GetWindowLong(hwndDlg,GWL_STYLE)&~WS_THICKFRAME);
                SetWindowPos(hwndDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_DRAWFRAME|SWP_NOACTIVATE);

                SaveRestoreRecRect(hwndDlg,true);

                SetDlgItemText(hwndDlg,IDC_REC,"[pause]");
                EnableWindow(GetDlgItem(hwndDlg,IDC_STOP),1);

                g_frate_valid=false;
                g_frate_avg=0.0;

                g_last_frame_capture_time = g_cap_prerolluntil=GetTickCount()+PREROLL_AMT;
                g_cap_state=1;
                UpdateCaption(hwndDlg);
                UpdateStatusText(hwndDlg);

              }
            }
          }
          else if (g_cap_state==1)
          {
            SetDlgItemText(hwndDlg,IDC_REC,"[unpause]");
            g_cap_state=2;
            UpdateCaption(hwndDlg);
            UpdateStatusText(hwndDlg);
          }
          else // unpause!
          {
            SetDlgItemText(hwndDlg,IDC_REC,"[pause]");
            g_last_frame_capture_time = g_cap_prerolluntil=GetTickCount()+PREROLL_AMT;
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
        l->ptMinTrackSize.x = 160;
        l->ptMinTrackSize.y = 120;

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
      g_skip_capture_until = GetTickCount()+200;
    break;
    case WM_SIZE:
      g_skip_capture_until = GetTickCount()+200;

      if (wParam != SIZE_MINIMIZED)
      {
        wndsize.onResize();
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

        InvalidateRect(hwndDlg,NULL,TRUE);
      }
    break;
  }
  return 0;
}


void RunLiceCapUI()
{
  DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIALOG1),GetDesktopWindow(),liceCapMainProc);
}
