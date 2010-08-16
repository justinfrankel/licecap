/*
  WDL - fileread.h
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
  

  This file provides the WDL_FileRead object, which can be used to read files.
  On windows systems it supports reading synchronous, asynchronous, memory mapped, and asynchronous unbuffered.
  On non-windows systems it acts as a wrapper for fopen()/etc.


*/


#ifndef _WDL_FILEREAD_H_
#define _WDL_FILEREAD_H_




#include "ptrlist.h"



#if defined(_WIN32) && !defined(WDL_NO_WIN32_FILEREAD)
  #ifndef WDL_WIN32_NATIVE_READ
    #define WDL_WIN32_NATIVE_READ
  #endif
#else
  #ifdef WDL_WIN32_NATIVE_READ
    #undef WDL_WIN32_NATIVE_READ
  #endif
#endif




#ifdef _MSC_VER
#define WDL_FILEREAD_POSTYPE __int64
#else
#define WDL_FILEREAD_POSTYPE long long
#endif
class WDL_FileRead
{

#ifdef WDL_WIN32_NATIVE_READ


class WDL_FileRead__ReadEnt
{
public:
  WDL_FileRead__ReadEnt(int sz, char *buf)
  {
    m_size=0;
    memset(&m_ol,0,sizeof(m_ol));
    m_ol.hEvent=CreateEvent(NULL,TRUE,TRUE,NULL);
    m_buf=buf;
  }
  ~WDL_FileRead__ReadEnt()
  {
    CloseHandle(m_ol.hEvent);
  }

  OVERLAPPED m_ol;
  DWORD m_size;
  LPVOID m_buf;
};

#endif

public:
  // allow_async=1 for unbuffered async, 2 for buffered async
  WDL_FileRead(const char *filename, int allow_async=1, int bufsize=8192, int nbufs=4, unsigned int mmap_minsize=0, unsigned int mmap_maxsize=0)
  {
#ifdef WDL_WIN32_NATIVE_READ

#define WIN32_UNBUF_ALIGN 8192

    m_sync_bufmode_used=m_sync_bufmode_pos=0;
    m_mmap_fmap=0;
    m_mmap_view=0;
    m_mmap_totalbufmode=0;
    m_fsize=0;
    m_async = (allow_async && GetVersion()<0x80000000) ? allow_async : 0;
    if (bufsize&(WIN32_UNBUF_ALIGN-1)) bufsize=(bufsize&~(WIN32_UNBUF_ALIGN-1))+WIN32_UNBUF_ALIGN; // ensure bufsize is multiple of 4kb

    int flags=FILE_ATTRIBUTE_NORMAL;
    if (m_async)
    {
      flags|=FILE_FLAG_OVERLAPPED;
      if (m_async==1) flags|=FILE_FLAG_NO_BUFFERING;
    }
    else if (nbufs*bufsize>=WIN32_UNBUF_ALIGN && !mmap_maxsize) flags|=FILE_FLAG_NO_BUFFERING; // non-async mode unbuffered if we do our own buffering

    m_fh = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,flags,NULL);

    if (m_fh != INVALID_HANDLE_VALUE)
    {
      DWORD h=0;
      DWORD l=GetFileSize(m_fh,&h);
      m_fsize=(((WDL_FILEREAD_POSTYPE)h)<<32)|l;

      if (!h && l < mmap_maxsize && !m_async)
      {
        if (l >= mmap_minsize)
        {
          m_mmap_fmap=CreateFileMapping(m_fh,NULL,PAGE_READONLY,NULL,0,NULL);
          if (m_mmap_fmap) 
          {
            m_mmap_view=MapViewOfFile(m_mmap_fmap,FILE_MAP_READ,0,0,(int)m_fsize);
            if (!m_mmap_view) 
            {
              CloseHandle(m_mmap_fmap);
              m_mmap_fmap=0;
            }
          }
        }
        else if (l>0)
        {
          m_mmap_totalbufmode = malloc(l);
          DWORD sz;
          ReadFile(m_fh,m_mmap_totalbufmode,l,&sz,NULL);
        }
      }

      if (m_async)
      {
        m_async_bufsize=bufsize;
        int x;
        char *bptr=(char *)m_bufspace.Resize(nbufs*bufsize + (WIN32_UNBUF_ALIGN-1));
        int a=((int)bptr)&(WIN32_UNBUF_ALIGN-1);
        if (a) bptr += WIN32_UNBUF_ALIGN-a;
        for (x = 0; x < nbufs; x ++)
        {
          WDL_FileRead__ReadEnt *t=new WDL_FileRead__ReadEnt(m_async_bufsize,bptr);
          m_empties.Add(t);
          bptr+=m_async_bufsize;
        }
      }
      else if (!m_mmap_view && !m_mmap_totalbufmode && nbufs*bufsize>=WIN32_UNBUF_ALIGN)
      {
        m_bufspace.Resize(nbufs*bufsize+(WIN32_UNBUF_ALIGN-1));
      }
    }

