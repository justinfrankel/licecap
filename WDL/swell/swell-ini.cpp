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

  This file implements basic win32 GetPrivateProfileString / etc support.
  It works by caching reads, but writing through on every write that is required (to ensure 
  that updates take, especially when being updated from multiple modules who have their own 
  cache of the .ini file).

  It is threadsafe, but in theory if two processes are trying to access the same ini, 
  results may go a bit unpredictable (but in general the file should NOT get corrupted,
  we hope).

*/

#ifndef SWELL_PROVIDED_BY_APP


#include "swell.h"
#include "../assocarray.h"
#include "../mutex.h"
#include "../queue.h"
#include <sys/stat.h>
#include <sys/types.h>

static void deleteStringKeyedArray(WDL_StringKeyedArray<char *> *p) {   delete p; }

struct iniFileContext
{
  iniFileContext() : m_sections(false,deleteStringKeyedArray) 
  { 
    m_curfn=NULL;
    m_curfp=NULL;
    m_lastaccesscnt=0;
    m_curfn_time=0;
  }
  ~iniFileContext() { }
  
  char *m_curfn;
  FILE *m_curfp;  
  WDL_UINT64 m_lastaccesscnt;
  time_t m_curfn_time;
  WDL_StringKeyedArray< WDL_StringKeyedArray<char *> * > m_sections;

};

#define NUM_OPEN_CONTEXTS 3
static iniFileContext s_ctxs[NUM_OPEN_CONTEXTS];
static WDL_Mutex m_mutex;

static time_t getfileupdtime(FILE *fp)
{
  if (!fp) return 0;
  struct stat st;
  if (fstat(fileno(fp),&st)) return 0;
  return st.st_mtime;
}

// return true on success
static iniFileContext *GetFileContext(const char *name)
{
  static WDL_UINT64 acc_cnt;
  int best_z = 0;
  {
    int w;
    bool had_z=false;
    WDL_UINT64 bestcnt = 0; bestcnt--;
    for (w=0;w<NUM_OPEN_CONTEXTS;w++)
    {
      if (s_ctxs[w].m_curfn && !stricmp(s_ctxs[w].m_curfn,name)) 
      {
        best_z=w;
        break;
      }

      if (!had_z)
      {
        if (!s_ctxs[w].m_curfn || !s_ctxs[w].m_curfp) { best_z = w; had_z=true; }    
        else if (s_ctxs[w].m_lastaccesscnt < bestcnt) { best_z = w; bestcnt = s_ctxs[w].m_lastaccesscnt; }
      }
    }
  }
    
  iniFileContext *ctx = &s_ctxs[best_z];
  ctx->m_lastaccesscnt=++acc_cnt;
  
  if (!ctx->m_curfn || stricmp(ctx->m_curfn,name) || !ctx->m_curfp || ctx->m_curfn_time != getfileupdtime(ctx->m_curfp))
  {
    ctx->m_sections.DeleteAll();
//    printf("reinitting to %s\n",name);
    if (!ctx->m_curfn || stricmp(ctx->m_curfn,name))
    {
      free(ctx->m_curfn);
      ctx->m_curfn=strdup(name);
    }
    if (ctx->m_curfp) fclose(ctx->m_curfp);
    FILE *fp = ctx->m_curfp=fopen(name,"r");
    
    if (!ctx->m_curfp) 
    {
//      printf("error opening %s\n",m_curfn);
      return NULL;
    }
    flockfile(fp);
    
    
    // parse .ini file
    WDL_StringKeyedArray<char *> *cursec=NULL;
    char buf[32768];
    int lcnt=0;
    for (;;)
    {
      buf[0]=0;
      fgets(buf,ctx->m_sections.GetSize()?sizeof(buf):2048,fp); // until we get a secction, read max of 2k (meaning the first section name has to be <2k)
      if (!buf[0] || feof(fp)) break;
      char *p=buf;
      if (lcnt++ == 8 && !ctx->m_sections.GetSize()) break; // dont bother reading more than 8 lines if no section encountered
      while (*p) p++;
      if (p>buf)
      {
        p--;
        while (p >= buf && (*p==' ' || *p == '\r' || *p == '\n' || *p == '\t')) p--;
        p[1]=0;
      }
      p=buf;
      while (*p == ' ' || *p == '\t') p++;
      if (p[0] == '[')
      {
        char *p2=p;
        while (*p2 && *p2 != ']') p2++;
        if (*p2)
        {
          *p2=0;
          if (cursec) cursec->Resort();
          
          if (p[1])
          {
            cursec = ctx->m_sections.Get(p+1);
            if (!cursec)
            {
              cursec = new WDL_StringKeyedArray<char *>(false,WDL_StringKeyedArray<char *>::freecharptr);
              ctx->m_sections.Insert(p+1,cursec);
            }
            else cursec->DeleteAll();
          }
          else cursec=0;
        }
      }
      else if (cursec)
      {
        char *t=strstr(p,"=");
        if (t)
        {
          *t++=0;
          if (*p) 
            cursec->AddUnsorted(p,strdup(t));
        }
      }
    }
    if (cursec) cursec->Resort();
    ctx->m_curfn_time = getfileupdtime(fp);
    funlockfile(fp);    
  }
  return ctx;
}

