/*
  WDL - convoengine.h
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
  

  This file provides an interface to the WDL fast convolution engine. This engine can convolve audio streams using
  either brute force (for small impulses), or a partitioned FFT scheme (for larger impulses). 

  Note that this library needs to have lookahead ability in order to process samples. Calling Add(somevalue) may produce Avail() < somevalue.

*/


#ifndef _WDL_CONVOENGINE_H_
#define _WDL_CONVOENGINE_H_

#include "queue.h"
#include "fastqueue.h"
#include "fft.h"

//#define WDL_CONVO_WANT_FULLPRECISION_IMPULSE_STORAGE // define this for slowerness with -138dB error difference in resulting output (+-1 LSB at 24 bit)

#ifdef WDL_CONVO_WANT_FULLPRECISION_IMPULSE_STORAGE 

typedef WDL_FFT_REAL WDL_CONVO_IMPULSEBUFf;
typedef WDL_FFT_COMPLEX WDL_CONVO_IMPULSEBUFCPLXf;

#else
typedef float WDL_CONVO_IMPULSEBUFf;
typedef struct
{
  WDL_CONVO_IMPULSEBUFf re, im;
}
WDL_CONVO_IMPULSEBUFCPLXf;
#endif

class WDL_ImpulseBuffer
{
public:
  WDL_ImpulseBuffer()
  {
    samplerate=44100.0;
    impulses.list.Add(new WDL_TypedBuf<WDL_FFT_REAL>);
  }
  ~WDL_ImpulseBuffer()
  {
    impulses.list.Empty(true);
  }

  int GetLength() { return impulses.list.GetSize() ? impulses[0].GetSize() : 0; }
  int SetLength(int samples); // resizes/clears all channels accordingly, returns actual size set (can be 0 if error)
  void SetNumChannels(int usench, bool duplicateExisting=true); // handles allocating/converting/etc
  int GetNumChannels() { return impulses.list.GetSize(); }

  double samplerate;
  struct  {
    WDL_PtrList<WDL_TypedBuf<WDL_FFT_REAL>  > list;

    WDL_TypedBuf<WDL_FFT_REAL> &operator[](size_t i) const
    {
      WDL_TypedBuf<WDL_FFT_REAL> *p = list.Get(i);
      if (WDL_NORMALLY(p != NULL)) return *p;
      return *list.Get(0); // if for some reason an out of range index was passed, return first item rather than crash
    }
  } impulses;

};

class WDL_ConvolutionEngine
{
public:
  WDL_ConvolutionEngine();
  ~WDL_ConvolutionEngine();

  int SetImpulse(WDL_ImpulseBuffer *impulse, int fft_size=-1, int impulse_sample_offset=0, int max_imp_size=0, bool forceBrute=false);
 
  int GetFFTSize() { return m_fft_size; }
  int GetLatency() { return m_fft_size/2; }
  
  void Reset(); // clears out any latent samples

  void Add(WDL_FFT_REAL **bufs, int len, int nch);

  int Avail(int wantSamples);
  WDL_FFT_REAL **Get(); // returns length valid
  void Advance(int len);

private:

  struct ImpChannelInfo {
    WDL_TypedBuf<WDL_CONVO_IMPULSEBUFf> imp;
    WDL_TypedBuf<char> zflag;
  };

  struct ProcChannelInfo {
    WDL_Queue samplesout;
    WDL_Queue samplesin2;
    WDL_FastQueue samplesin;

    WDL_TypedBuf<WDL_FFT_REAL> samplehist; // FFT'd sample blocks per channel
    WDL_TypedBuf<char> samplehist_zflag;
    WDL_TypedBuf<WDL_FFT_REAL> overlaphist;

    int hist_pos;
  };


  WDL_PtrList<ImpChannelInfo> m_impdata;

  int m_impulse_len;
  int m_fft_size;

  int m_proc_nch;
  WDL_PtrList<ProcChannelInfo> m_proc;


  WDL_TypedBuf<WDL_FFT_REAL> m_combinebuf;
  WDL_TypedBuf<WDL_FFT_REAL *> m_get_tmpptrs;

public:

  // _div stuff
  int m_zl_delaypos;
  int m_zl_dumpage;

//#define WDLCONVO_ZL_ACCOUNTING
#ifdef WDLCONVO_ZL_ACCOUNTING
  int m_zl_fftcnt;//removeme (testing of benchmarks)
#endif
  void AddSilenceToOutput(int len);

} WDL_FIXALIGN;

// low latency version
class WDL_ConvolutionEngine_Div
{
public:
  WDL_ConvolutionEngine_Div();
  ~WDL_ConvolutionEngine_Div();

  int SetImpulse(WDL_ImpulseBuffer *impulse, int maxfft_size=0, int known_blocksize=0, int max_imp_size=0, int impulse_offset=0, int latency_allowed=0);

  int GetLatency();
  void Reset();

  void Add(WDL_FFT_REAL **bufs, int len, int nch);

  int Avail(int wantSamples);
  WDL_FFT_REAL **Get(); // returns length valid
  void Advance(int len);

private:
  WDL_PtrList<WDL_ConvolutionEngine> m_engines;

  WDL_PtrList<WDL_Queue> m_sout;
  WDL_TypedBuf<WDL_FFT_REAL *> m_get_tmpptrs;

  bool m_need_feedsilence;

} WDL_FIXALIGN;


#endif
