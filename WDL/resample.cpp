/*
    WDL - resample.cpp
    Copyright (C) 2010 and later Cockos Incorporated

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
      
    You may also distribute this software under the LGPL v2 or later.

*/

#include "resample.h"
#include <math.h>

#include "denormal.h"

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

class WDL_Resampler::WDL_Resampler_IIRFilter
{
public:
  WDL_Resampler_IIRFilter() 
  { 
    m_fpos=-1;
    Reset(); 
  }
  ~WDL_Resampler_IIRFilter()
  {
  }

  void Reset() 
  { 
    memset(m_hist,0,sizeof(m_hist)); 
  }

  void setParms(double fpos, double Q)
  {
    if (fabs(fpos-m_fpos)<0.000001) return;
    m_fpos=fpos;

    double pos = fpos * PI;
    double cpos=cos(pos);
    double spos=sin(pos);
    
    double alpha=spos/(2.0*Q);
    
    double sc=1.0/( 1 + alpha);
    m_b1 = (1-cpos) * sc;
    m_b2 = m_b0 = m_b1*0.5;
    m_a1 =  -2 * cpos * sc;
    m_a2 = (1-alpha)*sc;

  }

  void Apply(WDL_ResampleSample *in1, WDL_ResampleSample *out1, int ns, int span, int w)
  {
    double b0=m_b0,b1=m_b1,b2=m_b2,a1=m_a1,a2=m_a2;
    double *hist=m_hist[w];
    while (ns--)
    {
      double in=*in1;
      in1+=span;
      double out = (double) ( in*b0 + hist[0]*b1 + hist[1]*b2 - hist[2]*a1 - hist[3]*a2);
      hist[1]=hist[0]; hist[0]=in;
      hist[3]=hist[2]; *out1 = hist[2]=denormal_filter_double(out);

      out1+=span;
    }
  }

private:
  double m_fpos;
  double m_a1,m_a2;
  double m_b0,m_b1,m_b2;
  double m_hist[WDL_RESAMPLE_MAX_FILTERS*WDL_RESAMPLE_MAX_NCH][4];
};


void inline WDL_Resampler::SincSample(WDL_ResampleSample *outptr, WDL_ResampleSample *inptr, double fracpos, int nch, WDL_SincFilterSample *filter, int filtsz)
{
  int oversize=m_lp_oversize;
  int x;
  fracpos *= oversize;
  int ifpos=(int)fracpos;
  filter += oversize-1-ifpos;
  fracpos -= ifpos;

  for (x = 0; x < nch; x ++)
  {
    double sum=0.0,sum2=0.0;
    WDL_SincFilterSample *fptr=filter;
    WDL_ResampleSample *iptr=inptr+x;
    int i=filtsz;
    while (i--)
    {
      sum += fptr[0]*iptr[0]; 
      sum2 += fptr[1]*iptr[0]; 
      iptr+=nch;
      fptr += oversize;
    }
    outptr[x]=sum*fracpos + sum2*(1.0-fracpos);
  }

}

void inline WDL_Resampler::SincSample1(WDL_ResampleSample *outptr, WDL_ResampleSample *inptr, double fracpos, WDL_SincFilterSample *filter, int filtsz)
{
  int oversize=m_lp_oversize;
  fracpos *= oversize;
  int ifpos=(int)fracpos;
  filter += oversize-1-ifpos;
  fracpos -= ifpos;

  double sum=0.0,sum2=0.0;
  WDL_SincFilterSample *fptr=filter;
  WDL_ResampleSample *iptr=inptr;
  int i=filtsz;
  while (i--)
  {
    sum += fptr[0]*iptr[0]; 
    sum2 += fptr[1]*iptr[0];
    iptr++;
    fptr += oversize;
  }
  outptr[0]=sum*fracpos+sum2*(1.0-fracpos);

}

void inline WDL_Resampler::SincSample2(WDL_ResampleSample *outptr, WDL_ResampleSample *inptr, double fracpos, WDL_SincFilterSample *filter, int filtsz)
{
  int oversize=m_lp_oversize;
  fracpos *= oversize;
  int ifpos=(int)fracpos;
  filter += oversize-1-ifpos;
  fracpos -= ifpos;

  double sum=0.0;
  double sum2=0.0;
  double sumb=0.0;
  double sum2b=0.0;
  WDL_SincFilterSample *fptr=filter;
  WDL_ResampleSample *iptr=inptr;
  int i=filtsz/2;
  while (i--)
  {
    sum += fptr[0]*iptr[0];
    sum2 += fptr[0]*iptr[1];
    sumb += fptr[1]*iptr[0];
    sum2b += fptr[1]*iptr[1];
    sum += fptr[oversize]*iptr[2];
    sum2 += fptr[oversize]*iptr[3];
    sumb += fptr[oversize+1]*iptr[2];
    sum2b += fptr[oversize+1]*iptr[3];
    iptr+=4;
    fptr += oversize*2;
  }
  outptr[0]=sum*fracpos + sumb*(1.0-fracpos);
  outptr[1]=sum2*fracpos + sum2b*(1.0-fracpos);

}



