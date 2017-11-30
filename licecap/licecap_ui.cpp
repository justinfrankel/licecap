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


#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <multimon.h>
#else
#include "../WDL/swell/swell.h"
#endif
#include "../WDL/queue.h"
#include "../WDL/mutex.h"
#include "../WDL/wdlcstring.h"


//#define TEST_MULTIPLE_MODES

#ifdef REAPER_LICECAP
  #define NO_LCF_SUPPORT
  #define VIDEO_ENCODER_SUPPORT
  #include "../jmde/fx/lice_imported.h"
  #include "../jmde/reaper_plugin.h"
  #include "../jmde/video2/video_encoder.h"
  bool WDL_ChooseFileForSave(HWND parent, const char *text, const char *initialdir, const char *initialfile, const char *extlist,const char *defext,bool preservecwd,char *fn, int fnsize,const char *dlgid=NULL, void *dlgProc=NULL, void *hi=NULL);

  void *(*reaperAPI_getfunc)(const char *p);
  int (*Audio_RegHardwareHook)(bool isAdd, audio_hook_register_t *reg); // return >0 on success
  REAPER_Resample_Interface *(*Resampler_Create)();
  void OnAudioBuffer(bool isPost, int len, double srate, struct audio_hook_register_t *reg); // called twice per frame, isPost being false then true

  unsigned WINAPI audioEncodeThread(void *p);
  
  audio_hook_register_t s_audiohook = { OnAudioBuffer };
  bool s_audiohook_reg;
  double s_audiohook_timepos;
  double s_audiohook_needsilence;
  WDL_TypedQueue<ReaSample> s_audiohook_samples;
  WDL_Mutex s_audiohook_samples_mutex;
  HANDLE s_audiothread;
  REAPER_Resample_Interface *s_rs;

#else

  #define LICE_CreateSysBitmap(w,h) new LICE_SysBitmap(w,h)
  #define LICE_CreateMemBitmap(w,h) new LICE_MemBitmap(w,h)

  #include "../WDL/lice/lice_lcf.h"
  #include "../WDL/filebrowse.h"
#endif




#include "../WDL/wdltypes.h"
#include "../WDL/wingui/wndsize.h"
#include "../WDL/wdlstring.h"


#include "licecap_version.h"

#include "resource.h"

HINSTANCE g_hInst;


#define MIN_SIZE_X 160
#define MIN_SIZE_Y 120



class gif_encoder
{

  LICE_IBitmap *lastbm; // set if a new frame is in progress
  void *ctx; 

  int lastbm_coords[4]; // coordinates of previous frame which need to be updated, [2], [3] will always be >0 if in progress
  int lastbm_accumdelay; // delay of previous frame which is latent
  int loopcnt;
  LICE_pixel trans_mask;

public:


  gif_encoder(void *gifctx, int use_loopcnt, int trans_chan_mask=0xff)
  {
    lastbm = NULL;
    memset(lastbm_coords,0,sizeof(lastbm_coords));
    lastbm_accumdelay = 0;
    ctx=gifctx;
    loopcnt=use_loopcnt;
    trans_mask = LICE_RGBA(trans_chan_mask,trans_chan_mask,trans_chan_mask,0);
  }
  ~gif_encoder()
  {
    frame_finish();
    LICE_WriteGIFEnd(ctx);
    delete lastbm;
  }
  
  
  bool frame_compare(LICE_IBitmap *bm, int diffs[4])
  {
    diffs[0]=diffs[1]=0;
    diffs[2]=bm->getWidth();
    diffs[3]=bm->getHeight();
    return !lastbm || LICE_BitmapCmpEx(lastbm, bm, trans_mask,diffs);
  }
  
  void frame_finish()
  {
    if (ctx && lastbm && lastbm_coords[2] > 0 && lastbm_coords[3] > 0)
    {
      LICE_SubBitmap bm(lastbm, lastbm_coords[0],lastbm_coords[1], lastbm_coords[2],lastbm_coords[3]);
      
      int del = lastbm_accumdelay;
      if (del<1) del=1;
      LICE_WriteGIFFrame(ctx,&bm,lastbm_coords[0],lastbm_coords[1],true,del,loopcnt);
    }
    lastbm_accumdelay=0;
    lastbm_coords[2]=lastbm_coords[3]=0;
  }
  
  void frame_advancetime(int amt)
  {
    lastbm_accumdelay+=amt;
  }
  
  void frame_new(LICE_IBitmap *ref, int x, int y, int w, int h)
  {
    if (w > 0 && h > 0)
    {
      frame_finish();
    
      lastbm_coords[0]=x;
      lastbm_coords[1]=y;
      lastbm_coords[2]=w;
      lastbm_coords[3]=h;
    
      if (!lastbm) lastbm = LICE_CreateMemBitmap(ref->getWidth(), ref->getHeight());
      LICE_Blit(lastbm, ref, x, y, x,y, w,h, 1.0f, LICE_BLIT_MODE_COPY);
    }
  }
  
  void clear_history() // forces next frame to be a fully new frame
  {
    frame_finish();
    delete lastbm;
    lastbm=NULL;
  }
  LICE_IBitmap *prev_bitmap() { return lastbm; }
};



int g_prefs; // &1=title frame, &2=giant font, &4=record mousedown, &8=timeline, &16=shift+space pause, &32=transparency-fu
int g_stop_after_msec;


int g_gif_loopcount=0;
int g_max_fps=8;  

char g_last_fn[2048];
WDL_String g_ini_file;

HWND g_hwnd;

typedef struct {
  DWORD   cbSize;
  DWORD   flags;
  HCURSOR hCursor;
  POINT   ptScreenPos;
} pCURSORINFO, *pPCURSORINFO, *pLPCURSORINFO;

#ifdef _WIN32

static void __LogicalToPhysicalPointForPerMonitorDPI(HWND hwnd, POINT *pt)
{
  static BOOL (WINAPI *ltppfpmd)(HWND hwnd, LPPOINT lpPoint);
  static bool tried;
  if (!tried)
  {
    *(void **)&ltppfpmd = GetProcAddress(GetModuleHandle("USER32"),"LogicalToPhysicalPointForPerMonitorDPI");
    tried=true;
  }
  if (ltppfpmd) ltppfpmd(hwnd,pt);
}

void DoMouseCursor(LICE_IBitmap* sbm, HWND h, int xoffs, int yoffs)
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

      __LogicalToPhysicalPointForPerMonitorDPI(g_hwnd,&ci.ptScreenPos);

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
#endif

#ifdef __APPLE__
  bool GetAsyncNSWindowRect(HWND hwnd, RECT *r);
  POINT s_async_offset;
  bool s_has_async_offset;
#endif

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

void draw_timedisp(LICE_IBitmap *bm, int sec, int timeposout[4], int bw, int bh)
{
  char timestr[256];
  int timepos[4]; // x,y,w,h
  MakeTimeStr(sec, timestr, bw, bh, timepos);
  if (bm)
  {
    LICE_FillRect(bm, timepos[0], timepos[1], timepos[2], timepos[3], LICE_RGBA(0,0,0,255), 1.0f, LICE_BLIT_MODE_COPY);
    LICE_DrawText(bm, timepos[0]+4, timepos[1]+4, timestr, LICE_RGBA(255,255,255,255), 1.0f, LICE_BLIT_MODE_COPY);
  }
  if (timeposout) memcpy(timeposout,timepos,sizeof(timepos));
}


#undef GetSystemMetrics

