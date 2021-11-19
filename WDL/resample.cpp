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

#if !defined(WDL_RESAMPLE_NO_SSE) && !defined(WDL_RESAMPLE_USE_SSE)
  #if defined(__SSE2__) || _M_IX86_FP >= 2 || (defined(_WIN64) && (_MSC_VER > 1400 || __INTEL_COMPILER > 0))
    #define WDL_RESAMPLE_USE_SSE
  #endif
#endif

#ifdef WDL_RESAMPLE_USE_SSE
  #include <emmintrin.h>
#endif

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

class WDL_Resampler::WDL_Resampler_Filter // pre/post filter
{
public:
  WDL_Resampler_Filter() : m_fpos(-1.0), m_state(-1), m_filtsz(0) { }

  void Reset() 
  { 
    memset(m_hist.Get(),0,m_hist.GetSize() * sizeof(double));
    m_state = -1;
  }

  void setParms(double fpos, double fpos2, double Q, int hist_sz, int nch)
  {
    if (fpos < 1.0)
    {
       // if initial filter state is close to nyquist, fade-in
       // (in case this resampler was newly activated on varispeed media)
      if (m_state == -1 && fpos >= 0.97) m_state=0;
      fpos *= fpos2;
    }
    else
    {
      fpos = 1.0;
    }

    m_filtsz = hist_sz;
    hist_sz *= 4 * nch;
    if (m_hist.GetSize() != hist_sz)
    {
      const int oldsz = m_hist.GetSize();
      double *p = m_hist.Resize(hist_sz,false);
      if (m_hist.GetSize() > oldsz)
        memset(p+oldsz,0,(m_hist.GetSize() - oldsz)*sizeof(double));
    }

    if (fabs(fpos-m_fpos)<0.000001) return;
    m_fpos=fpos;

    const double pos = (fpos == 1.0 ? fpos2 : fpos) * PI;
    const double cpos=cos(pos);
    const double spos=sin(pos);
    const double alpha=spos/(2.0*Q);
    const double sc=1.0/( 1 + alpha);
    m_b1 = (1-cpos) * sc;
    m_b2 = m_b0 = m_b1*0.5;
    m_a1 =  -2 * cpos * sc;
    m_a2 = (1-alpha)*sc;
  }

  void ApplyIIR(WDL_ResampleSample *out1, int ns, int span, double *hsave)
  {
    double b0=m_b0,b1=m_b1,b2=m_b2,a1=m_a1,a2=m_a2;
    double hist[4];
    memcpy(hist,hsave, sizeof(hist));
    while (ns--)
    {
      double in=*out1;
      double out = (double) ( in*b0 + hist[0]*b1 + hist[1]*b2 - hist[2]*a1 - hist[3]*a2);
      hist[1]=hist[0]; hist[0]=in;
      hist[3]=hist[2]; *out1 = hist[2]=denormal_filter_double(out);

      out1+=span;
    }
    memcpy(hsave, hist, sizeof(hist));
  }
  void ApplyIIRFade(WDL_ResampleSample *out1, int ns, int span, double *hsave, double v0, double dv)
  {
    double b0=m_b0,b1=m_b1,b2=m_b2,a1=m_a1,a2=m_a2;
    double hist[4];
    memcpy(hist,hsave, sizeof(hist));
    while (ns--)
    {
      double in=*out1;
      double out = (double) ( in*b0 + hist[0]*b1 + hist[1]*b2 - hist[2]*a1 - hist[3]*a2);
      hist[1]=hist[0]; hist[0]=in;
      hist[3]=hist[2];
      *out1 = (hist[2] = denormal_filter_double(out)) * v0 + in * (1.0-v0);
      v0 += dv;
      out1+=span;
    }
    memcpy(hsave, hist, sizeof(hist));
  }

  void ApplyBuffer(WDL_ResampleSample *buf, int ns, int nch)
  {
    double *hist = m_hist.Get();
    const int nf = m_filtsz;
    if (m_state == 0)
    {
      if (m_fpos >= 1.0) return;
      m_state = 1;
      for (int x=0; x < nch; x ++) for (int a = 0; a < nf; a ++, hist+=4) ApplyIIRFade(buf+x,ns,nch,hist,0.0,1.0/ns);
    }
    else if (m_fpos >= 1.0)
    {
      if (m_state > 0)
        for (int x=0; x < nch; x ++) for (int a = 0; a < nf; a ++, hist+=4) ApplyIIRFade(buf+x,ns,nch,hist,1.0,-1.0/ns);
      m_state = 0;
    }
    else
    {
      m_state = 1;
      for (int x=0; x < nch; x ++) for (int a = 0; a < nf; a ++, hist+=4) ApplyIIR(buf+x,ns,nch,hist);
    }
  }

private:
  double m_fpos;
  double m_a1,m_a2;
  double m_b0,m_b1,m_b2;

  WDL_TypedBuf<double> m_hist;

  int m_filtsz;
  int m_state;
};


template <class T1, class T2> static void inline SincSample(T1 *outptr, const T1 *inptr, double fracpos, int nch, const T2 *filter, int filtsz, int oversize)
{
  fracpos *= oversize;
  const int ifpos=(int)fracpos;
  filter += (oversize-ifpos) * filtsz;
  fracpos -= ifpos;

  int x;
  for (x = 0; x < nch; x ++)
  {
    double sum=0.0,sum2=0.0;
    const T2 *fptr2=filter;
    const T2 *fptr=fptr2 - filtsz;
    const T1 *iptr=inptr+x;
    int i=filtsz/2;
    while (i--)
    {
      sum += fptr[0]*iptr[0]; 
      sum2 += fptr2[0]*iptr[0]; 
      sum += fptr[1]*iptr[nch]; 
      sum2 += fptr2[1]*iptr[nch]; 
      iptr+=nch*2;
      fptr+=2;
      fptr2+=2;
    }
    outptr[x]=sum*fracpos + sum2*(1.0-fracpos);
  }

}

