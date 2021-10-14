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
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "convoengine.h"

#include "denormal.h"

//#define TIMING
#include "timing.c"

#define CONVOENGINE_SILENCE_THRESH 1.0e-12 // -240dB
#define CONVOENGINE_IMPULSE_SILENCE_THRESH 1.0e-15 // -300dB

static void WDL_CONVO_CplxMul2(WDL_FFT_COMPLEX *c, WDL_FFT_COMPLEX *a, WDL_CONVO_IMPULSEBUFCPLXf *b, int n)
{
  WDL_FFT_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  if (n<2 || (n&1)) return;

  do {
    t1 = a[0].re * b[0].re;
    t2 = a[0].im * b[0].im;
    t3 = a[0].im * b[0].re;
    t4 = a[0].re * b[0].im;
    t5 = a[1].re * b[1].re;
    t6 = a[1].im * b[1].im;
    t7 = a[1].im * b[1].re;
    t8 = a[1].re * b[1].im;
    t1 -= t2;
    t3 += t4;
    t5 -= t6;
    t7 += t8;
    c[0].re = t1;
    c[1].re = t5;
    c[0].im = t3;
    c[1].im = t7;
    a += 2;
    b += 2;
    c += 2;
  } while (n -= 2);
}
static void WDL_CONVO_CplxMul3(WDL_FFT_COMPLEX *c, WDL_FFT_COMPLEX *a, WDL_CONVO_IMPULSEBUFCPLXf *b, int n)
{
  WDL_FFT_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  if (n<2 || (n&1)) return;

  do {
    t1 = a[0].re * b[0].re;
    t2 = a[0].im * b[0].im;
    t3 = a[0].im * b[0].re;
    t4 = a[0].re * b[0].im;
    t5 = a[1].re * b[1].re;
    t6 = a[1].im * b[1].im;
    t7 = a[1].im * b[1].re;
    t8 = a[1].re * b[1].im;
    t1 -= t2;
    t3 += t4;
    t5 -= t6;
    t7 += t8;
    c[0].re += t1;
    c[1].re += t5;
    c[0].im += t3;
    c[1].im += t7;
    a += 2;
    b += 2;
    c += 2;
  } while (n -= 2);
}

static bool CompareQueueToBuf(WDL_FastQueue *q, const void *data, int len)
{
  int offs=0;
  while (len>0)
  {
    void *td=NULL;
    int sz=q->GetPtr(offs,&td);
    if (sz<1) return true; // not enough data = not equal!
    if (sz>len) sz=len;
    
    int i=sz/sizeof(WDL_FFT_REAL);
    WDL_FFT_REAL *a1=(WDL_FFT_REAL*)td;
    WDL_FFT_REAL *b1=(WDL_FFT_REAL*)data;
    while (i--) 
    {
      if (fabs(*a1-*b1)>CONVOENGINE_SILENCE_THRESH) return true;
      a1++;
      b1++;
    }

    data = ((char *)data)+sz;
    offs+=sz;
    len-=sz;
  }
  return false;
}


WDL_ConvolutionEngine::WDL_ConvolutionEngine()
{
  WDL_fft_init();
  m_fft_size=0;
  m_impdata.Add(new ImpChannelInfo);
  m_impulse_len=0;
  m_proc_nch=0;
}

WDL_ConvolutionEngine::~WDL_ConvolutionEngine()
{
  m_impdata.Empty(true);
  m_proc.Empty(true);
}