void my_getViewport(RECT *r, RECT *sr, bool wantWork) 
{
#ifdef _WIN32
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
#else
  return SWELL_GetViewPort(r,sr,wantWork);
#endif
}

void union_diffs(int a[4], const int b[4]) 
{
  if (a[2] < 1 || a[3] < 1) 
  {
    memcpy(a, b, 4*sizeof(int));
  }
  else if (b[2] > 0 && b[3] > 0)
  {
    a[2] += a[0]; // a2/3 are right/bottom
    a[3] += a[1];

    if (a[0] > b[0]) a[0] = b[0]; // left/top edges
    if (a[1] > b[1]) a[1] = b[1];

    if (a[2] < b[0] + b[2]) a[2] = b[0] + b[2]; // adjust right/bottom edges
    if (a[3] < b[1] + b[3]) a[3] = b[1] + b[3];
    

    a[2] -= a[0]; // a2/3 back to width/height
    a[3] -= a[1];
  }
  else
  {
    // do nothing, a is valid b is not
  }
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

#ifndef NO_LCF_SUPPORT
LICECaptureCompressor *g_cap_lcf;
#endif

#ifdef VIDEO_ENCODER_SUPPORT
VideoEncoder *g_cap_video;
char g_cap_video_ext[32];
double g_cap_video_lastt;
int g_cap_video_vbr=256, g_cap_video_abr=64;

class videoFrameWrap : public IVideoFrame
{
public:
  videoFrameWrap(LICE_IBitmap *bmp)  {
    m_refcnt = 1;
    m_w = bmp->getWidth();
    m_h = bmp->getHeight();
    m_bits = (char *)malloc(32 + m_w*m_h * 4);

    char *bits = get_bits(); // aligned copy
    if (bits)
    {
      const char *ptr = (const char *)bmp->getBits();
      int rs = bmp->getRowSpan() * 4;
      if (bmp->isFlipped())
      {
        ptr += (bmp->getHeight() - 1)*rs;
        rs = -rs;
      }
      int y;
      for (y = 0; y < m_h; y++)
      {
        memcpy(bits, ptr, m_w * 4);
        ptr += rs;
        bits += m_w * 4;
      }
    }
  }
  ~videoFrameWrap() { free(m_bits);  }
  int m_w, m_h;

  char *m_bits;
  int m_refcnt;

  virtual void AddRef() { m_refcnt++; };
  virtual void Release() { if (!--m_refcnt) delete this; };
  virtual char *get_bits() { return (char*)((((INT_PTR)m_bits) + 31)&~31); }
  virtual int get_w() { return m_w;  }
  virtual int get_h() { return m_h;  }
  virtual int get_fmt() { return 'RGBA'; }
  virtual int get_rowspan() { return m_w * 4; }
  virtual void resize_img(int wantw, int wanth, int wantfmt) {}
};


void EncodeFrameToVideo(VideoEncoder *enc, LICE_IBitmap *bm, bool force=false)
{
  if (!enc||!bm) return;
  const double pos=s_audiohook_timepos;
  if (pos < g_cap_video_lastt + 0.03 && !force) return; // too soon

  videoFrameWrap *wr = new videoFrameWrap(bm);

  g_cap_video_lastt=pos;
  enc->encodeVideoFrame(wr,pos);
  wr->Release();
}

#endif

gif_encoder *g_cap_gif;
#ifdef TEST_MULTIPLE_MODES
gif_encoder  *g_cap_gif2,*g_cap_gif3; // only used if TEST_MULTIPLE_MODES defined
#endif
int g_cap_gif_lastsec_written;



int g_titlems=1750;
char g_title[4096];
bool g_dotitle;

LICE_IBitmap *g_cap_bm,*g_cap_bm_inv;

DWORD g_last_wndstyle;

int g_reent=0;


static WDL_WndSizer g_wndsize;

#ifndef _WIN32
int g_capwnd_levelsave=-1;
#endif

static void GetViewRectSize(int *w, int *h)
{
  RECT r={0,0,320,240};
  GetWindowRect(GetDlgItem(g_hwnd,IDC_VIEWRECT),&r);
#ifdef _WIN32
  __LogicalToPhysicalPointForPerMonitorDPI(g_hwnd,(LPPOINT)&r);
  __LogicalToPhysicalPointForPerMonitorDPI(g_hwnd,((LPPOINT)&r)+1);
#endif
  if (w) *w=r.right-r.left - 2;
  if (h) *h=abs(r.bottom-r.top) - 2;

  
}

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
          int show = (rec->last.right > xmin || g_cap_state) ? SW_HIDE : SW_SHOWNA;
          HWND h = GetDlgItem(hwndDlg, ids[i]);
          if (h) ShowWindow(h, show);
        }
      }
    }
  }

  if (!g_cap_state)
  {
    int x, y;
    GetViewRectSize(&x,&y);

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
LICE_IBitmap *g_cap_bm_txt;  // is a LICE_SysBitmap

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
    GetViewRectSize(&x,&y);
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
    if (now < g_cap_prerolluntil) snprintf(pbuf,sizeof(pbuf),"PREROLL: %d - ",(g_cap_prerolluntil-now+999)/1000);
  }
  else if (g_cap_state == 2) 
  {
    strcpy(pbuf,"Paused - ");
  }
  snprintf(buf,sizeof(buf),"%s%s",pbuf,dims);

#ifndef NO_LCF_SUPPORT
  if (g_cap_lcf) strcat(buf, " LCF");
#endif
#ifdef VIDEO_ENCODER_SUPPORT
  if (g_cap_video) 
  {
    lstrcatn(buf, " ",sizeof(buf));
    lstrcatn(buf,g_cap_video_ext,sizeof(buf));
  }
#endif
  if (g_cap_gif) lstrcatn(buf, " GIF", sizeof(buf));
  
  if (g_cap_state)
  {
    snprintf_append(buf,sizeof(buf), " %d:%02d", g_ms_written/60000, (g_ms_written/1000)%60);
  }
  if (g_cap_state && g_frate_valid)
  {   
    snprintf_append(buf,sizeof(buf)," @ %.1ffps" ,g_frate_avg);
  }

  GetDlgItemText(hwndDlg,IDC_STATUS,oldtext,sizeof(oldtext));
  if (strcmp(buf,oldtext))
  {
    SetDlgItemText(hwndDlg,IDC_STATUS,buf);
  }
}


void UpdateCaption(HWND hwndDlg)
{
  if (!g_cap_state) 
  {
#ifdef REAPER_LICECAP
    SetWindowText(hwndDlg,"REAPER_LICEcap " LICECAP_VERSION " [stopped]");
#else
    SetWindowText(hwndDlg,"LICEcap " LICECAP_VERSION " [stopped]");
#endif
  }
  else
  {
    const char *p=WDL_get_filepart(g_last_fn);
    char buf[256];
    char pbuf[64];
    pbuf[0]=0;
    if (g_cap_state==1 && g_cap_prerolluntil)
    {
      DWORD now=timeGetTime();
      if (now < g_cap_prerolluntil)
      {
        snprintf(pbuf,sizeof(pbuf),"PREROLL: %d - ",(g_cap_prerolluntil-now+999)/1000);
      }
      else g_cap_prerolluntil=0;
    }
    snprintf(buf,sizeof(buf),"%s%.100s - LICEcap%s",pbuf,p,pbuf[0]?"":g_cap_state==1?" [recording]":" [paused]");
    SetWindowText(hwndDlg,buf);
  }
}