template <class T1, class T2> static void inline SincSampleN(T1 *outptr, const T1 *inptr, double fracpos, int nch, const T2 *filter, int filtsz, int oversize)
{
  const int ifpos=(int)(fracpos*oversize+0.5);
  filter += (oversize-ifpos) * filtsz;

  int x;
  for (x = 0; x < nch; x ++)
  {
    double sum2=0.0;
    const T2 *fptr2=filter;
    const T1 *iptr=inptr+x;
    int i=filtsz/2;
    while (i--)
    {
      sum2 += fptr2[0]*iptr[0]; 
      sum2 += fptr2[1]*iptr[nch]; 
      iptr+=nch*2;
      fptr2+=2;
    }
    outptr[x]=sum2;
  }

}

template <class T1, class T2> static void inline SincSample1(T1 *outptr, const T1 *inptr, double fracpos, const T2 *filter, int filtsz, int oversize)
{
  fracpos *= oversize;
  const int ifpos=(int)fracpos;
  fracpos -= ifpos;

  double sum=0.0,sum2=0.0;
  const T2 *fptr2=filter + (oversize-ifpos) * filtsz;
  const T2 *fptr=fptr2 - filtsz;
  const T1 *iptr=inptr;
  int i=filtsz/2;
  while (i--)
  {
    sum += fptr[0]*iptr[0]; 
    sum2 += fptr2[0]*iptr[0];
    sum += fptr[1]*iptr[1]; 
    sum2 += fptr2[1]*iptr[1];
    iptr+=2;
    fptr+=2;
    fptr2+=2;
  }
  outptr[0]=sum*fracpos+sum2*(1.0-fracpos);
}

template <class T1, class T2> static void inline SincSample1N(T1 *outptr, const T1 *inptr, double fracpos, const T2 *filter, int filtsz, int oversize)
{
  const int ifpos=(int)(fracpos*oversize+0.5);

  double sum2=0.0;
  const T2 *fptr2=filter + (oversize-ifpos) * filtsz;
  const T1 *iptr=inptr;
  int i=filtsz/2;
  while (i--)
  {
    sum2 += fptr2[0]*iptr[0];
    sum2 += fptr2[1]*iptr[1];
    iptr+=2;
    fptr2+=2;
  }
  outptr[0]=sum2;
}

template <class T1, class T2> static void inline SincSample2(T1 *outptr, const T1 *inptr, double fracpos, const T2 *filter, int filtsz, int oversize)
{
  fracpos *= oversize;
  const int ifpos=(int)fracpos;
  fracpos -= ifpos;

  const T2 *fptr2=filter + (oversize-ifpos) * filtsz;
  const T2 *fptr=fptr2 - filtsz;

  double sum=0.0;
  double sum2=0.0;
  double sumb=0.0;
  double sum2b=0.0;
  const T1 *iptr=inptr;
  int i=filtsz/2;
  while (i--)
  {
    sum += fptr[0]*iptr[0];
    sum2 += fptr[0]*iptr[1];
    sumb += fptr2[0]*iptr[0];
    sum2b += fptr2[0]*iptr[1];
    sum += fptr[1]*iptr[2];
    sum2 += fptr[1]*iptr[3];
    sumb += fptr2[1]*iptr[2];
    sum2b += fptr2[1]*iptr[3];
    iptr+=4;
    fptr+=2;
    fptr2+=2;
  }
  outptr[0]=sum*fracpos + sumb*(1.0-fracpos);
  outptr[1]=sum2*fracpos + sum2b*(1.0-fracpos);

}

template <class T1, class T2> static void inline SincSample2N(T1 *outptr, const T1 *inptr, double fracpos, const T2 *filter, int filtsz, int oversize)
{
  const int ifpos=(int)(fracpos*oversize+0.5);

  const T2 *fptr2=filter + (oversize-ifpos) * filtsz;

  double sumb=0.0;
  double sum2b=0.0;
  const T1 *iptr=inptr;
  int i=filtsz/2;
  while (i--)
  {
    sumb += fptr2[0]*iptr[0];
    sum2b += fptr2[0]*iptr[1];
    sumb += fptr2[1]*iptr[2];
    sum2b += fptr2[1]*iptr[3];
    iptr+=4;
    fptr2+=2;
  }
  outptr[0]=sumb;
  outptr[1]=sum2b;
}


#ifdef WDL_RESAMPLE_USE_SSE

static void inline SincSample(double *outptr, const double *inptr, double fracpos, int nch, const float *filter, int filtsz, int oversize)
{
  fracpos *= oversize;
  const int ifpos=(int)fracpos;
  filter += (oversize-ifpos) * filtsz;
  fracpos -= ifpos;

  int x;
  for (x = 0; x < nch; x ++)
  {
    double sum, sum2;
    const float *fptr2=filter;
    const float *fptr=fptr2 - filtsz;
    const double *iptr=inptr+x;
    int i=filtsz/2;

    __m128d xmm0 = _mm_setzero_pd();
    __m128d xmm1 = _mm_setzero_pd();
    __m128d xmm2, xmm3;

    while (i--)
    {
      xmm2 = _mm_set_pd(iptr[nch], iptr[0]);

      xmm3 = _mm_load_sd((double *)fptr);
      xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
      xmm3 = _mm_mul_pd(xmm3, xmm2);
      xmm0 = _mm_add_pd(xmm0, xmm3);

      xmm3 = _mm_load_sd((double *)fptr2);
      xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
      xmm3 = _mm_mul_pd(xmm3, xmm2);
      xmm1 = _mm_add_pd(xmm1, xmm3);

      iptr+=nch*2;
      fptr+=2;
      fptr2+=2;
    }

    xmm2 = xmm0;
    xmm0 = _mm_unpackhi_pd(xmm0, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm2);
    _mm_store_sd(&sum, xmm0);

    xmm3 = xmm1;
    xmm1 = _mm_unpackhi_pd(xmm1, xmm3);
    xmm1 = _mm_add_pd(xmm1, xmm3);
    _mm_store_sd(&sum2, xmm1);

    outptr[x]=sum*fracpos + sum2*(1.0-fracpos);
  }

}

