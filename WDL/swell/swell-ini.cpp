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
#include "../ptrlist.h"
#include "../mutex.h"
#include "../queue.h"
#include <sys/stat.h>
#include <sys/types.h>

class SWELL_ini_section
{
public:
  SWELL_ini_section(const char *name)
  {
    m_name=strdup(name);
  }
  ~SWELL_ini_section()
  { 
    free(m_name);
    m_items.Empty(true,free);
  }
  
  int FindItem(const char *name)
  {
    int ln=strlen(name);
    int x;
    for (x = 0; x < m_items.GetSize(); x ++)
    {
      char *p=m_items.Get(x);
      if (!strnicmp(p,name,ln) && p[ln]=='=') 
      {
        return x;
      }
    }
    return -1;
  }
  
  char *m_name;
  WDL_PtrList<char> m_items;
};


static WDL_PtrList<SWELL_ini_section> m_sections;
static char *m_curfn;
static time_t m_curfn_time;
static FILE *m_curfp;
static WDL_Mutex m_mutex;

static time_t getfileupdtime(FILE *fp)
{
  if (!fp) return 0;
  struct stat st;
  if (fstat(fileno(fp),&st)) return 0;
  return st.st_mtime;
}

// return true on success
static bool SetTheFile(const char *name)
{
  if (!m_curfn || stricmp(m_curfn,name) || !m_curfp || m_curfn_time != getfileupdtime(m_curfp))
  {
    m_sections.Empty(true);
//    printf("reinitting to %s\n",name);
    free(m_curfn);
    m_curfn=strdup(name);
    if (m_curfp) fclose(m_curfp);
    m_curfp=fopen(name,"r");
    
    if (!m_curfp) 
    {
//      printf("error opening %s\n",m_curfn);
      return false;
    }
    flockfile(m_curfp);
    
    
    // parse .ini file
    SWELL_ini_section *cursec=NULL;
    char buf[32768];
    int lcnt=0;
    for (;;)
    {
      buf[0]=0;
      fgets(buf,m_sections.GetSize()?sizeof(buf):2048,m_curfp); // until we get a secction, read max of 2k (meaning the first section name has to be <2k)
      if (!buf[0] || feof(m_curfp)) break;
      char *p=buf;
      if (lcnt++ == 8 && !m_sections.GetSize()) break; // dont bother reading more than 8 lines if no section encountered
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
          if (p[1])
            m_sections.Add(cursec=new SWELL_ini_section(p+1));
          else cursec=0;
        }
      }
      else if (cursec)
      {
        char *t=strstr(p,"=");
        if (t && t > p)
          cursec->m_items.Add(strdup(p));
      }
    }
    m_curfn_time = getfileupdtime(m_curfp);
    funlockfile(m_curfp);    
  }
  return true;
}

static void WriteBackFile()
{
  if (!m_curfn) return;
  if (m_curfp) fclose(m_curfp);
  m_curfp=fopen(m_curfn,"w");
  if (!m_curfp) return;
  flockfile(m_curfp);
  
  int x;
  for (x = 0; x < m_sections.GetSize(); x ++)
  {
    SWELL_ini_section *cursec=m_sections.Get(x);
    fprintf(m_curfp,"[%s]\n",cursec->m_name);
    int y;
    for (y=0;y<cursec->m_items.GetSize();y++)
    {
      fprintf(m_curfp,"%s\n",cursec->m_items.Get(y));
    }
    fprintf(m_curfp,"\n");
  }  
  
  fflush(m_curfp);
  m_curfn_time = getfileupdtime(m_curfp);
  funlockfile(m_curfp);
  
}


BOOL WritePrivateProfileString(const char *appname, const char *keyname, const char *val, const char *fn)
{
  if ((appname && !*appname) || (keyname && !*keyname)) return FALSE;
//  printf("writing %s %s %s %s\n",appname,keyname,val,fn);
  WDL_MutexLock lock(&m_mutex);
  
  SetTheFile(fn);
  
  int x;
  int doWrite=0;
  int hadsec=0;
  for (x = 0; x < m_sections.GetSize(); x ++)
  {
    if (!stricmp(m_sections.Get(x)->m_name,appname))
    {
      hadsec=1;
      if (!keyname)
      {
        m_sections.Delete(x,true);
        doWrite=1;
        break;
      }
      int f=m_sections.Get(x)->FindItem(keyname);
      if (f<0) 
      {
        if (val)
        {
          char *buf=(char *)malloc(strlen(keyname)+strlen(val)+2);
          sprintf(buf,"%s=%s",keyname,val);
          m_sections.Get(x)->m_items.Add(buf);
          doWrite=1;
        }
      }
      else
      {
        if (!val)
        {
          m_sections.Get(x)->m_items.Delete(f,true,free);
          if (!m_sections.Get(x)->m_items.GetSize())
            m_sections.Delete(x,true);
          doWrite=1;
        }
        else
        {
          if (strcmp(m_sections.Get(x)->m_items.Get(f)+strlen(keyname)+1,val))
          {
            free(m_sections.Get(x)->m_items.Get(f));
            char *buf=(char *)malloc(strlen(keyname)+strlen(val)+2);
            sprintf(buf,"%s=%s",keyname,val);
            m_sections.Get(x)->m_items.Set(f,buf);
            doWrite=1;
          }
        }
      }
      break;
    }
  }
  if (!hadsec && appname && val)
  {
    SWELL_ini_section *newsec=new SWELL_ini_section(appname);
    char *buf=(char *)malloc(strlen(keyname)+strlen(val)+2);
    sprintf(buf,"%s=%s",keyname,val);
    newsec->m_items.Add(buf);
    m_sections.Add(newsec);
    doWrite=1;
    
  }
  
  
  if (doWrite) 
  {
//    printf("writing back!\n");
    WriteBackFile();
  }
  return TRUE;
}

DWORD GetPrivateProfileString(const char *appname, const char *keyname, const char *def, char *ret, int retsize, const char *fn)
{
  WDL_MutexLock lock(&m_mutex);
  
//  printf("getprivateprofilestring: %s\n",fn);
  if (SetTheFile(fn))
  {
    if (!appname||!keyname)
    {
      WDL_Queue tmpbuf;
      int x;
      // todo: querying section and item lists, etc.
      if (!appname)
      {
        for (x = 0; x < m_sections.GetSize(); x ++)
          tmpbuf.Add(m_sections.Get(x)->m_name,strlen(m_sections.Get(x)->m_name)+1);
      }
      else
      {
        for (x = 0; x < m_sections.GetSize(); x ++)
        {
          if (!stricmp(m_sections.Get(x)->m_name,appname))
          {
            int y;
            for (y = 0; y < m_sections.Get(x)->m_items.GetSize(); y ++)
            {
              char * p = m_sections.Get(x)->m_items.Get(y);
              char *np=p;
              while (*np && *np != '=') np++;
              if (np>p)
              {
                tmpbuf.Add(p,np-p);
                char c=0;
                tmpbuf.Add(&c,1);
              }
            }
            break;
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
    
    int x;
    for (x = 0; x < m_sections.GetSize(); x ++)
    {
      if (!stricmp(m_sections.Get(x)->m_name,appname))
      {
        int f=m_sections.Get(x)->FindItem(keyname);
        if (f<0) 
        {
  //        printf("got seciton, no keyname\n");
          break;
        }
        // woo, got our result!
        lstrcpyn(ret,m_sections.Get(x)->m_items.Get(f)+strlen(keyname)+1,retsize);
//        printf("got %s %s %s %s\n",appname,keyname,ret,fn);
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