void SaveRestoreRecRect(HWND hwndDlg, bool restore)
{
#ifdef _WIN32
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
#endif
}
void SWELL_SetWindowResizeable(HWND, bool);

void Capture_Finish(HWND hwndDlg)
{
  SetDlgItemText(hwndDlg,IDC_REC,"Record...");
  EnableWindow(GetDlgItem(hwndDlg,IDC_STOP),0);

  if (g_cap_state)
  {
#ifdef _WIN32
    SaveRestoreRecRect(hwndDlg,false);
    SetWindowLong(hwndDlg,GWL_STYLE,g_last_wndstyle);
    SetWindowPos(hwndDlg,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_DRAWFRAME|SWP_NOACTIVATE);
    SaveRestoreRecRect(hwndDlg,true);
#else
    if (g_capwnd_levelsave>=0)
    {
      SWELL_SetWindowLevel(hwndDlg,g_capwnd_levelsave);
      g_capwnd_levelsave=-1;
    }
    SWELL_SetWindowResizeable(hwndDlg,true);
#endif
    g_cap_state=0;
  }

  UpdateDimBoxes(hwndDlg);

#ifndef NO_LCF_SUPPORT
  delete g_cap_lcf;
  g_cap_lcf=0;
#endif

#ifdef REAPER_LICECAP
  if (s_audiohook_reg)
  {
    Audio_RegHardwareHook(false,&s_audiohook);
    s_audiohook_reg=false;
    // kill thread and clear buffers

    if (s_audiothread)
    {
      WaitForSingleObject(s_audiothread,INFINITE);
      CloseHandle(s_audiothread);
      s_audiothread=0;
    }

    s_audiohook_samples.Clear();
    s_audiohook_samples.Compact();
  }
#endif

#ifdef VIDEO_ENCODER_SUPPORT
  delete g_cap_video;
  g_cap_video=0;
#endif
  if (g_cap_gif)
  {
    delete g_cap_gif;
    g_cap_gif=0;
  }

#ifdef TEST_MULTIPLE_MODES
  delete g_cap_gif2;
  g_cap_gif2 = 0;
  delete g_cap_gif3;
  g_cap_gif3 = 0;
#endif

  delete g_cap_bm;
  g_cap_bm=0;
}

#ifdef VIDEO_ENCODER_SUPPORT

WDL_DLGRET VideoOptionsProc(HWND hwndDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
  switch(Message)
  {
  case WM_INITDIALOG:
    {
      SetDlgItemInt(hwndDlg, IDC_EDIT1, g_cap_video_vbr, FALSE);
      SetDlgItemInt(hwndDlg, IDC_EDIT2, g_cap_video_abr, FALSE);
    }
    break;
  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDOK:
    case IDCANCEL:
      EndDialog(hwndDlg,0);
    }
    break;
    case WM_DESTROY:
    {
      BOOL t = FALSE;
      int a = GetDlgItemInt(hwndDlg, IDC_EDIT1, &t, FALSE);
      if(t && a>0) g_cap_video_vbr = a;
      t = FALSE;
      a = GetDlgItemInt(hwndDlg, IDC_EDIT2, &t, FALSE);
      if(t && a>0) g_cap_video_abr = a;
    }
    break;
  }
  return 0;
}
#endif

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
      if (g_prefs&32) CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
      

      if (g_prefs&64) 
      {
        CheckDlgButton(hwndDlg, IDC_CHECK2, BST_CHECKED);
      }
      else
      {
        EnableWindow(GetDlgItem(hwndDlg,IDC_STOPAFTER_SEC_LBL),0);
        EnableWindow(GetDlgItem(hwndDlg,IDC_STOPAFTER_SEC),0);
      }
      char buf[256];
      snprintf(buf, sizeof(buf), "%.1f", (double)g_titlems/1000.0);
      SetDlgItemText(hwndDlg, IDC_MS, buf);
      snprintf(buf, sizeof(buf), "%.1f", (double)g_stop_after_msec/1000.0);
      SetDlgItemText(hwndDlg, IDC_STOPAFTER_SEC, buf);

      SetDlgItemText(hwndDlg, IDC_TITLE, (g_title[0] ? g_title : "Title"));
      EnableWindow(GetDlgItem(hwndDlg, IDC_MS), (g_prefs&1));
      EnableWindow(GetDlgItem(hwndDlg, IDC_BIGFONT), (g_prefs&1));
      EnableWindow(GetDlgItem(hwndDlg, IDC_TITLE), (g_prefs&1));
      SetDlgItemInt(hwndDlg, IDC_LOOPCNT, g_gif_loopcount,FALSE);
#ifndef VIDEO_ENCODER_SUPPORT
      ShowWindow(GetDlgItem(hwndDlg, IDC_BUTTON1), false);
#endif
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
      if (IsDlgButtonChecked(hwndDlg, IDC_CHECK1)) g_prefs |= 32;
      if (IsDlgButtonChecked(hwndDlg, IDC_CHECK2)) g_prefs |= 64;

      char buf[256];
      GetDlgItemText(hwndDlg, IDC_MS, buf, sizeof(buf));
      g_titlems = (int)(atof(buf)*1000.0);

      GetDlgItemText(hwndDlg, IDC_STOPAFTER_SEC, buf, sizeof(buf));
      g_stop_after_msec=(int) (atof(buf)*1000.0);

      GetDlgItemText(hwndDlg, IDC_TITLE, g_title, sizeof(g_title));
      if (!strcmp(g_title, "Title")) g_title[0]=0;

      {
        BOOL t=FALSE;
        int a=GetDlgItemInt(hwndDlg,IDC_LOOPCNT,&t,FALSE);
        if (t) g_gif_loopcount=(a>0&&a<65536) ? a : 0;
      }

    }
    return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_CHECK2:
          {
            int use = !!IsDlgButtonChecked(hwndDlg, IDC_CHECK2);
            EnableWindow(GetDlgItem(hwndDlg,IDC_STOPAFTER_SEC_LBL),use);
            EnableWindow(GetDlgItem(hwndDlg,IDC_STOPAFTER_SEC),use);
          }
        break;
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
#ifdef _WIN32
          if (HIWORD(wParam) == EN_SETFOCUS)
          {
            SendDlgItemMessage(hwndDlg, IDC_TITLE, EM_SETSEL, 0, -1);
          }  
#endif
        return 0;
#ifdef VIDEO_ENCODER_SUPPORT
        case IDC_BUTTON1:
          DialogBox(g_hInst,MAKEINTRESOURCE(IDD_OPTIONS),hwndDlg,VideoOptionsProc);
        break;
#endif
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
      tbm = LICE_CreateMemBitmap(tw, th); 
      LICE_Clear(tbm, 0);                      
    }
    else
    {
      tbm = LICE_CreateMemBitmap(tw, th); 
      LICE_Copy(tbm, g_cap_bm_txt);
      LICE_FillRect(tbm, 0,0,w,h, LICE_RGBA(0,0,0,255), bgAlpha, LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);
    }
    /*
	    LICE_DrawRect(tbm,30,30,150,150,LICE_RGBA(255,0,0,255), 1.0f, LICE_BLIT_MODE_COPY);
	    LICE_DrawRect(tbm,31,31,148,148,LICE_RGBA(255,0,0,255), 1.0f, LICE_BLIT_MODE_COPY);
	    LICE_DrawRect(tbm,32,32,146,146,LICE_RGBA(255,0,0,255), 1.0f, LICE_BLIT_MODE_COPY);
    */
    char buf[4096];
    lstrcpyn_safe(buf, str,sizeof(buf));                    

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

