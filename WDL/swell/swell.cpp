
/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Losers (who use OS X))
   Copyright (C) 2006-2007, Cockos, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
  

    This file implements a few Windows calls using their posix equivilents

  */

#ifndef SWELL_PROVIDED_BY_APP


#include "swell.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#import <Carbon/Carbon.h>
#include <libkern/OSAtomic.h>
#include <pthread.h>
#include <sched.h>


char *lstrcpyn(char *dest, const char *src, int l)
{
  if (l<1) return dest;

  char *dsrc=dest;
  while (--l > 0)
  {
    char p=*src++;
    if (!p) break;
    *dest++=p;
  }
  *dest++=0;

  return dsrc;
}

void Sleep(int ms)
{
  usleep(ms?ms*1000:100);
}

DWORD GetTickCount()
{
  struct timeval tm={0,};
  gettimeofday(&tm,NULL);
  return tm.tv_sec*1000 + tm.tv_usec/1000;
}


static void intToFileTime(time_t t, FILETIME *out)
{
  unsigned long long a=(unsigned long long)t; // seconds since january 1st, 1970
  a+=(60*60*24*(365*4+1)/4)*(1970-1601); // this is approximate
  a*=1000*10000; // seconds to 1/10th microseconds (100 nanoseconds)
  out->dwLowDateTime=a & 0xffffffff;
  out->dwHighDateTime=a>>32;
}

BOOL GetFileTime(void *fh, FILETIME *lpCreationTime, FILETIME *lpLastAccessTime, FILETIME *lpLastWriteTime)
{
  if (!fh) return 0;
  FILE *fp=(FILE *)fh;
  struct stat st;
  if (fstat(fileno(fp),&st)) return 0;
  
  if (lpCreationTime) intToFileTime(st.st_ctime,lpCreationTime);
  if (lpLastAccessTime) intToFileTime(st.st_atime,lpLastAccessTime);
  if (lpLastWriteTime) intToFileTime(st.st_mtime,lpLastWriteTime);

  return 1;
}

BOOL SWELL_PtInRect(RECT *r, POINT p)
{
  if (!r) return FALSE;
  int tp=r->top;
  int bt=r->bottom;
  if (tp>bt)
  {
    bt=tp;
    tp=r->bottom;
  }
  return p.x>=r->left && p.x<r->right && p.y >= tp && p.y < bt;
}


int MulDiv(int a, int b, int c)
{
  if(c == 0) return 0;
  return (int)((double)a*(double)b/c);
}

DWORD GetModuleFileName(HINSTANCE ignored, char *fn, DWORD nSize)
{
  *fn=0;
  CFBundleRef bund=CFBundleGetMainBundle();
  if (bund) 
  {
    CFURLRef url=CFBundleCopyBundleURL(bund);
    if (url)
    {
      char buf[8192];
      if (CFURLGetFileSystemRepresentation(url,true,(UInt8*)buf,sizeof(buf)))
        lstrcpyn(fn,buf,nSize);

      CFRelease(url);
    }
  }
  return strlen(fn);
}


unsigned int  _controlfp(unsigned int flag, unsigned int mask)
{
#ifndef __ppc__
  unsigned short ret;
  mask &= _MCW_RC; // don't let the caller set anything other than round control for now
  __asm__ __volatile__("fnstcw %0\n\t":"=m"(ret));
  ret=(ret&~(mask<<2))|(flag<<2);
  
  if (mask) __asm__ __volatile__(
	  "fldcw %0\n\t"::"m"(ret));
  return (unsigned int) (ret>>2);
#else
  return 0;
#endif
}


enum
{
  INTERNAL_OBJECT_START= 0x1000001,
  INTERNAL_OBJECT_THREAD,
  INTERNAL_OBJECT_EVENT,
  INTERNAL_OBJECT_END
};

typedef struct
{
   int type; // INTERNAL_OBJECT_*
   int count; // reference count
} SWELL_InternalObjectHeader;

typedef struct
{
  SWELL_InternalObjectHeader hdr;
  DWORD (*threadProc)(LPVOID);
  void *threadParm;
  pthread_t pt;
  DWORD retv;
  bool done;
} SWELL_InternalObjectHeader_Thread;

typedef struct
{
  SWELL_InternalObjectHeader hdr;
  
  pthread_mutex_t mutex;
  pthread_cond_t cond;

  bool isSignal;
  bool isManualReset;
  
} SWELL_InternalObjectHeader_Event;


