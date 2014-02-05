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
    tmp[0]=0;
    #if defined(_WIN32) && defined(_MSC_VER)
      l = _vsnprintf(tmp,sizeof(tmp),fmt,va); // _vsnprintf() does not always null terminate
      if (l < 0 || l >= (int)sizeof(tmp))
      {
        tmp[sizeof(tmp)-1] = 0;
        l = (int)strlen(tmp);
      }
    #else
      // vsnprintf() on non-win32, always null terminates
      l = vsnprintf(tmp,sizeof(tmp),fmt,va);
      if (l > (int)sizeof(tmp)-1) l = (int)sizeof(tmp)-1;
    #endif
    use_buf = tmp;
    l++; // include NULL term
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
  int m_indent;
  int m_tmpflag;
  bool m_errcnt;
};

int ProjectStateContext_File::GetLine(char *buf, int buflen)
{
  if (!m_rd||buflen<2) return -1;

  buf[0]=0;
  for (;;)
  {
    buf[0]=0;
    int i=0;
    while (i<buflen-1)
    {
      if (!m_rd->Read(buf+i,1)) { if (!i) return -1; break; }

      if (buf[i] == '\r' || buf[i] == '\n')  
      {
        if (!i) continue; // skip over leading newlines
        break;
      }

      if (!i && (buf[0] == ' ' || buf[0] == '\t')) continue; // skip leading blanks

      i++;
    }
    if (i<1) continue;

    buf[i]=0;

    if (buf[0]) return 0;
  }
  return -1;
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
      tmp[0]=0;
      #if defined(_WIN32) && defined(_MSC_VER)
        l = _vsnprintf(tmp,sizeof(tmp),fmt,va); // _vsnprintf() does not always null terminate
        if (l < 0 || l >= (int)sizeof(tmp)) 
        {
          tmp[sizeof(tmp)-1] = 0;
          l = (int)strlen(tmp);
        }
     #else
       // vsnprintf() on non-win32, always null terminates
       l = vsnprintf(tmp,sizeof(tmp),fmt,va);
       if (l > (int)sizeof(tmp)-1) l=(int)sizeof(tmp)-1;
     #endif

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
    tmp[0] = 0;
    #if defined(_WIN32) && defined(_MSC_VER)
      int l = _vsnprintf(tmp,sizeof(tmp),fmt,va); // _vsnprintf() does not always null terminate
      if (l < 0 || l >= (int)sizeof(tmp)) 
      {
        tmp[sizeof(tmp)-1]=0;
        l = (int)strlen(tmp);
      }
    #else
      // vsnprintf() on non-win32, always null terminates
      int l = vsnprintf(tmp,sizeof(tmp),fmt,va);
      if (l > (int)sizeof(tmp)-1) l = (int)sizeof(tmp)-1;
    #endif
    if (l>=0) m_fq->Add(tmp, l+1);
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


void makeEscapedConfigString(const char *in, WDL_String *out)
{
  int flags=0;
  const char *p=in;
  while (*p && flags!=7)
  {
    char c=*p++;
    if (c=='"') flags|=1;
    else if (c=='\'') flags|=2;
    else if (c=='`') flags|=4;
  }
  if (flags!=7)
  {
    const char *src=(flags&1)?((flags&2)?"`":"'"):"\"";
    out->Set(src);
    out->Append(in);
    out->Append(src);
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