#ifdef TEST_MULTIPLE_MODES
  if (g_cap_gif2)
  {
    g_cap_gif2->frame_finish(); 
    g_cap_gif2->frame_new(g_cap_bm,0,0,g_cap_bm->getWidth(),g_cap_bm->getHeight());
    g_cap_gif2->frame_advancetime(ms);
  }
  if (g_cap_gif3)
  {
    g_cap_gif3->frame_finish(); 
    g_cap_gif3->frame_new(g_cap_bm,0,0,g_cap_bm->getWidth(),g_cap_bm->getHeight());
    g_cap_gif3->frame_advancetime(ms);
  }
#endif

  if (g_cap_gif)
  {
    g_cap_gif->frame_finish(); 
    g_cap_gif->frame_new(g_cap_bm,0,0,g_cap_bm->getWidth(),g_cap_bm->getHeight());
    g_cap_gif->frame_advancetime(ms);
    g_cap_gif_lastsec_written=-1;
  }

#ifdef VIDEO_ENCODER_SUPPORT
  if (g_cap_video)
  {
    int nf=100;
    if (nf>ms) nf=ms;
    EncodeFrameToVideo(g_cap_video,g_cap_bm,true);
    s_audiohook_samples_mutex.Enter();
    s_audiohook_needsilence += nf*0.001;
    s_audiohook_timepos+=nf*0.001;
    s_audiohook_samples_mutex.Leave();

    EncodeFrameToVideo(g_cap_video,g_cap_bm,true);
    s_audiohook_samples_mutex.Enter();
    s_audiohook_needsilence += (ms-nf)*0.001;
    s_audiohook_timepos+=(ms-nf)*0.001;
    s_audiohook_samples_mutex.Leave();
  }
#endif