BOOL CloseHandle(HANDLE hand)
{
  SWELL_InternalObjectHeader *hdr=(SWELL_InternalObjectHeader*)hand;
  if (!hdr) return FALSE;
  if (hdr->type <= INTERNAL_OBJECT_START || hdr->type >= INTERNAL_OBJECT_END) return FALSE;
  
  if (!OSAtomicDecrement32(&hdr->count))
  {
    switch (hdr->type)
    {
      case INTERNAL_OBJECT_EVENT:
        {
          SWELL_InternalObjectHeader_Event *evt=(SWELL_InternalObjectHeader_Event*)hdr;
          pthread_cond_destroy(&evt->cond);
          pthread_mutex_destroy(&evt->mutex);
        }
      break;
      case INTERNAL_OBJECT_THREAD:
        {
          SWELL_InternalObjectHeader_Thread *thr = (SWELL_InternalObjectHeader_Thread*)hdr;
          void *tmp;
          pthread_join(thr->pt,&tmp);
          pthread_detach(thr->pt);
        }
      break;
    }
    free(hdr);
  }
  return TRUE;
}


DWORD WaitForSingleObject(HANDLE hand, DWORD msTO)
{
  SWELL_InternalObjectHeader *hdr=(SWELL_InternalObjectHeader*)hand;
  if (!hdr) return WAIT_FAILED;
  
  switch (hdr->type)
  {
    case INTERNAL_OBJECT_THREAD:
      {
        SWELL_InternalObjectHeader_Thread *thr = (SWELL_InternalObjectHeader_Thread*)hdr;
        void *tmp;
        if (!thr->done) 
        {
          if (!msTO) return WAIT_TIMEOUT;
          if (msTO != INFINITE)
          {
            DWORD d=GetTickCount()+msTO;
            while (GetTickCount()<d && !thr->done) Sleep(1);
            if (!thr->done) return WAIT_TIMEOUT;
          }
        }
    
        if (!pthread_join(thr->pt,&tmp)) return WAIT_OBJECT_0;      
      }
    break;
    case INTERNAL_OBJECT_EVENT:
      {
        SWELL_InternalObjectHeader_Event *evt = (SWELL_InternalObjectHeader_Event*)hdr;    
        int rv=WAIT_OBJECT_0;
        pthread_mutex_lock(&evt->mutex);
        if (msTO == 0)  
        {
          if (!evt->isSignal) rv=WAIT_TIMEOUT;
        }
        else if (msTO == INFINITE)
        {
          while (!evt->isSignal) pthread_cond_wait(&evt->cond,&evt->mutex);
        }
        else
        {
          // timed wait
          struct timespec ts={msTO/1000, (msTO%1000)*1000000};      
          while (!evt->isSignal) 
          {
            if (pthread_cond_timedwait_relative_np(&evt->cond,&evt->mutex,&ts)==ETIMEDOUT)
            {
              rv = WAIT_TIMEOUT;
              break;
            }
            // we should track/correct the timeout amount here since in theory we could end up waiting a bit longer!
          }
        }    
        if (!evt->isManualReset && rv==WAIT_OBJECT_0) evt->isSignal=false;
        pthread_mutex_unlock(&evt->mutex);
  
        return rv;
      }
    break;
  }
  
  return WAIT_FAILED;
}

static void *__threadproc(void *parm)
{
  void *arp=SWELL_InitAutoRelease();
  
  SWELL_InternalObjectHeader_Thread *t=(SWELL_InternalObjectHeader_Thread*)parm;
  t->retv=t->threadProc(t->threadParm);  
  t->done=1;
  CloseHandle(parm);  

  SWELL_QuitAutoRelease(arp);
  
  pthread_exit(0);
  return 0;
}

DWORD GetCurrentThreadId()
{
  return (DWORD)pthread_self();
}

HANDLE CreateEvent(void *SA, BOOL manualReset, BOOL initialSig, const char *ignored)
{
  SWELL_InternalObjectHeader_Event *buf = (SWELL_InternalObjectHeader_Event*)malloc(sizeof(SWELL_InternalObjectHeader_Event));
  buf->hdr.type=INTERNAL_OBJECT_EVENT;
  buf->hdr.count=1;
  buf->isSignal = !!initialSig;
  buf->isManualReset = !!manualReset;
  
  pthread_mutex_init(&buf->mutex,NULL);
  pthread_cond_init(&buf->cond,NULL);
  
  return (HANDLE)buf;
}


