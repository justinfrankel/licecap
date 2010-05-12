#include <windows.h>
#include <stdio.h>
#include "../WDL/lice/lice_lcf.h"
#include "../WDL/wdltypes.h"
#include "../WDL/wingui/wndsize.h"


#include "resource.h"

LICECaptureCompressor *g_cap_lcf;
void *g_cap_gif;

LICE_MemBitmap *g_cap_lastbm; // used for gif, so we can know time until next frame, etc
LICE_SysBitmap *g_cap_bm;
DWORD g_cap_lastt;

void Capture_Finish()
{
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
      wndsize.init_item(IDC_BROWSE,0,1,0,1);
      wndsize.init_item(IDC_MAXFPS_LBL,0,1,0,1);
      wndsize.init_item(IDC_MAXFPS,0,1,0,1);
      wndsize.init_item(IDC_STATUS,0,1,1,1);
      wndsize.init_item(IDC_REC,1,1,1,1);
      wndsize.init_item(IDC_STOP,1,1,1,1);

      SendMessage(hwndDlg,WM_SIZE,0,0);
    return 1;
    case WM_DESTROY:

      Capture_Finish();

    break;
    case WM_CLOSE:
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