static void inline SincSampleN(double *outptr, const double *inptr, double fracpos, int nch, const float *filter, int filtsz, int oversize)
{
  const int ifpos=(int)(fracpos*oversize+0.5);
  filter += (oversize-ifpos) * filtsz;

  int x;
  for (x = 0; x < nch; x ++)
  {
    double sum2;
    const float *fptr2=filter;
    const double *iptr=inptr+x;
    int i=filtsz/2;

    __m128d xmm0 = _mm_setzero_pd();
    __m128d xmm1 = _mm_setzero_pd();
    __m128d xmm2, xmm3;

    while (i >= 2)
    {
      xmm2 = _mm_set_pd(iptr[nch], iptr[0]);

      xmm3 = _mm_load_sd((double *)fptr2);
      xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
      xmm3 = _mm_mul_pd(xmm3, xmm2);
      xmm0 = _mm_add_pd(xmm0, xmm3);

      xmm2 = _mm_set_pd(iptr[nch*3], iptr[nch*2]);

      xmm3 = _mm_load_sd((double *)fptr2 + 1);
      xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
      xmm3 = _mm_mul_pd(xmm3, xmm2);
      xmm1 = _mm_add_pd(xmm1, xmm3);

      iptr+=nch*4;
      fptr2+=4;
      i-=2;
    }

    if (i)
    {
      xmm2 = _mm_set_pd(iptr[nch], iptr[0]);

      xmm3 = _mm_load_sd((double *)fptr2);
      xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
      xmm3 = _mm_mul_pd(xmm3, xmm2);
      xmm0 = _mm_add_pd(xmm0, xmm3);
    }

    xmm1 = _mm_add_pd(xmm1, xmm0);

    xmm3 = xmm1;
    xmm1 = _mm_unpackhi_pd(xmm1, xmm3);
    xmm1 = _mm_add_pd(xmm1, xmm3);
    _mm_store_sd(&sum2, xmm1);

    outptr[x]=sum2;
  }

}

static void inline SincSample1(double *outptr, const double *inptr, double fracpos, const float *filter, int filtsz, int oversize)
{
  fracpos *= oversize;
  const int ifpos=(int)fracpos;
  fracpos -= ifpos;

  double sum, sum2;
  const float *fptr2=filter + (oversize-ifpos) * filtsz;
  const float *fptr=fptr2 - filtsz;
  const double *iptr=inptr;
  int i=filtsz/2;

  __m128d xmm0 = _mm_setzero_pd();
  __m128d xmm1 = _mm_setzero_pd();
  __m128d xmm2, xmm3;

  while (i >= 2)
  {
    xmm2 = _mm_loadu_pd(iptr);

    xmm3 = _mm_cvtps_pd(_mm_load_ps(fptr));
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm3);

    xmm3 = _mm_cvtps_pd(_mm_load_ps(fptr2));
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm1 = _mm_add_pd(xmm1, xmm3);

    xmm2 = _mm_loadu_pd(iptr+2);

    xmm3 = _mm_load_sd((double *)fptr + 1);
    xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm3);

    xmm3 = _mm_load_sd((double *)fptr2 + 1);
    xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm1 = _mm_add_pd(xmm1, xmm3);

    iptr+=4;
    fptr+=4;
    fptr2+=4;
    i-=2;
  }

  if (i)
  {
    xmm2 = _mm_loadu_pd(iptr);

    xmm3 = _mm_load_sd((double *)fptr);
    xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm3);

    xmm3 = _mm_load_sd((double *)fptr2);
    xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm1 = _mm_add_pd(xmm1, xmm3);
  }

  xmm2 = xmm0;
  xmm0 = _mm_unpackhi_pd(xmm0, xmm2);
  xmm0 = _mm_add_pd(xmm0, xmm2);
  _mm_store_sd(&sum, xmm0);

  xmm3 = xmm1;
  xmm1 = _mm_unpackhi_pd(xmm1, xmm3);
  xmm1 = _mm_add_pd(xmm1, xmm3);
  _mm_store_sd(&sum2, xmm1);

  outptr[0]=sum*fracpos+sum2*(1.0-fracpos);
}

static void inline SincSample1N(double *outptr, const double *inptr, double fracpos, const float *filter, int filtsz, int oversize)
{
  const int ifpos=(int)(fracpos*oversize+0.5);

  double sum2;
  const float *fptr2=filter + (oversize-ifpos) * filtsz;
  const double *iptr=inptr;
  int i=filtsz/2;

  __m128d xmm0 = _mm_setzero_pd();
  __m128d xmm1 = _mm_setzero_pd();
  __m128d xmm2, xmm3;

  while (i >= 2)
  {
    xmm2 = _mm_loadu_pd(iptr);

    xmm3 = _mm_cvtps_pd(_mm_load_ps(fptr2));
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm3);

    xmm2 = _mm_loadu_pd(iptr+2);

    xmm3 = _mm_load_sd((double *)fptr2 + 1);
    xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm1 = _mm_add_pd(xmm1, xmm3);

    iptr+=4;
    fptr2+=4;
    i-=2;
  }

  if (i)
  {
    xmm2 = _mm_loadu_pd(iptr);

    xmm3 = _mm_load_sd((double *)fptr2);
    xmm3 = _mm_cvtps_pd(_mm_castpd_ps(xmm3));
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm3);
  }

  xmm1 = _mm_add_pd(xmm1, xmm0);

  xmm3 = xmm1;
  xmm1 = _mm_unpackhi_pd(xmm1, xmm3);
  xmm1 = _mm_add_pd(xmm1, xmm3);
  _mm_store_sd(&sum2, xmm1);

  outptr[0]=sum2;
}

