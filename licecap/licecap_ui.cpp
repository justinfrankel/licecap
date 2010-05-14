#include <windows.h>
#include <stdio.h>
#include "../WDL/lice/lice_lcf.h"
#include "../WDL/wdltypes.h"
#include "../WDL/wingui/wndsize.h"
#include "../WDL/filebrowse.h"


#include "resource.h"

int g_max_fps=5;

char g_last_fn[2048];


int g_cap_state; // 1=rec, 2=pause

LICECaptureCompressor *g_cap_lcf;
void *g_cap_gif;

LICE_MemBitmap *g_cap_lastbm; // used for gif, so we can know time until next frame, etc
LICE_SysBitmap *g_cap_bm;
DWORD g_cap_lastt;

DWORD g_last_wndstyle;


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
    sprintf(buf,"%.100s - LICEcap [%s]",p,g_cap_state==1?"recording":"paused");
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

    return 1;
    case WM_DESTROY:

      Capture_Finish(hwndDlg);

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

        break;
        case IDC_REC:

          if (!g_cap_state)
          {
            const char *extlist="LiceCap files (*.lcf)\0*.lcf\0GIF files (*.gif)\0\0";
            if (WDL_ChooseFileForSave(hwndDlg,"Choose file for recording",NULL,g_last_fn,extlist,"lcf",false,g_last_fn,sizeof(g_last_fn)))
            {

              if (1)
              {

                SaveRestoreRecRect(hwndDlg,false);

                g_last_wndstyle = SetWindowLong(hwndDlg,GWL_STYLE,GetWindowLong(hwndDlg,GWL_STYLE)&~WS_THICKFRAME);
                SetWindowPos(hwndDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_DRAWFRAME|SWP_NOACTIVATE);

                SaveRestoreRecRect(hwndDlg,true);

                SetDlgItemText(hwndDlg,IDC_REC,"[pause]");
                EnableWindow(GetDlgItem(hwndDlg,IDC_STOP),1);

                g_cap_state=1;
                UpdateCaption(hwndDlg);

              }
            }
          }
          else if (g_cap_state==1)
          {
            SetDlgItemText(hwndDlg,IDC_REC,"[unpause]");
            g_cap_state=2;
            UpdateCaption(hwndDlg);
          }
          else // unpause!
          {
            SetDlgItemText(hwndDlg,IDC_REC,"[pause]");
            g_cap_state=1;
            UpdateCaption(hwndDlg);
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
    case WM_SIZE:

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
