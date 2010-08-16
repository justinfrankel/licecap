/*
  WDL - convoengine.cpp
  Copyright (C) 2006 and later Cockos Incorporated

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

#ifdef _WIN32
#include <windows.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include "convoengine.h"


#define MAX_SIZE_FOR_BRUTE 64

WDL_ConvolutionEngine::WDL_ConvolutionEngine()
{
  WDL_fft_init();
  m_impulse_nch=1;
  m_fft_size=0;
  m_impulse_len=0;
  m_proc_nch=0;
}

WDL_ConvolutionEngine::~WDL_ConvolutionEngine()
{
}

int WDL_ConvolutionEngine::SetImpulse(WDL_ImpulseBuffer *impulse, int fft_size, int impulse_sample_offset, int max_imp_size)
{
  int impulse_len=0;
  int x;
  for (x = 0; x < impulse->nch; x ++)
  {
    int l=impulse->impulses[x].GetSize()-impulse_sample_offset;
    if (max_imp_size && l>max_imp_size) l=max_imp_size;
    if (impulse_len < l) impulse_len=l;
  }
  m_impulse_nch=impulse->nch;
  m_impulse_len=impulse_len;
  m_proc_nch=-1;


  if (impulse_len<=MAX_SIZE_FOR_BRUTE && fft_size <= 0)
  {
    m_fft_size=0;

    // save impulse
    for (x = 0; x < impulse->nch; x ++)
    {
      WDL_FFT_REAL *imp=impulse->impulses[x].Get()+impulse_sample_offset;
      int lenout=impulse->impulses[x].GetSize()-impulse_sample_offset;  
      if (max_imp_size && lenout>max_imp_size) lenout=max_imp_size;

      WDL_FFT_REAL *impout=m_impulse[x].Resize(lenout)+lenout;
      while (lenout-->0) *--impout = *imp++;
    }

    for (x = 0; x < WDL_CONVO_MAX_PROC_NCH; x ++)
    {
      m_samplesin[x].Clear();
      m_samplesin2[x].Clear();
      m_samplesout[x].Clear();
    }

    return 0;
  }

 
  if (fft_size<=0)
  {
    int msz=fft_size<=-16? -fft_size*2 : 32768;

    fft_size=32;
    while (fft_size < impulse_len*2 && fft_size < msz) fft_size*=2;
  }

  m_fft_size=fft_size;

  int impchunksize=fft_size/2;
  int nblocks=(impulse_len+impchunksize-1)/impchunksize;
  //char buf[512];
  //sprintf(buf,"il=%d, ffts=%d, cs=%d, nb=%d\n",impulse_len,fft_size,impchunksize,nblocks);
  //OutputDebugString(buf);

 
  WDL_FFT_REAL scale=(WDL_FFT_REAL) (1.0/fft_size);
  for (x = 0; x < impulse->nch; x ++)
  {
    WDL_FFT_REAL *imp=impulse->impulses[x].Get()+impulse_sample_offset;
    WDL_FFT_REAL *impout=m_impulse[x].Resize(nblocks*fft_size*2);
    char *zbuf=m_impulse_zflag[x].Resize(nblocks);
    int lenout=impulse->impulses[x].GetSize()-impulse_sample_offset;  
    if (max_imp_size && lenout>max_imp_size) lenout=max_imp_size;
      
    int bl;
    for (bl = 0; bl < nblocks; bl ++)
    {

      int thissz=lenout;
      if (thissz > impchunksize) thissz=impchunksize;

      lenout -= thissz;
      int i=0;    
      WDL_FFT_REAL mv=0.0;

      for (; i < thissz; i ++)
      {
        WDL_FFT_REAL v=*imp++;
        WDL_FFT_REAL v2=(WDL_FFT_REAL)fabs(v);
        if (v2 > mv) mv=v2;

        impout[i*2]=v * scale;
        impout[i*2+1]=0.0;
      }
      for (; i < fft_size; i ++)
      {
        impout[i*2]=0.0;
        impout[i*2+1]=0.0;
      }
      if (mv>1.0e-14)
      {
        *zbuf++=1;
        WDL_fft((WDL_FFT_COMPLEX*)impout,fft_size,0);
      }
      else *zbuf++=0;

      impout+=fft_size*2;
    }
  }
  return m_fft_size/2;
}


void WDL_ConvolutionEngine::Reset() // clears out any latent samples
{
  int x;
  memset(m_hist_pos,0,sizeof(m_hist_pos));
  for (x = 0; x < WDL_CONVO_MAX_PROC_NCH; x ++)
  {
    m_samplesin[x].Clear();
    m_samplesin2[x].Clear();
    m_samplesout[x].Clear();
    memset(m_samplehist[x].Get(),0,m_samplehist[x].GetSize()*sizeof(WDL_FFT_REAL));
    memset(m_overlaphist[x].Get(),0,m_overlaphist[x].GetSize()*sizeof(WDL_FFT_REAL));
  }
}

void WDL_ConvolutionEngine::Add(WDL_FFT_REAL **bufs, int len, int nch)
{
  if (m_fft_size<1)
  {
    int ch;
    m_proc_nch=nch;
    for (ch = 0; ch < nch; ch ++)
    {
      int wch=ch;
      if (wch >=m_impulse_nch) wch-=m_impulse_nch;
      WDL_FFT_REAL *imp=m_impulse[wch].Get();
      int imp_len = m_impulse[wch].GetSize();


      if (imp_len>0) 
      {
        if (m_samplesin2[ch].Available()<imp_len*sizeof(WDL_FFT_REAL)) 
        {
          int sza=imp_len*sizeof(WDL_FFT_REAL)-m_samplesin2[ch].Available();
          memset(m_samplesin2[ch].Add(NULL,sza),0,sza);
        }
        WDL_FFT_REAL *psrc;
        
        if (bufs && bufs[ch])
          psrc=(WDL_FFT_REAL*)m_samplesin2[ch].Add(bufs[ch],len*sizeof(WDL_FFT_REAL));
        else
        {
          psrc=(WDL_FFT_REAL*)m_samplesin2[ch].Add(NULL,len*sizeof(WDL_FFT_REAL));
          memset(psrc,0,len*sizeof(WDL_FFT_REAL));
        }

        WDL_FFT_REAL *pout=(WDL_FFT_REAL*)m_samplesout[ch].Add(NULL,len*sizeof(WDL_FFT_REAL));
        int x;
        for (x=0; x < len; x ++)
        {
          int i=imp_len;
          double sum=0.0,sum2=0.0;
          WDL_FFT_REAL *sp=psrc+x-imp_len + 1;
          WDL_FFT_REAL *ip=imp;
          int j=i/4; i&=3;
          while (j--)
          {
            sum+=ip[0] * sp[0] + 
                 ip[2] * sp[2]; 
            sum2+=ip[1] * sp[1] +
                 ip[3] * sp[3] ;
            ip+=4;
            sp+=4;
          }

          while (i--)
          {
            sum+=*ip++ * *sp++;
          }
          pout[x]=(WDL_FFT_REAL) (sum+sum2);
        }
        m_samplesin2[ch].Advance(len*sizeof(WDL_FFT_REAL));
        m_samplesin2[ch].Compact();
      }
      else
      {
        if (bufs && bufs[ch]) m_samplesout[ch].Add(bufs[ch],len*sizeof(WDL_FFT_REAL));
        else
        {
          memset(m_samplesout[ch].Add(NULL,len*sizeof(WDL_FFT_REAL)),0,len*sizeof(WDL_FFT_REAL));
        }
      }

    }
    return;
  }


  int impchunksize=m_fft_size/2;
  int nblocks=(m_impulse_len+impchunksize-1)/impchunksize;

  if (m_proc_nch != nch)
  {
    m_proc_nch=nch;
    memset(m_hist_pos,0,sizeof(m_hist_pos));
    int x;
    int mso=0;
    for (x = 0; x < WDL_CONVO_MAX_PROC_NCH; x ++)
    {
      int so=m_samplesin[x].Available() + m_samplesout[x].Available();
      if (so>mso) mso=so;

      if (x>=nch)
      {
        m_samplesin[x].Clear();
        m_samplesout[x].Clear();
      }
      else 
      {
        if (m_impulse_len<1||!nblocks) 
        {
          if (m_samplesin[x].Available())
          {
            int s=m_samplesin[x].Available();
            void *buf=m_samplesout[x].Add(NULL,s);
            m_samplesin[x].GetToBuf(0,buf,s);
            m_samplesin[x].Clear();
          }
        }

        if (so < mso)
        {
          memset(m_samplesout[x].Add(NULL,mso-so),0,mso-so);
        }
      }
      
      int sz=0;
      if (x<nch) sz=nblocks*m_fft_size;

      m_samplehist[x].Resize(sz*2);
      m_overlaphist[x].Resize(x<nch ? m_fft_size/2 : 0);
      memset(m_samplehist[x].Get(),0,m_samplehist[x].GetSize()*sizeof(WDL_FFT_REAL));
      memset(m_overlaphist[x].Get(),0,m_overlaphist[x].GetSize()*sizeof(WDL_FFT_REAL));
    }
  }

  int ch;
  if (m_impulse_len<1||!nblocks) 
  {
    for (ch = 0; ch < nch; ch ++)
    {
      if (bufs && bufs[ch])
        m_samplesout[ch].Add(bufs[ch],len*sizeof(WDL_FFT_REAL));
      else
        memset(m_samplesout[ch].Add(NULL,len*sizeof(WDL_FFT_REAL)),0,len*sizeof(WDL_FFT_REAL));
    }
    // pass through
    return;
  }

  for (ch = 0; ch < nch; ch ++)
  {
    if (!m_samplehist[ch].GetSize()||!m_overlaphist[ch].GetSize()) continue;

    m_samplesin[ch].Add(bufs ? bufs[ch] : NULL,len*sizeof(WDL_FFT_REAL));

  }
}

int WDL_ConvolutionEngine::Avail(int want)
{
  if (m_fft_size<1)
  {
    return m_samplesout[0].Available()/sizeof(WDL_FFT_REAL);
  }

  int chunksize=m_fft_size/2;
  int nblocks=(m_impulse_len+chunksize-1)/chunksize;
  // clear combining buffer
  WDL_FFT_REAL *workbuf2 = m_combinebuf.Resize(m_fft_size*2); // temp space

  int ch=0;
  int sz=m_fft_size/2;
  for (ch = 0; ch < m_proc_nch; ch ++)
  {
    if (!m_samplehist[ch].GetSize()||!m_overlaphist[ch].GetSize()) continue;
    int srcc=ch;
    if (srcc>=m_impulse_nch) srcc=m_impulse_nch-1;

    while (m_samplesin[ch].Available()/(int)sizeof(WDL_FFT_REAL) >= sz && m_samplesout[ch].Available() < want*sizeof(WDL_FFT_REAL))
    {
      if (++m_hist_pos[ch] >= nblocks) m_hist_pos[ch]=0;

      // get samples from input, to history
      WDL_FFT_REAL *optr = m_samplehist[ch].Get()+m_hist_pos[ch]*m_fft_size*2;   
      m_samplesin[ch].GetToBuf(0,optr+sz,sz*sizeof(WDL_FFT_REAL));
      m_samplesin[ch].Advance(sz*sizeof(WDL_FFT_REAL));

      int i;
      for (i = 0; i < sz; i ++) // unpack samples
      {
        optr[i*2]=optr[sz+i];
        optr[i*2+1]=0.0;
      }
      memset(optr+sz*2,0,sz*2*sizeof(WDL_FFT_REAL));
      
      WDL_fft((WDL_FFT_COMPLEX*)optr,m_fft_size,0);
     
      int applycnt=0;
      for (i = 0; i < nblocks; i ++)
      {
        int histpos = m_hist_pos[ch]+nblocks-i;
        if (histpos >= nblocks) histpos -= nblocks;

        if (m_impulse_zflag[srcc].GetSize() == nblocks && !m_impulse_zflag[srcc].Get()[i]) continue;

        WDL_FFT_REAL *impulseptr=m_impulse[srcc].Get() + m_fft_size*i*2;
        WDL_FFT_REAL *samplehist=m_samplehist[ch].Get() + m_fft_size*histpos*2;

        if (applycnt++) // add to output
          WDL_fft_complexmul3((WDL_FFT_COMPLEX*)workbuf2,(WDL_FFT_COMPLEX*)samplehist,(WDL_FFT_COMPLEX*)impulseptr,m_fft_size);   
        else // replace output
          WDL_fft_complexmul2((WDL_FFT_COMPLEX*)workbuf2,(WDL_FFT_COMPLEX*)samplehist,(WDL_FFT_COMPLEX*)impulseptr,m_fft_size);  

      }
      if (!applycnt)
        memset(workbuf2,0,m_fft_size*2*sizeof(WDL_FFT_REAL));
      else
        WDL_fft((WDL_FFT_COMPLEX*)workbuf2,m_fft_size,1);

      WDL_FFT_REAL *olhist=m_overlaphist[ch].Get(); // errors from last time

      WDL_FFT_REAL *p1=workbuf2,*p3=workbuf2+m_fft_size,*p1o=workbuf2;
      int s=sz/2;
      while (s--)
      {
        p1o[0] = p1[0]+olhist[0];
        p1o[1] = p1[2]+olhist[1];
        p1o+=2;
        p1+=4;

        olhist[0]=p3[0];
        olhist[1]=p3[2];
        p3+=4;

        olhist+=2;
      }
      // add samples to output
      m_samplesout[ch].Add(workbuf2,sz*sizeof(WDL_FFT_REAL));
    } // while available
  }

  return m_samplesout[0].Available()/sizeof(WDL_FFT_REAL);
}

WDL_FFT_REAL **WDL_ConvolutionEngine::Get() 
{
  int x;
  for (x = 0; x < m_proc_nch; x ++)
  {
    m_get_tmpptrs[x]=(WDL_FFT_REAL *)m_samplesout[x].Get();
  }
  return m_get_tmpptrs;
}

void WDL_ConvolutionEngine::Advance(int len)
{
  int x;
  for (x = 0; x < m_proc_nch; x ++)
  {
    m_samplesout[x].Advance(len*sizeof(WDL_FFT_REAL));
    m_samplesout[x].Compact();
  }
}



/****************************************************************
**  low latency version
*/