    m_async_readpos=m_file_position=0;
#else
    m_fp=fopen(filename,"rb");
    if(m_fp)
    {
      fseek(m_fp,0,SEEK_END);
      m_lastsize=ftell(m_fp);
      fseek(m_fp,0,SEEK_SET);
    }
    else
      m_lastsize=0;
    m_lastpos=0;
#endif
  }

  ~WDL_FileRead()
  {
#ifdef WDL_WIN32_NATIVE_READ
    // todo, async close stuff?
    int x;
    for (x = 0; x < m_empties.GetSize();x ++) delete m_empties.Get(x);
    m_empties.Empty();
    for (x = 0; x < m_full.GetSize();x ++) delete m_full.Get(x);
    m_full.Empty();
    for (x = 0; x < m_pending.GetSize();x ++) 
    {
      WaitForSingleObject(m_pending.Get(x)->m_ol.hEvent,INFINITE);
      delete m_pending.Get(x);
    }
    m_pending.Empty();

    free(m_mmap_totalbufmode);
    m_mmap_totalbufmode=0;

    if (m_mmap_view) UnmapViewOfFile(m_mmap_view);
    m_mmap_view=0;

    if (m_mmap_fmap) CloseHandle(m_mmap_fmap);
    m_mmap_fmap=0;

    if (m_fh != INVALID_HANDLE_VALUE) CloseHandle(m_fh);
    m_fh=INVALID_HANDLE_VALUE;
#else
    if (m_fp) fclose(m_fp);
    m_fp=0;
#endif

  }

  bool IsOpen()
  {
#ifdef WDL_WIN32_NATIVE_READ
    return (m_fh != INVALID_HANDLE_VALUE);
#else
    return m_fp != NULL;
#endif
  }

#ifdef WDL_WIN32_NATIVE_READ

  int RunReads()
  {
    int retval=0;

    while (m_pending.GetSize())
    {
      WDL_FileRead__ReadEnt *ent=m_pending.Get(0);
      DWORD s=0;

      if (!ent->m_size && !GetOverlappedResult(m_fh,&ent->m_ol,&s,FALSE)) break;
      m_pending.Delete(0);
      if (!ent->m_size) ent->m_size=s;
      m_full.Add(ent);
    }


    int x=m_empties.GetSize();

    if (x>0)
    {
      int cnt=0;
      if (m_async_readpos < m_file_position)  m_async_readpos = m_file_position;

      if (m_async==1) m_async_readpos &= ~((WDL_FILEREAD_POSTYPE) WIN32_UNBUF_ALIGN-1);

      while (x>0)
      {

        if (m_async_readpos >= m_fsize) break;

        WDL_FileRead__ReadEnt *t=m_empties.Get(--x);

        ResetEvent(t->m_ol.hEvent);

        *(WDL_FILEREAD_POSTYPE *)&t->m_ol.Offset = m_async_readpos;

        m_async_readpos += m_async_bufsize;
        DWORD dw;
        if (ReadFile(m_fh,t->m_buf,m_async_bufsize,&dw,&t->m_ol))
        {
          if (!dw) 
          {
            retval++;
            break;
          }

          m_empties.Delete(x);
          t->m_size=dw;
          m_pending.Add(t);
        }
        else
        {
          if (GetLastError() != ERROR_IO_PENDING) 
          {
            retval++;
            break;
          }
          t->m_size=0;
          m_empties.Delete(x);
          m_pending.Add(t);
        }
        //if (cnt++>1)
        break;
      }
    }
    return retval;
  }

  int AsyncRead(char *buf, int maxlen)
  {
    char *obuf=buf;
    int lenout=0;
    if (m_file_position+maxlen > m_fsize)
    {
      maxlen=(int) (m_fsize-m_file_position);
    }
    if (maxlen<1) return 0;

    int errcnt=0;
    do
    {
      while (m_full.GetSize() > 0)
      {
        WDL_FileRead__ReadEnt *ti=m_full.Get(0);
        WDL_FILEREAD_POSTYPE tiofs=*(WDL_FILEREAD_POSTYPE *)&ti->m_ol.Offset;
        if (m_file_position >= tiofs && m_file_position < tiofs + ti->m_size)
        {
          if (maxlen < 1) break;

          int l=ti->m_size-(int) (m_file_position-tiofs);
          if (l > maxlen) l=maxlen;

          memcpy(buf,(char *)ti->m_buf+m_file_position - tiofs,l);
          buf += l;
          m_file_position += l;
          maxlen -= l;
          lenout += l;
        }
        else
        {
          m_empties.Add(ti);
          m_full.Delete(0);
        }  
      }
      
      if (maxlen > 0 && m_async_readpos != m_file_position)
      {
        int x;
        for (x = 0; x < m_pending.GetSize(); x ++)
        {
          WDL_FileRead__ReadEnt *ent=m_pending.Get(x);
          WDL_FILEREAD_POSTYPE tiofs=*(WDL_FILEREAD_POSTYPE *)&ent->m_ol.Offset;
          if (m_file_position >= tiofs && m_file_position < tiofs + m_async_bufsize) break;
        }
        if (x == m_pending.GetSize())
        {
          m_async_readpos=m_file_position;
        }
      }

      errcnt+=RunReads();
      
      if (maxlen > 0 && m_pending.GetSize() && !m_full.GetSize())
      {
        WDL_FileRead__ReadEnt *ent=m_pending.Get(0);
        m_pending.Delete(0);

        if (ent->m_size) m_full.Add(ent);
        else
        {
//          WaitForSingleObject(ent->m_ol.hEvent,INFINITE);

          DWORD s=0;
          if (GetOverlappedResult(m_fh,&ent->m_ol,&s,TRUE))
          {
            ent->m_size=s;
            m_full.Add(ent);
          }
          else
          {
            ent->m_size=0;
            m_empties.Add(ent);
          }
        }
      }
    }
    while (maxlen > 0 && (m_pending.GetSize()||m_full.GetSize()) && !errcnt);
    if (!errcnt) RunReads();

    return lenout;
  }