WDL_Resampler::WDL_Resampler()
{
  m_filterq=0.707f;
  m_filterpos=0.693f; // .792 ?

  m_sincoversize=0;
  m_lp_oversize=1; 
  m_sincsize=0;
  m_filtercnt=1;
  m_interp=true;
  m_feedmode=false;

  m_filter_coeffs_size=0; 
  m_sratein=44100.0; 
  m_srateout=44100.0; 
  m_ratio=1.0; 
  m_filter_ratio=-1.0; 
  m_iirfilter=0;

  Reset(); 
}

WDL_Resampler::~WDL_Resampler()
{
  delete m_iirfilter;
}

void WDL_Resampler::Reset(double fracpos)
{
  m_last_requested=0;
  m_filtlatency=0;
  m_fracpos=fracpos; 
  m_samples_in_rsinbuf=0; 
  if (m_iirfilter) m_iirfilter->Reset();   
}

void WDL_Resampler::SetMode(bool interp, int filtercnt, bool sinc, int sinc_size, int sinc_interpsize)
{
  m_sincsize = sinc && sinc_size>= 4 ? sinc_size > 8192 ? 8192 : sinc_size : 0;
  m_sincoversize = m_sincsize  ? (sinc_interpsize<= 1 ? 1 : sinc_interpsize>=4096 ? 4096 : sinc_interpsize) : 1;

  m_filtercnt = m_sincsize ? 0 : (filtercnt<=0?0 : filtercnt >= WDL_RESAMPLE_MAX_FILTERS ? WDL_RESAMPLE_MAX_FILTERS : filtercnt);
  m_interp=interp && !m_sincsize;
//  char buf[512];
//  sprintf(buf,"setting interp=%d, filtercnt=%d, sinc=%d,%d\n",m_interp,m_filtercnt,m_sincsize,m_sincoversize);
//  OutputDebugString(buf);

  if (!m_sincsize) 
  {
    m_filter_coeffs.Resize(0);
    m_filter_coeffs_size=0;
  }
  if (!m_filtercnt) 
  {
    delete m_iirfilter;
    m_iirfilter=0;
  }
}

void WDL_Resampler::SetRates(double rate_in, double rate_out) 
{
  if (rate_in<1.0) rate_in=1.0;
  if (rate_out<1.0) rate_out=1.0;
  if (rate_in != m_sratein || rate_out != m_srateout)
  {
    m_sratein=rate_in; 
    m_srateout=rate_out;  
    m_ratio=m_sratein / m_srateout;
  }
}


void WDL_Resampler::BuildLowPass(double filtpos) // only called in sinc modes
{
  int wantsize=m_sincsize;
  int wantinterp=m_sincoversize;

  if (m_filter_ratio!=filtpos || 
      m_filter_coeffs_size != wantsize ||
      m_lp_oversize != wantinterp)
  {
    m_lp_oversize = wantinterp;
    m_filter_ratio=filtpos;

    // build lowpass filter
    int allocsize = (wantsize+1)*m_lp_oversize;
    WDL_SincFilterSample *cfout=m_filter_coeffs.Resize(allocsize);
    if (m_filter_coeffs.GetSize()==allocsize)
    {
      m_filter_coeffs_size=wantsize;

      int sz=wantsize*m_lp_oversize;
      int hsz=sz/2;
      double filtpower=0.0;
      double windowpos = 0.0;
      double dwindowpos = 2.0 * PI/(double)(sz);
      double dsincpos  = PI / m_lp_oversize * filtpos; // filtpos is outrate/inrate, i.e. 0.5 is going to half rate
      double sincpos = dsincpos * (double)(-hsz);
    
      int x;
      for (x = -hsz; x < hsz+m_lp_oversize; x ++) 
      {
        double val = 0.35875 - 0.48829 * cos(windowpos) + 0.14128 * cos(2*windowpos) - 0.01168 * cos(6*windowpos); // blackman-harris
        if (x) val *= sin(sincpos) / sincpos;

        windowpos+=dwindowpos;
        sincpos += dsincpos;

        cfout[hsz+x] = (WDL_SincFilterSample)val;
        if (x < hsz) filtpower += val;
      }
      filtpower = m_lp_oversize/filtpower;
      for (x = 0; x < sz+m_lp_oversize; x ++) 
      {
        cfout[x] = (WDL_SincFilterSample) (cfout[x]*filtpower);
      }
    }
    else m_filter_coeffs_size=0;

  }
}