WDL_ConvolutionEngine_Div::WDL_ConvolutionEngine_Div()
{
  m_proc_nch=2;
  m_need_feedsilence=true;
}

int WDL_ConvolutionEngine_Div::SetImpulse(WDL_ImpulseBuffer *impulse, int maxfft_size)
{
  m_need_feedsilence=true;

  m_engines.Empty(true);
  if (maxfft_size<0)maxfft_size=-maxfft_size;
  if (!maxfft_size || maxfft_size>65536) maxfft_size=65536;
  int fftsize=MAX_SIZE_FOR_BRUTE;
  int offs=0;
  int implen=impulse->impulses[0].GetSize();
  while (offs < implen)
  {
    WDL_ConvolutionEngine *eng=new WDL_ConvolutionEngine;
    int timplen=fftsize<=MAX_SIZE_FOR_BRUTE?fftsize:(fftsize/2);
    if (fftsize>=maxfft_size) { timplen=0; fftsize=maxfft_size; } // no limit on fft size
    eng->SetImpulse(impulse,fftsize>MAX_SIZE_FOR_BRUTE?fftsize:0,offs,timplen);
    offs+=timplen;
    m_engines.Add(eng);
    if (fftsize>=maxfft_size) break;
    fftsize*=2;
  }
  
  return GetLatency();
}

