#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "projectcontext.h"

#include "fileread.h"
#include "filewrite.h"
#include "heapbuf.h"
#include "wdlstring.h"
#include "fastqueue.h"
#include "lineparse.h"


//#define WDL_MEMPROJECTCONTEXT_USE_ZLIB 1

#ifdef WDL_MEMPROJECTCONTEXT_USE_ZLIB

#define WDL_MEMPROJECTCONTEXT_ZLIB_CHUNKSIZE 65536
#include "zlib/zlib.h"

#endif

#include "denormal.h"


char *projectcontext_fastDoubleToString(double value, char *bufOut, int prec_digits)
{
  value = denormal_filter_double2(value);

  if (value<0.0)
  {
    value=-value;
    *bufOut++ = '-';
  }
  if (value > 2147483647.0)
  {
    if (value >= 1.0e40) sprintf(bufOut, "%e", value);
    else sprintf(bufOut, "%.*f", min(prec_digits,8), value);
    while (*bufOut) bufOut++;
    return bufOut;
  }

  unsigned int value_i, frac, frac2;
  int prec_digits2 = 0;

  if (prec_digits>0)
  {
    static const unsigned int scales[9] = 
    { 
      10,
      100,
      1000,
      10000,
      100000,
      1000000,
      10000000,
      100000000,
      1000000000
    };

    value_i = (unsigned int)value;
    const int value_digits = 
       value_i >= 10000 ? (
         value_i >= 1000000 ?
           (value_i >= 100000000 ? 
            (value_i >= 1000000000 ? 10 : 9) : 
             (value_i >= 10000000 ? 8 : 7)) :
            (value_i >= 100000 ? 6 : 5)
       )
       :
       (
         value_i >= 100 ?
           (value_i >= 1000 ? 4 : 3) :
           (value_i >= 10 ? 2 : 1)
       );

           // double precision is limited to about 17 decimal digits of meaningful values
    if (prec_digits + value_digits > 17) prec_digits = 17-value_digits;

    if (prec_digits > 9) 
    {
      prec_digits2 = prec_digits - 9;
      prec_digits = 9;
      if (prec_digits2 > 9) prec_digits2 = 9;
    }

    const unsigned int prec_scale = scales[prec_digits-1];
    const double dfrac = (value - value_i) * prec_scale;
    if (prec_digits2 > 0)
    {
      const unsigned int prec_scale2 = scales[prec_digits2-1];
      frac = (unsigned int) dfrac;

      double dfrac2 = (dfrac - frac) * prec_scale2;
      frac2 = (unsigned int) (dfrac2 + 0.5);

      const int prec_scale2_small = min(prec_scale2/1024,10);

      if (frac2 <= prec_scale2_small) frac2=0;
      else if (frac2 >= prec_scale2 - prec_scale2_small - 1) frac2=prec_scale2;

      if (frac2 >= prec_scale2) 
      {
        frac2 -= prec_scale2;
        frac++;
      }
    }
    else
    {
      frac = (unsigned int) (dfrac + 0.5);
      frac2 = 0;
      const int prec_scale_small = min(prec_scale/1024,10);
      if (frac <= prec_scale_small) frac=0;
      else if (frac>=prec_scale-prec_scale_small - 1) frac=prec_scale;
    }

    if (frac >= prec_scale) // round up to next integer
    {
      frac -= prec_scale;
      value_i++;
    }
  }
  else // round to int
  {
    value_i = (unsigned int)(value+0.5);
    frac2 = frac = 0;
  }

  char digs[32];

  if (value_i)
  {
    int tmp=value_i;
    int x = 0;
    do {
      const int a = (tmp%10);
      tmp/=10;
      digs[x++]='0' + a;
    } while (tmp);

    while (x>0) *bufOut++ = digs[--x];
  }
  else
  {
    *bufOut++ = '0';
  }


  if (frac || frac2)
  {
    int x = 0;
    int tmp=frac;
    int dleft = prec_digits;
    *bufOut++='.';

    if (frac) do
    {   
      const int a = (tmp%10);
      tmp /= 10;
      if (x || a || frac2) digs[x++] = '0'+a;
    } while (dleft-- > 0 && tmp);

    while (dleft-->0) *bufOut++ = '0';
    while (x>0) *bufOut++ = digs[--x];
    // x is 0

    if (frac2)
    {
      tmp=frac2;
      dleft = prec_digits2;
      do
      {   
        const int a = (tmp%10);
        tmp /= 10;
        if (x || a) digs[x++] = '0'+a;
      } while (dleft-- > 0 && tmp);

      while (dleft-->0) *bufOut++ = '0';
      while (x>0) *bufOut++ = digs[--x];
    }
  }

  *bufOut = 0;
  return bufOut;
}

