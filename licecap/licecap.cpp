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
    if (deflateInit(&m_compstream,9)!=Z_OK)
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

      deflateReset(&m_compstream);

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
  while (flush||data_size>0)
  {
    m_compstream.next_in = (unsigned char *)data;
    m_compstream.avail_in = data_size;
    m_compstream.total_in = 0;

    int av = m_current_block.Available();
    int alloc_out = (data_size*9)/8+16384;
    m_compstream.next_out = (unsigned char *)m_current_block.Add(NULL,alloc_out);
    m_compstream.avail_out = alloc_out;
    m_compstream.total_out=0;

    int e = deflate(&m_compstream,flush?Z_FULL_FLUSH:Z_NO_FLUSH);

//    if (e != Z_OK) printf("error calling deflate (%d)\n",e);

    m_current_block.Add(NULL, m_compstream.total_out - alloc_out ); // un-add unused data
    if (av<4 && m_current_block.Available()>=4)
    {
      printf("first int = %08x\n",*(int*)m_current_block.Get());
    }

    data = (char *)data + m_compstream.total_in;
    data_size -= m_compstream.total_in;

    if (flush && !m_compstream.total_out) break;
    

  }
  if (data_size>0)
    printf("error, unused data!\n");
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
    inflateEnd(&m_compstream);
    delete m_file;
  }

  bool IsOpen() { return !!m_file; }

  //int GetOffset(); // current frame offset (ms)
//  int Seek(int offset_ms); // return -1 on fail (out of range)

  bool NextFrame() // TRUE if out of frames
  {
    if (m_frameidx++ >= m_frame_deltas.GetSize())
    {
      if (!ReadHdr()) return true;
    }
    return false;
  }
  LICE_IBitmap *GetCurrentFrame(); // can return NULL if error


private:
  LICE_MemBitmap m_workbm;

  struct 
  {
    int bpp;
    int w, h;
    int bsize_w, bsize_h;
  } m_curhdr;

  int m_frameidx;

  bool ReadHdr();

  z_stream m_compstream;
  
  WDL_FileRead *m_file;

  WDL_Queue m_tmp;

  WDL_TypedBuf<int> m_frame_deltas;

  WDL_HeapBuf m_srcdata;
  WDL_HeapBuf m_decompdata;
};

LICECaptureDecompressor::LICECaptureDecompressor(const char *fn)
{
  m_frameidx=0;
  memset(&m_compstream,0,sizeof(m_compstream));
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

  if (m_file)
  {
    if (inflateInit(&m_compstream)!=Z_OK)
    {
      delete m_file;
      m_file=0;
    }
  }
}

bool LICECaptureDecompressor::ReadHdr() // todo: eventually make this read/decompress the next header as it goes
{
  m_tmp.Clear();
  int hdr_sz = (4*7);
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

  m_frame_deltas.Resize(nf);

  if (m_frame_deltas.GetSize()!=nf) return false;

  if (m_file->Read(m_frame_deltas.Get(),nf*4)!=nf*4) return false;
  int x;
  for(x=0;x<nf;x++)
  {
    WDL_Queue::WDL_Queue__bswap_buffer(m_frame_deltas.Get()+x,4);
  }
  
  m_srcdata.Resize(csize);
  if (m_srcdata.GetSize()!=csize) return false;
  int rdsamt = m_file->Read(m_srcdata.Get(),csize);
  if (rdsamt !=csize) 
  {
    return false;
  }

  int dsize = m_curhdr.w*m_curhdr.h*(m_curhdr.bpp+7)/8*nf;
  m_decompdata.Resize(dsize,false);
  if (m_decompdata.GetSize()!=dsize) return false;



  int srcpos = 0;
  int destpos=0;

  printf("first int = %08x\n",*(int *)m_srcdata.Get());

  while (destpos < m_decompdata.GetSize())
  {
    m_compstream.next_in = (unsigned char *)m_srcdata.Get() + srcpos;
    m_compstream.avail_in = m_srcdata.GetSize() - srcpos;
    if (m_compstream.avail_in>16384) m_compstream.avail_in=16384;
    m_compstream.total_in = 0;
  
    m_compstream.next_out = (unsigned char *)m_decompdata.Get() + destpos;
    m_compstream.avail_out = m_decompdata.GetSize() - destpos;
    if (m_compstream.avail_out > 65536) m_compstream.avail_out=65536;
    m_compstream.total_out = 0;

    int e = inflate(&m_compstream,0);
    if (e != Z_OK&&e!=Z_STREAM_END) 
    {
      printf("inflate error: %d (%d/%d)\n",e,m_compstream.total_out,m_compstream.avail_out);
      return false;
    }   
    srcpos += m_compstream.total_in;
    destpos += m_compstream.total_out;

    if (!m_compstream.total_in && !m_compstream.total_out)
    {
      printf("Errrrr\n");
      return false;
    }
  }

  inflateReset(&m_compstream);

  m_frameidx=0;

  return true;
}

LICE_IBitmap *LICECaptureDecompressor::GetCurrentFrame()
{
  int nf = m_frame_deltas.GetSize();
  int fidx = m_frameidx;
  printf("req %d of %d\n",fidx,nf);
  if (fidx >=0 && fidx < nf)
  {
    printf("bpp=%d\n",m_curhdr.bpp);
    if (m_curhdr.bpp == 16 && m_decompdata.GetSize() == 2*nf*m_curhdr.w*m_curhdr.h)
    {
      m_workbm.resize(m_curhdr.w,m_curhdr.h);
      //unsigned short *
      // format of m_decompdata is:
      // nf frames of slice1, nf frames of slice2, etc

      LICE_pixel *pout = m_workbm.getBits();
      int span = m_workbm.getRowSpan();

      int ypos,
          toth=m_curhdr.h,
          totw=m_curhdr.w;
      unsigned short *sliceptr = (unsigned short *)m_decompdata.Get();

      for (ypos = 0; ypos < toth; ypos++)
      {
        int hei = toth-ypos;
        if (hei>m_curhdr.bsize_h) hei=m_curhdr.bsize_h;
        int xpos;
        for (xpos=0; xpos<totw; xpos++)
        {
          int wid  = totw-xpos;
          if (wid>m_curhdr.bsize_w) wid=m_curhdr.bsize_w;

          int sz1=wid*hei;

          unsigned short *rdptr = sliceptr + sz1*fidx;
          LICE_pixel *dest = pout + xpos + ypos*span;
          int y;
          for (y=0;y<hei;y++)
          {
            int x=wid;
            LICE_pixel *wr = dest;
            while (x--)
            {
              unsigned short px = *rdptr++;
              *wr++ = LICE_RGBA((px<<3)&0xF8,(px>>3)&0xFC,(px>>8)&0xF8,255);
            }
            dest+=span;
          }
          sliceptr += sz1*nf;

        }
      }


      return &m_workbm;
    }
  }
  return NULL;
}

int main(int argc, char **argv)
{
  if (argc==2)
  {
    LICECaptureDecompressor tc(argv[1]);
    if (tc.IsOpen())
    {
      printf("getting frame\n");
      int x;
      for (x=0;;x++)
      {
        LICE_IBitmap *bm = tc.GetCurrentFrame();
        if (!bm) break;
        tc.NextFrame();
        char buf[512];
        sprintf(buf,"C:/temp/foo_%03d.png",x);
        LICE_WritePNG(buf,bm);
      }
    }

  }
  else
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
  }
  
  return 0;
}