int WDL_ConvolutionEngine_Div::GetLatency()
{
  return m_engines.GetSize() ?m_engines.Get(0)->GetLatency() : 0;
}


void WDL_ConvolutionEngine_Div::Reset()
{
  int x;
  for (x = 0; x < m_engines.GetSize(); x ++)
  {
    WDL_ConvolutionEngine *eng=m_engines.Get(x);
    eng->Reset();
  }
  for (x = 0; x < WDL_CONVO_MAX_PROC_NCH; x ++)
  {
    m_samplesout[x].Clear();
  }

  m_need_feedsilence=true;
}

WDL_ConvolutionEngine_Div::~WDL_ConvolutionEngine_Div()
{
  m_engines.Empty(true);
}

void WDL_ConvolutionEngine_Div::Add(WDL_FFT_REAL **bufs, int len, int nch)
{
  bool ns=m_need_feedsilence;
  m_need_feedsilence=false;

  m_proc_nch=nch;
  int x;
  for (x = 0; x < m_engines.GetSize(); x ++)
  {
    WDL_ConvolutionEngine *eng=m_engines.Get(x);
    if (ns && x && eng->GetLatency()) eng->Add(NULL,eng->GetLatency(),nch);

    eng->Add(bufs,len,nch);
  }
}
WDL_FFT_REAL **WDL_ConvolutionEngine_Div::Get() 
{
  int x;
  for (x = 0; x < m_proc_nch; x ++)
  {
    m_get_tmpptrs[x]=(WDL_FFT_REAL *)m_samplesout[x].Get();
  }
  return m_get_tmpptrs;
}