int ProjectContextFormatString(char *outbuf, size_t outbuf_size, const char *fmt, va_list va)
{
  int wroffs=0;

  while (*fmt && outbuf_size > 1)
  {
    char c = *fmt++;
    if (c != '%') 
    {
      outbuf[wroffs++] = c;
      outbuf_size--;
      continue;
    }

    if (*fmt == '%')
    {
      outbuf[wroffs++] = '%';
      outbuf_size--;
      fmt++;
      continue;
    }


    const char *ofmt = fmt-1;
    bool want_abort=false;

    int has_prec=0;
    int prec=0;
    if (*fmt == '.')
    {
      has_prec=1;
      fmt++;
      while (*fmt >= '0' && *fmt <= '9') prec = prec*10 + (*fmt++-'0');
      if (*fmt != 'f' || prec < 0 || prec>20)
      {
        want_abort=true;
      }
    }
    else if (*fmt == '0')
    {
      has_prec=2;
      fmt++;
      while (*fmt >= '0' && *fmt <= '9') prec = prec*10 + (*fmt++-'0');
      if (*fmt != 'x' && *fmt != 'X' && *fmt != 'd' && *fmt != 'u')
      {
        want_abort=true;
      }
    }

    c = *fmt++;
    if (!want_abort) switch (c)
    {
      case '@':
      case 'p':
      case 's':
      {
        const char *str=va_arg(va,const char *);
        const char qc = outbuf_size >= 3 && c != 's' ? getConfigStringQuoteChar(str) : ' ';
        
        if (qc != ' ')
        {
          outbuf[wroffs++] = qc ? qc : '`';
          outbuf_size-=2; // will add trailing quote below
        }
        
        if (str) while (outbuf_size > 1 && *str)
        {
          char v = *str++;
          if (!qc && v == '`') v = '\'';
          outbuf[wroffs++] = v;
          outbuf_size--;
        }

        if (qc != ' ')
        {
          outbuf[wroffs++] = qc ? qc : '`';
          // outbuf_size already decreased above
        }
      }
      break;
      case 'c':
      {
        int v = va_arg(va,int);
        outbuf[wroffs++] = v&0xff;
        outbuf_size--;
      }
      break;
      case 'd':
      {
        int v = va_arg(va,int);
        if (v<0)
        {
          outbuf[wroffs++] = '-';
          outbuf_size--;
          v=-v; // this won't handle -2147483648 right, todo special case?
        }

        char tab[32];
        int x=0;
        do
        {
          tab[x++] = v%10;
          v/=10;
        }
        while (v);
        if (has_prec == 2) while (x<prec) { tab[x++] = 0; }

        while (--x >= 0 && outbuf_size>1)
        {
          outbuf[wroffs++] = '0' + tab[x];
          outbuf_size--;
        }
      }
      break;
      case 'u':
      {
        unsigned int v = va_arg(va,unsigned int);

        char tab[32];
        int x=0;
        do
        {
          tab[x++] = v%10;
          v/=10;
        }
        while (v);
        if (has_prec == 2) while (x<prec) { tab[x++] = 0; }

        while (--x >= 0 && outbuf_size>1)
        {
          outbuf[wroffs++] = '0' + tab[x];
          outbuf_size--;
        }
      }
      break;
      case 'x':
      case 'X':
      {
        const char base = (c - 'x') + 'a';
        unsigned int v = va_arg(va,unsigned int);

        char tab[32];
        int x=0;
        do
        {
          tab[x++] = v&0xf;
          v>>=4;
        }
        while (v);
      
        if (has_prec == 2) while (x<prec) { tab[x++] = 0; }

        while (--x >= 0 && outbuf_size>1)
        {
          if (tab[x] < 10)
            outbuf[wroffs++] = '0' + tab[x];
          else 
            outbuf[wroffs++] = base + tab[x] - 10;

          outbuf_size--;
        }
      }
      break;
      case 'f':
      {
        double v = va_arg(va,double);
        if (outbuf_size<64)
        {
          char tmp[64];
          projectcontext_fastDoubleToString(v,tmp,has_prec?prec:6);
          const char *str = tmp;
          while (outbuf_size > 1 && *str)
          {
            outbuf[wroffs++] = *str++;
            outbuf_size--;
          }
        }
        else
        {
          const char *p=projectcontext_fastDoubleToString(v,outbuf+wroffs,has_prec?prec:6);
          int amt = (int) (p-(outbuf+wroffs));
          wroffs += amt;
          outbuf_size-=amt;
        }
      }
      break;
      default:
        want_abort=true;
      break;
    }   
    if (want_abort)
    {
#if defined(_WIN32) && defined(_DEBUG)
      OutputDebugString("ProjectContextFormatString(): falling back to stock vsnprintf because of:");
      OutputDebugString(ofmt);
#endif
      fmt=ofmt;
      break;
    }
  }

  outbuf += wroffs;
  outbuf[0] = 0;
  if (outbuf_size<2||!*fmt)
    return wroffs;

#if defined(_WIN32) && defined(_MSC_VER)
  const int l = _vsnprintf(outbuf,outbuf_size,fmt,va); // _vsnprintf() does not always null terminate
  if (l < 0 || l >= (int)outbuf_size)
  {
    outbuf[outbuf_size-1] = 0;
    return wroffs + (int)strlen(outbuf);
  }
#else
  // vsnprintf() on non-win32, always null terminates
  const int l = vsnprintf(outbuf,outbuf_size,fmt,va);
  if (l >= (int)outbuf_size-1) return wroffs + (int)outbuf_size-1;
#endif
  return wroffs+l;
}



