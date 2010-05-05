#include <stdio.h>
#include <windows.h>
#include <signal.h>


#include "../WDL/lice/lice_lcf.h"

bool g_done=false;

void sigfuncint(int a)
{
  printf("Got signal!\n");
  g_done=true;
}


int main(int argc, char **argv)
{
  signal(SIGINT,sigfuncint);
  if (argc==4 && !strcmp(argv[1],"-d"))
  {
    LICECaptureDecompressor tc(argv[2]);
    if (tc.IsOpen())
    {
      int x;
      for (x=0;!g_done;x++)
      {
        LICE_IBitmap *bm = tc.GetCurrentFrame();
        if (!bm) break;
        tc.NextFrame();
        char buf[512];
        sprintf(buf,"%s%03d.png",argv[3],x);
        LICE_WritePNG(buf,bm);
      }
    }
    else printf("Error opening '%s'\n",argv[2]);
  }
  else if (argc==4 && !strcmp(argv[1],"-e"))
  {
    DWORD st = GetTickCount();
    int nmsec= atoi(argv[3])*1000;
    int x=0;
    HWND h = GetDesktopWindow();
    LICE_SysBitmap bm;
    RECT r;
    GetClientRect(h,&r);
    bm.resize(r.right,r.bottom);
    LICECaptureCompressor *tc = new LICECaptureCompressor(argv[2],r.right,r.bottom);
    if (tc->IsOpen()) while (GetTickCount() < st + nmsec && !g_done)
    {
      HDC hdc = GetDC(h);
      if (hdc)
      {
        BitBlt(bm.getDC(),0,0,r.right,r.bottom,hdc,0,0,SRCCOPY);
        ReleaseDC(h,hdc);
      }

      printf("Encoding frame: %d\n",++x);
      tc->OnFrame(&bm,1);
    
      while (GetTickCount() < st + 200*x && !g_done) Sleep(1);
    }
    printf("Flushing frames\n");
    tc->OnFrame(NULL,0);
    WDL_INT64 outsz=tc->GetOutSize();
    WDL_INT64 intsz = tc->GetInSize();
    delete tc;
    st = GetTickCount()-st;
    printf("%d %dx%d frames in %.1fs, %.1f fps %.1fMB/s (%.1fMB/s -> %.3fMB/s)\n",x,r.right,r.bottom,st/1000.0,x*1000.0/st,
      r.right*r.bottom*4 * (x*1000.0/st) / 1024.0/1024.0,
      intsz /1024.0/1024.0 / (st/1000.0),
      outsz /1024.0/1024.0 / (st/1000.0));
  }
  else 
    printf("usage: licecap [-d file.lcf fnoutbase] | [-e file.lcf nsec]\n");
  
  return 0;
}