#ifndef NO_LCF_SUPPORT
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
#endif
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
        snprintf(buf, sizeof(buf), "%.1f", (double)g_insert_ms/1000.0);
        SetDlgItemText(hwnd, IDC_MS, buf);
        snprintf(buf, sizeof(buf), "%.1f", g_insert_alpha);
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
				  GetDlgItemText(hwnd, IDC_ALPHA, buf, sizeof(buf));
				  g_insert_alpha = wdl_min(1.0f, atof(buf));
        }
        break;
        case IDC_MS:
				{
				  char buf[256];
				  GetDlgItemText(hwnd, IDC_MS, buf, sizeof(buf));
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

#ifndef _WIN32
bool GetScreenData(int xpos, int ypos, LICE_IBitmap *bmOut);

#endif


void SaveConfig(HWND hwndDlg)
{
  char buf[1024];
  RECT r;
  GetWindowRect(hwndDlg,&r);
  sprintf(buf,"%d %d %d %d",(int)r.left,(int)r.top,(int)r.right,(int)r.bottom);
  WritePrivateProfileString("licecap","wnd_r",buf,g_ini_file.Get());
  sprintf(buf, "%d", g_max_fps);
  WritePrivateProfileString("licecap","maxfps",buf,g_ini_file.Get());
  sprintf(buf, "%d", g_prefs);
  WritePrivateProfileString("licecap","prefs",buf,g_ini_file.Get());
  sprintf(buf, "%d", g_titlems);
  WritePrivateProfileString("licecap","titlems",buf,g_ini_file.Get());
  sprintf(buf, "%d", g_gif_loopcount);
  WritePrivateProfileString("licecap","gifloopcnt",buf,g_ini_file.Get());
  sprintf(buf, "%d", g_stop_after_msec);
  WritePrivateProfileString("licecap","stopafter",buf,g_ini_file.Get());
  
  

  WritePrivateProfileString("licecap","title",g_title,g_ini_file.Get());

#ifdef VIDEO_ENCODER_SUPPORT
  sprintf(buf, "%d", g_cap_video_vbr);
  WritePrivateProfileString("licecap","video_vbr",buf,g_ini_file.Get());
  sprintf(buf, "%d", g_cap_video_abr);
  WritePrivateProfileString("licecap","video_abr",buf,g_ini_file.Get());
#endif

}

static WDL_DLGRET liceCapMainProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static POINT s_last_mouse;
  switch (uMsg)
  {
    case WM_INITDIALOG:
      g_hwnd=hwndDlg;
#ifdef _WIN32
      SetClassLongPtr(hwndDlg,GCLP_HICON,(LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1)));
#elif defined(__APPLE__)
      extern void SWELL_SetWindowShadow(HWND, bool);
      void SetNSWindowOpaque(HWND, bool);
      SWELL_SetWindowShadow(hwndDlg,false);
      SetNSWindowOpaque(hwndDlg,false);
#endif

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

      g_gif_loopcount = GetPrivateProfileInt("licecap","gifloopcnt",g_gif_loopcount,g_ini_file.Get());
      g_max_fps = GetPrivateProfileInt("licecap", "maxfps", g_max_fps, g_ini_file.Get());
      SetDlgItemInt(hwndDlg,IDC_MAXFPS,g_max_fps,FALSE);
      --g_reent;

      Capture_Finish(hwndDlg);

      UpdateCaption(hwndDlg);
      UpdateStatusText(hwndDlg);

      SetTimer(hwndDlg,1,30,NULL);

      {
        char buf[1024];
        GetPrivateProfileString("licecap","wnd_r","",buf,sizeof(buf),g_ini_file.Get());
        int a[4]={0,};
        const char *p=buf;
        int x;
        const int maxcnt  = 4;
        for (x=0;x<maxcnt;x++)
        {
          a[x] = atoi(p);
          if (x==maxcnt-1) break;
          while (*p && *p != ' ') p++;
          while (*p == ' ') p++;
        }
        if (*p && a[2]>a[0] && a[3]!=a[1]) SetWindowPos(hwndDlg,NULL,a[0],a[1],a[2]-a[0],a[3]-a[1],SWP_NOZORDER|SWP_NOACTIVATE);

      }
      UpdateDimBoxes(hwndDlg);

      g_prefs = GetPrivateProfileInt("licecap", "prefs", g_prefs, g_ini_file.Get());
      g_titlems = GetPrivateProfileInt("licecap", "titlems", g_titlems, g_ini_file.Get());
      g_stop_after_msec = GetPrivateProfileInt("licecap", "stopafter", g_stop_after_msec, g_ini_file.Get());

      GetPrivateProfileString("licecap","title","",g_title,sizeof(g_title),g_ini_file.Get());

#ifdef VIDEO_ENCODER_SUPPORT
      g_cap_video_vbr = GetPrivateProfileInt("licecap", "video_vbr", g_cap_video_vbr, g_ini_file.Get());
      g_cap_video_abr = GetPrivateProfileInt("licecap", "video_abr", g_cap_video_abr, g_ini_file.Get());
#endif

    return 1;
    case WM_DESTROY:

      Capture_Finish(hwndDlg);

      SaveConfig(hwndDlg);

      g_wndsize.init(hwndDlg);
      g_hwnd=NULL;
#ifndef _WIN32
      SWELL_PostQuitMessage(0);
#endif

    break;
    case WM_TIMER:
      if (wParam==1)
      {     
        DWORD now=timeGetTime();
        bool need_stop=false;

        if (g_cap_state==1 && g_cap_bm && now >= g_cap_prerolluntil && now >= g_skip_capture_until)
        {
          if (now >= g_last_frame_capture_time + (1000/(wdl_max(g_max_fps,1))))
          {
            g_ms_written += now-g_last_frame_capture_time;

#ifdef _WIN32
            HWND h = GetDesktopWindow();

            HDC hdc = GetDC(h);
            if (hdc)
            {
              RECT r;
              GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);
              int bw = g_cap_bm->getWidth();
              int bh = g_cap_bm->getHeight();

              __LogicalToPhysicalPointForPerMonitorDPI(g_hwnd,(LPPOINT)&r);
              
              LICE_Clear(g_cap_bm,0);
              BitBlt(g_cap_bm->getDC(),0,0,bw,bh,hdc,r.left+1,r.top+1,SRCCOPY);
              ReleaseDC(h,hdc);
              DoMouseCursor(g_cap_bm,h,-(r.left+1),-(r.top+1));                        
#else
            int bw = g_cap_bm->getWidth();
            int bh = g_cap_bm->getHeight();
            RECT r2,r3;
            HWND sub = GetDlgItem(hwndDlg,IDC_VIEWRECT);
            GetWindowRect(sub,&r2);
#ifdef __APPLE__
            if (s_has_async_offset && GetAsyncNSWindowRect(hwndDlg,&r3))
            {
              const int w = r2.right-r2.left;
              const int h = r2.bottom-r2.top;
              r2.left = r3.left + s_async_offset.x;
              r2.top = r3.top + s_async_offset.y;

              r2.bottom = r2.top + h;
              r2.right = r2.left + w;
            }
#endif

            if (GetScreenData(r2.left,wdl_min(r2.top,r2.bottom),g_cap_bm_inv?g_cap_bm_inv:g_cap_bm))
            {
              void DoMouseCursor(LICE_IBitmap *,int,int);
              DoMouseCursor(g_cap_bm_inv?g_cap_bm_inv:g_cap_bm,-(r2.left+1),-(r2.bottom+1));                        
#endif
              const bool dotime = !!(g_prefs&8);

              const int frame_time_in_seconds = g_ms_written/1000;
              
#ifdef VIDEO_ENCODER_SUPPORT
              if (g_cap_video)
              {
                if (dotime) draw_timedisp(g_cap_bm,frame_time_in_seconds,NULL,bw,bh);

                EncodeFrameToVideo(g_cap_video,g_cap_bm);
              }
#endif
#ifndef NO_LCF_SUPPORT
              if (g_cap_lcf)
              {
                if (dotime) draw_timedisp(g_cap_bm,frame_time_in_seconds,NULL,bw,bh);

                int del = now-g_last_frame_capture_time;
                if (g_dotitle)
                {
                  del += g_titlems;
                  g_dotitle=false;
                }
                g_cap_lcf->OnFrame(g_cap_bm,del);
              }
#endif

              if (g_cap_gif)
              {
                // draw old time display for frame_compare(), so that it finds the portion other than the time display that changes
                int old_time_coords[4]={0,};
                if (dotime && g_cap_gif_lastsec_written>=0)
                  draw_timedisp(g_cap_bm,g_cap_gif_lastsec_written,old_time_coords,bw,bh);
            
                g_cap_gif->frame_advancetime(now-g_last_frame_capture_time);
#ifdef TEST_MULTIPLE_MODES
                if (g_cap_gif2) g_cap_gif2->frame_advancetime(now-g_last_frame_capture_time);
                if (g_cap_gif3) g_cap_gif3->frame_advancetime(now-g_last_frame_capture_time);
#endif

                int diffs[4];
                
                if (g_cap_gif->frame_compare(g_cap_bm,diffs))
                {
                  g_cap_gif->frame_finish();
#ifdef TEST_MULTIPLE_MODES
                  if (g_cap_gif2) g_cap_gif2->frame_finish();
                  if (g_cap_gif3) g_cap_gif3->frame_finish();
#endif

                  if (dotime && frame_time_in_seconds != g_cap_gif_lastsec_written)
                  {
                    int pos[4];
                    draw_timedisp(NULL,frame_time_in_seconds,pos,bw,bh);

                    union_diffs(pos, old_time_coords);

                    if (diffs[0]+diffs[2] >= pos[0] && diffs[1]+diffs[3] >= pos[1])
                    {
                      union_diffs(diffs, pos); // add pos into diffs for display update

                      draw_timedisp(g_cap_bm,frame_time_in_seconds,pos,bw,bh);
                      g_cap_gif_lastsec_written = frame_time_in_seconds;
                    }
                  }

                  g_cap_gif->frame_new(g_cap_bm,diffs[0],diffs[1],diffs[2],diffs[3]);
#ifdef TEST_MULTIPLE_MODES
                  if (g_cap_gif2) g_cap_gif2->frame_new(g_cap_bm,diffs[0],diffs[1],diffs[2],diffs[3]);
                  if (g_cap_gif3) g_cap_gif3->frame_new(g_cap_bm,diffs[0],diffs[1],diffs[2],diffs[3]);
#endif
                }

                if (dotime && frame_time_in_seconds != g_cap_gif_lastsec_written)
                {
                  // time changed and wasn't previously included, so include as a dedicated frame
                  g_cap_gif->frame_finish();
#ifdef TEST_MULTIPLE_MODES
                  if (g_cap_gif2) g_cap_gif2->frame_finish();
                  if (g_cap_gif3) g_cap_gif3->frame_finish();
#endif

                  int pos[4];
                  draw_timedisp(g_cap_bm,frame_time_in_seconds,pos,bw,bh);
                  union_diffs(pos, old_time_coords);

                  g_cap_gif_lastsec_written = frame_time_in_seconds;
                  g_cap_gif->frame_new(g_cap_bm,pos[0],pos[1],pos[2],pos[3]);
#ifdef TEST_MULTIPLE_MODES
                  if (g_cap_gif2) g_cap_gif2->frame_new(g_cap_bm,pos[0],pos[1],pos[2],pos[3]);
                  if (g_cap_gif3) g_cap_gif3->frame_new(g_cap_bm,pos[0],pos[1],pos[2],pos[3]);
#endif
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

              if (g_prefs&64) 
              {
                if (g_ms_written > g_stop_after_msec)
                {
                  need_stop=true;
                }
              }
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
        if (need_stop)
          SendMessage(hwndDlg,WM_COMMAND,IDC_STOP,0);

      }
    break;
#ifdef _WIN32
    case WM_HOTKEY:
      if (lParam == MAKELPARAM(MOD_CONTROL|MOD_ALT, 'P') && (g_prefs&16)) // prefs check not necessary
      {
        SendMessage(hwndDlg, WM_COMMAND, IDC_REC, 0);
      }
    break;   
#else
    case WM_PAINT:

      // osx hook
      {
        PAINTSTRUCT ps;
        if (BeginPaint(hwndDlg,&ps))
        {
          RECT r;
          void DrawTransparentRectInCurrentContext(RECT r);
          GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r);
          ScreenToClient(hwndDlg,(LPPOINT)&r);
          ScreenToClient(hwndDlg,((LPPOINT)&r)+1);
          DrawTransparentRectInCurrentContext(r);
          HPEN pen = CreatePen(PS_SOLID,0,RGB(128,128,128));
          HGDIOBJ oldPen = SelectObject(ps.hdc,pen);
          
          r.left--;r.top--;
          MoveToEx(ps.hdc,r.left,r.top,NULL);
          LineTo(ps.hdc,r.right,r.top);
          LineTo(ps.hdc,r.right,r.bottom);
          LineTo(ps.hdc,r.left,r.bottom);
          LineTo(ps.hdc,r.left,r.top);
          
          GetClientRect(hwndDlg,&r);
          r.right--;r.bottom--;
          MoveToEx(ps.hdc,r.right,r.top,NULL);
          LineTo(ps.hdc,r.right,r.bottom);
          LineTo(ps.hdc,r.left,r.bottom);
          LineTo(ps.hdc,r.left,r.top);
          
          SelectObject(ps.hdc,oldPen);
          DeleteObject(pen);
          EndPaint(hwndDlg,&ps);
        }
      }      
    break;
#endif

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
            GetViewRectSize(&ox,&oy);

            int nx=ox;
            int ny=oy;

            BOOL t;
            int a=GetDlgItemInt(hwndDlg, IDC_XSZ, &t, FALSE);
            if (t && a >= MIN_SIZE_X) nx=a;
            a=GetDlgItemInt(hwndDlg, IDC_YSZ, &t, FALSE);
            if (t && a >= MIN_SIZE_Y) ny=a;

            if (nx != ox || ny != oy)
            {
              RECT r;
              GetWindowRect(hwndDlg, &r);
              int w=(r.right-r.left)+(nx-ox);
              int h=abs(r.bottom-r.top)+(ny-oy);
              SetWindowPos(hwndDlg, 0, 0, 0, w, h, SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);
#ifndef _WIN32
              void RefreshWindowShadows(HWND);
              RefreshWindowShadows(hwndDlg);
              
#endif
            }
          }
        break;

        case IDC_STOP:
          ShowWindow(GetDlgItem(hwndDlg, IDC_INSERT), SW_HIDE);
          g_insert_cnt=0;
          Capture_Finish(hwndDlg);
          UpdateCaption(hwndDlg);
          UpdateStatusText(hwndDlg);

#ifdef _WIN32
          if (g_prefs&16)
          {          
            UnregisterHotKey(hwndDlg, IDC_REC);
          }
#endif
        break;

        case IDC_INSERT:
          if (!g_cap_bm_txt)
          {
            int w,h;
            GetViewRectSize(&w,&h);
            g_cap_bm_txt = LICE_CreateSysBitmap(w,h);
            LICE_Copy(g_cap_bm_txt, g_cap_bm);
          }
          DialogBox(g_hInst,MAKEINTRESOURCE(IDD_INSERT),hwndDlg,InsertProc);
          g_last_frame_capture_time=timeGetTime();
        break;

        case IDC_REC:

          if (!g_cap_state)
          {
            //g_title[0]=0;
            const char *tab[][2]={
              { "GIF files (*.gif)\0*.gif\0", ".gif" },
            #ifndef NO_LCF_SUPPORT
              { "LiceCap files (*.lcf)\0*.lcf\0", ".lcf" },
            #endif
            #ifdef VIDEO_ENCODER_SUPPORT
              { "WEBM files (*.webm)\0*.webm\0", ".webm" },
            #endif
              {NULL,NULL},
            };

            WDL_Queue qb;
            int bm=0;
            const int lfnlen=strlen(g_last_fn);
            int x;
            if (lfnlen >= 3) for (x=0;tab[x][0];x++)
            {
              const int tx1l = strlen(tab[x][1]);
              if (lfnlen > tx1l && !stricmp(g_last_fn + lfnlen - tx1l, tab[x][1])) 
              {
                bm=x;
                break;
              }
            }
            for (x=bm;tab[x][0];x++)
            {
              const char *p=tab[x][0];
              while (*p)
              {
                int l = strlen(p)+1;
                qb.Add(p,l);
                p+=l;
              }
            }
            for (x=0;x<bm;x++)
            {
              const char *p=tab[x][0];
              while (*p)
              {
                int l = strlen(p)+1;
                qb.Add(p,l);
                p+=l;
              }
            }

            qb.Add("",1);

            if (WDL_ChooseFileForSave(hwndDlg, "Choose file for recording", NULL, g_last_fn,
              (char*)qb.Get(), tab[bm][1] + 1, false, g_last_fn, sizeof(g_last_fn),
              MAKEINTRESOURCE(IDD_SAVEOPTS),(void*)SaveOptsProc, 
#ifdef _WIN32
                                      g_hInst
#else
                                      SWELL_curmodule_dialogresource_head
#endif
                                      ))
            {
              WritePrivateProfileString("licecap","lastfn",g_last_fn,g_ini_file.Get());

#ifdef _WIN32
              if (g_prefs&16)
              {          
                RegisterHotKey(hwndDlg, IDC_REC, MOD_CONTROL|MOD_ALT, 'P');               
              }
#endif

              int w,h;
              GetViewRectSize(&w,&h);
              
              delete g_cap_bm;
#ifdef _WIN32
              g_cap_bm = LICE_CreateSysBitmap(w,h);
#else
              delete g_cap_bm_inv;
              g_cap_bm_inv = LICE_CreateSysBitmap(w,h);
              g_cap_bm = new LICE_WrapperBitmap(g_cap_bm_inv->getBits(),g_cap_bm_inv->getWidth(),g_cap_bm_inv->getHeight(),g_cap_bm_inv->getRowSpan(),true);
#endif

              g_dotitle = ((g_prefs&1) && g_titlems);

              if (strlen(g_last_fn)>4 && !stricmp(g_last_fn+strlen(g_last_fn)-4,".gif"))
              {
                void *ctx = LICE_WriteGIFBeginNoFrame(g_last_fn,w,h,(g_prefs&32) ? (-1)&~7 : 0,true);
                if (ctx) g_cap_gif = new gif_encoder(ctx,g_gif_loopcount,0xf8);
                g_cap_gif_lastsec_written = -1;

#ifdef TEST_MULTIPLE_MODES
                char tmp[1024];
                sprintf(tmp,"%s.trans-nostats.gif",g_last_fn);
                ctx = LICE_WriteGIFBeginNoFrame(tmp,w,h,(-1)&~(7|0x100),true);
                if (ctx) g_cap_gif2 = new gif_encoder(ctx,g_gif_loopcount,0xf8);

                sprintf(tmp,"%s.trans-0.gif",g_last_fn);
                ctx = LICE_WriteGIFBeginNoFrame(tmp,w,h,0,true);
                if (ctx) g_cap_gif3 = new gif_encoder(ctx,g_gif_loopcount,0xf8);
#endif

              }
#ifdef VIDEO_ENCODER_SUPPORT
              if (strlen(g_last_fn)>5 && !stricmp(g_last_fn+strlen(g_last_fn)-5,".webm"))
              {
#ifdef REAPER_LICECAP
                static VideoEncoder *(*video_createEncoder2)(const char *);
                if (!video_createEncoder2)
                  *(void **)&video_createEncoder2 = reaperAPI_getfunc("video_createEncoder2");

                if (video_createEncoder2) g_cap_video = video_createEncoder2("WEBM");
#endif

                if (g_cap_video)
                {
                  if (!g_cap_video->open(g_last_fn,"webm", "vp8", "vorbis", g_cap_video_vbr, 100, w,h, 30, g_cap_video_abr, 44100,2))
                  {
                    delete g_cap_video;
                    g_cap_video=0;
                  }
                  else
                  {
                    strcpy(g_cap_video_ext,"WEBM");
#ifdef REAPER_LICECAP
                    s_audiohook_needsilence=0.0;
                    g_cap_video_lastt=-1.0;
                    s_audiohook_timepos=0.0;
                    if (!s_audiohook_reg)
                    {
                      Audio_RegHardwareHook(true,&s_audiohook);
                      s_audiohook_reg=true;
                    }
                    if (!s_audiothread)
                    {
                      // create thread
                      unsigned id;
                      s_audiothread = (HANDLE)_beginthreadex(NULL,0,audioEncodeThread,0,0,&id);
                    }
#endif

                  }
                }

              }
#endif

#ifndef NO_LCF_SUPPORT
              if (strlen(g_last_fn)>4 && !stricmp(g_last_fn+strlen(g_last_fn)-4,".lcf"))
              {
                g_cap_lcf = new LICECaptureCompressor(g_last_fn,w,h);
                if (!g_cap_lcf->IsOpen())
                {
                  delete g_cap_lcf;
                  g_cap_lcf = NULL;
                }
              }
#endif

              if (g_cap_gif
#ifndef NO_LCF_SUPPORT
                || g_cap_lcf
#endif
#ifdef VIDEO_ENCODER_SUPPORT
                || g_cap_video
#endif
                )
              {

                if (g_dotitle)
                  WriteTextFrame(g_title,g_titlems,true,w,h);

#ifdef _WIN32
                SaveRestoreRecRect(hwndDlg,false);

                g_last_wndstyle = SetWindowLong(hwndDlg,GWL_STYLE,GetWindowLong(hwndDlg,GWL_STYLE)&~WS_THICKFRAME);
                SetWindowPos(hwndDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_DRAWFRAME|SWP_NOACTIVATE);

                SaveRestoreRecRect(hwndDlg,true);
#else
                if (g_capwnd_levelsave < 0)
                {
                  g_capwnd_levelsave=SWELL_SetWindowLevel(hwndDlg,1000);
                }
                SWELL_SetWindowResizeable(hwndDlg,false);

#ifdef __APPLE__
                {
                  s_has_async_offset=false;

                  RECT r,r2;
                  if (GetAsyncNSWindowRect(hwndDlg,&r))
                  {
                    GetWindowRect(GetDlgItem(hwndDlg,IDC_VIEWRECT),&r2);
                    s_async_offset.x = r2.left - r.left;
                    s_async_offset.y = r2.top - r.top;
                    s_has_async_offset = true;
                  }
                }
#endif
#endif

                SetDlgItemText(hwndDlg,IDC_REC,"[pause]");
                EnableWindow(GetDlgItem(hwndDlg,IDC_STOP),1);

                g_frate_valid=false;
                g_frate_avg=0.0;
                g_ms_written = 0;

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
#ifndef _WIN32
        case IDCANCEL:
          SendMessage(hwndDlg,WM_CLOSE,0,0);
        return 1;
#endif
      }
    break;
    case WM_CLOSE:
      if (!g_cap_state)
      {

        #if defined(_WIN32) || defined(REAPER_LICECAP)
                EndDialog(hwndDlg,0);
        #else
                DestroyWindow(hwndDlg);
        #endif
      }
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
       

#ifdef _WIN32
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
#endif
        UpdateDimBoxes(hwndDlg);

        InvalidateRect(hwndDlg,NULL,TRUE);
        
      }
    break;
  }
  return 0;
}



#ifndef REAPER_LICECAP

#ifdef _WIN32

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  bool want_appdata = true;
  g_ini_file.Set("licecap.ini");

  // if exepath\licecap.ini is present, use it
  {
    char exepath[2048];
    exepath[0]=0;
    GetModuleFileName(NULL,exepath,sizeof(exepath));
    char *p=exepath;
    while (*p) p++;
    while (p > exepath && *p != '\\') p--; *p=0;

    if (exepath[0])
    {
      g_ini_file.Set(exepath);
      g_ini_file.Append("\\licecap.ini");
      FILE *fp=fopen(g_ini_file.Get(),"r");
      if (fp) 
      {
        fclose(fp);
        want_appdata = false;
      }
    }
  }

  // use appdata/licecap.ini
  if (want_appdata)
  {
    HKEY k;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",0,KEY_READ,&k) == ERROR_SUCCESS)
    {
      char buf[1024];
      DWORD b=sizeof(buf);
      DWORD t=REG_SZ;
      if (RegQueryValueEx(k,"AppData",0,&t,(unsigned char *)buf,&b) == ERROR_SUCCESS && t == REG_SZ)
      {
        g_ini_file.Set(buf);
        g_ini_file.Append("\\licecap.ini");
      }
      RegCloseKey(k);
    }
  }


  GetPrivateProfileString("licecap","lastfn","",g_last_fn,sizeof(g_last_fn),g_ini_file.Get());

  g_hInst = hInstance;
  DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),GetDesktopWindow(),liceCapMainProc);
  return 0;
}

