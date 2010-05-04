#include <stdio.h>
#include <windows.h>

#include "../WDL/zlib/zlib.h"
#include "../WDL/lice/lice.h"
#include "../WDL/filewrite.h"
#include "../WDL/fileread.h"
#include "../WDL/ptrlist.h"
#include "../WDL/queue.h"


class LICECaptureCompressor
{
public:
  LICECaptureCompressor(const char *outfn, int w, int h, int interval=20, int bsize_w=256, int bsize_h=16);

  ~LICECaptureCompressor();

  bool IsOpen() { return !!m_file; }
  void OnFrame(LICE_IBitmap *fr, int delta_t_ms);

  WDL_INT64 GetOutSize() { return m_outsize; }
  WDL_INT64 GetInSize() { return m_inbytes; }


private:
  WDL_FileWrite *m_file;
  WDL_INT64 m_outsize,m_inbytes;
  int m_inframes, m_outframes;

  int m_w,m_h,m_interval,m_bsize_w,m_bsize_h;


  struct frameRec
  {
    frameRec(int sz) { data=(unsigned short *)malloc(sz*sizeof(short)); delta_t_ms=0; }
    ~frameRec() { free(data); }
    unsigned short *data; // shorts
    int delta_t_ms; // time (ms) since last frame
  };
  WDL_PtrList<frameRec> m_framelists[2];
  WDL_Queue m_current_block;
  WDL_Queue m_hdrqueue;

  int m_state, m_which,m_outchunkpos,m_numrows,m_numcols;


  z_stream m_compstream;

  void BitmapToFrameRec(LICE_IBitmap *fr, frameRec *dest);
  void DeflateBlock(void *data, int data_size, bool flush);
  void AddHdrInt(int a) { m_hdrqueue.AddToLE(&a); }


};


LICECaptureCompressor::LICECaptureCompressor(const char *outfn, int w, int h, int interval, int bsize_w, int bsize_h)
{
  m_inframes = m_outframes=0;
  m_file = new WDL_FileWrite(outfn,1,512*1024);
  if (!m_file->IsOpen()) { delete m_file; m_file=0; }

  memset(&m_compstream,0,sizeof(m_compstream));
  if (m_file)
  {
    if (deflateInit(&m_compstream,7)!=Z_OK)
    {
      delete m_file; 
      m_file=0;
    }
  }

  m_inbytes=0;
  m_outsize=0;
  m_w=w;
  m_h=h;
  m_interval=interval;
  m_bsize_w=bsize_w;
  m_bsize_h=bsize_h;
  m_state=0;
  m_which=0;
  m_outchunkpos=0;
  m_numcols = (m_w+bsize_w-1) / (bsize_w>0?bsize_w:1);
  if (m_numcols<1) m_numcols=1;
  m_numrows = (m_h+bsize_h-1)/ (bsize_h>0?bsize_h:1);
}