class ProjectStateContext_Mem : public ProjectStateContext
{
public:

  ProjectStateContext_Mem(WDL_HeapBuf *hb, int rwflags) 
  { 
    m_rwflags=rwflags;
    m_heapbuf=hb; 
    m_pos=0; 
    m_tmpflag=0;
#ifdef WDL_MEMPROJECTCONTEXT_USE_ZLIB
    memset(&m_compstream,0,sizeof(m_compstream));
    m_usecomp=0;
#endif
  }

  virtual ~ProjectStateContext_Mem() 
  { 
    #ifdef WDL_MEMPROJECTCONTEXT_USE_ZLIB
      if (m_usecomp==1)
      {
        FlushComp(true);
        deflateEnd(&m_compstream);
      }
      else if (m_usecomp==2)
      {
        inflateEnd(&m_compstream);
      }
    #endif
  };

  virtual void WDL_VARARG_WARN(printf,2,3) AddLine(const char *fmt, ...);
  virtual int GetLine(char *buf, int buflen); // returns -1 on eof  

  virtual WDL_INT64 GetOutputSize() { return m_heapbuf ? m_heapbuf->GetSize() : 0; }

  virtual int GetTempFlag() { return m_tmpflag; }
  virtual void SetTempFlag(int flag) { m_tmpflag=flag; }
  
  int m_pos;
  WDL_HeapBuf *m_heapbuf;
  int m_tmpflag;
  int m_rwflags;

#ifdef WDL_MEMPROJECTCONTEXT_USE_ZLIB
  int DecompressData()
  {
    if (m_pos >= m_heapbuf->GetSize()) return 1;

    m_compstream.next_in = (unsigned char *)m_heapbuf->Get() + m_pos;
    m_compstream.avail_in = m_heapbuf->GetSize()-m_pos;
    m_compstream.total_in = 0;
    
    int outchunk = 65536;
    m_compstream.next_out = (unsigned char *)m_compdatabuf.Add(NULL,outchunk);
    m_compstream.avail_out = outchunk;
    m_compstream.total_out = 0;

    int e = inflate(&m_compstream,Z_NO_FLUSH);

    m_pos += m_compstream.total_in;
    m_compdatabuf.Add(NULL,m_compstream.total_out - outchunk); // rewind

    return e != Z_OK;
  }