int WDL_ConvolutionEngine::SetImpulse(WDL_ImpulseBuffer *impulse, int fft_size, int impulse_sample_offset, int max_imp_size, bool forceBrute)
{
  int impulse_len=0;
  int x;
  int nch=impulse->GetNumChannels();
  for (x = 0; x < nch; x ++)
  {
    int l=impulse->impulses[x].GetSize()-impulse_sample_offset;
    if (max_imp_size && l>max_imp_size) l=max_imp_size;
    if (impulse_len < l) impulse_len=l;
  }

  if (nch>1) // detect mono signals pretending to be multichannel
  {
    for (x = 1; x < nch; x ++)
    {
      if (impulse->impulses[x].GetSize()!=impulse->impulses[0].GetSize()||
          memcmp(impulse->impulses[x].Get(),impulse->impulses[0].Get(),
            impulse->impulses[0].GetSize()*sizeof(WDL_FFT_REAL)))
            break;
    }
    if (x >= nch) nch=1;
  }

  m_impulse_len=impulse_len;
  m_proc_nch=-1;

  while (m_impdata.GetSize() > nch)
    m_impdata.Delete(m_impdata.GetSize()-1,true);
  while (m_impdata.GetSize() < nch)
    m_impdata.Add(new ImpChannelInfo);

  if (forceBrute)
  {
    m_fft_size=0;

    // save impulse
    for (x = 0; x < m_impdata.GetSize(); x ++)
    {
      WDL_FFT_REAL *imp=impulse->impulses[x].Get()+impulse_sample_offset;
      int lenout=impulse->impulses[x].GetSize()-impulse_sample_offset;  
      if (max_imp_size && lenout>max_imp_size) lenout=max_imp_size;

      WDL_CONVO_IMPULSEBUFf *impout=m_impdata.Get(x)->imp.Resize(lenout)+lenout;
      while (lenout-->0) *--impout = (WDL_CONVO_IMPULSEBUFf) *imp++;
    }

    for (x = 0; x < m_proc.GetSize(); x ++)
    {
      ProcChannelInfo *inf = m_proc.Get(x);
      inf->samplesin.Clear();
      inf->samplesin2.Clear();
      inf->samplesout.Clear();
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

  const bool smallerSizeMode=sizeof(WDL_CONVO_IMPULSEBUFf)!=sizeof(WDL_FFT_REAL);
 
  WDL_FFT_REAL scale=(WDL_FFT_REAL) (1.0/fft_size);
  for (x = 0; x < m_impdata.GetSize(); x ++)
  {
    WDL_FFT_REAL *imp=impulse->impulses[x].Get()+impulse_sample_offset;

    WDL_FFT_REAL *imp2=x < m_impdata.GetSize()-1 ? impulse->impulses[x+1].Get()+impulse_sample_offset : NULL;

    WDL_CONVO_IMPULSEBUFf *impout=m_impdata.Get(x)->imp.Resize((nblocks+!!smallerSizeMode)*fft_size*2);
    char *zbuf=m_impdata.Get(x)->zflag.Resize(nblocks);
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
      WDL_FFT_REAL mv2=0.0;
      WDL_FFT_REAL *imptmp = (WDL_FFT_REAL *)impout; //-V615

      for (; i < thissz; i ++)
      {
        WDL_FFT_REAL v=*imp++;
        WDL_FFT_REAL v2=(WDL_FFT_REAL)fabs(v);
        if (v2 > mv) mv=v2;

        imptmp[i*2]=denormal_filter_aggressive(v * scale);

        if (imp2)
        {
          v=*imp2++;
          v2=(WDL_FFT_REAL)fabs(v);
          if (v2>mv2) mv2=v2;
          imptmp[i*2+1]=denormal_filter_aggressive(v*scale);
        }
        else imptmp[i*2+1]=0.0;
      }
      for (; i < fft_size; i ++)
      {
        imptmp[i*2]=0.0;
        imptmp[i*2+1]=0.0;
      }
      if (mv>CONVOENGINE_IMPULSE_SILENCE_THRESH||mv2>CONVOENGINE_IMPULSE_SILENCE_THRESH)
      {
        *zbuf++=mv>CONVOENGINE_IMPULSE_SILENCE_THRESH ? 2 : 1; // 1 means only second channel has content
        WDL_fft((WDL_FFT_COMPLEX*)impout,fft_size,0);

        if (smallerSizeMode)
        {
          int x,n=fft_size*2;
          for(x=0;x<n;x++) impout[x]=(WDL_CONVO_IMPULSEBUFf)imptmp[x];
        }
      }
      else *zbuf++=0;

      impout+=fft_size*2;
    }
  }
  return m_fft_size/2;
}


void WDL_ConvolutionEngine::Reset() // clears out any latent samples
{
  for (int x = 0; x < m_proc.GetSize(); x ++)
  {
    ProcChannelInfo *inf = m_proc.Get(x);
    inf->samplesin.Clear();
    inf->samplesin2.Clear();
    inf->samplesout.Clear();
    inf->hist_pos = 0;
    memset(inf->samplehist_zflag.Get(),0,inf->samplehist_zflag.GetSize());
    memset(inf->samplehist.Get(),0,inf->samplehist.GetSize()*sizeof(WDL_FFT_REAL));
    memset(inf->overlaphist.Get(),0,inf->overlaphist.GetSize()*sizeof(WDL_FFT_REAL));
  }
}

void WDL_ConvolutionEngine::Add(WDL_FFT_REAL **bufs, int len, int nch)
{
  while (m_proc.GetSize() < nch) m_proc.Add(new ProcChannelInfo);
  while (m_proc.GetSize() > nch) m_proc.Delete(m_proc.GetSize()-1,true);

  if (m_fft_size<1)
  {
    m_proc_nch=nch;

    for (int ch = 0; ch < nch; ch ++)
    {
      int wch = ch % m_impdata.GetSize();
      WDL_CONVO_IMPULSEBUFf *imp=m_impdata.Get(wch)->imp.Get();
      int imp_len = m_impdata.Get(wch)->imp.GetSize();
      ProcChannelInfo *pinf = m_proc.Get(ch);

      if (imp_len>0) 
      {
        if (pinf->samplesin2.Available()<imp_len*(int)sizeof(WDL_FFT_REAL))
        {
          int sza=imp_len*sizeof(WDL_FFT_REAL)-pinf->samplesin2.Available();
          memset(pinf->samplesin2.Add(NULL,sza),0,sza);
        }
        WDL_FFT_REAL *psrc;
        
        if (bufs && bufs[ch])
          psrc=(WDL_FFT_REAL*)pinf->samplesin2.Add(bufs[ch],len*sizeof(WDL_FFT_REAL));
        else
        {
          psrc=(WDL_FFT_REAL*)pinf->samplesin2.Add(NULL,len*sizeof(WDL_FFT_REAL));
          memset(psrc,0,len*sizeof(WDL_FFT_REAL));
        }

        WDL_FFT_REAL *pout=(WDL_FFT_REAL*)pinf->samplesout.Add(NULL,len*sizeof(WDL_FFT_REAL));
        int x;
        int len1 = len&~1;
        for (x=0; x < len1 ; x += 2)
        {
          int i=imp_len;
          double sum=0.0,sum2=0.0;
          WDL_FFT_REAL *sp=psrc+x-imp_len + 1;
          WDL_CONVO_IMPULSEBUFf *ip=imp;
          int j=i/4; i&=3;
          while (j--) // produce 2 samples, 4 impulse samples at a time
          {
            double a = ip[0],b=ip[1],aa=ip[2],bb=ip[3];
            double c = sp[1],d=sp[2],cc=sp[3];
            sum+=a * sp[0] + b * c + aa * d + bb * cc;
            sum2+=a * c + b * d + aa * cc + bb * sp[4];
            ip+=4;
            sp+=4;
          }

          while (i--)
          {
            double a = *ip++;
            sum+=a * sp[0];
            sum2+=a * sp[1];
            sp++;
          }
          pout[x]=(WDL_FFT_REAL) sum;
          pout[x+1]=(WDL_FFT_REAL) sum2;
        }
        for(;x<len;x++) // any odd samples left
        {
          int i=imp_len;
          double sum=0.0;
          WDL_FFT_REAL *sp=psrc+x-imp_len + 1;
          WDL_CONVO_IMPULSEBUFf *ip=imp;
          int j=i/4; i&=3;
          while (j--)
          {
            sum+=ip[0] * sp[0] + ip[1] * sp[1] + ip[2] * sp[2] + ip[3] * sp[3];
            ip+=4;
            sp+=4;
          }

          while (i--) sum+=*ip++ * *sp++;
          pout[x]=(WDL_FFT_REAL) sum;
        }
        pinf->samplesin2.Advance(len*sizeof(WDL_FFT_REAL));
        pinf->samplesin2.Compact();
      }
      else
      {
        if (bufs && bufs[ch]) pinf->samplesout.Add(bufs[ch],len*sizeof(WDL_FFT_REAL));
        else
        {
          memset(pinf->samplesout.Add(NULL,len*sizeof(WDL_FFT_REAL)),0,len*sizeof(WDL_FFT_REAL));
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

    int mso=0;
    for (int ch = 0; ch < nch; ch ++)
    {
      ProcChannelInfo *pinf = m_proc.Get(ch);
      pinf->hist_pos = 0;
      int so=pinf->samplesin.Available() + pinf->samplesout.Available();
      if (so>mso) mso=so;

      if (m_impulse_len<1||!nblocks) 
      {
        if (pinf->samplesin.Available())
        {
          int s=pinf->samplesin.Available();
          void *buf=pinf->samplesout.Add(NULL,s);
          pinf->samplesin.GetToBuf(0,buf,s);
          pinf->samplesin.Clear();
        }
      }

      if (so < mso)
      {
        memset(pinf->samplesout.Add(NULL,mso-so),0,mso-so);
      }
      
      const int sz=nblocks*m_fft_size;

      memset(pinf->samplehist_zflag.Resize(nblocks),0,nblocks);
      pinf->samplehist.Resize(sz*2);
      pinf->overlaphist.Resize(m_fft_size/2);
      memset(pinf->samplehist.Get(),0,pinf->samplehist.GetSize()*sizeof(WDL_FFT_REAL));
      memset(pinf->overlaphist.Get(),0,pinf->overlaphist.GetSize()*sizeof(WDL_FFT_REAL));
    }
  }

  if (m_impulse_len<1||!nblocks) 
  {
    for (int ch = 0; ch < nch; ch ++)
    {
      ProcChannelInfo *pinf = m_proc.Get(ch);
      if (bufs && bufs[ch])
        pinf->samplesout.Add(bufs[ch],len*sizeof(WDL_FFT_REAL));
      else
        memset(pinf->samplesout.Add(NULL,len*sizeof(WDL_FFT_REAL)),0,len*sizeof(WDL_FFT_REAL));
    }
    // pass through
    return;
  }

  for (int ch = 0; ch < nch; ch ++)
  {
    ProcChannelInfo *pinf = m_proc.Get(ch);
    if (!pinf->samplehist.GetSize()||!pinf->overlaphist.GetSize()) continue;
    pinf->samplesin.Add(bufs ? bufs[ch] : NULL,len*sizeof(WDL_FFT_REAL));
  }
}

void WDL_ConvolutionEngine::AddSilenceToOutput(int len)
{  
  for (int ch = 0; ch < m_proc_nch; ch++)
  {
    ProcChannelInfo *pinf = m_proc.Get(ch);
    memset(pinf->samplesout.Add(NULL,len*sizeof(WDL_FFT_REAL)),0,len*sizeof(WDL_FFT_REAL));
  }
}

int WDL_ConvolutionEngine::Avail(int want)
{
  if (m_fft_size<1)
  {
    ProcChannelInfo *pinf = m_proc.Get(0);
    return pinf ? pinf->samplesout.Available()/sizeof(WDL_FFT_REAL) : 0;
  }

  const int sz=m_fft_size/2;
  const int chunksize=m_fft_size/2;
  const int nblocks=(m_impulse_len+chunksize-1)/chunksize;
  // clear combining buffer
  WDL_FFT_REAL *workbuf2 = m_combinebuf.Resize(m_fft_size*4); // temp space

  int ch;

  for (ch = 0; ch < m_proc_nch; ch ++)
  {
    ProcChannelInfo *pinf = m_proc.Get(ch);
    ProcChannelInfo *pinf2 = ch+1 < m_proc_nch ? m_proc.Get(ch+1) : NULL;

    if (!pinf->samplehist.GetSize()||!pinf->overlaphist.GetSize()) continue;
    int srcc=ch % m_impdata.GetSize();

    bool allow_mono_input_mode=true;
    bool mono_impulse_mode=false;

    if (m_impdata.GetSize()==1 && pinf2 &&
        pinf2->samplehist.GetSize()&&pinf2->overlaphist.GetSize() &&
        pinf->samplesin.Available()==pinf2->samplesin.Available() &&
        pinf->samplesout.Available()==pinf2->samplesout.Available()
        )
    { // 2x processing mode
      mono_impulse_mode=true;
      allow_mono_input_mode=false;
    }


    const int in_needed=sz;

    // useSilentList[x] = 1 for mono signal, 2 for stereo, 0 for silent
    char *useSilentList=pinf->samplehist_zflag.GetSize()==nblocks ? pinf->samplehist_zflag.Get() : NULL;
    while (pinf->samplesin.Available()/(int)sizeof(WDL_FFT_REAL) >= sz &&
           pinf->samplesout.Available() < want*(int)sizeof(WDL_FFT_REAL))
    {
      int histpos;
      if ((histpos=++pinf->hist_pos) >= nblocks) histpos=pinf->hist_pos=0;

      // get samples from input, to history
      WDL_FFT_REAL *optr = pinf->samplehist.Get()+histpos*m_fft_size*2;

      pinf->samplesin.GetToBuf(0,optr+sz,in_needed*sizeof(WDL_FFT_REAL));
      pinf->samplesin.Advance(in_needed*sizeof(WDL_FFT_REAL));


      bool mono_input_mode=false;

      bool nonzflag=false;
      if (mono_impulse_mode)
      {
        if (++pinf2->hist_pos >= nblocks) pinf2->hist_pos=0;
        pinf2->samplesin.GetToBuf(0,workbuf2,sz*sizeof(WDL_FFT_REAL));
        pinf2->samplesin.Advance(sz*sizeof(WDL_FFT_REAL));
        int i;
        for (i = 0; i < sz; i ++) // unpack samples
        {
          WDL_FFT_REAL f = optr[i*2]=denormal_filter_aggressive(optr[sz+i]);
          if (!nonzflag && (f<-CONVOENGINE_SILENCE_THRESH || f>CONVOENGINE_SILENCE_THRESH)) nonzflag=true;
          f=optr[i*2+1]=denormal_filter_aggressive(workbuf2[i]);
          if (!nonzflag && (f<-CONVOENGINE_SILENCE_THRESH || f>CONVOENGINE_SILENCE_THRESH)) nonzflag=true;
        }
      }
      else
      {
        if (allow_mono_input_mode && 
          pinf2 &&
          srcc<m_impdata.GetSize()-1 &&
          !CompareQueueToBuf(&pinf2->samplesin,optr+sz,sz*sizeof(WDL_FFT_REAL))
          )
        {
          mono_input_mode=true;
        }
        else 
        {
          allow_mono_input_mode=false;
        }

        int i;
        for (i = 0; i < sz; i ++) // unpack samples
        {
          WDL_FFT_REAL f=optr[i*2]=denormal_filter_aggressive(optr[sz+i]);
          optr[i*2+1]=0.0;
          if (!nonzflag && (f<-CONVOENGINE_SILENCE_THRESH || f>CONVOENGINE_SILENCE_THRESH)) nonzflag=true;
        }
      }

      int i;
      for (i = 1; mono_input_mode && i < nblocks; i ++) // start @ 1, since hist[histpos] is no longer used for here
      {
        int srchistpos = histpos-i;
        if (srchistpos < 0) srchistpos += nblocks;
        if (!useSilentList || useSilentList[srchistpos]==2) mono_input_mode=false;
      }

      if (nonzflag||!useSilentList) memset(optr+sz*2,0,sz*2*sizeof(WDL_FFT_REAL));


#ifdef WDLCONVO_ZL_ACCOUNTING
      m_zl_fftcnt++;
#endif

      if (nonzflag) WDL_fft((WDL_FFT_COMPLEX*)optr,m_fft_size,0);

      if (useSilentList) useSilentList[histpos]=nonzflag ? (mono_input_mode ? 1 : 2) : 0;
    
      int mzfl=2;
      if (mono_input_mode)
      {
        mzfl=1;

        pinf2->samplesin.Advance(sz*sizeof(WDL_FFT_REAL));

        // save a valid copy in sample hist incase we switch from mono to stereo
        if (++pinf2->hist_pos >= nblocks) pinf2->hist_pos=0;
        if (pinf2->samplehist_zflag.GetSize()==nblocks)
          pinf2->samplehist_zflag.Get()[pinf2->hist_pos] = nonzflag ? 1 : 0;

        WDL_FFT_REAL *optr2 = pinf2->samplehist.Get()+pinf2->hist_pos*m_fft_size*2;
        memcpy(optr2,optr,m_fft_size*2*sizeof(WDL_FFT_REAL));
      }

      int applycnt=0;
      char *useImpSilentList=m_impdata.Get(srcc)->zflag.GetSize() == nblocks ? m_impdata.Get(srcc)->zflag.Get() : NULL;

      WDL_CONVO_IMPULSEBUFf *impulseptr=m_impdata.Get(srcc)->imp.Get();
      for (i = 0; i < nblocks; i ++, impulseptr+=m_fft_size*2)
      {
        int srchistpos = histpos-i;
        if (srchistpos < 0) srchistpos += nblocks;

        if (useImpSilentList && useImpSilentList[i]<mzfl) continue;
        if (useSilentList && !useSilentList[srchistpos]) continue; // silent block

        WDL_FFT_REAL *samplehist=pinf->samplehist.Get() + m_fft_size*srchistpos*2;

        if (applycnt++) // add to output
          WDL_CONVO_CplxMul3((WDL_FFT_COMPLEX*)workbuf2,(WDL_FFT_COMPLEX*)samplehist,(WDL_CONVO_IMPULSEBUFCPLXf*)impulseptr,m_fft_size);   
        else // replace output
          WDL_CONVO_CplxMul2((WDL_FFT_COMPLEX*)workbuf2,(WDL_FFT_COMPLEX*)samplehist,(WDL_CONVO_IMPULSEBUFCPLXf*)impulseptr,m_fft_size);  

      }
      if (!applycnt)
        memset(workbuf2,0,m_fft_size*2*sizeof(WDL_FFT_REAL));
      else
        WDL_fft((WDL_FFT_COMPLEX*)workbuf2,m_fft_size,1);

      WDL_FFT_REAL *olhist=pinf->overlaphist.Get(); // errors from last time
      WDL_FFT_REAL *p1=workbuf2,*p3=workbuf2+m_fft_size,*p1o=workbuf2;

      if (mono_impulse_mode||mono_input_mode)
      {
        WDL_FFT_REAL *p2o=workbuf2+m_fft_size*2;
        WDL_FFT_REAL *olhist2=pinf2->overlaphist.Get(); // errors from last time
        int s=sz/2;
        while (s--)
        {
          p2o[0] = p1[1]+olhist2[0];
          p2o[1] = p1[3]+olhist2[1];
          p1o[0] = p1[0]+olhist[0];
          p1o[1] = p1[2]+olhist[1];
          p1o+=2;
          p2o+=2;
          p1+=4;

          olhist[0]=p3[0];
          olhist[1]=p3[2];
          olhist2[0]=p3[1];
          olhist2[1]=p3[3];
          p3+=4;

          olhist+=2;
          olhist2+=2;
        }
        // add samples to output
        pinf->samplesout.Add(workbuf2,sz*sizeof(WDL_FFT_REAL));
        pinf2->samplesout.Add(workbuf2+m_fft_size*2,sz*sizeof(WDL_FFT_REAL));
      }
      else
      {
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
        pinf->samplesout.Add(workbuf2,sz*sizeof(WDL_FFT_REAL));
      }
    } // while available

    if (mono_impulse_mode) ch++;
  }

  int mv = want;
  for (ch=0;ch<m_proc_nch;ch++)
  {
    const ProcChannelInfo *pinf = m_proc.Get(ch);
    int v = pinf ? pinf->samplesout.Available()/sizeof(WDL_FFT_REAL) : 0;
    if (!ch || v<mv)mv=v;
  }
  return mv;
}

WDL_FFT_REAL **WDL_ConvolutionEngine::Get() 
{
  WDL_FFT_REAL **ret = m_get_tmpptrs.ResizeOK(m_proc_nch,false);
  if (WDL_NORMALLY(ret))
    for (int x = 0; x < m_proc_nch; x ++) ret[x]=(WDL_FFT_REAL *)m_proc.Get(x)->samplesout.Get();
  return ret;
}

void WDL_ConvolutionEngine::Advance(int len)
{
  for (int ch = 0; ch < m_proc_nch; ch ++)
  {
    ProcChannelInfo *pinf = m_proc.Get(ch);
    pinf->samplesout.Advance(len*sizeof(WDL_FFT_REAL));
    pinf->samplesout.Compact();
  }
}



/****************************************************************
**  low latency version
*/

WDL_ConvolutionEngine_Div::WDL_ConvolutionEngine_Div()
{
  timingInit();
  for (int x = 0; x < 2; x ++) m_sout.Add(new WDL_Queue);
  m_need_feedsilence=true;
}

int WDL_ConvolutionEngine_Div::SetImpulse(WDL_ImpulseBuffer *impulse, int maxfft_size, int known_blocksize, int max_imp_size, int impulse_offset, int latency_allowed)
{
  m_need_feedsilence=true;

  m_engines.Empty(true);
  if (maxfft_size<0)maxfft_size=-maxfft_size;
  maxfft_size*=2;
  if (!maxfft_size || maxfft_size>32768) maxfft_size=32768;


  const int MAX_SIZE_FOR_BRUTE=64;

  int fftsize = MAX_SIZE_FOR_BRUTE;
  int impulsechunksize = MAX_SIZE_FOR_BRUTE;

  if (known_blocksize && !(known_blocksize&(known_blocksize-1)) && known_blocksize>MAX_SIZE_FOR_BRUTE*2)
  {
    fftsize=known_blocksize/2;
    impulsechunksize=known_blocksize/2;
  }
  if (latency_allowed*2 > fftsize)
  {
    int x = 16;
    while (x <= latency_allowed) x*=2;
    if (x>32768) x=32768;
    fftsize=impulsechunksize=x;
  }

  int offs=0;
  int samplesleft=impulse->impulses[0].GetSize()-impulse_offset;
  if (max_imp_size>0 && samplesleft>max_imp_size) samplesleft=max_imp_size;

  do
  {
    WDL_ConvolutionEngine *eng=new WDL_ConvolutionEngine;

    bool wantBrute = !latency_allowed && !offs;
    if (impulsechunksize*(wantBrute ? 2 : 3) >= samplesleft) impulsechunksize=samplesleft; // early-out, no point going to a larger FFT (since if we did this, we wouldnt have enough samples for a complete next pass)
    if (fftsize>=maxfft_size) { impulsechunksize=samplesleft; fftsize=maxfft_size; } // if FFTs are as large as possible, finish up

    eng->SetImpulse(impulse,fftsize,offs+impulse_offset,impulsechunksize, wantBrute);
    eng->m_zl_delaypos = offs;
    eng->m_zl_dumpage=0;
    m_engines.Add(eng);

#ifdef WDLCONVO_ZL_ACCOUNTING
    char buf[512];
    wsprintf(buf,"ce%d: offs=%d, len=%d, fftsize=%d\n",m_engines.GetSize(),offs,impulsechunksize,fftsize);
    OutputDebugString(buf);
#endif

    samplesleft -= impulsechunksize;
    offs+=impulsechunksize;

#if 1 // this seems about 10% faster (maybe due to better cache use from less sized ffts used?)
    impulsechunksize=offs*3;
    fftsize=offs*2;
#else
    impulsechunksize=fftsize;

    fftsize*=2;
#endif
  }
  while (samplesleft > 0);
  
  return GetLatency();
}

int WDL_ConvolutionEngine_Div::GetLatency()
{
  return m_engines.GetSize() ? m_engines.Get(0)->GetLatency() : 0;
}


void WDL_ConvolutionEngine_Div::Reset()
{
  int x;
  for (x = 0; x < m_engines.GetSize(); x ++)
  {
    WDL_ConvolutionEngine *eng=m_engines.Get(x);
    eng->Reset();
  }
  for (x = 0; x < m_sout.GetSize(); x ++)
  {
    m_sout.Get(x)->Clear();
  }

  m_need_feedsilence=true;
}

WDL_ConvolutionEngine_Div::~WDL_ConvolutionEngine_Div()
{
  timingPrint();
  m_engines.Empty(true);
  m_sout.Empty(true);
}

void WDL_ConvolutionEngine_Div::Add(WDL_FFT_REAL **bufs, int len, int nch)
{
  while (m_sout.GetSize() < nch)
    m_sout.Add(new WDL_Queue);
  while (m_sout.GetSize() > nch)
    m_sout.Delete(m_sout.GetSize()-1,true);

  bool ns=m_need_feedsilence;
  m_need_feedsilence=false;

  int x;
  for (x = 0; x < m_engines.GetSize(); x ++)
  {
    WDL_ConvolutionEngine *eng=m_engines.Get(x);
    if (ns)
    {
      eng->m_zl_dumpage = (x>0 && x < m_engines.GetSize()-1) ? (eng->GetLatency()/4) : 0; // reduce max number of ffts per block by staggering them

      if (eng->m_zl_dumpage>0)
        eng->Add(NULL,eng->m_zl_dumpage,nch); // added silence to input (to control when fft happens)
    }

    eng->Add(bufs,len,nch);

    if (ns) eng->AddSilenceToOutput(eng->m_zl_delaypos); // add silence to output (to delay output to its correct time)

  }
}
WDL_FFT_REAL **WDL_ConvolutionEngine_Div::Get() 
{
  WDL_FFT_REAL **ret = m_get_tmpptrs.ResizeOK(m_sout.GetSize(),false);
  if (WDL_NORMALLY(ret))
    for (int x = 0; x < m_sout.GetSize(); x ++) ret[x]=(WDL_FFT_REAL *)m_sout.Get(x)->Get();
  return ret;
}

void WDL_ConvolutionEngine_Div::Advance(int len)
{
  int x;
  for (x = 0; x < m_sout.GetSize(); x ++)
  {
    WDL_Queue *q = m_sout.Get(x);
    q->Advance(len*sizeof(WDL_FFT_REAL));
    q->Compact();
  }
}

int WDL_ConvolutionEngine_Div::Avail(int wantSamples)
{
  timingEnter(1);
  int wso=wantSamples;
  int x;
#ifdef WDLCONVO_ZL_ACCOUNTING
  int cnt=0;
  static int maxcnt=-1;
  int h=0;
#endif
  for (x = 0; x < m_engines.GetSize(); x ++)
  {
    WDL_ConvolutionEngine *eng=m_engines.Get(x);
#ifdef WDLCONVO_ZL_ACCOUNTING
    eng->m_zl_fftcnt=0;
#endif
    int a=eng->Avail(wso+eng->m_zl_dumpage) - eng->m_zl_dumpage;
#ifdef WDLCONVO_ZL_ACCOUNTING
    cnt += !!eng->m_zl_fftcnt;

#if 0
    if (eng->m_zl_fftcnt)
      h|=1<<x;
    
    if (eng->m_zl_fftcnt && x==m_engines.GetSize()-1 && cnt>1)
    {
      char buf[512];
      wsprintf(buf,"fft flags=%08x (%08x=max)\n",h,1<<x);
      OutputDebugString(buf);
    }
#endif
#endif
    if (a < wantSamples) wantSamples=a;
  }

#ifdef WDLCONVO_ZL_ACCOUNTING
  static DWORD lastt=0;
  if (cnt>maxcnt)maxcnt=cnt;
  if (GetTickCount()>lastt+1000)
  {
    lastt=GetTickCount();
    char buf[512];
    wsprintf(buf,"maxcnt=%d\n",maxcnt);
    OutputDebugString(buf);
    maxcnt=-1;
  }
#endif
  if (wantSamples>0)
  {
    const int add_sz = wantSamples*sizeof(WDL_FFT_REAL);
    for (x =0; x < m_sout.GetSize(); x ++)
    {
      void *add = m_sout.Get(x)->Add(NULL,add_sz);
      if (WDL_NORMALLY(add != NULL)) memset(add,0,add_sz);
    }

    for (x = 0; x < m_engines.GetSize(); x ++)
    {
      WDL_ConvolutionEngine *eng=m_engines.Get(x);
      if (eng->m_zl_dumpage>0) { eng->Advance(eng->m_zl_dumpage); eng->m_zl_dumpage=0; }

      WDL_FFT_REAL **p=eng->Get();
      if (p)
      {
        int i;
        for (i =0; i < m_sout.GetSize(); i ++)
        {
          WDL_Queue *q = m_sout.Get(i);
          const int qsz = q->Available();
          if (WDL_NORMALLY(qsz >= add_sz))
          {
            WDL_FFT_REAL *o=(WDL_FFT_REAL *)((char *)q->Get() + qsz - add_sz);
            const WDL_FFT_REAL *in=p[i];
            int j=wantSamples;
            while (j-->0) *o++ += *in++;
          }
        }
      }
      eng->Advance(wantSamples);
    }
  }
  timingLeave(1);

  WDL_Queue *q0 = m_sout.Get(0);
  int av=WDL_NORMALLY(q0 != NULL) ? (int) (q0->Available()/sizeof(WDL_FFT_REAL)) : 0;
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


int WDL_ImpulseBuffer::SetLength(int samples)
{
  const int nch = impulses.list.GetSize();
  if (!nch) return 0;
  for (int x=0;x<nch;x++)
  {
    int cursz=impulses[x].GetSize();
    if (cursz!=samples) 
    {
      impulses[x].Resize(samples,false);

      if (impulses[x].GetSize()!=samples) // validate length!
      {
        // ERROR! FREE ALL!
        for(x=0;x<impulses.list.GetSize();x++) impulses[x].Resize(0);
        return 0;
      }
    }

    if (cursz<samples)
      memset(impulses[x].Get()+cursz,0,(samples-cursz)*sizeof(WDL_FFT_REAL));     
  }
  return impulses[0].GetSize();
}


void WDL_ImpulseBuffer::SetNumChannels(int usench, bool duplicateExisting)
{
  if (usench<1) usench=1;

  const int old_nch = impulses.list.GetSize();
  if (usench > old_nch)
  {
    while (impulses.list.GetSize() < usench)
      impulses.list.Add(new WDL_TypedBuf<WDL_FFT_REAL>);

    const int len = SetLength(GetLength());

    int x,ax=0;
    if (duplicateExisting && len>0 && old_nch>0) for(x=old_nch;x<usench;x++)
    {
      memcpy(impulses[x].Get(),impulses[ax].Get(),len*sizeof(WDL_FFT_REAL)); // duplicate channels
      if (++ax>=old_nch) ax=0;
    }
  }
  else while (usench<impulses.list.GetSize())
  {
    impulses.list.Delete(impulses.list.GetSize()-1,true);
  }
}