static void WriteBackFile(iniFileContext *ctx)
{
  if (!ctx||!ctx->m_curfn) return;
  if (ctx->m_curfp) fclose(ctx->m_curfp);
  FILE *fp = ctx->m_curfp=fopen(ctx->m_curfn,"w");
  if (!ctx->m_curfp) return;
  
  flockfile(fp);
  
  int x;
  for (x = 0; ; x ++)
  {
    const char *secname=NULL;
    WDL_StringKeyedArray<char *> * cursec = ctx->m_sections.Enumerate(x,&secname);
    if (!cursec || !secname) break;
    
    fprintf(fp,"[%s]\n",secname);
    int y;
    for (y=0;;y++)
    {
      const char *keyname = NULL;
      const char *keyvalue = cursec->Enumerate(y,&keyname);
      if (!keyvalue || !keyname) break;
      if (*keyname) fprintf(fp,"%s=%s\n",keyname,keyvalue);
    }
    fprintf(fp,"\n");
  }  
  
  fflush(fp);
  ctx->m_curfn_time = getfileupdtime(fp);
  funlockfile(fp);
  
}


BOOL WritePrivateProfileString(const char *appname, const char *keyname, const char *val, const char *fn)
{
  if (!appname || (keyname && !*keyname)) return FALSE;
//  printf("writing %s %s %s %s\n",appname,keyname,val,fn);
  WDL_MutexLock lock(&m_mutex);
  
  iniFileContext *ctx = GetFileContext(fn);
  if (!ctx) return FALSE;
    
  if (!keyname)
  {
    if (ctx->m_sections.Get(appname))
    {
      ctx->m_sections.Delete(appname);
      WriteBackFile(ctx);
    }
  }
  else 
  {
    WDL_StringKeyedArray<char *> * cursec = ctx->m_sections.Get(appname);
    if (!val)
    {
      if (cursec && cursec->Get(keyname))
      {
        cursec->Delete(keyname);
        WriteBackFile(ctx);
      }
    }
    else
    {
      const char *p;
      if (!cursec || !(p=cursec->Get(keyname)) || strcmp(p,val))
      {
        if (!cursec) 
        {
          cursec = new WDL_StringKeyedArray<char *>(false,WDL_StringKeyedArray<char *>::freecharptr);   
          ctx->m_sections.Insert(keyname,cursec);
        }
        cursec->Insert(keyname,strdup(val));
        WriteBackFile(ctx);
      }
    }

  }

  return TRUE;
}