  void FlushComp(bool eof)
  {
    while (m_compdatabuf.Available()>=WDL_MEMPROJECTCONTEXT_ZLIB_CHUNKSIZE || eof)
    {
      if (!m_heapbuf->GetSize()) m_heapbuf->SetGranul(256*1024);
      m_compstream.next_in = (unsigned char *)m_compdatabuf.Get();
      m_compstream.avail_in = m_compdatabuf.Available();
      m_compstream.total_in = 0;
     
      int osz = m_heapbuf->GetSize();

      int newsz=osz + max(m_compstream.avail_in,8192) + 256;
      m_compstream.next_out = (unsigned char *)m_heapbuf->Resize(newsz, false) + osz;
      if (m_heapbuf->GetSize()!=newsz) return; // ERROR
      m_compstream.avail_out = newsz-osz;
      m_compstream.total_out=0;

      deflate(&m_compstream,eof?Z_SYNC_FLUSH:Z_NO_FLUSH);

      m_heapbuf->Resize(osz+m_compstream.total_out,false);
      m_compdatabuf.Advance(m_compstream.total_in);
      if (m_compstream.avail_out) break; // no need to process anymore

    }
    m_compdatabuf.Compact();
  }

  // these will be used for either decompression or compression, fear
  int m_usecomp; // 0=?, -1 = fail, 1=yes
  WDL_Queue m_compdatabuf;
  z_stream m_compstream;
#endif

};

void ProjectStateContext_Mem::AddLine(const char *fmt, ...)
{
  if (!m_heapbuf || !(m_rwflags&2)) return;

  char tmp[8192];

  const char *use_buf;
  int l;

  va_list va;
  va_start(va,fmt);

  if (fmt && fmt[0] == '%' && (fmt[1] == 's' || fmt[1] == 'S') && !fmt[2])
  {
    use_buf = va_arg(va,const char *);
    l=use_buf ? ((int)strlen(use_buf)+1) : 0;
  }
  else
  {
    l = ProjectContextFormatString(tmp,sizeof(tmp),fmt, va) + 1;
    use_buf = tmp;
  }
  va_end(va);

  if (l < 1) return;


#ifdef WDL_MEMPROJECTCONTEXT_USE_ZLIB
  if (!m_usecomp)
  {
    if (deflateInit(&m_compstream,WDL_MEMPROJECTCONTEXT_USE_ZLIB)==Z_OK) m_usecomp=1;
    else m_usecomp=-1;
  }

  if (m_usecomp==1)
  {
    m_compdatabuf.Add(use_buf,(int)l);
    FlushComp(false);
    return;
  }
#endif


  const int sz=m_heapbuf->GetSize();
  if (!sz && m_heapbuf->GetGranul() < 256*1024)
  {
    m_heapbuf->SetGranul(256*1024);
  }

  const int newsz = sz + l;
  char *p = (char *)m_heapbuf->Resize(newsz);
  if (m_heapbuf->GetSize() != newsz)
  {
    // ERROR, resize to 0 and return
    m_heapbuf->Resize(0);
    m_heapbuf=0;
    return; 
  }
  memcpy(p+sz,use_buf,l);
}