static void inline SincSample2(double *outptr, const double *inptr, double fracpos, const float *filter, int filtsz, int oversize)
{
  fracpos *= oversize;
  const int ifpos=(int)fracpos;
  fracpos -= ifpos;

  const float *fptr2=filter + (oversize-ifpos) * filtsz;
  const float *fptr=fptr2 - filtsz;

  double sum, sum2, sumb, sum2b;
  const double *iptr=inptr;
  int i=filtsz/2;

  __m128d xmm0 = _mm_setzero_pd();
  __m128d xmm1 = _mm_setzero_pd();
  __m128d xmm2, xmm3;
  __m128 xmm4;

  while (i--)
  {
    xmm2 = _mm_loadu_pd(iptr);

    xmm4 = _mm_set1_ps(fptr[0]);
    xmm3 = _mm_cvtps_pd(xmm4);
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm3);

    xmm4 = _mm_set1_ps(fptr2[0]);
    xmm3 = _mm_cvtps_pd(xmm4);
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm1 = _mm_add_pd(xmm1, xmm3);

    xmm2 = _mm_loadu_pd(iptr+2);

    xmm4 = _mm_set1_ps(fptr[1]);
    xmm3 = _mm_cvtps_pd(xmm4);
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm3);

    xmm4 = _mm_set1_ps(fptr2[1]);
    xmm3 = _mm_cvtps_pd(xmm4);
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm1 = _mm_add_pd(xmm1, xmm3);

    iptr+=4;
    fptr+=2;
    fptr2+=2;
  }

  xmm2 = xmm0;
  _mm_store_sd(&sum, xmm0);
  xmm2 = _mm_unpackhi_pd(xmm2, xmm0);
  _mm_store_sd(&sum2, xmm2);

  xmm3 = xmm1;
  _mm_store_sd(&sumb, xmm1);
  xmm3 = _mm_unpackhi_pd(xmm3, xmm1);
  _mm_store_sd(&sum2b, xmm3);

  outptr[0]=sum*fracpos + sumb*(1.0-fracpos);
  outptr[1]=sum2*fracpos + sum2b*(1.0-fracpos);
}

static void inline SincSample2N(double *outptr, const double *inptr, double fracpos, const float *filter, int filtsz, int oversize)
{
  const int ifpos=(int)(fracpos*oversize+0.5);

  const float *fptr2=filter + (oversize-ifpos) * filtsz;

  double sumb, sum2b;
  const double *iptr=inptr;
  int i=filtsz/2;

  __m128d xmm0 = _mm_setzero_pd();
  __m128d xmm1 = _mm_setzero_pd();
  __m128d xmm2, xmm3;
  __m128 xmm4;

  while (i--)
  {
    xmm2 = _mm_loadu_pd(iptr);

    xmm4 = _mm_set1_ps(fptr2[0]);
    xmm3 = _mm_cvtps_pd(xmm4);
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm3);

    xmm2 = _mm_loadu_pd(iptr+2);

    xmm4 = _mm_set1_ps(fptr2[1]);
    xmm3 = _mm_cvtps_pd(xmm4);
    xmm3 = _mm_mul_pd(xmm3, xmm2);
    xmm1 = _mm_add_pd(xmm1, xmm3);

    iptr+=4;
    fptr2+=2;
  }

  xmm1 = _mm_add_pd(xmm1, xmm0);

  xmm3 = xmm1;
  _mm_store_sd(&sumb, xmm1);
  xmm3 = _mm_unpackhi_pd(xmm3, xmm1);
  _mm_store_sd(&sum2b, xmm3);

  outptr[0]=sumb;
  outptr[1]=sum2b;
}


static void inline SincSample(double *outptr, const double *inptr, double fracpos, int nch, const double *filter, int filtsz, int oversize)
{
  fracpos *= oversize;
  const int ifpos=(int)fracpos;
  filter += (oversize-ifpos) * filtsz;
  fracpos -= ifpos;

  int x;
  for (x = 0; x < nch; x ++)
  {
    double sum, sum2;
    const double *fptr2=filter;
    const double *fptr=fptr2 - filtsz;
    const double *iptr=inptr+x;
    int i=filtsz/2;

    __m128d xmm0 = _mm_setzero_pd();
    __m128d xmm1 = _mm_setzero_pd();
    __m128d xmm2 = _mm_setzero_pd();
    __m128d xmm3 = _mm_setzero_pd();
    __m128d xmm4, xmm5;

    while (i >= 2)
    {
      xmm4 = _mm_set_pd(iptr[nch], iptr[0]);

      xmm5 = _mm_load_pd(fptr);
      xmm5 = _mm_mul_pd(xmm5, xmm4);
      xmm0 = _mm_add_pd(xmm0, xmm5);

      xmm5 = _mm_load_pd(fptr2);
      xmm5 = _mm_mul_pd(xmm5, xmm4);
      xmm1 = _mm_add_pd(xmm1, xmm5);

      xmm4 = _mm_set_pd(iptr[nch*3], iptr[nch*2]);

      xmm5 = _mm_load_pd(fptr+2);
      xmm5 = _mm_mul_pd(xmm5, xmm4);
      xmm2 = _mm_add_pd(xmm2, xmm5);

      xmm5 = _mm_load_pd(fptr2+2);
      xmm5 = _mm_mul_pd(xmm5, xmm4);
      xmm3 = _mm_add_pd(xmm3, xmm5);

      iptr+=nch*4;
      fptr+=4;
      fptr2+=4;
      i-=2;
    }

    if (i)
    {
      xmm4 = _mm_set_pd(iptr[nch], iptr[0]);

      xmm5 = _mm_load_pd(fptr);
      xmm5 = _mm_mul_pd(xmm5, xmm4);
      xmm0 = _mm_add_pd(xmm0, xmm5);

      xmm5 = _mm_load_pd(fptr2);
      xmm5 = _mm_mul_pd(xmm5, xmm4);
      xmm1 = _mm_add_pd(xmm1, xmm5);
    }

    xmm0 = _mm_add_pd(xmm0, xmm2);
    xmm1 = _mm_add_pd(xmm1, xmm3);

    xmm2 = xmm0;
    xmm0 = _mm_unpackhi_pd(xmm0, xmm2);
    xmm0 = _mm_add_pd(xmm0, xmm2);
    _mm_store_sd(&sum, xmm0);

    xmm3 = xmm1;
    xmm1 = _mm_unpackhi_pd(xmm1, xmm3);
    xmm1 = _mm_add_pd(xmm1, xmm3);
    _mm_store_sd(&sum2, xmm1);

    outptr[x]=sum*fracpos + sum2*(1.0-fracpos);
  }

}

