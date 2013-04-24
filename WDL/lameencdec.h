/*
  WDL - lameencdec.h
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
  

  This file provides a simple interface for using lame_enc.dll on Windows for MP3 encoding and decoding.

*/


#ifndef _LAMEENCDEC_H_
#define _LAMEENCDEC_H_

#include "queue.h"
#include "wdlstring.h"

#ifdef _WIN32
  #ifndef _WIN64
    #define USE_LAME_BLADE_API
  #endif
#endif

#ifndef USE_LAME_BLADE_API
  #define DYNAMIC_LAME

  #ifdef DYNAMIC_LAME
    typedef void *lame_t;
  #else
    #include <lame/lame.h>
  #endif
#endif



class LameEncoder
{
  public:

    LameEncoder(int srate, int nch, int bitrate, int stereomode = 1, int quality = 0, int vbrmethod = -1, int vbrquality = 2, int vbrmax = 320, int abr = 128);
    ~LameEncoder();

    int Status() { return errorstat; } // 1=no dll, 2=error

    void Encode(float *in, int in_spls, int spacing=1);

    WDL_Queue outqueue;

    void reinit() 
    { 
      spltmp[0].Advance(spltmp[0].Available());  
      spltmp[0].Compact();
      spltmp[1].Advance(spltmp[1].Available());  
      spltmp[1].Compact();
    }

    static bool CheckDLL(); // returns true if dll is present
    static void InitDLL(const char *extrapath=NULL); // call with extrapath != NULL if you want to try loading from another path

    void SetVBRFilename(const char *fn)
    {
      m_vbrfile.Set(fn);
    }

    int GetNumChannels() { return m_encoder_nch; }
    
  private:
    int m_nch,m_encoder_nch;
    WDL_Queue spltmp[2];
    WDL_HeapBuf outtmp;
    WDL_String m_vbrfile;
    int errorstat;
    int in_size_samples;
#ifdef USE_LAME_BLADE_API
    UINT_PTR hbeStream;
#else
    lame_t m_lamestate;
#endif
};

class LameDecoder
{
  public:
    LameDecoder();
    ~LameDecoder();

    int GetSampleRate() { return m_srate?m_srate:48000; }
    int GetNumChannels() { return m_nch?m_nch:1; }

    WDL_HeapBuf m_samples; // we let the size get as big as it needs to, so we don't worry about tons of mallocs/etc
    int m_samples_used;

    void *DecodeGetSrcBuffer(int srclen)
    {
      if (srctmp.GetSize() < srclen) srctmp.Resize(srclen);
      return srctmp.Get();
    }
    void DecodeWrote(int srclen);

    int GetError() { return errorstat; }

    void Reset();

    int m_samplesdec;

  private:
    WDL_HeapBuf srctmp;
    int errorstat;
    int m_srate,m_nch;
    int m_samples_remove;

#ifdef USE_LAME_BLADE_API
    void *decinst;
#else
    lame_t *decinst;
#endif
};



#endif