#else
INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2)
{
  switch (msg)
  {
    case SWELLAPP_ONLOAD:
      {
        char exepath[2048];
        exepath[0]=0;
        GetModuleFileName(NULL,exepath,sizeof(exepath));
        char *p=exepath;
        while (*p) p++;
        while (p > exepath && *p != '/') p--; 
        *p=0;
      
        g_ini_file.Set(exepath);
        g_ini_file.Append("/licecap.ini");
        FILE *fp = fopen(g_ini_file.Get(),"r");
        if (fp) 
        {
          fclose(fp);
        }
        else
        {
          char *p=getenv("HOME");
          if (p && *p)
          {
            g_ini_file.Set(p);
            g_ini_file.Append("/Library/Application Support/LICEcap");
            mkdir(g_ini_file.Get(),0777);
            
            g_ini_file.Append("/licecap.ini");
            fp=fopen(g_ini_file.Get(),"a");
            if (fp)
            {
              fclose(fp);
            }
            else
            {
              g_ini_file.Set(exepath);
              g_ini_file.Append("/licecap.ini");
            }
          }
        }
        GetPrivateProfileString("licecap","lastfn","",g_last_fn,sizeof(g_last_fn),g_ini_file.Get());
      }
    break;
    case SWELLAPP_LOADED:
    
      CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,liceCapMainProc);
      ShowWindow(g_hwnd,SW_SHOW);
    break;
    case SWELLAPP_ONCOMMAND:
      if (g_hwnd) SendMessage(g_hwnd,WM_COMMAND,parm1&0xffff,0);
      break;
    case SWELLAPP_DESTROY:
      if (g_hwnd && !g_cap_state) DestroyWindow(g_hwnd);
      break;
  //  case SWELLAPP_PROCESSMESSAGE:
  //    if (MainProcessMessage((MSG*)parm1)>0) return 1;
  //    return 0;
      
  }
  return 0;
}