static void inline SincSampleN(double *outptr, const double *inptr, double fracpos, int nch, const double *filter, int filtsz, int oversize)
{
  const int ifpos=(int)(fracpos*oversize+0.5);
  filter += (oversize-ifpos) * filtsz;

  int x;
  for (x = 0; x < nch; x ++)
  {
    double sum2;
    const double *fptr2=filter;
    const double *iptr=inptr+x;
    int i=filtsz/2;

    __m128d xmm0 = _mm_setzero_pd();
    __m128d xmm1 = _mm_setzero_pd();
    __m128d xmm2, xmm3;

    while (i >= 2)
    {
      xmm2 = _mm_set_pd(iptr[nch], iptr[0]);

      xmm3 = _mm_load_pd(fptr2);
      xmm3 = _mm_mul_pd(xmm3, xmm2);
      xmm0 = _mm_add_pd(xmm0, xmm3);

      xmm2 = _mm_set_pd(iptr[nch*3], iptr[nch*2]);

      xmm3 = _mm_load_pd(fptr2+2);
      xmm3 = _mm_mul_pd(xmm3, xmm2);
      xmm1 = _mm_add_pd(xmm1, xmm3);

      iptr+=nch*4;
      fptr2+=4;
      i-=2;
    }

    if (i)
    {
      xmm2 = _mm_set_pd(iptr[nch], iptr[0]);

      xmm3 = _mm_load_pd(fptr2);
      xmm3 = _mm_mul_pd(xmm3, xmm2);
      xmm0 = _mm_add_pd(xmm0, xmm3);
    }

    xmm1 = _mm_add_pd(xmm1, xmm0);

    xmm3 = xmm1;
    xmm1 = _mm_unpackhi_pd(xmm1, xmm3);
    xmm1 = _mm_add_pd(xmm1, xmm3);
    _mm_store_sd(&sum2, xmm1);

    outptr[x]=sum2;
  }

}

static void inline SincSample1(double *outptr, const double *inptr, double fracpos, const double *filter, int filtsz, int oversize)
{
  fracpos *= oversize;
  const int ifpos=(int)fracpos;
  fracpos -= ifpos;

  double sum, sum2;
  const double *fptr2=filter + (oversize-ifpos) * filtsz;
  const double *fptr=fptr2 - filtsz;
  const double *iptr=inptr;
  int i=filtsz/2;

  __m128d xmm0 = _mm_setzero_pd();
  __m128d xmm1 = _mm_setzero_pd();
  __m128d xmm2 = _mm_setzero_pd();
  __m128d xmm3 = _mm_setzero_pd();
  __m128d xmm4, xmm5;

  while (i >= 2)
  {
    xmm4 = _mm_loadu_pd(iptr);

    xmm5 = _mm_load_pd(fptr);
    xmm5 = _mm_mul_pd(xmm5, xmm4);
    xmm0 = _mm_add_pd(xmm0, xmm5);

    xmm5 = _mm_load_pd(fptr2);
    xmm5 = _mm_mul_pd(xmm5, xmm4);
    xmm1 = _mm_add_pd(xmm1, xmm5);

    xmm4 = _mm_loadu_pd(iptr+2);

    xmm5 = _mm_load_pd(fptr+2);
    xmm5 = _mm_mul_pd(xmm5, xmm4);
    xmm2 = _mm_add_pd(xmm2, xmm5);

    xmm5 = _mm_load_pd(fptr2+2);
    xmm5 = _mm_mul_pd(xmm5, xmm4);
    xmm3 = _mm_add_pd(xmm3, xmm5);

    iptr+=4;
    fptr+=4;
    fptr2+=4;
    i-=2;
  }

  if (i)
  {
    xmm4 = _mm_loadu_pd(iptr);

    xmm5 = _mm_load_pd(fptr);
    xmm5 = _mm_mul_pd(xmm5, xmm4);
    xmm0 = _mm_add_pd(xmm0, xmm5);

    xmm5 = _mm_load_pd(fptr2);
    xmm5 = _mm_mul_pd(xmm5, xmm4);
    xmm1 = _mm_add_pd(xmm1, xmm5);
  }

  xmm0 = _mm_add_pd(xmm0, xmm2);
  xmm1 = _mm_add_pd(xmm1, xmm3);

  xmm2 = xmm0;
  xmm0 = _mm_unpackhi_pd(xmm0, xmm2);
  xmm0 = _mm_add_pd(xmm0, xmm2);
  _mm_store_sd(&sum, xmm0);

  xmm3 = xmm1;
  xmm1 = _mm_unpackhi_pd(xmm1, xmm3);
  xmm1 = _mm_add_pd(xmm1, xmm3);
  _mm_store_sd(&sum2, xmm1);

  outptr[0]=sum*fracpos+sum2*(1.0-fracpos);
}

static void inline SincSample1N(double *outptr, const double *inptr, double fracpos, const double *filter, int filtsz, int oversize)
{
  const int ifpos=(int)(fracpos*oversize+0.5);

  double sum2;
  const double *fptr2=filter + (oversize-ifpos) * filtsz;
  const double *iptr=inptr;
  int i=filtsz/2;

  __m128d xmm0 = _mm_setzero_pd();
  __m128d xmm1 = _mm_setzero_pd();
  __m128d xmm2;

  while (i >= 2)
  {
    xmm2 = _mm_loadu_pd(iptr);
    xmm2 = _mm_mul_pd(xmm2, _mm_load_pd(fptr2));
    xmm0 = _mm_add_pd(xmm0, xmm2);

    xmm2 = _mm_loadu_pd(iptr+2);
    xmm2 = _mm_mul_pd(xmm2, _mm_load_pd(fptr2+2));
    xmm1 = _mm_add_pd(xmm1, xmm2);

    iptr+=4;
    fptr2+=4;
    i-=2;
  }

  if (i)
  {
    xmm2 = _mm_loadu_pd(iptr);
    xmm2 = _mm_mul_pd(xmm2, _mm_load_pd(fptr2));
    xmm0 = _mm_add_pd(xmm0, xmm2);
  }

  xmm1 = _mm_add_pd(xmm1, xmm0);

  xmm2 = xmm1;
  xmm1 = _mm_unpackhi_pd(xmm1, xmm2);
  xmm1 = _mm_add_pd(xmm1, xmm2);
  _mm_store_sd(&sum2, xmm1);

  outptr[0]=sum2;
}

