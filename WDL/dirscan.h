/*
    WDL - dirscan.h
    Copyright (C) 2005 and later Cockos Incorporated
  
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
      
*/

/*

  This file provides the interface and implementation for WDL_DirScan, a simple 
  (and somewhat portable) directory reading class. On non-Win32 systems it wraps
  opendir()/readdir()/etc. On Win32, it uses FindFirst*, and supports wildcards as 
  well.

 
*/


#ifndef _WDL_DIRSCAN_H_
#define _WDL_DIRSCAN_H_

#include "wdlstring.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

class WDL_DirScan
{
  public:
    WDL_DirScan() : 
#ifdef _WIN32
       m_h(INVALID_HANDLE_VALUE)
  #ifndef WDL_NO_SUPPORT_UTF8
      , m_wcmode(false)
  #endif
#else
       m_h(NULL), m_ent(NULL)
#endif
    {
    }

    ~WDL_DirScan()
    {
      Close();
    }

    int First(const char *dirname
#ifdef _WIN32
                , int isExactSpec=0
#endif
     ) // returns 0 if success
    {
      size_t l=strlen(dirname);
      if (l < 1) return -1;

      WDL_String scanstr(dirname);
#ifdef _WIN32
      if (!isExactSpec) 
#endif
	 if (dirname[l-1] == '\\' || dirname[l-1] == '/')  scanstr.SetLen((int)l-1);
   
   m_leading_path.Set(scanstr.Get());

#ifdef _WIN32
      if (!isExactSpec) scanstr.Append("\\*");
      else
      {
        // remove trailing stuff from m_leading_path
        char *p=m_leading_path.Get();
        while (*p) p++;
        while (p > m_leading_path.Get() && *p != '/' && *p != '\\') p--;
        if (p > m_leading_path.Get()) *p=0;
      }
#else
   if (l && !scanstr.Get()[0])
     scanstr.Set("/"); // fix for scanning /
#endif

      Close();
#ifdef _WIN32
    #ifndef WDL_NO_SUPPORT_UTF8
      m_h=INVALID_HANDLE_VALUE;
      m_wcmode = GetVersion()< 0x80000000;

      if (m_wcmode)
      {
        int reqbuf = MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,scanstr.Get(),-1,NULL,0);
        if (reqbuf > 1000)
        {
          WDL_TypedBuf<WCHAR> tmp;
          tmp.Resize(reqbuf+10);
          if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,scanstr.Get(),-1,tmp.Get(),tmp.GetSize()))
            m_h=FindFirstFileW(tmp.Get(),&m_fd);
        }
        else
        {
          WCHAR wfilename[1024];
          if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,scanstr.Get(),-1,wfilename,1024))
            m_h=FindFirstFileW(wfilename,&m_fd);
        }
      }
      
      if (m_h==INVALID_HANDLE_VALUE) m_wcmode=false;
      
      if (m_h==INVALID_HANDLE_VALUE)
    #endif
        m_h=FindFirstFile(scanstr.Get(),(WIN32_FIND_DATA*)&m_fd);
      return (m_h == INVALID_HANDLE_VALUE);
#else
      m_ent=0;
      m_h=opendir(scanstr.Get());
      return !m_h || Next();
#endif
    }
    int Next() // returns 0 on success
    {
#ifdef _WIN32
      if (m_h == INVALID_HANDLE_VALUE) return -1;
  #ifndef WDL_NO_SUPPORT_UTF8
      if (m_wcmode) return !FindNextFileW(m_h,&m_fd);
  #endif
      return !FindNextFile(m_h,(WIN32_FIND_DATA*)&m_fd);
#else
      if (!m_h) return -1;
      return !(m_ent=readdir(m_h));
#endif
    }
    void Close()
    {
#ifdef _WIN32
      if (m_h != INVALID_HANDLE_VALUE) FindClose(m_h);
      m_h=INVALID_HANDLE_VALUE;
#else
      if (m_h) closedir(m_h);
      m_h=0; m_ent=0;
#endif
    }

#ifdef _WIN32
    char *GetCurrentFN() 
    { 
#ifndef WDL_NO_SUPPORT_UTF8
      if (m_wcmode)
      {
        if (!WideCharToMultiByte(CP_UTF8,0,m_fd.cFileName,-1,m_tmpbuf,sizeof(m_tmpbuf),NULL,NULL))
          m_tmpbuf[0]=0;
        return m_tmpbuf;
      }
#endif
      return ((WIN32_FIND_DATA *)&m_fd)->cFileName; 
    }
#else
    char *GetCurrentFN() const { return m_ent?m_ent->d_name : (char *)""; }
#endif
    void GetCurrentFullFN(WDL_String *str)
    { 
      str->Set(m_leading_path.Get()); 
#ifdef _WIN32
      str->Append("\\"); 
#else
      str->Append("/"); 
#endif
      str->Append(GetCurrentFN()); 
    }
    int GetCurrentIsDirectory() const
    { 
#ifdef _WIN32
       return !!(m_fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY); 
#else
       return m_ent && (m_ent->d_type & DT_DIR);
#endif
    }

    // these are somewhat windows specific calls, eh
#ifdef _WIN32
    DWORD GetCurrentFileSize(DWORD *HighWord=NULL) const { if (HighWord) *HighWord = m_fd.nFileSizeHigh; return m_fd.nFileSizeLow; }
    void GetCurrentLastWriteTime(FILETIME *ft) const { *ft = m_fd.ftLastWriteTime; }
    void GetCurrentLastAccessTime(FILETIME *ft) const { *ft = m_fd.ftLastAccessTime; }
    void GetCurrentCreationTime(FILETIME *ft) const { *ft = m_fd.ftCreationTime; }
#elif defined(_WDL_SWELL_H_)

  // todo: compat for more of these functions
  
  void GetCurrentLastWriteTime(FILETIME *ft)
  { 
    WDL_String tmp;
    GetCurrentFullFN(&tmp);
    struct stat st={0,};
    stat(tmp.Get(),&st);
    unsigned long long a=(unsigned long long)st.st_mtime; // seconds since january 1st, 1970
    a+=((unsigned long long)(60*60*24*(365*4+1)/4))*(unsigned long long)(1970-1601); // this is approximate
    a*=1000*10000; // seconds to 1/10th microseconds (100 nanoseconds)
    ft->dwLowDateTime=a & 0xffffffff;
    ft->dwHighDateTime=a>>32;
  }
  DWORD GetCurrentFileSize(DWORD *HighWord=NULL)
  { 
    WDL_String tmp;
    GetCurrentFullFN(&tmp);
    struct stat st={0,};
    stat(tmp.Get(),&st);
    
    if (HighWord) *HighWord = (DWORD)(st.st_size>>32); 
    return (DWORD)(st.st_size&0xffffffff); 
  }
  
#endif

  private:
#ifdef _WIN32

#ifndef WDL_NO_SUPPORT_UTF8
    bool m_wcmode;
    WIN32_FIND_DATAW m_fd;
    char m_tmpbuf[MAX_PATH*5]; // even if each byte gets encoded as 4 utf-8 bytes this should be plenty ;)
#else
    WIN32_FIND_DATA m_fd;
#endif
    HANDLE m_h;
#else
    DIR *m_h;
    struct dirent *m_ent;
#endif
    WDL_String m_leading_path;
} WDL_FIXALIGN;

#endif