#endif

  void *GetMappedView(int offs, int *len)
  {
#ifdef WDL_WIN32_NATIVE_READ
    if (!m_mmap_view && !m_mmap_totalbufmode) return 0;
    int maxl=(int) (m_fsize-(WDL_FILEREAD_POSTYPE)offs);
    if (*len > maxl) *len=maxl;
    if (m_mmap_view)
      return (char *)m_mmap_view + offs;
    else
      return (char *)m_mmap_totalbufmode + offs;
      
#else
    return NULL;
#endif
  }

  int Read(void *buf, int len)
  {
#ifdef WDL_WIN32_NATIVE_READ
    if (m_fh == INVALID_HANDLE_VALUE||len<1) return 0;

    if (m_mmap_view||m_mmap_totalbufmode)
    {
      int maxl=(int) (m_fsize-m_file_position);
      if (maxl > len) maxl=len;
      if (maxl < 0) maxl=0;
      if (maxl>0)
      {
        if (m_mmap_view)
          memcpy(buf,(char *)m_mmap_view + (int)m_file_position,maxl);
        else
          memcpy(buf,(char *)m_mmap_totalbufmode + (int)m_file_position,maxl);
          
      }
      m_file_position+=maxl;
      return maxl;     
    }

    if (m_async)
    {
      return AsyncRead((char *)buf,len);
    }
    else
    {
      if (m_bufspace.GetSize()>=WIN32_UNBUF_ALIGN*2-1)
      {
        int rdout=0;
        int sz=m_bufspace.GetSize()-(WIN32_UNBUF_ALIGN-1);
        char *srcbuf=(char *)m_bufspace.Get(); // read size
        if (((int)srcbuf)&(WIN32_UNBUF_ALIGN-1)) srcbuf += WIN32_UNBUF_ALIGN-(((int)srcbuf)&(WIN32_UNBUF_ALIGN-1));
        while (len > rdout)
        {
          int a=m_sync_bufmode_used-m_sync_bufmode_pos;
          if (a>(len-rdout)) a=(len-rdout);
          if (a>0)
          {
            memcpy((char*)buf+rdout,srcbuf+m_sync_bufmode_pos,a);
            rdout+=a;
            m_sync_bufmode_pos+=a;
            m_file_position+=a;
          }

          if (len > rdout)
          {
            DWORD o;
            m_sync_bufmode_used=0;
            m_sync_bufmode_pos=0;
            if (m_file_position&(WIN32_UNBUF_ALIGN-1))
            {
              int offs = (int)(m_file_position&(WIN32_UNBUF_ALIGN-1));
              LONG high=(LONG) ((m_file_position-offs)>>32);
              SetFilePointer(m_fh,(LONG)((m_file_position-offs)&0xFFFFFFFFi64),&high,FILE_BEGIN);
              m_sync_bufmode_pos=offs;
            }
            if (!ReadFile(m_fh,srcbuf,sz,&o,NULL) || o<1 || m_sync_bufmode_pos>=(int)o) 
            {
              break;
            }
            m_sync_bufmode_used=o;
          }

        }
        return rdout;
      }
      else
      {
        DWORD dw=0;
        ReadFile(m_fh,buf,len,&dw,NULL);
        m_file_position+=dw;
        return dw;
      }
    }
#else
    if (!m_fp || len<1) return 0;

    int ret=fread(buf,1,len,m_fp);
    m_lastpos+=ret;
    return ret;
#endif

    
  }

  WDL_FILEREAD_POSTYPE GetSize()
  {
#ifdef WDL_WIN32_NATIVE_READ
    if (m_fh == INVALID_HANDLE_VALUE) return 0;
    return m_fsize;
#else
    if (!m_fp) return -1;
    return m_lastsize;
#endif
  }

  WDL_FILEREAD_POSTYPE GetPosition()
  {
#ifdef WDL_WIN32_NATIVE_READ
    if (m_fh == INVALID_HANDLE_VALUE) return -1;
    return m_file_position;
#else
    if (!m_fp) return -1;
    return m_lastpos;

#endif
  }

  bool SetPosition(WDL_FILEREAD_POSTYPE pos) // returns 0 on success
  {
#ifdef WDL_WIN32_NATIVE_READ
    if (m_fh == INVALID_HANDLE_VALUE) return true;
    if (pos < 0) pos=0;
    if (pos > m_fsize) pos=m_fsize;

    WDL_FILEREAD_POSTYPE oldpos=m_file_position;
    int seeked=0;
    if (m_file_position!=pos)
    {
      m_file_position=pos;
      seeked=1;
    }
    if (m_mmap_view||m_mmap_totalbufmode||!seeked) return false;
    if (m_async)
    {
      WDL_FileRead__ReadEnt *ent;

      if (pos > m_async_readpos || !(ent=m_full.Get(0)) || 
          pos < *(WDL_FILEREAD_POSTYPE *)&ent->m_ol.Offset)
      {
        m_async_readpos=pos;
      }

      return FALSE;
    }

    if (m_bufspace.GetSize()>=WIN32_UNBUF_ALIGN*2-1)
    {
      if (pos >= oldpos-m_sync_bufmode_pos && pos < oldpos-m_sync_bufmode_pos + m_sync_bufmode_used)
      {
        int diff=(int) (pos-oldpos);
        m_sync_bufmode_pos+=diff;

        return 0;
      }
      m_sync_bufmode_pos=m_sync_bufmode_used=0;
    }

    LONG high=(LONG) (m_file_position>>32);
    return SetFilePointer(m_fh,(LONG)(m_file_position&0xFFFFFFFFi64),&high,FILE_BEGIN)==0xFFFFFFFF && GetLastError() != NO_ERROR;
#else
    if (!m_fp) return true;
    if (pos != m_lastpos)   return !!fseek(m_fp,m_lastpos=pos,SEEK_SET);
    return false;
#endif
  }
  


#ifdef WDL_WIN32_NATIVE_READ
  HANDLE GetHandle() { return m_fh; }
  HANDLE m_fh;
  HANDLE m_mmap_fmap;
  LPVOID m_mmap_view;
  LPVOID m_mmap_totalbufmode;
  int m_mmap_size;
  int m_async; // 1=nobuf, 2=buffered async
  WDL_FILEREAD_POSTYPE m_fsize;

  int m_async_bufsize;
  WDL_PtrList<WDL_FileRead__ReadEnt> m_empties;
  WDL_PtrList<WDL_FileRead__ReadEnt> m_pending;
  WDL_PtrList<WDL_FileRead__ReadEnt> m_full;
  WDL_HeapBuf m_bufspace;
  int m_sync_bufmode_used, m_sync_bufmode_pos;

  WDL_FILEREAD_POSTYPE m_file_position,m_async_readpos;
  
#else
  int m_lastsize;
  int m_lastpos;
  FILE *m_fp;
  FILE *GetHandle() { return m_fp; }
#endif
};






#endif