static void inline SincSample2(double *outptr, const double *inptr, double fracpos, const double *filter, int filtsz, int oversize)
{
  fracpos *= oversize;
  const int ifpos=(int)fracpos;
  fracpos -= ifpos;

  const double *fptr2=filter + (oversize-ifpos) * filtsz;
  const double *fptr=fptr2 - filtsz;

  double sum, sum2, sumb, sum2b;
  const double *iptr=inptr;
  int i=filtsz/2;

  __m128d xmm0 = _mm_setzero_pd();
  __m128d xmm1 = _mm_setzero_pd();
  __m128d xmm2 = _mm_setzero_pd();
  __m128d xmm3 = _mm_setzero_pd();
  __m128d xmm4, xmm5, xmm6, xmm7;

  while (i--)
  {
    xmm4 = _mm_load_pd(fptr);
    xmm5 = _mm_load_pd(fptr2);

    xmm6 = _mm_loadu_pd(iptr);

    xmm7 = xmm4;
    xmm7 = _mm_unpacklo_pd(xmm7, xmm4);
    xmm7 = _mm_mul_pd(xmm7, xmm6);
    xmm0 = _mm_add_pd(xmm0, xmm7);

    xmm7 = xmm5;
    xmm7 = _mm_unpacklo_pd(xmm7, xmm5);
    xmm7 = _mm_mul_pd(xmm7, xmm6);
    xmm1 = _mm_add_pd(xmm1, xmm7);

    xmm6 = _mm_loadu_pd(iptr+2);

    xmm4 = _mm_unpackhi_pd(xmm4, xmm4);
    xmm4 = _mm_mul_pd(xmm4, xmm6);
    xmm2 = _mm_add_pd(xmm2, xmm4);

    xmm5 = _mm_unpackhi_pd(xmm5, xmm5);
    xmm5 = _mm_mul_pd(xmm5, xmm6);
    xmm3 = _mm_add_pd(xmm3, xmm5);

    iptr+=4;
    fptr+=2;
    fptr2+=2;
  }

  xmm0 = _mm_add_pd(xmm0, xmm2);
  xmm1 = _mm_add_pd(xmm1, xmm3);

  xmm2 = xmm0;
  _mm_store_sd(&sum, xmm0);
  xmm2 = _mm_unpackhi_pd(xmm2, xmm0);
  _mm_store_sd(&sum2, xmm2);

  xmm3 = xmm1;
  _mm_store_sd(&sumb, xmm1);
  xmm3 = _mm_unpackhi_pd(xmm3, xmm1);
  _mm_store_sd(&sum2b, xmm3);

  outptr[0]=sum*fracpos + sumb*(1.0-fracpos);
  outptr[1]=sum2*fracpos + sum2b*(1.0-fracpos);
}

static void inline SincSample2N(double *outptr, const double *inptr, double fracpos, const double *filter, int filtsz, int oversize)
{
  const int ifpos=(int)(fracpos*oversize+0.5);

  const double *fptr2=filter + (oversize-ifpos) * filtsz;

  double sumb, sum2b;
  const double *iptr=inptr;
  int i=filtsz/2;

  __m128d xmm0 = _mm_setzero_pd();
  __m128d xmm1 = _mm_setzero_pd();
  __m128d xmm2 = _mm_setzero_pd();
  __m128d xmm3 = _mm_setzero_pd();
  __m128d xmm4, xmm5, xmm6;

  while (i >= 2)
  {
    xmm4 = _mm_load_pd(fptr2);
    xmm5 = xmm4;

    xmm6 = _mm_loadu_pd(iptr);
    xmm4 = _mm_unpacklo_pd(xmm4, xmm5);
    xmm6 = _mm_mul_pd(xmm6, xmm4);
    xmm0 = _mm_add_pd(xmm0, xmm6);

    xmm6 = _mm_loadu_pd(iptr+2);
    xmm5 = _mm_unpackhi_pd(xmm5, xmm5);
    xmm6 = _mm_mul_pd(xmm6, xmm5);
    xmm1 = _mm_add_pd(xmm1, xmm6);

    xmm4 = _mm_load_pd(fptr2+2);
    xmm5 = xmm4;

    xmm6 = _mm_loadu_pd(iptr+4);
    xmm4 = _mm_unpacklo_pd(xmm4, xmm5);
    xmm6 = _mm_mul_pd(xmm6, xmm4);
    xmm2 = _mm_add_pd(xmm2, xmm6);

    xmm6 = _mm_loadu_pd(iptr+6);
    xmm5 = _mm_unpackhi_pd(xmm5, xmm5);
    xmm6 = _mm_mul_pd(xmm6, xmm5);
    xmm3 = _mm_add_pd(xmm3, xmm6);

    iptr+=8;
    fptr2+=4;
    i-=2;
  }

  if (i)
  {
    xmm4 = _mm_load_pd(fptr2);
    xmm5 = xmm4;

    xmm6 = _mm_loadu_pd(iptr);
    xmm4 = _mm_unpacklo_pd(xmm4, xmm5);
    xmm6 = _mm_mul_pd(xmm6, xmm4);
    xmm0 = _mm_add_pd(xmm0, xmm6);

    xmm6 = _mm_loadu_pd(iptr+2);
    xmm5 = _mm_unpackhi_pd(xmm5, xmm5);
    xmm6 = _mm_mul_pd(xmm6, xmm5);
    xmm1 = _mm_add_pd(xmm1, xmm6);
  }

  xmm0 = _mm_add_pd(xmm0, xmm2);
  xmm1 = _mm_add_pd(xmm1, xmm3);

  xmm1 = _mm_add_pd(xmm1, xmm0);

  xmm3 = xmm1;
  _mm_store_sd(&sumb, xmm1);
  xmm3 = _mm_unpackhi_pd(xmm3, xmm1);
  _mm_store_sd(&sum2b, xmm3);

  outptr[0]=sumb;
  outptr[1]=sum2b;
}