void WDL_ConvolutionEngine_Div::Advance(int len)
{
  int x;
  for (x = 0; x < m_proc_nch; x ++)
  {
    m_samplesout[x].Advance(len*sizeof(WDL_FFT_REAL));
    m_samplesout[x].Compact();
  }
}

int WDL_ConvolutionEngine_Div::Avail(int wantSamples)
{
  int wso=wantSamples;
  int x;
  for (x = 0; x < m_engines.GetSize(); x ++)
  {
    WDL_ConvolutionEngine *eng=m_engines.Get(x);
    int a=eng->Avail(wso);
    if (a < wantSamples) wantSamples=a;
  }
  if (wantSamples>0)
  {
    WDL_FFT_REAL *tp[WDL_CONVO_MAX_PROC_NCH];
    for (x =0; x < m_proc_nch; x ++)
    {
      memset(tp[x]=(WDL_FFT_REAL*)m_samplesout[x].Add(NULL,wantSamples*sizeof(WDL_FFT_REAL)),0,wantSamples*sizeof(WDL_FFT_REAL));
    }

    for (x = 0; x < m_engines.GetSize(); x ++)
    {
      WDL_ConvolutionEngine *eng=m_engines.Get(x);
      WDL_FFT_REAL **p=eng->Get();
      if (p)
      {
        int i;
        for (i =0; i < m_proc_nch; i ++)
        {
          WDL_FFT_REAL *o=tp[i];
          WDL_FFT_REAL *in=p[i];
          int j=wantSamples;
          while (j--) *o++ += *in++;
        }
      }
      eng->Advance(wantSamples);
    }
  }

  int av=m_samplesout[0].Available()/sizeof(WDL_FFT_REAL);
  return av>wso ? wso : av;
}