void LICECaptureCompressor::OnFrame(LICE_IBitmap *fr, int delta_t_ms)
{
  if (fr) 
  {
    if (fr->getWidth()!=m_w || fr->getHeight()!=m_h) return;

    frameRec *rec = m_framelists[m_which].Get(m_state);
    if (!rec)
    {
      rec = new frameRec(m_w*m_h);
      m_framelists[m_which].Add(rec);
    }
    BitmapToFrameRec(fr,rec);
    m_state++;
    m_inframes++;
  }


  bool isLastBlock = m_state >= m_interval || !fr;

  if (m_framelists[!m_which].GetSize())
  {
    int compressTo;
    if (isLastBlock) compressTo = m_numcols*m_numrows;
    else compressTo = (m_state * m_numcols*m_numrows) / m_interval;

    frameRec **list = m_framelists[!m_which].GetList();
    int list_size = m_framelists[!m_which].GetSize();

    // compress some data
    int chunkpos = m_outchunkpos;
    while (chunkpos < compressTo)
    {
      int xpos = (chunkpos%m_numcols) * m_bsize_w;
      int ypos = (chunkpos/m_numcols) * m_bsize_h;

      int wid = m_w-xpos;
      int hei = m_h-ypos;
      if (wid > m_bsize_w) wid=m_bsize_w;
      if (hei > m_bsize_h) hei=m_bsize_h;

      int i;
      int rdoffs = xpos + ypos*m_w;
      int rdspan = m_w;
      for(i=0;i<list_size; i++)
      {
        unsigned short *rd = list[i]->data + rdoffs;
        int a=hei;
        while (a--)
        {
          DeflateBlock(rd,wid*sizeof(short),false);
          rd+=rdspan;
        }
      }

      chunkpos++;
    }
    m_outchunkpos=chunkpos;
  }

  if (isLastBlock)
  {
    if (m_framelists[!m_which].GetSize())
    {
      m_outframes += m_framelists[!m_which].GetSize();
      // todo: write header
      DeflateBlock(NULL,0,true);

      m_hdrqueue.Clear();
      AddHdrInt(16);
      AddHdrInt(m_w);
      AddHdrInt(m_h);
      AddHdrInt(m_bsize_w);
      AddHdrInt(m_bsize_h);
      int nf = m_framelists[!m_which].GetSize();
      AddHdrInt(nf);
      int sz = m_current_block.Available();
      AddHdrInt(sz);
      {
        int x;
        for(x=0;x<nf;x++)
          AddHdrInt(m_framelists[!m_which].Get(x)->delta_t_ms);
      }


      m_file->Write(m_hdrqueue.Get(),m_hdrqueue.Available());
      m_outsize += m_hdrqueue.Available();
      m_file->Write(m_current_block.Get(),sz);
      m_outsize += sz;

      m_current_block.Clear();
    }


    int old_state=m_state;
    m_state=0;
    m_outchunkpos=0;
    m_which=!m_which;


    if (old_state>0 && !fr)
    {
      while (m_framelists[!m_which].GetSize() > old_state)
        m_framelists[!m_which].Delete(m_framelists[!m_which].GetSize()-1,true);

      OnFrame(NULL,0);
    }

    if (!fr)
    {
      m_framelists[0].Empty(true);
      m_framelists[1].Empty(true);
    }
  }
}

void LICECaptureCompressor::BitmapToFrameRec(LICE_IBitmap *fr, frameRec *dest)
{
  unsigned short *outptr = dest->data;
  const LICE_pixel *p = fr->getBits();
  int span = fr->getWidth();
  if (fr->isFlipped())
  {
    p+=(fr->getHeight()-1)*span;
    span=-span;
  }
  int h = fr->getHeight(),w=fr->getWidth();
  while (h--)
  {
    int x=w;
    const LICE_pixel *sp = p;
    while (x--)
    {
      LICE_pixel pix = *sp++;
      *outptr++ = (LICE_GETR(pix)>>3) | ((LICE_GETG(pix)&0xFC)<<3) | ((LICE_GETB(pix)&0xF8)<<11);
    }
    p += span;
  }
}

void LICECaptureCompressor::DeflateBlock(void *data, int data_size, bool flush)
{
  m_inbytes += data_size;
  m_compstream.next_in = (unsigned char *)data;
  m_compstream.avail_in = data_size;
  m_compstream.total_in = 0;

  for (;;)
  {
    int alloc_out = (data_size*9)/8+16384;
    m_compstream.next_out = (unsigned char *)m_current_block.Add(NULL,alloc_out);
    m_compstream.avail_out = alloc_out;
    m_compstream.total_out=0;

    deflate(&m_compstream,flush?Z_FULL_FLUSH:Z_NO_FLUSH);

    m_current_block.Add(NULL, m_compstream.total_out - alloc_out ); // un-add unused data
    if (!m_compstream.total_out) break;
  }
}



LICECaptureCompressor::~LICECaptureCompressor()
{
  // process any pending frames
  if (m_file)
  {
    OnFrame(NULL,0);
    deflateEnd(&m_compstream);
  }

  delete m_file;
  m_framelists[0].Empty(true);
  m_framelists[1].Empty(true);
}