HANDLE CreateThread(void *TA, DWORD stackSize, DWORD (*ThreadProc)(LPVOID), LPVOID parm, DWORD cf, DWORD *tidOut)
{
  SWELL_EnsureMultithreadedCocoa();
  SWELL_InternalObjectHeader_Thread *buf = (SWELL_InternalObjectHeader_Thread *)malloc(sizeof(SWELL_InternalObjectHeader_Thread));
  buf->hdr.type=INTERNAL_OBJECT_THREAD;
  buf->hdr.count=2;
  buf->threadProc=ThreadProc;
  buf->threadParm = parm;
  buf->retv=0;
  buf->pt=0;
  buf->done=0;
  pthread_create(&buf->pt,NULL,__threadproc,buf);
  
  if (tidOut) *tidOut=(DWORD)buf->pt;

  return (HANDLE)buf;
}


BOOL SetThreadPriority(HANDLE hand, int prio)
{
  SWELL_InternalObjectHeader_Thread *evt=(SWELL_InternalObjectHeader_Thread*)hand;
  if (!evt || evt->hdr.type != INTERNAL_OBJECT_THREAD) return FALSE;
  
  if (evt->done) return FALSE;
    
  int pol;
  struct sched_param param;
  if (!pthread_getschedparam(evt->pt,&pol,&param))
  {

//    printf("thread prio %d(%d,%d), %d(FIFO=%d, RR=%d)\n",param.sched_priority, sched_get_priority_min(pol),sched_get_priority_max(pol), pol,SCHED_FIFO,SCHED_RR);
    param.sched_priority = 31 + prio;
    int mt=sched_get_priority_min(pol);
    if (param.sched_priority<mt||param.sched_priority > (mt=sched_get_priority_max(pol)))param.sched_priority=mt;
    
    if (!pthread_setschedparam(evt->pt,pol,&param))
    {
      return TRUE;
    }
  }
  
  
  
  return FALSE;
}

BOOL SetEvent(HANDLE hand)
{
  SWELL_InternalObjectHeader_Event *evt=(SWELL_InternalObjectHeader_Event*)hand;
  if (!evt || evt->hdr.type != INTERNAL_OBJECT_EVENT) return FALSE;
  
  pthread_mutex_lock(&evt->mutex);
  if (!evt->isSignal)
  {
    evt->isSignal = true;
    if (evt->isManualReset) pthread_cond_broadcast(&evt->cond);
    else pthread_cond_signal(&evt->cond);
  }
  pthread_mutex_unlock(&evt->mutex);
  return TRUE;
}
BOOL ResetEvent(HANDLE hand)
{
  SWELL_InternalObjectHeader_Event *evt=(SWELL_InternalObjectHeader_Event*)hand;
  if (!evt || evt->hdr.type != INTERNAL_OBJECT_EVENT) return FALSE;
  
  evt->isSignal=false;
  
  return TRUE;
}



BOOL WinOffsetRect(LPRECT lprc, int dx, int dy)
{
  if(!lprc) return 0;
  lprc->left+=dx;
  lprc->top+=dy;
  lprc->right+=dx;
  lprc->bottom+=dy;
  return TRUE;
}

BOOL WinSetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
{
  if(!lprc) return 0;
  lprc->left = xLeft;
  lprc->top = yTop;
  lprc->right = xRight;
  lprc->bottom = yBottom;
  return TRUE;
}


int WinIntersectRect(RECT *out, RECT *in1, RECT *in2)
{
  memset(out,0,sizeof(RECT));
  if (in1->right <= in1->left) return false;
  if (in2->right <= in2->left) return false;
  if (in1->bottom <= in1->top) return false;
  if (in2->bottom <= in2->top) return false;
  
  // left is maximum of minimum of right edges and max of left edges
  out->left = max(in1->left,in2->left);
  out->right = min(in1->right,in2->right);
  out->top=max(in1->top,in2->top);
  out->bottom = min(in1->bottom,in2->bottom);
  
  return out->right>out->left && out->bottom>out->top;
}
void WinUnionRect(RECT *out, RECT *in1, RECT *in2)
{
  out->left = min(in1->left,in2->left);
  out->top = min(in1->top,in2->top);
  out->right=max(in1->right,in2->right);
  out->bottom=max(in1->bottom,in2->bottom);
}



#endif