int ProjectStateContext_Mem::GetLine(char *buf, int buflen) // returns -1 on eof
{
  if (!m_heapbuf || !(m_rwflags&1)) return -1;

  buf[0]=0;


#ifdef WDL_MEMPROJECTCONTEXT_USE_ZLIB
  if (!m_usecomp)
  {
    unsigned char hdr[]={0x78,0x01};
    if (m_heapbuf->GetSize()>2 && !memcmp(hdr,m_heapbuf->Get(),4) && inflateInit(&m_compstream)==Z_OK) m_usecomp=2;
    else m_usecomp=-1;
  }
  if (m_usecomp==2)
  {
    int x=0;
    for (;;)
    {
      const char *p = (const char*) m_compdatabuf.Get();
      for (x = 0; x < m_compdatabuf.Available() && p[x] && p[x] != '\r' && p[x] != '\n'; x ++);
      while (x >= m_compdatabuf.Available())
      {
        int err = DecompressData();
        p = (const char *)m_compdatabuf.Get();
        for (; x < m_compdatabuf.Available() && p[x] && p[x] != '\r' && p[x] != '\n'; x ++);

        if (err) break;
      }

      if (x||!m_compdatabuf.Available()) break;

      m_compdatabuf.Advance(1); // skip over nul or newline char
    }

    if (!x) return -1;

    if (buflen > 0 && buf)
    {
      int l = min(buflen-1,x);
      memcpy(buf,m_compdatabuf.Get(),l);
      buf[l]=0;
    }

    m_compdatabuf.Advance(x+1);
    m_compdatabuf.Compact();
    return 0;
  }
#endif
  

  int avail = m_heapbuf->GetSize() - m_pos;
  const char *p=(const char *)m_heapbuf->Get() + m_pos;
  while (avail > 0 && (p[0] =='\r'||p[0]=='\n'||!p[0]||p[0] == ' ' || p[0] == '\t'))
  {
    p++;
    m_pos++;
    avail--;
  }
  if (avail <= 0) return -1;
  
  int x;
  for (x = 0; x < avail && p[x] && p[x] != '\r' && p[x] != '\n'; x ++);
  m_pos += x+1;

  if (buflen > 0&&buf)
  {
    int l = buflen-1;
    if (l>x) l=x;
    memcpy(buf,p,l);
    buf[l]=0;
  }
  return 0;
}

class ProjectStateContext_File : public ProjectStateContext
{
public:

  ProjectStateContext_File(WDL_FileRead *rd, WDL_FileWrite *wr) 
  { 
    m_rd=rd; 
    m_wr=wr; 
    m_indent=0; 
    m_bytesOut=0;
    m_errcnt=false; 
    m_tmpflag=0;
    rdbuf_pos = rdbuf_valid = 0;
  }
  virtual ~ProjectStateContext_File(){ delete m_rd; delete m_wr; };

  virtual void WDL_VARARG_WARN(printf,2,3) AddLine(const char *fmt, ...);
  virtual int GetLine(char *buf, int buflen); // returns -1 on eof

  virtual WDL_INT64 GetOutputSize() { return m_bytesOut; }

  virtual int GetTempFlag() { return m_tmpflag; }
  virtual void SetTempFlag(int flag) { m_tmpflag=flag; }    

  bool HasError() { return m_errcnt; }

  WDL_INT64 m_bytesOut WDL_FIXALIGN;

  WDL_FileRead *m_rd;
  WDL_FileWrite *m_wr;

  char rdbuf[4096];
  int rdbuf_pos, rdbuf_valid;

  int m_indent;
  int m_tmpflag;
  bool m_errcnt;

};

int ProjectStateContext_File::GetLine(char *buf, int buflen)
{
  if (!m_rd||buflen<2) return -1;

  int i=0;
  while (i<buflen-1)
  {
    if (rdbuf_pos>=rdbuf_valid)
    {
      rdbuf_pos = 0;
      rdbuf_valid = m_rd->Read(rdbuf, sizeof(rdbuf));
      if (rdbuf_valid<1) break;
    }
    const char c = rdbuf[rdbuf_pos++];

    if (!i)
    {
      if (c != '\r' && c != '\n' && c != ' ' && c != '\t')
        buf[i++] = c;
    }
    else
    {
      if (c == '\r' || c == '\n') break;
      buf[i++] = c;
    }
  }
  buf[i]=0;
  return buf[0] ? 0 : -1;
}

