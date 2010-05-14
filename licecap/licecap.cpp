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


typedef struct {
  DWORD   cbSize;
  DWORD   flags;
  HCURSOR hCursor;
  POINT   ptScreenPos;
} pCURSORINFO, *pPCURSORINFO, *pLPCURSORINFO;

void DoMouseCursor(HDC hdc, HWND h, int xoffs, int yoffs)
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
      DrawIconEx(hdc,ci.ptScreenPos.x-inf.xHotspot + xoffs,ci.ptScreenPos.y-inf.yHotspot + yoffs,ci.hCursor,0,0,0,NULL,DI_NORMAL);
      if (inf.hbmColor) DeleteObject(inf.hbmColor);
      if (inf.hbmMask) DeleteObject(inf.hbmMask);
    }
  }
  else printf("fail cursor\n");
}

int main(int argc, char **argv)
{
  signal(SIGINT,sigfuncint);
  if (argc==4 && !strcmp(argv[1],"-d"))
  {
    LICECaptureDecompressor tc(argv[2],true);
    if (tc.IsOpen())
    {
      int x;

      if (strstr(argv[3],".gif"))
      {
        void *wr=LICE_WriteGIFBeginNoFrame(argv[3],tc.GetWidth(),tc.GetHeight(),0,true);

        if (wr)
        {
          void *octree = LICE_CreateOctree(256);
          if (octree)
          {
            printf("building palette...");
            fflush(stdout);
            for (x=0;!g_done;x++)
            {
              LICE_IBitmap *bm = tc.GetCurrentFrame();
              if (!bm) break;
              LICE_BuildOctree(octree, bm);
              printf(".");
              fflush(stdout);
              tc.NextFrame();
            }
            LICE_SetGIFColorMapFromOctree(wr, octree, 256);
            printf("done\n");
            LICE_DestroyOctree(octree);
          }

          tc.Seek(0);
          for (x=0;!g_done;x++)
          {
            LICE_IBitmap *bm = tc.GetCurrentFrame();
            if (!bm) break;
            tc.NextFrame();

            LICE_WriteGIFFrame(wr,bm,0,0,true,tc.GetTimeToNextFrame());
          }
          LICE_WriteGIFEnd(wr);
        }
        else
        {
           printf("error writing gif '%s'\n",argv[3]);
        }
      }
      else for (x=0;!g_done;x++)
      {
        LICE_IBitmap *bm = tc.GetCurrentFrame();
        if (!bm) break;
        tc.NextFrame();
        char buf[512];
        if (1)
        {
          sprintf(buf,"%s%03d.png",argv[3],x);
          LICE_WritePNG(buf,bm);
        }
        else
        {
          sprintf(buf,"%s%03d.gif",argv[3],x);
          LICE_WriteGIF(buf,bm,0,false);
        }

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

    LICE_MemBitmap *lastbm=NULL;

    bool gifMode=false,pngMode=false;
    if (strstr(argv[2],".gif")) gifMode=true;
    if (strstr(argv[2],".png")) pngMode=true;

    LICECaptureCompressor *tc = NULL;
    void *gif_wr=NULL;
    bool newCmap = true; //for gif mode
    
    if (!gifMode&&!pngMode) tc = new LICECaptureCompressor(argv[2],r.right,r.bottom);
    if (gifMode||pngMode||tc->IsOpen())
    {
      printf("Encoding %dx%d target %.1f fps (press Ctrl+C to stop):\n",r.right,r.bottom,1000.0/fr);

      DWORD lastt=GetTickCount();
      while (!g_done)
      {
        HDC hdc = GetDC(h);
        if (hdc)
        {
          BitBlt(bm.getDC(),0,0,r.right,r.bottom,hdc,0,0,SRCCOPY);
          ReleaseDC(h,hdc);
        }

        DoMouseCursor(bm.getDC(),h,0,0);

        DWORD thist = GetTickCount();

        x++;
        printf("Frame: %d (%.1ffps, offs=%d)\r",x,x*1000.0/(thist-st),thist-lastt);

        if (tc) 
          tc->OnFrame(&bm,thist - lastt);
        else if (pngMode)
        {
          char buf[512];
          strcpy(buf,argv[2]);
          char *p=buf;
          while(*p)p++;
          while(p>buf&&*p!='.')p--;
          if (p>buf)
          {
            sprintf(p,"-%03d.png",x-1);
          }
          LICE_WritePNG(buf,&bm,false);
        }
        else if (gifMode)
        {
          if (!gif_wr)
          {
            if (!lastbm) 
            {
              lastbm = new LICE_MemBitmap;
            }
            else
            {
              gif_wr=LICE_WriteGIFBegin(argv[2],lastbm,0,thist-lastt,false);
              if (!gif_wr)
              {
                printf("error writing to gif\n");
                break;
              }
            }
          }
          else if (gif_wr)
            LICE_WriteGIFFrame(gif_wr,lastbm,0,0,newCmap,thist-lastt);

          if (lastbm) LICE_Copy(lastbm,&bm);
        }
        lastt = thist;
    
        while (GetTickCount() < (DWORD) (st + x*fr) && !g_done) Sleep(1);
      }
      printf("\nFlushing frames\n");
      st = GetTickCount()-st;
      WDL_INT64 intsz=0,outsz=0;
      if (tc)
      {
        tc->OnFrame(NULL,0);
        outsz=tc->GetOutSize();
        intsz = tc->GetInSize();
        delete tc;
      }
      if (gif_wr)
      {
        if (lastbm)
          LICE_WriteGIFFrame(gif_wr,lastbm,0,0,newCmap,GetTickCount()-lastt);
        LICE_WriteGIFEnd(gif_wr);
      }
      delete lastbm;
      lastbm=0;

      printf("%d %dx%d frames in %.1fs, %.1f fps %.1fMB/s (%.1fMB/s -> %.3fMB/s)\n",x,r.right,r.bottom,st/1000.0,x*1000.0/st,
        r.right*r.bottom*4 * (x*1000.0/st) / 1024.0/1024.0,
        intsz /1024.0/1024.0 / (st/1000.0),
        outsz /1024.0/1024.0 / (st/1000.0));
    }
    else printf("Error opening output\n");

  }
  else 
    printf("usage: licecap [-d file.lcf fnout[.gif]] | [-e file.[lcf|gif] [fps]]\n");
  
  return 0;
}