#endif // WDL_RESAMPLE_USE_SSE


WDL_Resampler::WDL_Resampler()
{
  m_sinc_ideal_calced = -1;
  m_filterq=0.707f;
  m_filterpos=0.693f; // .792 ?

  m_sincoversize=0;
  m_lp_oversize=1; 
  m_sincsize=0;
  m_prepost_filtercnt = 1;
  m_interp=true;
  m_feedmode=false;

  m_filter_coeffs_size=0; 
  m_sratein=44100.0; 
  m_srateout=44100.0; 
  m_ratio=1.0; 
  m_filter_ratio=-1.0; 
  m_pre_filter = NULL;
  m_post_filter = NULL;

  Reset(); 
}

WDL_Resampler::~WDL_Resampler()
{
  delete m_pre_filter;
  delete m_post_filter;
}

void WDL_Resampler::Reset(double fracpos)
{
  m_last_requested=0;
  m_filtlatency=0;
  m_fracpos=fracpos; 
  m_samples_in_rsinbuf=0; 
  if (m_sinc_ideal_calced == -2) m_sinc_ideal_calced = -1;
  if (m_pre_filter) m_pre_filter->Reset();
  if (m_post_filter) m_post_filter->Reset();
}

void WDL_Resampler::SetMode(bool interp, int filtercnt, bool sinc, int sinc_size, int sinc_interpsize)
{
  m_sincsize = sinc && sinc_size>= 4 ? sinc_size > 8192 ? 8192 : (sinc_size&~1) : 0;
  m_sincoversize = m_sincsize  ? (sinc_interpsize<= 1 ? 1 : sinc_interpsize>=8192 ? 8192 : sinc_interpsize) : 1;

  m_prepost_filtercnt = m_sincsize ? 0 : wdl_max(filtercnt,0);
  m_interp=interp && !m_sincsize;

  if (!m_sincsize) 
  {
    m_filter_coeffs.Resize(0);
    m_filter_coeffs_size=0;
  }
  if (!m_prepost_filtercnt)
  {
    delete m_pre_filter;
    m_pre_filter=NULL;
    delete m_post_filter;
    m_post_filter=NULL;
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
    m_sinc_ideal_calced = -1;
  }
}

const WDL_SincFilterSample *WDL_Resampler::BuildLowPass(double filtpos, bool *isIdeal) // only called in sinc modes
{
  const int wantsize=m_sincsize;
  int wantinterp=m_sincoversize;

  int ideal_interp = 0;
  if (wantinterp)
  {
    if (m_sinc_ideal_calced == -1)
    {
      if (m_ratio < 1.0)
      {
        const double drat = m_srateout/m_sratein;
        const int irat = (int) (drat + 0.5);
        if (irat > 1 && irat==drat) ideal_interp=irat;
      }
      else
      {
        const int irat = (int) (m_ratio + 0.5);
        if (m_ratio == irat) ideal_interp=1; // eg 96k to 48k, only need one table
      }

      if (!ideal_interp)
      {
        // if whole integer rates, calculate GCD
        const int in1 = (int)m_sratein, out1 = (int)m_srateout;
        if (out1 > 0 && in1 > 0 && m_sratein == (double)in1 && m_srateout == (double)out1)
        {
          // don't bother finding the GCD if it's lower than is useful
          int min_cd =  out1 / (2*wantinterp);
          if (min_cd < 1) min_cd = 1;

          int n1 = out1, n2=in1;
          while (n2 >= min_cd)
          {
            const int tmp = n1;
            n1 = n2;
            n2 = tmp % n2;
          }
          if (!n2)
            ideal_interp = out1 / n1;
        }
      }
      if (ideal_interp > 0 && m_fracpos != 0.0)
      {
        // should maybe see if m_fracpos maps losslessly to ideal_interp
        ideal_interp = -2;
      }
      m_sinc_ideal_calced = ideal_interp;
    }
    else
      ideal_interp = m_sinc_ideal_calced;

    if (ideal_interp > 0 && ideal_interp <= wantinterp*2) // use ideal filter for reduced cpu use even if it means more memory
    {
      wantinterp = ideal_interp;
    }
  }

  *isIdeal = ideal_interp == wantinterp;
  if (m_filter_ratio!=filtpos || 
      m_filter_coeffs_size != wantsize ||
      m_lp_oversize != wantinterp)
  {
    m_lp_oversize = wantinterp;
    m_filter_ratio=filtpos;

    // build lowpass filter
    const int allocsize = wantsize*(m_lp_oversize+1);
    const int alignedsize = allocsize + 16/sizeof(WDL_SincFilterSample) - 1;
    if (m_filter_coeffs.ResizeOK(alignedsize))
    {
      WDL_SincFilterSample *cfout=m_filter_coeffs.GetAligned(16);
      m_filter_coeffs_size=wantsize;

      const double dwindowpos = 2.0 * PI/(double)wantsize;
      const double dsincpos  = PI * filtpos; // filtpos is outrate/inrate, i.e. 0.5 is going to half rate
      const int hwantsize=wantsize/2, hwantinterp=wantinterp/2;

      double filtpower=0.0;
      WDL_SincFilterSample *ptrout = cfout;
      int slice;
      for (slice=0;slice<=hwantinterp;slice++)
      {
        const double frac = slice / (double)wantinterp;
        const int center_x = slice == 0 ? hwantsize : -1;

        const int n = ((slice < hwantinterp) | (wantinterp & 1)) ? wantsize : hwantsize;
        int x;
        for (x=0;x<n;x++)
        {          
          if (x==center_x) 
          {
            // we know this will be 1.0
            *ptrout++ = 1.0;
          }
          else
          {
            const double xfrac = frac + x;
            const double windowpos = dwindowpos * xfrac;
            const double sincpos = dsincpos * (xfrac - hwantsize);

            // blackman-harris * sinc
            const double val = (0.35875 - 0.48829 * cos(windowpos) + 0.14128 * cos(2*windowpos) - 0.01168 * cos(3*windowpos)) * sin(sincpos) / sincpos; 
            filtpower += slice ? val*2 : val;
            *ptrout++ = (WDL_SincFilterSample)val;
          }

        }
      }

      filtpower = wantinterp/(filtpower+1.0);
      const int n = allocsize/2;
      int x;
      for (x = 0; x < n; x ++)
      {
        cfout[x] = (WDL_SincFilterSample) (cfout[x]*filtpower);
      }

      int y;
      for (x = n, y = n - 1; y >= 0; ++x, --y) cfout[x] = cfout[y];
    }
    else m_filter_coeffs_size=0;

  }
  return m_filter_coeffs_size > 0 ? m_filter_coeffs.GetAligned(16) : NULL;
}