void ProjectStateContext_File::AddLine(const char *fmt, ...)
{
  if (m_wr && !m_errcnt)
  {
    int err=0;

    char tmp[8192];
    const char *use_buf;
    va_list va;
    va_start(va,fmt);
    int l;

    if (fmt && fmt[0] == '%' && (fmt[1] == 's' || fmt[1] == 'S') && !fmt[2])
    {
      // special case "%s" passed, directly use it
      use_buf = va_arg(va,const char *);
      l=use_buf ? (int)strlen(use_buf) : -1;
    }
    else
    {
      l = ProjectContextFormatString(tmp,sizeof(tmp),fmt, va);
      use_buf = tmp;
    }

    va_end(va);
    if (l < 0) return;


    int a=m_indent;
    if (use_buf[0] == '<') m_indent+=2;
    else if (use_buf[0] == '>') a=(m_indent-=2);
    
    if (a>0) 
    {
      m_bytesOut+=a;
      char tb[128];
      memset(tb,' ',a < (int)sizeof(tb) ? a : (int)sizeof(tb));
      while (a>0) 
      {
        const int tl = a < (int)sizeof(tb) ? a : (int)sizeof(tb);
        a-=tl;     
        m_wr->Write(tb,tl);
      }
    }

    err |= m_wr->Write(use_buf,l) != l;
    err |= m_wr->Write("\r\n",2) != 2;
    m_bytesOut += 2 + l;

    if (err) m_errcnt=true;
  }
}



ProjectStateContext *ProjectCreateFileRead(const char *fn)
{
  WDL_FileRead *rd = new WDL_FileRead(fn);
  if (!rd || !rd->IsOpen())
  {
    delete rd;
    return NULL;
  }
  return new ProjectStateContext_File(rd,NULL);
}
ProjectStateContext *ProjectCreateFileWrite(const char *fn)
{
  WDL_FileWrite *wr = new WDL_FileWrite(fn);
  if (!wr || !wr->IsOpen())
  {
    delete wr;
    return NULL;
  }
  return new ProjectStateContext_File(NULL,wr);
}


ProjectStateContext *ProjectCreateMemCtx(WDL_HeapBuf *hb)
{
  return new ProjectStateContext_Mem(hb,3);
}

ProjectStateContext *ProjectCreateMemCtx_Read(const WDL_HeapBuf *hb)
{
  return new ProjectStateContext_Mem((WDL_HeapBuf *)hb,1);
}
ProjectStateContext *ProjectCreateMemCtx_Write(WDL_HeapBuf *hb)
{
  return new ProjectStateContext_Mem(hb,2);
}




class ProjectStateContext_FastQueue : public ProjectStateContext
{
public:

  ProjectStateContext_FastQueue(WDL_FastQueue *fq) 
  { 
    m_fq = fq;
    m_tmpflag=0;
  }

  virtual ~ProjectStateContext_FastQueue() 
  { 
  };

  virtual void WDL_VARARG_WARN(printf,2,3) AddLine(const char *fmt, ...);
  virtual int GetLine(char *buf, int buflen) { return -1; }//unsup

  virtual WDL_INT64 GetOutputSize() { return m_fq ? m_fq->Available() : 0; }

  virtual int GetTempFlag() { return m_tmpflag; }
  virtual void SetTempFlag(int flag) { m_tmpflag=flag; }
  
  WDL_FastQueue *m_fq;
  int m_tmpflag;


};

void ProjectStateContext_FastQueue::AddLine(const char *fmt, ...)
{
  if (!m_fq) return;

  va_list va;
  va_start(va,fmt);

  if (fmt && fmt[0] == '%' && (fmt[1] == 's' || fmt[1] == 'S') && !fmt[2])
  {
    const char *p = va_arg(va,const char *);
    if (p) m_fq->Add(p, (int) strlen(p) + 1);
  }
  else
  {
    char tmp[8192];
    const int l = ProjectContextFormatString(tmp,sizeof(tmp),fmt, va);
    if (l>0) m_fq->Add(tmp, l+1);
  }
  va_end(va);
}







ProjectStateContext *ProjectCreateMemWriteFastQueue(WDL_FastQueue *fq) // only write!
{
  return new ProjectStateContext_FastQueue(fq);
}

bool ProjectContext_GetNextLine(ProjectStateContext *ctx, LineParser *lpOut)
{
  for (;;)
  {
    char linebuf[4096];
    if (ctx->GetLine(linebuf,sizeof(linebuf))) 
    {
      lpOut->parse("");
      return false;
    }

    if (lpOut->parse(linebuf)||lpOut->getnumtokens()<=0) continue;
    
    return true; // success!

  }
}