#endif

// end of standalone
#endif //!REAPER_LICECAP

#ifdef REAPER_LICECAP

// reaper plugin

static bool (*__WDL_ChooseFileForSave)(HWND parent, const char *text, const char *initialdir, const char *initialfile, const char *extlist, const char *defext, bool preservecwd, char *fn, int fnsize, const char *dlgid, void *dlgProc, void *hi);
static void *(*__LICE_WriteGIFBeginNoFrame)(const char *filename, int w, int h, int transparent_alpha, bool dither, bool is_append);
static bool (*__LICE_WriteGIFFrame)(void *handle, LICE_IBitmap *frame, int xpos, int ypos, bool perImageColorMap, int frame_delay, int nreps);
static bool (*__LICE_WriteGIFEnd)(void *handle);


unsigned WINAPI audioEncodeThread(void *p)
{
  WDL_TypedBuf<ReaSample> tmp;
  while (s_audiohook_reg)
  {
    int ns=0;
    if (s_audiohook_samples.Available())
    {
      s_audiohook_samples_mutex.Enter();
      ns = s_audiohook_samples.Available();      
      if (tmp.Resize(ns) && tmp.GetSize()==ns)
      {
        memcpy(tmp.Get(),s_audiohook_samples.Get(),ns*sizeof(ReaSample));
        s_audiohook_samples.Advance(ns);
        s_audiohook_samples.Compact();
      }
      else
      {
        ns=0;
      }
      s_audiohook_samples_mutex.Leave();
    }
    if (!ns) Sleep(1);
    else
    {
      // encode block
      if (g_cap_video)
      {
        ReaSample *s[2] = { tmp.Get(), tmp.Get()+1};
        g_cap_video->encodeAudio(s,ns/2,2,0,2);
      }
    }
  }
  return 0;
}