double WDL_Resampler::GetCurrentLatency() 
{ 
  double v=((double)m_samples_in_rsinbuf-m_filtlatency)/m_sratein;
  
  if (v<0.0)v=0.0;
  return v;
}

int WDL_Resampler::ResamplePrepare(int out_samples, int nch, WDL_ResampleSample **inbuffer) 
{   
  if (nch > WDL_RESAMPLE_MAX_NCH || nch < 1) return 0;

  int fsize=0;
  if (m_sincsize>1) fsize = m_sincsize;

  int hfs=fsize/2;
  if (hfs>1 && m_samples_in_rsinbuf<hfs-1)
  {
    m_filtlatency+=hfs-1 - m_samples_in_rsinbuf;

    m_samples_in_rsinbuf=hfs-1;

    if (m_samples_in_rsinbuf>0)
    {      
      WDL_ResampleSample *p = m_rsinbuf.Resize(m_samples_in_rsinbuf*nch,false);
      memset(p,0,sizeof(WDL_ResampleSample)*m_rsinbuf.GetSize());
    }
  }

  int sreq = 0;
    
  if (!m_feedmode) sreq = (int)(m_ratio * out_samples) + 4 + fsize - m_samples_in_rsinbuf;
  else sreq = out_samples;

  if (sreq<0)sreq=0;
  
again:
  m_rsinbuf.Resize((m_samples_in_rsinbuf+sreq)*nch,false);

  int sz = m_rsinbuf.GetSize()/(nch?nch:1) - m_samples_in_rsinbuf;
  if (sz!=sreq)
  {
    if (sreq>4 && !sz)
    {
      sreq/=2;
      goto again; // try again with half the size
    }
    // todo: notify of error?
    sreq=sz;
  }

  *inbuffer = m_rsinbuf.Get() + m_samples_in_rsinbuf*nch;

  m_last_requested=sreq;
  return sreq;
}