DWORD GetPrivateProfileString(const char *appname, const char *keyname, const char *def, char *ret, int retsize, const char *fn)
{
  WDL_MutexLock lock(&m_mutex);
  
//  printf("getprivateprofilestring: %s\n",fn);
  iniFileContext *ctx= GetFileContext(fn);
  
  if (ctx)
  {
    if (!appname||!keyname)
    {
      WDL_Queue tmpbuf;
      if (!appname)
      {
        int x;
        for (x = 0;; x ++)
        {
          const char *secname=NULL;
          if (!ctx->m_sections.Enumerate(x,&secname) || !secname) break;
          if (*secname) tmpbuf.Add(secname,strlen(secname)+1);
        }
      }
      else
      {
        WDL_StringKeyedArray<char *> *cursec = ctx->m_sections.Get(appname);
        if (cursec)
        {
          int y;
          for (y = 0; ; y ++)
          {            
            const char *keyname=NULL;
            if (!cursec->Enumerate(y,&keyname)||!keyname) break;
            if (*keyname) tmpbuf.Add(keyname,strlen(keyname)+1);
          }
        }
      }
      
      int sz=tmpbuf.GetSize()-1;
      if (sz<0)
      {
        ret[0]=ret[1]=0;
        return 0;
      }
      if (sz > retsize-2) sz=retsize-2;
      memcpy(ret,tmpbuf.Get(),sz);
      ret[sz]=ret[sz+1]=0;
        
      return sz;
    }
    
    WDL_StringKeyedArray<char *> *cursec = ctx->m_sections.Get(appname);
    if (cursec)
    {
      const char *val = cursec->Get(keyname);
      if (val)
      {
        lstrcpyn(ret,val,retsize);
        return strlen(ret);
      }
    }
  }
//  printf("def %s %s %s %s\n",appname,keyname,def,fn);
  lstrcpyn(ret,def?def:"",retsize);
  return strlen(ret);
}

int GetPrivateProfileInt(const char *appname, const char *keyname, int def, const char *fn)
{
  char buf[512];
  GetPrivateProfileString(appname,keyname,"",buf,sizeof(buf),fn);
  if (buf[0])
  {
    int a=atoi(buf);
    if (a||buf[0]=='0') return a;
  }
  return def;
}

static bool __readbyte(char *src, unsigned char *out)
{
  unsigned char cv=0;
  int s=4;
  while(s>=0)
  {
    if (*src >= '0' && *src <= '9') cv += (*src-'0')<<s;
    else if (*src >= 'a' && *src <= 'f') cv += (*src-'a' + 10)<<s;
    else if (*src >= 'A' && *src <= 'F') cv += (*src-'A' + 10)<<s;
    else return false;
    src++;
    s-=4;
  }
  
  *out=cv;
  return true;
}

BOOL GetPrivateProfileStruct(const char *appname, const char *keyname, void *buf, int bufsz, const char *fn)
{
  if (!appname || !keyname) return 0;
  char *tmp=(char *)malloc((bufsz+1)*2+16); 
  if (!tmp) return 0;

  BOOL ret=0;
  GetPrivateProfileString(appname,keyname,"",tmp,(bufsz+1)*2+15,fn);
  if (strlen(tmp) == (bufsz+1)*2)
  {
    unsigned char sum=0;
    unsigned char *bufout=(unsigned char *)buf;
    char *src=tmp;
    unsigned char cv;
    while (bufsz-->0)
    {
      if (!__readbyte(src,&cv)) break;
      *bufout++ = cv;
      sum += cv;
      src+=2;
    }
    ret = bufsz<0 && __readbyte(src,&cv) && cv==sum;
  }
  free(tmp);
  //printf("getprivateprofilestruct returning %d\n",ret);
  return ret;
}

BOOL WritePrivateProfileStruct(const char *appname, const char *keyname, const void *buf, int bufsz, const char *fn)
{
  if (!keyname || !buf) return WritePrivateProfileString(appname,keyname,(const char *)buf,fn);
  char *tmp=(char *)malloc((bufsz+1)*2+1);
  if (!tmp) return 0;
  char *p = tmp;
  unsigned char sum=0;
  unsigned char *src=(unsigned char *)buf;
  while (bufsz-- > 0)
  {
    sprintf(p,"%02X",*src);
    sum+=*src++;
    p+=2;
  }
  sprintf(p,"%02X",sum);

  BOOL ret=WritePrivateProfileString(appname,keyname,tmp,fn);
  free(tmp);
  return ret;
}

#endif