#ifdef WDL_TEST_CONVO

#include <stdio.h>

int main(int argc, char **argv)
{
  if (argc!=5)
  {
    printf("usage: convoengine fftsize implen oneoffs pingoffs\n");
    return -1;
  }

  int fftsize=atoi(argv[1]);
  int implen = atoi(argv[2]);
  int oneoffs = atoi(argv[3]);
  int pingoffs=atoi(argv[4]);

  if (implen < 1 || oneoffs < 0 || oneoffs >= implen || pingoffs < 0)
  {
    printf("invalid parameters\n");
    return -1;
  }

  WDL_ImpulseBuffer imp;
  imp.nch=1;
  memset(imp.impulses[0].Resize(implen),0,implen*sizeof(WDL_FFT_REAL));
  imp.impulses[0].Get()[oneoffs]=1.0;


#if WDL_TEST_CONVO==2
  WDL_ConvolutionEngine_Div engine;
#else
  WDL_ConvolutionEngine engine;
#endif
  engine.SetImpulse(&imp,fftsize);
  WDL_TypedBuf<WDL_FFT_REAL> m_tmpbuf;
  memset(m_tmpbuf.Resize(pingoffs+1),0,pingoffs*sizeof(WDL_FFT_REAL));
  m_tmpbuf.Get()[pingoffs]=1.0;
  WDL_FFT_REAL *p=m_tmpbuf.Get();
  engine.Add(&p,pingoffs+1,1);
  
  p=m_tmpbuf.Resize(4096);
  memset(p,0,m_tmpbuf.GetSize()*sizeof(WDL_FFT_REAL));

  int avail;
  while ((avail=engine.Avail(pingoffs+oneoffs + 8192)) < pingoffs+oneoffs + 8192)
  {
    engine.Add(&p,4096,1);
  }
  WDL_FFT_REAL **output = engine.Get();
  if (!output || !*output)
  {
    printf("cant get output\n");
    return -1;
  }
  int x;
  for (x = 0; x < avail; x ++)
  {
    WDL_FFT_REAL val=output[0][x];
    WDL_FFT_REAL expval = (x==pingoffs+oneoffs) ? 1.0:0.0;
    if (fabs(val-expval)>0.000000001)
    {
      printf("%d: %.4fdB - %f %f\n",x,log10(max(val,0.000000000001))*20.0 - log10(max(expval,0.000000000001))*20.0,val,expval);
    }
  }

  return 0;
}

#endif