double WDL_Resampler::GetCurrentLatency() 
{ 
  double v=((double)m_samples_in_rsinbuf-m_filtlatency)/m_sratein;
  
  if (v<0.0)v=0.0;
  return v;
}

int WDL_Resampler::ResamplePrepare(int out_samples, int nch, WDL_ResampleSample **inbuffer) 
{   
  if (nch < 1) return 0;

  if (m_prepost_filtercnt > 0)
  {
    if (!m_pre_filter) m_pre_filter = new WDL_Resampler_Filter;
    m_pre_filter->setParms(1.0/m_ratio,m_filterpos,m_filterq, m_prepost_filtercnt,nch);
    if (!m_post_filter) m_post_filter = new WDL_Resampler_Filter;
    m_post_filter->setParms(m_ratio,m_filterpos,m_filterq, m_prepost_filtercnt,nch);
  }

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
  if (nch < 1) return 0;
#ifdef WDL_DENORMAL_WANTS_SCOPED_FTZ
  WDL_denormal_ftz_scope ftz_force;
#endif

  if (m_pre_filter && nsamples_in > 0) // filter input
  {
    m_pre_filter->ApplyBuffer(m_rsinbuf.Get() + m_samples_in_rsinbuf*nch,nsamples_in,nch);
  }

  // prevent the caller from corrupting the internal state
  m_samples_in_rsinbuf += nsamples_in < m_last_requested ? nsamples_in : m_last_requested; 

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

  bool isideal = false;
  if (m_sincsize) // sinc interpolating
  {
    const WDL_SincFilterSample *filter;
    if (m_ratio > 1.0) filter=BuildLowPass(1.0 / (m_ratio*1.03), &isideal);
    else filter=BuildLowPass(1.0, &isideal);

    const int oversize = m_lp_oversize;
    int filtsz=m_filter_coeffs_size;
    int filtlen = rsinbuf_availtemp - filtsz;
    outlatadj=filtsz/2-1;

    if (WDL_NOT_NORMALLY(!filter)) {} 
    else if (nch == 1)
    {
      if (isideal)
        while (ns--)
        {
          int ipos = (int)srcpos;

          if (ipos >= filtlen-1)  break; // quit decoding, not enough input samples

          SincSample1N(outptr,localin + ipos,srcpos-ipos,filter,filtsz,oversize);
          outptr ++;
          srcpos+=drspos;
          ret++;
        }
      else
        while (ns--)
        {
          int ipos = (int)srcpos;

          if (ipos >= filtlen-1)  break; // quit decoding, not enough input samples

          SincSample1(outptr,localin + ipos,srcpos-ipos,filter,filtsz,oversize);
          outptr ++;
          srcpos+=drspos;
          ret++;
        }
    }
    else if (nch==2)
    {
      if (isideal)
        while (ns--)
        {
          int ipos = (int)srcpos;

          if (ipos >= filtlen-1) break; // quit decoding, not enough input samples

          SincSample2N(outptr,localin + ipos*2,srcpos-ipos,filter,filtsz,oversize);
          outptr+=2;
          srcpos+=drspos;
          ret++;
        }
      else 
        while (ns--)
        {
          int ipos = (int)srcpos;

          if (ipos >= filtlen-1) break; // quit decoding, not enough input samples

          SincSample2(outptr,localin + ipos*2,srcpos-ipos,filter,filtsz,oversize);
          outptr+=2;
          srcpos+=drspos;
          ret++;
        }
    }
    else
    {
      if (isideal)
        while (ns--)
        {
          int ipos = (int)srcpos;

          if (ipos >= filtlen-1)  break; // quit decoding, not enough input samples

          SincSampleN(outptr,localin + ipos*nch,srcpos-ipos,nch,filter,filtsz,oversize);
          outptr += nch;
          srcpos+=drspos;
          ret++;
        }
      else
        while (ns--)
        {
          int ipos = (int)srcpos;

          if (ipos >= filtlen-1)  break; // quit decoding, not enough input samples

          SincSample(outptr,localin + ipos*nch,srcpos-ipos,nch,filter,filtsz,oversize);
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


  if (ret > 0 && m_post_filter) // filter output
  {
    m_post_filter->ApplyBuffer(out,ret,nch);
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
  if (isrcpos > m_samples_in_rsinbuf) isrcpos=m_samples_in_rsinbuf;
  m_fracpos = srcpos - isrcpos;

  if (m_sincsize && isideal)
    m_fracpos = floor(m_lp_oversize*m_fracpos + 0.5)/m_lp_oversize;

  m_samples_in_rsinbuf -= isrcpos;
  if (m_samples_in_rsinbuf <= 0) m_samples_in_rsinbuf=0;
  else
    memmove(localin, localin + isrcpos*nch,m_samples_in_rsinbuf*sizeof(WDL_ResampleSample)*nch);


  return ret;
}
