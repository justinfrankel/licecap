
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
#include <sys/socket.h>
#include <sys/fcntl.h>


#include "swell-internal.h"


#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <sched.h>
#endif

#ifdef __linux__
#include <linux/sched.h>
#endif

#include <pthread.h>


#include "../wdlatomic.h"
#include "../mutex.h"
#include "../assocarray.h"

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
  a+=(60*60*24*(365*4+1)/4)*(long long)(1970-1601); // this is approximate
  a*=1000*10000; // seconds to 1/10th microseconds (100 nanoseconds)
  out->dwLowDateTime=a & 0xffffffff;
  out->dwHighDateTime=a>>32;
}

BOOL GetFileTime(int filedes, FILETIME *lpCreationTime, FILETIME *lpLastAccessTime, FILETIME *lpLastWriteTime)
{
  if (filedes<0) return 0;
  struct stat st;
  if (fstat(filedes,&st)) return 0;
  
  if (lpCreationTime) intToFileTime(st.st_ctime,lpCreationTime);
  if (lpLastAccessTime) intToFileTime(st.st_atime,lpLastAccessTime);
  if (lpLastWriteTime) intToFileTime(st.st_mtime,lpLastWriteTime);

  return 1;
}

BOOL SWELL_PtInRect(const RECT *r, POINT p)
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