int WDL_Resampler::ResampleOut(WDL_ResampleSample *out, int nsamples_in, int nsamples_out, int nch)
{
  if (nch > WDL_RESAMPLE_MAX_NCH || nch < 1) return 0;

  if (m_filtercnt>0)
  {
    if (m_ratio > 1.0 && nsamples_in > 0) // filter input
    {
      if (!m_iirfilter) m_iirfilter = new WDL_Resampler_IIRFilter;

      int n=m_filtercnt;
      m_iirfilter->setParms((1.0/m_ratio)*m_filterpos,m_filterq);

      WDL_ResampleSample *buf=(WDL_ResampleSample *)m_rsinbuf.Get() + m_samples_in_rsinbuf*nch;
      int a,x;
      int offs=0;
      for (x=0; x < nch; x ++)
        for (a = 0; a < n; a ++)
          m_iirfilter->Apply(buf+x,buf+x,nsamples_in,nch,offs++);
    }
  }

  m_samples_in_rsinbuf += min(nsamples_in,m_last_requested); // prevent the user from corrupting the internal state


  int rsinbuf_availtemp = m_samples_in_rsinbuf;

  if (nsamples_in < m_last_requested) // flush out to ensure we can deliver
  {
    int fsize=(m_last_requested-nsamples_in)*2 + m_sincsize*2;

    int alloc_size=(m_samples_in_rsinbuf+fsize)*nch;
    WDL_ResampleSample *zb=m_rsinbuf.Resize(alloc_size,false);
    if (m_rsinbuf.GetSize()==alloc_size)
    {
      memset(zb+m_samples_in_rsinbuf*nch,0,fsize*nch*sizeof(WDL_ResampleSample));
      rsinbuf_availtemp = m_samples_in_rsinbuf+fsize;
    }
  }

  int ret=0;
  double srcpos=m_fracpos;
  double drspos = m_ratio;
  WDL_ResampleSample *localin = m_rsinbuf.Get();

  WDL_ResampleSample *outptr=out;

  int ns=nsamples_out;

  int outlatadj=0;

  if (m_sincsize) // sinc interpolating
  {
    if (m_ratio > 1.0) BuildLowPass(1.0 / (m_ratio*1.03));
    else BuildLowPass(1.0);

    int filtsz=m_filter_coeffs_size;
    int filtlen = rsinbuf_availtemp - filtsz;
    outlatadj=filtsz/2-1;
    WDL_SincFilterSample *filter=m_filter_coeffs.Get();   

    if (nch == 1)
    {
      while (ns--)
      {
        int ipos = (int)srcpos;

        if (ipos >= filtlen-1)  break; // quit decoding, not enough input samples

        SincSample1(outptr,localin + ipos,srcpos-ipos,filter,filtsz);
        outptr ++;
        srcpos+=drspos;
        ret++;
      }
    }
    else if (nch==2)
    {
      while (ns--)
      {
        int ipos = (int)srcpos;

        if (ipos >= filtlen-1)  break; // quit decoding, not enough input samples

        SincSample2(outptr,localin + ipos*2,srcpos-ipos,filter,filtsz);
        outptr+=2;
        srcpos+=drspos;
        ret++;
      }
    }
    else
    {
      while (ns--)
      {
        int ipos = (int)srcpos;

        if (ipos >= filtlen-1)  break; // quit decoding, not enough input samples

        SincSample(outptr,localin + ipos*nch,srcpos-ipos,nch,filter,filtsz);
        outptr += nch;
        srcpos+=drspos;
        ret++;
      }
    }
  }
  else if (!m_interp) // point sampling
  {
    if (nch == 1)
    {
      while (ns--)
      {
        int ipos = (int)srcpos;
        if (ipos >= rsinbuf_availtemp)  break; // quit decoding, not enough input samples

        *outptr++ = localin[ipos];
        srcpos+=drspos;
        ret++;
      }
    }
    else if (nch == 2)
    {
      while (ns--)
      {
        int ipos = (int)srcpos;
        if (ipos >= rsinbuf_availtemp)  break; // quit decoding, not enough input samples

        ipos+=ipos;

        outptr[0] = localin[ipos];
        outptr[1] = localin[ipos+1];
        outptr+=2;
        srcpos+=drspos;
        ret++;
      }
    }
    else
      while (ns--)
      {
        int ipos = (int)srcpos;
        if (ipos >= rsinbuf_availtemp)  break; // quit decoding, not enough input samples
    
        memcpy(outptr,localin + ipos*nch,nch*sizeof(WDL_ResampleSample));
        outptr += nch;
        srcpos+=drspos;
        ret++;
      }
  }
  else // linear interpolation
  {
    if (nch == 1)
    {
      while (ns--)
      {
        int ipos = (int)srcpos;
        double fracpos=srcpos-ipos; 

        if (ipos >= rsinbuf_availtemp-1) 
        {
          break; // quit decoding, not enough input samples
        }

        double ifracpos=1.0-fracpos;
        WDL_ResampleSample *inptr = localin + ipos;
        *outptr++ = inptr[0]*(ifracpos) + inptr[1]*(fracpos);
        srcpos+=drspos;
        ret++;
      }
    }
    else if (nch == 2)
    {
      while (ns--)
      {
        int ipos = (int)srcpos;
        double fracpos=srcpos-ipos; 

        if (ipos >= rsinbuf_availtemp-1) 
        {
          break; // quit decoding, not enough input samples
        }

        double ifracpos=1.0-fracpos;
        WDL_ResampleSample *inptr = localin + ipos*2;
        outptr[0] = inptr[0]*(ifracpos) + inptr[2]*(fracpos);
        outptr[1] = inptr[1]*(ifracpos) + inptr[3]*(fracpos);
        outptr += 2;
        srcpos+=drspos;
        ret++;
      }
    }
    else
    {
      while (ns--)
      {
        int ipos = (int)srcpos;
        double fracpos=srcpos-ipos; 

        if (ipos >= rsinbuf_availtemp-1) 
        {
          break; // quit decoding, not enough input samples
        }

        double ifracpos=1.0-fracpos;
        int ch=nch;
        WDL_ResampleSample *inptr = localin + ipos*nch;
        while (ch--)
        {
          *outptr++ = inptr[0]*(ifracpos) + inptr[nch]*(fracpos);
          inptr++;
        }
        srcpos+=drspos;
        ret++;
      }
    }
  }


  if (m_filtercnt>0)
  {
    if (m_ratio < 1.0 && ret>0) // filter output
    {
      if (!m_iirfilter) m_iirfilter = new WDL_Resampler_IIRFilter;
      int n=m_filtercnt;
      m_iirfilter->setParms(m_ratio*m_filterpos,m_filterq);

      int x,a;
      int offs=0;
      for (x=0; x < nch; x ++)
        for (a = 0; a < n; a ++)
          m_iirfilter->Apply(out+x,out+x,ret,nch,offs++);
    }
  }

  

  if (ret>0 && rsinbuf_availtemp>m_samples_in_rsinbuf) // we had to pad!!
  {
    // check for the case where rsinbuf_availtemp>m_samples_in_rsinbuf, decrease ret down to actual valid samples
    double adj=(srcpos-m_samples_in_rsinbuf + outlatadj) / drspos;
    if (adj>0)
    {
      ret -= (int) (adj + 0.5);
      if (ret<0)ret=0;
    }
  }

  int isrcpos=(int)srcpos;
  m_fracpos = srcpos - isrcpos;
  m_samples_in_rsinbuf -= isrcpos;
  if (m_samples_in_rsinbuf <= 0) m_samples_in_rsinbuf=0;
  else
    memcpy(localin, localin + isrcpos*nch,m_samples_in_rsinbuf*sizeof(WDL_ResampleSample)*nch);


  return ret;
}