class LICECaptureDecompressor
{
public:
  LICECaptureDecompressor(const char *fn);
  ~LICECaptureDecompressor()
  {
    delete m_file;
  }

  bool IsOpen() { return !!m_file; }

  int GetOffset(); // current frame offset (ms)
  int Seek(int offset_ms); // return -1 on fail (out of range)

  LICE_IBitmap *GetCurrentFrame(); // can return NULL if error


private:
  LICE_MemBitmap m_workbm;

  struct 
  {
    int bpp;
    int w, h;
    int bsize_w, bsize_h;
  } m_curhdr;

  bool ReadHdr()
  {
    m_tmp.Clear();
    int hdr_sz = (4*6);
    if (m_file->Read(m_tmp.Add(NULL,hdr_sz),hdr_sz)!=hdr_sz) return false;
    m_tmp.GetTFromLE(&m_curhdr.bpp);
    m_tmp.GetTFromLE(&m_curhdr.w);
    m_tmp.GetTFromLE(&m_curhdr.h);
    m_tmp.GetTFromLE(&m_curhdr.bsize_w);
    m_tmp.GetTFromLE(&m_curhdr.bsize_h);
    int nf;
    m_tmp.GetTFromLE(&nf);
    int csize;
    m_tmp.GetTFromLE(&csize);
    
    if (nf<1 || nf > 1024) return false;

    if (m_file->Read(m_frame_deltas.Resize(nf),nf*4)!=nf*4) return false;
    int x;
    for(x=0;x<nf;x++)
    {
      WDL_Queue::WDL_Queue__bswap_buffer(m_frame_deltas.Get()+x,4);
    }
    

    return true;
  }

  
  WDL_FileRead *m_file;

  WDL_Queue m_tmp;

  WDL_TypedBuf<int> m_frame_deltas;

  WDL_HeapBuf m_srcdata;
  WDL_TypedBuf<unsigned short> m_decompdata;
};

LICECaptureDecompressor::LICECaptureDecompressor(const char *fn)
{
  memset(&m_curhdr,0,sizeof(m_curhdr));
  m_file = new WDL_FileRead(fn,2,1024*1024);
  if (m_file->IsOpen())
  {
    if (!ReadHdr())
      memset(&m_curhdr,0,sizeof(m_curhdr));
  }


  if (m_curhdr.bpp==16) 
  {
    delete m_file;
    m_file=0;
  }
}

int main()
{
  DWORD st = GetTickCount();
  int n=50;
  int x;
  HWND h = GetDesktopWindow();
  LICE_SysBitmap bm;
  RECT r;
  GetClientRect(h,&r);
  bm.resize(r.right,r.bottom);
  LICECaptureCompressor *tc = new LICECaptureCompressor("c:/temp/foo.dat",r.right,r.bottom);
  if (tc->IsOpen()) for (x=0;x<n;x++)
  {
    HDC hdc = GetDC(h);
    if (hdc)
    {
      BitBlt(bm.getDC(),0,0,r.right,r.bottom,hdc,0,0,SRCCOPY);
      ReleaseDC(h,hdc);
    }
    tc->OnFrame(&bm,1);
    
    while (GetTickCount() < st + 200*x) Sleep(1);
  }
  tc->OnFrame(NULL,0);
  WDL_INT64 outsz=tc->GetOutSize();
  WDL_INT64 intsz = tc->GetInSize();
  delete tc;
  st = GetTickCount()-st;
  printf("%d %dx%d frames in %.1fs, %.1f fps %.1fMB/s (%.1fMB/s -> %.1fMB/s)\n",x,r.right,r.bottom,st/1000.0,x*1000.0/st,
    r.right*r.bottom*4 * (x*1000.0/st) / 1024.0/1024.0,
    intsz /1024.0/1024.0 / (st/1000.0),
    outsz /1024.0/1024.0 / (st/1000.0));
  
  return 0;
}