bool ProjectContext_EatCurrentBlock(ProjectStateContext *ctx)
{
  int child_count=1;
  if (ctx) for (;;)
  {
    char linebuf[4096];
    if (ctx->GetLine(linebuf,sizeof(linebuf))) break;

    bool comment_state=false;
    LineParser lp(comment_state);
    if (lp.parse(linebuf)||lp.getnumtokens()<=0) continue;
    if (lp.gettoken_str(0)[0] == '>')  if (--child_count < 1) return true;
    if (lp.gettoken_str(0)[0] == '<') child_count++;
  }

  return false;
}


static void pc_base64encode(const unsigned char *in, char *out, int len)
{
  char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int shift = 0;
  int accum = 0;

  while (len>0)
  {
    len--;
    accum <<= 8;
    shift += 8;
    accum |= *in++;
    while ( shift >= 6 )
    {
      shift -= 6;
      *out++ = alphabet[(accum >> shift) & 0x3F];
    }
  }
  if (shift == 4)
  {
    *out++ = alphabet[(accum & 0xF)<<2];
    *out++='=';  
  }
  else if (shift == 2)
  {
    *out++ = alphabet[(accum & 0x3)<<4];
    *out++='=';  
    *out++='=';  
  }

  *out++=0;
}

static int pc_base64decode(const char *src, unsigned char *dest, int destsize)
{
  int accum=0, nbits=0, wpos=0;
  while (*src && wpos < destsize)
  {
    int x=0;
    char c=*src++;
    if (c >= 'A' && c <= 'Z') x=c-'A';
    else if (c >= 'a' && c <= 'z') x=c-'a' + 26;
    else if (c >= '0' && c <= '9') x=c-'0' + 52;
    else if (c == '+') x=62;
    else if (c == '/') x=63;
    else break;

    accum = (accum << 6) | x;
    nbits += 6;   

    while (nbits >= 8 && wpos < destsize)
    {
      nbits-=8;
      dest[wpos++] = (char)((accum>>nbits)&0xff);
    }
  }
  return wpos;
}


int cfg_decode_binary(ProjectStateContext *ctx, WDL_HeapBuf *hb) // 0 on success, doesnt clear hb
{
  int child_count=1;
  bool comment_state=false;
  for (;;)
  {
    char linebuf[4096];
    if (ctx->GetLine(linebuf,sizeof(linebuf))) break;

    LineParser lp(comment_state);
    if (lp.parse(linebuf)||lp.getnumtokens()<=0) continue;

    if (lp.gettoken_str(0)[0] == '<') child_count++;
    else if (lp.gettoken_str(0)[0] == '>') { if (child_count-- == 1) return 0; }
    else if (child_count == 1)
    {     
      unsigned char buf[8192];
      int buf_l=pc_base64decode(lp.gettoken_str(0),buf,sizeof(buf));
      int os=hb->GetSize();
      hb->Resize(os+buf_l);
      memcpy((char *)hb->Get()+os,buf,buf_l);
    }
  }
  return -1;  
}

void cfg_encode_binary(ProjectStateContext *ctx, const void *ptr, int len)
{
  const unsigned char *p=(const unsigned char *)ptr;
  while (len>0)
  {
    char buf[256];
    int thiss=len;
    if (thiss > 96) thiss=96;
    pc_base64encode(p,buf,thiss);

    ctx->AddLine("%s",buf);
    p+=thiss;
    len-=thiss;
  }
}


int cfg_decode_textblock(ProjectStateContext *ctx, WDL_String *str) // 0 on success, appends to str
{
  int child_count=1;
  bool comment_state=false;
  for (;;)
  {
    char linebuf[4096];
    if (ctx->GetLine(linebuf,sizeof(linebuf))) break;

    if (!linebuf[0]) continue;
    LineParser lp(comment_state);
    if (!lp.parse(linebuf)&&lp.getnumtokens()>0) 
    {
      if (lp.gettoken_str(0)[0] == '<') { child_count++; continue; }
      else if (lp.gettoken_str(0)[0] == '>') { if (child_count-- == 1) return 0; continue; }
    }
    if (child_count == 1)
    {     
      char *p=linebuf;
      while (*p == ' ' || *p == '\t') p++;
      if (*p == '|')
      {
        if (str->Get()[0]) str->Append("\r\n");
        str->Append(++p);
      }
    }
  }
  return -1;  
}

