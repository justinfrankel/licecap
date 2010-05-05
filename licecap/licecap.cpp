#include <stdio.h>
#include <windows.h>
#include <signal.h>


#include "../WDL/lice/lice_lcf.h"

bool g_done=false;

void sigfuncint(int a)
{
  printf("\n\nGot signal!\n");
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
  else if ((argc==3||argc==4) && !strcmp(argv[1],"-e"))
  {
    DWORD st = GetTickCount();
    double fr = argc==4 ? atof(argv[3]) : 5.0;
    if (fr < 1.0) fr=1.0;
    fr = 1000.0/fr;

    int x=0;
    HWND h = GetDesktopWindow();
    LICE_SysBitmap bm;
    RECT r;
    GetClientRect(h,&r);
    bm.resize(r.right,r.bottom);
    LICECaptureCompressor *tc = new LICECaptureCompressor(argv[2],r.right,r.bottom);
    if (tc->IsOpen())
    {
      printf("Encoding %dx%d target %.1f fps:\n",r.right,r.bottom,1000.0/fr);

      while (!g_done)
      {
        HDC hdc = GetDC(h);
        if (hdc)
        {
          BitBlt(bm.getDC(),0,0,r.right,r.bottom,hdc,0,0,SRCCOPY);
          ReleaseDC(h,hdc);
        }

        x++;
        printf("Frame: %d (%.1ffps)\r",x,x*1000.0/(GetTickCount()-st));
        tc->OnFrame(&bm,1);
    
        while (GetTickCount() < (DWORD) (st + x*fr) && !g_done) Sleep(1);
      }
      printf("\nFlushing frames\n");
      st = GetTickCount()-st;
      tc->OnFrame(NULL,0);

      WDL_INT64 outsz=tc->GetOutSize();
      WDL_INT64 intsz = tc->GetInSize();
      delete tc;
      printf("%d %dx%d frames in %.1fs, %.1f fps %.1fMB/s (%.1fMB/s -> %.3fMB/s)\n",x,r.right,r.bottom,st/1000.0,x*1000.0/st,
        r.right*r.bottom*4 * (x*1000.0/st) / 1024.0/1024.0,
        intsz /1024.0/1024.0 / (st/1000.0),
        outsz /1024.0/1024.0 / (st/1000.0));
    }
    else printf("Error opening output\n");

  }
  else 
    printf("usage: licecap [-d file.lcf fnoutbase] | [-e file.lcf [fps]]\n");
  
  return 0;
}