unsigned int  _controlfp(unsigned int flag, unsigned int mask)
{
#if !defined(__ppc__) && !defined(__LP64__) && !defined(__arm__)
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


BOOL CloseHandle(HANDLE hand)
{
  SWELL_InternalObjectHeader *hdr=(SWELL_InternalObjectHeader*)hand;
  if (!hdr) return FALSE;
  if (hdr->type <= INTERNAL_OBJECT_START || hdr->type >= INTERNAL_OBJECT_END) return FALSE;
  
  if (!wdl_atomic_decr(&hdr->count))
  {
    switch (hdr->type)
    {
      case INTERNAL_OBJECT_FILE:
        {
          SWELL_InternalObjectHeader_File *file = (SWELL_InternalObjectHeader_File*)hdr;
          if (file->fp) fclose(file->fp);
        }
      break;
      case INTERNAL_OBJECT_EXTERNALSOCKET: return FALSE; // pure sockets are not to be closed this way;
      case INTERNAL_OBJECT_SOCKETEVENT:
        {
          SWELL_InternalObjectHeader_SocketEvent *se= (SWELL_InternalObjectHeader_SocketEvent *)hdr;
          if (se->socket[0]>=0) close(se->socket[0]);
          if (se->socket[1]>=0) close(se->socket[1]);
        }
      break;
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
#ifdef __APPLE__
      case INTERNAL_OBJECT_NSTASK:
        {
          SWELL_InternalObjectHeader_NSTask *nst = (SWELL_InternalObjectHeader_NSTask*)hdr;
          extern void SWELL_ReleaseNSTask(void *);
          if (nst->task) SWELL_ReleaseNSTask(nst->task);
        }
      break;
#endif
    }
    free(hdr);
  }
  return TRUE;
}

HANDLE CreateEventAsSocket(void *SA, BOOL manualReset, BOOL initialSig, const char *ignored)
{
  SWELL_InternalObjectHeader_SocketEvent *buf = (SWELL_InternalObjectHeader_SocketEvent*)malloc(sizeof(SWELL_InternalObjectHeader_SocketEvent));
  buf->hdr.type=INTERNAL_OBJECT_SOCKETEVENT;
  buf->hdr.count=1;
  buf->autoReset = !manualReset;
  buf->socket[0]=buf->socket[1]=-1;
  if (socketpair(AF_UNIX,SOCK_STREAM,0,buf->socket)<0) 
  { 
    free(buf);
    return 0;
  }
  fcntl(buf->socket[0], F_SETFL, fcntl(buf->socket[0],F_GETFL) | O_NONBLOCK); // nonblocking

  char c=0;
  if (initialSig&&buf->socket[1]>=0) write(buf->socket[1],&c,1);

  return buf;
}

DWORD WaitForAnySocketObject(int numObjs, HANDLE *objs, DWORD msTO) // only supports special (socket) handles at the moment 
{
  int max_s=0;
  fd_set s;
  FD_ZERO(&s);
  int x;
  for (x = 0; x < numObjs; x ++)
  {
    SWELL_InternalObjectHeader_SocketEvent *se = (SWELL_InternalObjectHeader_SocketEvent *)objs[x];
    if ((se->hdr.type == INTERNAL_OBJECT_EXTERNALSOCKET || se->hdr.type == INTERNAL_OBJECT_SOCKETEVENT) && se->socket[0]>=0)
    {
      FD_SET(se->socket[0],&s);
      if (se->socket[0] > max_s) max_s = se->socket[0];
    }
  }

  if (max_s>0)
  {
again:
    struct timeval tv;
    tv.tv_sec = msTO/1000;
    tv.tv_usec = (msTO%1000)*1000;
    if (select(max_s+1,&s,NULL,NULL,msTO==INFINITE?NULL:&tv)>0) for (x = 0; x < numObjs; x ++)
    {
      SWELL_InternalObjectHeader_SocketEvent *se = (SWELL_InternalObjectHeader_SocketEvent *)objs[x];
      if ((se->hdr.type == INTERNAL_OBJECT_EXTERNALSOCKET || se->hdr.type == INTERNAL_OBJECT_SOCKETEVENT) && se->socket[0]>=0) 
      {
        if (FD_ISSET(se->socket[0],&s)) 
        {
          if (se->hdr.type == INTERNAL_OBJECT_SOCKETEVENT && se->autoReset)
          {
            char buf[128];
            if (read(se->socket[0],buf,sizeof(buf))<1) goto again;
          }
          return WAIT_OBJECT_0 + x;
        }
      }
    }
  }
  
  return WAIT_TIMEOUT;
}

DWORD WaitForSingleObject(HANDLE hand, DWORD msTO)
{
  SWELL_InternalObjectHeader *hdr=(SWELL_InternalObjectHeader*)hand;
  if (!hdr) return WAIT_FAILED;
  
  switch (hdr->type)
  {
#ifdef __APPLE__
    case INTERNAL_OBJECT_NSTASK:
      {
        SWELL_InternalObjectHeader_NSTask *nst = (SWELL_InternalObjectHeader_NSTask*)hdr;
        extern DWORD SWELL_WaitForNSTask(void *,DWORD);
        if (nst->task) return SWELL_WaitForNSTask(nst->task,msTO);
      }
    break;
#endif
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
    case INTERNAL_OBJECT_EXTERNALSOCKET:
    case INTERNAL_OBJECT_SOCKETEVENT:
      {
        SWELL_InternalObjectHeader_SocketEvent *se = (SWELL_InternalObjectHeader_SocketEvent *)hdr;
        if (se->socket[0]<0) Sleep(msTO!=INFINITE?msTO:1);
        else
        {
          fd_set s;
          FD_ZERO(&s);
again:
          FD_SET(se->socket[0],&s);
          struct timeval tv;
          tv.tv_sec = msTO/1000;
          tv.tv_usec = (msTO%1000)*1000;
          if (select(se->socket[0]+1,&s,NULL,NULL,msTO==INFINITE?NULL:&tv)>0 && FD_ISSET(se->socket[0],&s)) 
          {
            if (se->hdr.type == INTERNAL_OBJECT_SOCKETEVENT && se->autoReset)
            {
              char buf[128];
              if (read(se->socket[0],buf,sizeof(buf))<1) goto again;
            }
            return WAIT_OBJECT_0;
          } 
          return WAIT_TIMEOUT;
        }
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
#ifdef SWELL_TARGET_OSX
          struct timespec ts;
          ts.tv_sec = msTO/1000;
          ts.tv_nsec = (msTO%1000)*1000000;
#endif
          while (!evt->isSignal) 
          {
#ifdef SWELL_TARGET_OSX
            if (pthread_cond_timedwait_relative_np(&evt->cond,&evt->mutex,&ts)==ETIMEDOUT)
            {
              rv = WAIT_TIMEOUT;
              break;
            }
#else
            struct timeval tm={0,};
            gettimeofday(&tm,NULL);
            struct timespec ts;
            ts.tv_sec = msTO/1000 + tm.tv_sec;
            ts.tv_nsec = (tm.tv_usec + (msTO%1000)*1000) * 1000;
            if (ts.tv_nsec>=1000000000) 
            {
              int n = ts.tv_nsec/1000000000;
              ts.tv_sec+=n;
              ts.tv_nsec -= ((long long)n * (long long)1000000000);
            }
            if (pthread_cond_timedwait(&evt->cond,&evt->mutex,&ts))
            {
              rv = WAIT_TIMEOUT;
              break;
            }
#endif
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
#ifdef SWELL_TARGET_OSX
  void *arp=SWELL_InitAutoRelease();
#endif
  
  SWELL_InternalObjectHeader_Thread *t=(SWELL_InternalObjectHeader_Thread*)parm;
  t->retv=t->threadProc(t->threadParm);  
  t->done=1;
  CloseHandle(parm);  

#ifdef SWELL_TARGET_OSX
  SWELL_QuitAutoRelease(arp);
#endif
  
  pthread_exit(0);
  return 0;
}

DWORD GetCurrentThreadId()
{
  return (DWORD)(INT_PTR)pthread_self(); // this is incorrect on x64
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
#ifdef SWELL_TARGET_OSX
  SWELL_EnsureMultithreadedCocoa();
#endif
  SWELL_InternalObjectHeader_Thread *buf = (SWELL_InternalObjectHeader_Thread *)malloc(sizeof(SWELL_InternalObjectHeader_Thread));
  buf->hdr.type=INTERNAL_OBJECT_THREAD;
  buf->hdr.count=2;
  buf->threadProc=ThreadProc;
  buf->threadParm = parm;
  buf->retv=0;
  buf->pt=0;
  buf->done=0;
  pthread_create(&buf->pt,NULL,__threadproc,buf);
  
  if (tidOut) *tidOut=(DWORD)(INT_PTR)buf->pt; // incorrect on x64

  return (HANDLE)buf;
}


BOOL SetThreadPriority(HANDLE hand, int prio)
{
  SWELL_InternalObjectHeader_Thread *evt=(SWELL_InternalObjectHeader_Thread*)hand;

#ifdef __linux__
  static int s_rt_max;
  if (!evt && prio >= 0x10000 && prio < 0x10000 + 100)
  {
    s_rt_max = prio - 0x10000;
    return TRUE;
  }
#endif

  if (!evt || evt->hdr.type != INTERNAL_OBJECT_THREAD) return FALSE;
  
  if (evt->done) return FALSE;
    
  int pol;
  struct sched_param param;
  memset(&param,0,sizeof(param));

#ifdef __linux__
  // linux only has meaningful priorities if using realtime threads,
  // for this to be enabled the caller should use:
  // #ifdef __linux__
  // SetThreadPriority(NULL,0x10000 + max_thread_priority (0..99));
  // #endif
  if (s_rt_max < 1 || prio <= THREAD_PRIORITY_NORMAL)
  {
    pol = SCHED_NORMAL;
    param.sched_priority=0;
  }
  else 
  {
    int lb = s_rt_max;
    if (prio < THREAD_PRIORITY_TIME_CRITICAL) 
    {
      lb--;
      if (prio < THREAD_PRIORITY_HIGHEST)  
      {
        lb--;
        if (prio < THREAD_PRIORITY_ABOVE_NORMAL) lb--;
      }
    }
    param.sched_priority = lb < 1 ? 1 : lb;
    pol = SCHED_RR;
  }
  return !pthread_setschedparam(evt->pt,pol,&param);
#else
  if (!pthread_getschedparam(evt->pt,&pol,&param))
  {
    // this is for darwin, but might work elsewhere
    param.sched_priority = 31 + prio;

    int mt=sched_get_priority_min(pol);
    if (param.sched_priority<mt||param.sched_priority > (mt=sched_get_priority_max(pol)))param.sched_priority=mt;
    
    if (!pthread_setschedparam(evt->pt,pol,&param))
    {
      return TRUE;
    }
  }
  return FALSE;
#endif
}

BOOL SetEvent(HANDLE hand)
{
  SWELL_InternalObjectHeader_Event *evt=(SWELL_InternalObjectHeader_Event*)hand;
  if (!evt) return FALSE;
  if (evt->hdr.type == INTERNAL_OBJECT_EVENT) 
  {
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
  if (evt->hdr.type == INTERNAL_OBJECT_SOCKETEVENT)
  {
    SWELL_InternalObjectHeader_SocketEvent *se=(SWELL_InternalObjectHeader_SocketEvent*)hand;
    if (se->socket[1]>=0)
    {
      if (se->socket[0]>=0) 
      {
        fd_set s;
        FD_ZERO(&s);
        FD_SET(se->socket[0],&s);
        struct timeval tv={0,};
        if (select(se->socket[0]+1,&s,NULL,NULL,&tv)>0 && FD_ISSET(se->socket[0],&s)) return TRUE; // already set
      }
      char c=0; 
      write(se->socket[1],&c,1); 
    }
    return TRUE;
  }
  return FALSE;
}
BOOL ResetEvent(HANDLE hand)
{
  SWELL_InternalObjectHeader_Event *evt=(SWELL_InternalObjectHeader_Event*)hand;
  if (!evt) return FALSE;
  if (evt->hdr.type == INTERNAL_OBJECT_EVENT) 
  {
    evt->isSignal=false;
    return TRUE;
  }
  if (evt->hdr.type == INTERNAL_OBJECT_SOCKETEVENT) 
  {
    SWELL_InternalObjectHeader_SocketEvent *se=(SWELL_InternalObjectHeader_SocketEvent*)hand;
    if (se->socket[0]>=0)
    {
      char buf[128];
      read(se->socket[0],buf,sizeof(buf));
    }
    return TRUE;
  }
  return FALSE;
}

HANDLE CreateFile( const char * lpFileName,
                  DWORD dwDesiredAccess,
                  DWORD dwShareMode,
                  void *lpSecurityAttributes,
                  DWORD dwCreationDisposition,
                  DWORD dwFlagsAndAttributes,
                  HANDLE hTemplateFile)
{
  return 0;// INVALID_HANDLE_VALUE;
}

DWORD SetFilePointer(HANDLE hFile, DWORD low, DWORD *high)
{ 
  SWELL_InternalObjectHeader_File *file=(SWELL_InternalObjectHeader_File*)hFile;
  if (!file || file->hdr.type != INTERNAL_OBJECT_FILE || !file->fp || (high && *high) || fseek(file->fp,low,SEEK_SET)==-1) { if (high) *high=0xffffffff; return 0xffffffff; }
  return ftell(file->fp);
}

DWORD GetFilePointer(HANDLE hFile, DWORD *high)
{
  int pos;
  SWELL_InternalObjectHeader_File *file=(SWELL_InternalObjectHeader_File*)hFile;
  if (!file || file->hdr.type != INTERNAL_OBJECT_FILE || !file->fp || (pos=ftell(file->fp))==-1) { if (high) *high=0xffffffff; return 0xffffffff; }
  if (high) *high=0;
  return (DWORD)pos;
}

DWORD GetFileSize(HANDLE hFile, DWORD *high)
{
  SWELL_InternalObjectHeader_File *file=(SWELL_InternalObjectHeader_File*)hFile;
  if (!file || file->hdr.type != INTERNAL_OBJECT_FILE || !file->fp) { if (high) *high=0xffffffff; return 0xffffffff; }
  
  int a=ftell(file->fp);
  fseek(file->fp,0,SEEK_END);
  int ret=ftell(file->fp);
  fseek(file->fp,a,SEEK_SET);
  
  if (high) *high=ret==-1 ? 0xffffffff: 0;
  return (DWORD)ret;
}



BOOL WriteFile(HANDLE hFile,void *buf, DWORD len, DWORD *lenOut, void *ovl)
{
  SWELL_InternalObjectHeader_File *file=(SWELL_InternalObjectHeader_File*)hFile;
  if (!file || file->hdr.type != INTERNAL_OBJECT_FILE || !file->fp || !buf || !len) return FALSE;
  int lo=fwrite(buf,1,len,file->fp);
  if (lenOut) *lenOut = lo;
  return !!lo;
}

BOOL ReadFile(HANDLE hFile,void *buf, DWORD len, DWORD *lenOut, void *ovl)
{
  SWELL_InternalObjectHeader_File *file=(SWELL_InternalObjectHeader_File*)hFile;
  if (!file || file->hdr.type != INTERNAL_OBJECT_FILE || !file->fp || !buf || !len) return FALSE;
  int lo=fread(buf,1,len,file->fp);
  if (lenOut) *lenOut = lo;
  return !!lo;
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


int WinIntersectRect(RECT *out, const RECT *in1, const RECT *in2)
{
  RECT tmp = *in1; in1 = &tmp;
  memset(out,0,sizeof(RECT));
  if (in1->right <= in1->left) return false;
  if (in2->right <= in2->left) return false;
  if (in1->bottom <= in1->top) return false;
  if (in2->bottom <= in2->top) return false;
  
  // left is maximum of minimum of right edges and max of left edges
  out->left = wdl_max(in1->left,in2->left);
  out->right = wdl_min(in1->right,in2->right);
  out->top=wdl_max(in1->top,in2->top);
  out->bottom = wdl_min(in1->bottom,in2->bottom);
  
  return out->right>out->left && out->bottom>out->top;
}
void WinUnionRect(RECT *out, const RECT *in1, const RECT *in2)
{
  out->left = wdl_min(in1->left,in2->left);
  out->top = wdl_min(in1->top,in2->top);
  out->right=wdl_max(in1->right,in2->right);
  out->bottom=wdl_max(in1->bottom,in2->bottom);
}


typedef struct
{
  int sz;
  int refcnt;
} GLOBAL_REC;


void *GlobalLock(HANDLE h)
{
  if (!h) return 0;
  GLOBAL_REC *rec=((GLOBAL_REC*)h)-1;
  rec->refcnt++;
  return h;
}
int GlobalSize(HANDLE h)
{
  if (!h) return 0;
  GLOBAL_REC *rec=((GLOBAL_REC*)h)-1;
  return rec->sz;
}

void GlobalUnlock(HANDLE h)
{
  if (!h) return;
  GLOBAL_REC *rec=((GLOBAL_REC*)h)-1;
  rec->refcnt--;
}
void GlobalFree(HANDLE h)
{
  if (!h) return;
  GLOBAL_REC *rec=((GLOBAL_REC*)h)-1;
  if (rec->refcnt)
  {
    // note error freeing locked ram
  }
  free(rec);
  
}
HANDLE GlobalAlloc(int flags, int sz)
{
  if (sz<0)sz=0;
  GLOBAL_REC *rec=(GLOBAL_REC*)malloc(sizeof(GLOBAL_REC)+sz);
  if (!rec) return 0;
  rec->sz=sz;
  rec->refcnt=0;
  if (flags&GMEM_FIXED) memset(rec+1,0,sz);
  return rec+1;
}

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

static WDL_Mutex s_libraryMutex;
static int libkeycomp(void **p1, void **p2)
{
  INT_PTR a=(INT_PTR)(*p1) - (INT_PTR)(*p2);
  if (a<0)return -1;
  if (a>0) return 1;
  return 0;
}
static WDL_AssocArray<void *, SWELL_HINSTANCE *> s_loadedLibs(libkeycomp); // index by OS-provided handle (rather than filename since filenames could be relative etc)

HINSTANCE LoadLibrary(const char *fn)
{
  return LoadLibraryGlobals(fn,false);
}
HINSTANCE LoadLibraryGlobals(const char *fn, bool symbolsAsGlobals)
{
  if (!fn || !*fn) return NULL;
  
  void *inst = NULL, *bundleinst=NULL;

#ifdef __APPLE__
  struct stat ss;
  if (stat(fn,&ss) || (ss.st_mode&S_IFDIR))
  {
    CFStringRef str=(CFStringRef)SWELL_CStringToCFString(fn); 
    CFURLRef r=CFURLCreateWithFileSystemPath(NULL,str,kCFURLPOSIXPathStyle,true);
    CFRelease(str);
  
    bundleinst=(void *)CFBundleCreate(NULL,r);
    CFRelease(r);
    
    if (bundleinst)
    {
      if (!CFBundleLoadExecutable((CFBundleRef)bundleinst))
      {
        CFRelease((CFBundleRef)bundleinst);
        bundleinst=NULL;
      }
    }      
  }
#endif

  if (!bundleinst)
  {
    inst=dlopen(fn,RTLD_NOW|(symbolsAsGlobals?RTLD_GLOBAL:RTLD_LOCAL));
    if (!inst) return 0;
  }

  WDL_MutexLock lock(&s_libraryMutex);
  
  SWELL_HINSTANCE *rec = s_loadedLibs.Get(bundleinst ? bundleinst : inst);
  if (!rec) 
  { 
    rec = (SWELL_HINSTANCE *)calloc(sizeof(SWELL_HINSTANCE),1);
    rec->instptr = inst;
    rec->bundleinstptr =  bundleinst;
    rec->refcnt = 1;
    s_loadedLibs.Insert(bundleinst ? bundleinst : inst,rec);
  
    int (*SWELL_dllMain)(HINSTANCE, DWORD, LPVOID) = 0;
    BOOL (*dllMain)(HINSTANCE, DWORD, LPVOID) = 0;
    *(void **)&SWELL_dllMain = GetProcAddress(rec,"SWELL_dllMain");
    if (SWELL_dllMain)
    {
      void *SWELLAPI_GetFunc(const char *name);
      
      if (!SWELL_dllMain(rec,DLL_PROCESS_ATTACH,(void*)NULL)) // todo: eventually pass SWELLAPI_GetFunc, maybe?
      {
        FreeLibrary(rec);
        return 0;
      }
      *(void **)&dllMain = GetProcAddress(rec,"DllMain");
      if (dllMain)
      {
        if (!dllMain(rec,DLL_PROCESS_ATTACH,NULL))
        { 
          SWELL_dllMain(rec,DLL_PROCESS_DETACH,(void*)NULL);
          FreeLibrary(rec);
          return 0;
        }
      }
    }
    rec->SWELL_dllMain = SWELL_dllMain;
    rec->dllMain = dllMain;
  }
  else rec->refcnt++;

  return rec;
}

void *GetProcAddress(HINSTANCE hInst, const char *procName)
{
  if (!hInst) return 0;

  SWELL_HINSTANCE *rec=(SWELL_HINSTANCE*)hInst;

  void *ret = NULL;
#ifdef __APPLE__
  if (rec->bundleinstptr)
  {
    CFStringRef str=(CFStringRef)SWELL_CStringToCFString(procName); 
    ret = (void *)CFBundleGetFunctionPointerForName((CFBundleRef)rec->bundleinstptr, str);
    if (ret) rec->lastSymbolRequested=ret;
    CFRelease(str);
    return ret;
  }
#endif
  if (rec->instptr)  ret=(void *)dlsym(rec->instptr, procName);
  if (ret) rec->lastSymbolRequested=ret;
  return ret;
}

BOOL FreeLibrary(HINSTANCE hInst)
{
  if (!hInst) return FALSE;

  WDL_MutexLock lock(&s_libraryMutex);

  bool dofree=false;
  SWELL_HINSTANCE *rec=(SWELL_HINSTANCE*)hInst;
  if (--rec->refcnt<=0) 
  {
    dofree=true;
    s_loadedLibs.Delete(rec->bundleinstptr ? rec->bundleinstptr : rec->instptr); 
    
    if (rec->SWELL_dllMain) 
    {
      rec->SWELL_dllMain(rec,DLL_PROCESS_DETACH,NULL);
      if (rec->dllMain) rec->dllMain(rec,DLL_PROCESS_DETACH,NULL);
    }
  }

#ifdef __APPLE__
  if (rec->bundleinstptr)
  {
    CFRelease((CFBundleRef)rec->bundleinstptr);
  }
#endif
  if (rec->instptr) dlclose(rec->instptr); 
  
  if (dofree) free(rec);
  return TRUE;
}

void* SWELL_GetBundle(HINSTANCE hInst)
{
  SWELL_HINSTANCE* rec=(SWELL_HINSTANCE*)hInst;
  if (rec) return rec->bundleinstptr;
  return NULL;
}

DWORD GetModuleFileName(HINSTANCE hInst, char *fn, DWORD nSize)
{
  *fn=0;

  void *instptr = NULL, *bundleinstptr=NULL, *lastSymbolRequested=NULL;
  if (hInst)
  {
    SWELL_HINSTANCE *p = (SWELL_HINSTANCE*)hInst;
    instptr = p->instptr;
    bundleinstptr = p->bundleinstptr;
    lastSymbolRequested=p->lastSymbolRequested;
  }
#ifdef __APPLE__
  if (!instptr || bundleinstptr)
  {
    CFBundleRef bund=bundleinstptr ? (CFBundleRef)bundleinstptr : CFBundleGetMainBundle();
    if (bund) 
    {
      CFURLRef url=CFBundleCopyBundleURL(bund);
      if (url)
      {
        char buf[8192];
        if (CFURLGetFileSystemRepresentation(url,true,(UInt8*)buf,sizeof(buf))) lstrcpyn(fn,buf,nSize);
        CFRelease(url);
      }
    }
    return strlen(fn);
  }
#else
  if (!instptr) // get exe file name
  {
    char tmp[64];
    sprintf(tmp,"/proc/%d/exe",getpid());
    int sz=readlink(tmp,fn,nSize);
    if (sz<0)sz=0;
    else if (sz>=nSize)sz=nSize-1;
    fn[sz]=0;
    return sz;
  }
#endif

  if (instptr && lastSymbolRequested)
  {
    Dl_info inf={0,};
    dladdr(lastSymbolRequested,&inf);
    if (inf.dli_fname)
    {
      lstrcpyn(fn,inf.dli_fname,nSize);
      return strlen(fn);
    }
  }
  return 0;
}

#ifdef __APPLE__

void SWELL_GenerateGUID(void *g)
{
  CFUUIDRef r = CFUUIDCreate(NULL);
  if (r)
  {
    CFUUIDBytes a = CFUUIDGetUUIDBytes(r);
    if (g) memcpy(g,&a,16);
    CFRelease(r);
  }
}

#endif


void GetTempPath(int bufsz, char *buf)
{
  if (bufsz<2)
  {
    if (bufsz>0) *buf=0;
    return;
  }

#ifdef __APPLE__
  const char *p = getenv("TMPDIR");
#else
  const char *p = getenv("TEMP");
#endif
  if (!p || !*p) p="/tmp/";
  lstrcpyn(buf, p, bufsz);

  size_t len = strlen(buf);
  if (!len || buf[len-1] != '/')
  {
    if (len > bufsz-2) len = bufsz-2;

    buf[len] = '/';
    buf[len+1]=0;
  }
}

#endif