int cfg_decode_textblock(ProjectStateContext *ctx, WDL_FastString *str) // 0 on success, appends to str
{
  int child_count=1;
  bool comment_state=false;
  for (;;)
  {
    char linebuf[4096];
    if (ctx->GetLine(linebuf,sizeof(linebuf))) break;

    if (!linebuf[0]) continue;
    LineParser lp(comment_state);
    if (!lp.parse(linebuf)&&lp.getnumtokens()>0) 
    {
      if (lp.gettoken_str(0)[0] == '<') { child_count++; continue; }
      else if (lp.gettoken_str(0)[0] == '>') { if (child_count-- == 1) return 0; continue; }
    }
    if (child_count == 1)
    {     
      char *p=linebuf;
      while (*p == ' ' || *p == '\t') p++;
      if (*p == '|')
      {
        if (str->Get()[0]) str->Append("\r\n");
        str->Append(++p);
      }
    }
  }
  return -1;  
}


void cfg_encode_textblock(ProjectStateContext *ctx, const char *text)
{
  WDL_String tmpcopy(text);
  char *txt=(char*)tmpcopy.Get();
  while (*txt)
  {
    char *ntext=txt;
    while (*ntext && *ntext != '\r' && *ntext != '\n') ntext++;
    if (ntext > txt || *ntext)
    {
      char ov=*ntext;
      *ntext=0;
      ctx->AddLine("|%s",txt);
      *ntext=ov;
    }
    txt=ntext;
    if (*txt == '\r')
    {
      if (*++txt== '\n') txt++;
    }
    else if (*txt == '\n')
    {
      if (*++txt == '\r') txt++;
    }
  }
}

char getConfigStringQuoteChar(const char *p)
{
  if (!p || !*p) return '"';

  char fc = *p;
  int flags=0;
  while (*p && flags!=15)
  {
    char c=*p++;
    if (c=='"') flags|=1;
    else if (c=='\'') flags|=2;
    else if (c=='`') flags|=4;
    else if (c == ' ' || c == '\t') flags |= 8;
  }
#ifndef PROJECTCONTEXT_USE_QUOTES_WHEN_NO_SPACES
  if (!(flags & 8) && fc != '"' && fc != '\'' && fc != '`' && fc != '#' && fc != ';') return ' ';
#endif

  if (!(flags & 1)) return '"';
  if (!(flags & 2)) return '\'';
  if (!(flags & 4)) return '`';
  return 0;
}

void makeEscapedConfigString(const char *in, WDL_String *out)
{
  char c;
  if (!in || !*in) out->Set("\"\"");
  else if ((c = getConfigStringQuoteChar(in)))
  {
    if (c == ' ') 
    {
      out->Set(in);
    }
    else
    {
      out->Set(&c,1);
      out->Append(in);
      out->Append(&c,1);
    }
  }
  else  // ick, change ` into '
  {
    out->Set("`");
    out->Append(in);
    out->Append("`");
    char *p=out->Get()+1;
    while (*p && p[1])
    {
      if (*p == '`') *p='\'';
      p++;
    }
  }
}

void makeEscapedConfigString(const char *in, WDL_FastString *out)
{
  char c;
  if (!in || !*in) out->Set("\"\"");
  else if ((c = getConfigStringQuoteChar(in)))
  {
    if (c == ' ') 
    {
      out->Set(in);
    }
    else
    {
      out->Set(&c,1);
      out->Append(in);
      out->Append(&c,1);
    }
  }
  else  // ick, change ` into '
  {
    out->Set("`");
    out->Append(in);
    out->Append("`");
    char *p=(char *)out->Get()+1;
    while (*p && p[1])
    {
      if (*p == '`') *p='\'';
      p++;
    }
  }
}