void OnAudioBuffer(bool isPost, int len, double srate, struct audio_hook_register_t *reg) // called twice per frame, isPost being false then true
{
  if (isPost)
  {
    ReaSample *s1 = reg->GetBuffer(true,0);
    ReaSample *s2 = reg->GetBuffer(true,1);
    if (!s2) s2=s1;
    if (s1)
    {
      s_audiohook_samples_mutex.Enter();
      if (s_audiohook_needsilence>0.0)
      {
        int n = (int) (s_audiohook_needsilence * srate + 0.5);
        if (n>0)
        {
          ReaSample *fo = s_audiohook_samples.Add(NULL,n*2);
          if (fo) memset(fo,0,sizeof(*fo)*n*2);
        }
        s_audiohook_needsilence=0.0;
      }

      if (g_cap_state == 1 && !g_cap_prerolluntil && s_audiohook_samples.GetSize() < 44100*2 * 4)
      {
        s_audiohook_timepos += len/srate;

        if (!s_rs)
          s_rs=Resampler_Create();

        if (s_rs)
        {
          s_rs->SetRates(srate,44100.0);
          s_rs->Extended(RESAMPLE_EXT_SETFEEDMODE,(void*)(INT_PTR)1,0,0);
          ReaSample *bptr=NULL;
          int a=s_rs->ResamplePrepare(len,2,&bptr);
          if (a>len) a=len;
          if (bptr && a>0)
          {
            int x;
            for(x=0;x<a;x++)
            {
              *bptr++ = *s1++;
              *bptr++ = *s2++;
            }

            ReaSample *fo = s_audiohook_samples.Add(NULL,2*len);
            if (fo)
            {
              int b=s_rs->ResampleOut(fo,a,len,2);
              if (b < len)
                s_audiohook_samples.Add(NULL,2*(b - len)); // back up
            }
          }
        }
      }
      s_audiohook_samples_mutex.Leave();
    }
  }
}

void *LICE_WriteGIFBeginNoFrame(const char *filename, int w, int h, int transparent_alpha, bool dither, bool is_append)
{
  if (!__LICE_WriteGIFBeginNoFrame || !__LICE_WriteGIFFrame || !__LICE_WriteGIFEnd)
  {
    *(void **)&__LICE_WriteGIFBeginNoFrame = reaperAPI_getfunc("LICE_WriteGIFBeginNoFrame2");
    *(void **)&__LICE_WriteGIFFrame = reaperAPI_getfunc("LICE_WriteGIFFrame");
    *(void **)&__LICE_WriteGIFEnd = reaperAPI_getfunc("LICE_WriteGIFEnd");
    if (!__LICE_WriteGIFBeginNoFrame || !__LICE_WriteGIFFrame || !__LICE_WriteGIFEnd)
      return NULL;
  }
  return __LICE_WriteGIFBeginNoFrame(filename,w,h,transparent_alpha,dither,is_append);
}

bool LICE_WriteGIFFrame(void *handle, LICE_IBitmap *frame, int xpos, int ypos, bool perImageColorMap, int frame_delay, int nreps)
{
  return __LICE_WriteGIFFrame(handle,frame,xpos,ypos,perImageColorMap,frame_delay,nreps);
}

bool LICE_WriteGIFEnd(void *handle)
{
  return __LICE_WriteGIFEnd(handle);
}


bool WDL_ChooseFileForSave(HWND parent, const char *text, const char *initialdir, const char *initialfile, const char *extlist, const char *defext, bool preservecwd, char *fn, int fnsize, const char *dlgid, void *dlgProc,  void *hi)
{
  if (__WDL_ChooseFileForSave)
    return __WDL_ChooseFileForSave(parent,text,initialdir,initialfile,extlist,defext,preservecwd,fn,fnsize,dlgid,dlgProc,hi);
  return false;
}

int g_command_id;

static unsigned WINAPI uiThread(void *p)
{
  DialogBox(g_hInst,MAKEINTRESOURCE(IDD_DIALOG1),GetDesktopWindow(),liceCapMainProc);
  return 0;
}

static bool hookCommandProc(int command, int flag)
{
  if (g_command_id && command == g_command_id)
  {
    if (!g_hwnd)
    {
      unsigned id;
      HANDLE th=(HANDLE)_beginthreadex(NULL,0,uiThread,NULL,0,&id);
      CloseHandle(th);
      Sleep(100);
    }
    else
      SetForegroundWindow(g_hwnd);
    return true;
  }
  return false;
}


extern "C"
{

REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
{
  g_hInst=hInstance;
  if (rec)
  {
    if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
      return 0;

    IMPORT_LICE_RPLUG(rec);

    const char *(*get_ini_file)();    
    *(void **)&get_ini_file = rec->GetFunc("get_ini_file");
    *(void **)&Audio_RegHardwareHook = rec->GetFunc("Audio_RegHardwareHook");
    *(void **)&__WDL_ChooseFileForSave = rec->GetFunc("WDL_ChooseFileForSave");
    *(void **)&Resampler_Create = rec->GetFunc("Resampler_Create");
    
    
    if (!get_ini_file || !Audio_RegHardwareHook || !Resampler_Create || !__WDL_ChooseFileForSave || !VERIFY_LICE_IMPORTED() || !__LICE_MeasureText || !__LICE_DrawText) return 0;

    reaperAPI_getfunc = rec->GetFunc;
    g_ini_file.Set(get_ini_file());
    GetPrivateProfileString("licecap","lastfn","",g_last_fn,sizeof(g_last_fn),g_ini_file.Get());

    g_command_id = rec->Register("command_id","reaper_licecap_window");
    if (g_command_id)
    {
      static gaccel_register_t acc;
      acc.accel.cmd = g_command_id;
      acc.desc = "Show REAPER_LICEcap window";
      rec->Register("gaccel",&acc);
      rec->Register("hookcommand",(void*)hookCommandProc);
    }

    return 1;
  }
  else
  {
    //unload
    if(g_hwnd)
    {
      PostMessage(g_hwnd, WM_CLOSE, 0, 0);
      DWORD t = GetTickCount();
      while(g_hwnd && (GetTickCount()-t)<5000)
        Sleep(100);
    }

  }
  return 0;
}


};


#endif //REAPER_LICECAP







#ifndef _WIN32

#include "../WDL/swell/swell-dlggen.h"
#include "licecap.rc_mac_dlg"

#endif






