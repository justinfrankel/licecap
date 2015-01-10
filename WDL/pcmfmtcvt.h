/*
    WDL - pcmfmtcvt.h
    Copyright (C) 2005 and later, Cockos Incorporated
   
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

/*

  This file provides some simple functions for dealing with PCM audio.
  Specifically: 
    + convert between 16/24/32 bit integer samples and flaots (only really tested on little-endian (i.e. x86) systems)
    + mix (and optionally resample, using low quality linear interpolation) a block of floats to another.
 
*/

#ifndef _PCMFMTCVT_H_
#define _PCMFMTCVT_H_


#include "wdltypes.h"

#ifndef PCMFMTCVT_DBL_TYPE
#define PCMFMTCVT_DBL_TYPE double
#endif

static inline int float2int(PCMFMTCVT_DBL_TYPE d)
{
  return (int) d;
//	  int tmp;
  //  __asm__ __volatile__ ("fistpl %0" : "=m" (tmp) : "t" (d) : "st") ;
  //  return tmp;
}


#define float_TO_INT16(out,in) \
		if ((in)<0.0) { if ((in) <= -1.0) (out) = -32768; else (out) = (short) (float2int(((in) * 32768.0)-0.5)); } \
		else { if ((in) >= (32766.5f/32768.0f)) (out) = 32767; else (out) = (short) float2int((in) * 32768.0 + 0.5); }

#define INT16_TO_float(out,in) { (out) = (float)(((double)in)/32768.0);  }

#define double_TO_INT16(out,in) \
		if ((in)<0.0) { if ((in) <= -1.0) (out) = -32768; else (out) = (short) (float2int(((in) * 32768.0)-0.5)); } \
		else { if ((in) >= (32766.5/32768.0)) (out) = 32767; else (out) = (short) float2int((in) * 32768.0 + 0.5); }

#define INT16_TO_double(out,in) { (out) = (((PCMFMTCVT_DBL_TYPE)in)/32768.0); }


static inline void i32_to_float(int i32, float *p)
{
  *p = (float) ((((double) i32)) * (1.0 / (2147483648.0)));
}

static inline void float_to_i32(float *vv, int *i32)
{
  float v = *vv;
  if (v < 0.0) 
  {
	  if (v < -1.0) *i32 = 0x80000000;
	  else *i32=float2int(v*2147483648.0-0.5);
  }
  else
  {
	  if (v >= (2147483646.5f/2147483648.0f)) *i32 = 0x7FFFFFFF;
	  else *i32=float2int(v*2147483648.0+0.5);
  }
}


static inline void i32_to_double(int i32, PCMFMTCVT_DBL_TYPE *p)
{
  *p = ((((PCMFMTCVT_DBL_TYPE) i32)) * (1.0 / (2147483648.0)));
}

static inline void double_to_i32(PCMFMTCVT_DBL_TYPE *vv, int *i32)
{
  PCMFMTCVT_DBL_TYPE v = *vv;
  if (v < 0.0) 
  {
	  if (v < -1.0) *i32 = 0x80000000;
	  else *i32=float2int(v*2147483648.0-0.5);
  }
  else
  {
	  if (v >= (2147483646.5/2147483648.0)) *i32 = 0x7FFFFFFF;
	  else *i32=float2int(v*2147483648.0+0.5);
  }
}



static inline void i24_to_float(unsigned char *i24, float *p)
{
  int val=(i24[0]) | (i24[1]<<8) | (i24[2]<<16);
  if (val&0x800000) 
  {
	  val|=0xFF000000;
  	  *p = (float) ((((double) val)) * (1.0 / (8388608.0)));
  }
  else 
  {
	  val&=0xFFFFFF;
  	  *p = (float) ((((double) val)) * (1.0 / (8388608.0)));
  }

}

static inline void float_to_i24(float *vv, unsigned char *i24)
{
  float v = *vv;
  if (v < 0.0) 
  {
	  if (v < -1.0)
	  {
    		i24[0]=i24[1]=0x00;
    		i24[2]=0x80;
	  }
	  else
	  {
    		int i=float2int(v*8388608.0-0.5);
    		i24[0]=(i)&0xff;
    		i24[1]=(i>>8)&0xff;
    		i24[2]=(i>>16)&0xff;
	  }
  }
  else
  {
	  if (v >= (8388606.5f/8388608.0f))
	  {
    		i24[0]=i24[1]=0xff;
    		i24[2]=0x7f;
	  }
	  else
	  {
  		
    		int i=float2int(v*8388608.0+0.5);
    		i24[0]=(i)&0xff;
    		i24[1]=(i>>8)&0xff;
    		i24[2]=(i>>16)&0xff;
	  }
  }
}


static inline void i24_to_double(unsigned char *i24, PCMFMTCVT_DBL_TYPE *p)
{
  int val=(i24[0]) | (i24[1]<<8) | (i24[2]<<16);
  if (val&0x800000) 
  {
	  val|=0xFF000000;
  	  *p = ((((PCMFMTCVT_DBL_TYPE) val)) * (1.0 / (8388608.0)));
  }
  else 
  {
	  val&=0xFFFFFF;
  	  *p = ((((PCMFMTCVT_DBL_TYPE) val)) * (1.0 / (8388608.0)));
  }

}

static inline void double_to_i24(PCMFMTCVT_DBL_TYPE *vv, unsigned char *i24)
{
  PCMFMTCVT_DBL_TYPE v = *vv;
  if (v < 0.0) 
  {
	  if (v < -1.0)
	  {
    		i24[0]=i24[1]=0x00;
    		i24[2]=0x80;
	  }
	  else
	  {
    		int i=float2int(v*8388608.0-0.5);
    		i24[0]=(i)&0xff;
    		i24[1]=(i>>8)&0xff;
    		i24[2]=(i>>16)&0xff;
	  }
  }
  else
  {
	  if (v >= (8388606.5/8388608.0))
	  {
    		i24[0]=i24[1]=0xff;
    		i24[2]=0x7f;
	  }
	  else
	  {
  		
    		int i=float2int(v*8388608.0+0.5);
    		i24[0]=(i)&0xff;
    		i24[1]=(i>>8)&0xff;
    		i24[2]=(i>>16)&0xff;
	  }
  }
}

static void pcmToFloats(void *src, int items, int bps, int src_spacing, float *dest, int dest_spacing)
{
  if (bps == 32)
  {
    int *i1=(int *)src;
    while (items--)
    {          
      i32_to_float(*i1,dest);
      i1+=src_spacing;
      dest+=dest_spacing;      
    }
  }
  else if (bps == 24)
  {
    unsigned char *i1=(unsigned char *)src;
    int adv=3*src_spacing;
    while (items--)
    {          
      i24_to_float(i1,dest);
      dest+=dest_spacing;
      i1+=adv;
    }
  }
  else if (bps == 16)
  {
    short *i1=(short *)src;
    while (items--)
    {          
      INT16_TO_float(*dest,*i1);
      i1+=src_spacing;
      dest+=dest_spacing;
    }
  }
}

static void floatsToPcm(float *src, int src_spacing, int items, void *dest, int bps, int dest_spacing)
{
  if (bps==32)
  {
    int *o1=(int*)dest;
    while (items--)
    {
      float_to_i32(src,o1);
      src+=src_spacing;
      o1+=dest_spacing;
    }
  }
  else if (bps == 24)
  {
    unsigned char *o1=(unsigned char*)dest;
    int adv=dest_spacing*3;
    while (items--)
    {
      float_to_i24(src,o1);
      src+=src_spacing;
      o1+=adv;
    }
  }
  else if (bps==16)
  {
    short *o1=(short*)dest;
    while (items--)
    {
      float_TO_INT16(*o1,*src);
      src+=src_spacing;
      o1+=dest_spacing;
    }
  }
}


static void pcmToDoubles(void *src, int items, int bps, int src_spacing, PCMFMTCVT_DBL_TYPE *dest, int dest_spacing, int byteadvancefor24=0)
{
  if (bps == 32)
  {
    int *i1=(int *)src;
    while (items--)
    {          
      i32_to_double(*i1,dest);
      i1+=src_spacing;
      dest+=dest_spacing;      
    }
  }
  else if (bps == 24)
  {
    unsigned char *i1=(unsigned char *)src;
    int adv=3*src_spacing+byteadvancefor24;
    while (items--)
    {          
      i24_to_double(i1,dest);
      dest+=dest_spacing;
      i1+=adv;
    }
  }
  else if (bps == 16)
  {
    short *i1=(short *)src;
    while (items--)
    {          
      INT16_TO_double(*dest,*i1);
      i1+=src_spacing;
      dest+=dest_spacing;
    }
  }
}

static void doublesToPcm(PCMFMTCVT_DBL_TYPE *src, int src_spacing, int items, void *dest, int bps, int dest_spacing, int byteadvancefor24=0)
{
  if (bps==32)
  {
    int *o1=(int*)dest;
    while (items--)
    {
      double_to_i32(src,o1);
      src+=src_spacing;
      o1+=dest_spacing;
    }
  }
  else if (bps == 24)
  {
    unsigned char *o1=(unsigned char*)dest;
    int adv=dest_spacing*3+byteadvancefor24;
    while (items--)
    {
      double_to_i24(src,o1);
      src+=src_spacing;
      o1+=adv;
    }
  }
  else if (bps==16)
  {
    short *o1=(short*)dest;
    while (items--)
    {
      double_TO_INT16(*o1,*src);
      src+=src_spacing;
      o1+=dest_spacing;
    }
  }
}

static int resampleLengthNeeded(int src_srate, int dest_srate, int dest_len, double *state)
{
  // safety
  if (!src_srate) src_srate=48000;
  if (!dest_srate) dest_srate=48000;
  if (src_srate == dest_srate) return dest_len;
  return (int) (((double)src_srate*(double)dest_len/(double)dest_srate)+*state);

}

static void mixFloats(float *src, int src_srate, int src_nch,  // lengths are sample pairs
                            float *dest, int dest_srate, int dest_nch, 
                            int dest_len, float vol, float pan, double *state)
{
  // fucko: better resampling, this is shite
  int x;
  if (pan < -1.0f) pan=-1.0f;
  else if (pan > 1.0f) pan=1.0f;
  if (vol > 4.0f) vol=4.0f;
  if (vol < 0.0f) vol=0.0f;

  if (!src_srate) src_srate=48000;
  if (!dest_srate) dest_srate=48000;

  double vol1=vol,vol2=vol;
  if (dest_nch == 2)
  {
    if (pan < 0.0f)  vol2 *= 1.0f+pan;
    else if (pan > 0.0f) vol1 *= 1.0f-pan;
  }

  double rspos=*state;
  double drspos = (double)src_srate/(double)dest_srate;

  for (x = 0; x < dest_len; x ++)
  {

    double ls;
    double rs;
    if (src_srate != dest_srate)
    {
      int ipos = (int)rspos;
      double fracpos=rspos-ipos; 
      if (src_nch == 2)
      {
        ipos+=ipos;
        ls=src[ipos]*(1.0-fracpos) + src[ipos+2]*fracpos;
        rs=src[ipos+1]*(1.0-fracpos) + src[ipos+3]*fracpos;
      }
      else 
      {
        rs=ls=src[ipos]*(1.0-fracpos) + src[ipos+1]*fracpos;
      }
    }
    else
    {
      if (src_nch == 2)
      {
        int t=x+x;
        ls=src[t];
        rs=src[t+1];
      }
      else
      {
        rs=ls=src[x];
      }
    }

    rspos+=drspos;

    ls *= vol1;
    if (ls > 1.0) ls=1.0;
    else if (ls<-1.0) ls=-1.0;

    if (dest_nch == 2)
    {
      rs *= vol2;
      if (rs > 1.0) rs=1.0;
      else if (rs<-1.0) rs=-1.0;

      dest[0]+=(float) ls;
      dest[1]+=(float) rs;
      dest+=2;
    }
    else
    {
      dest[0]+=(float) ls;
      dest++;
    }
  }
  *state = rspos - (int)rspos;
}

static void mixFloatsNIOutput(float *src, int src_srate, int src_nch,  // lengths are sample pairs. input is interleaved samples, output not
                            float **dest, int dest_srate, int dest_nch, 
                            int dest_len, float vol, float pan, double *state)
{
  // fucko: better resampling, this is shite
  int x;
  if (pan < -1.0f) pan=-1.0f;
  else if (pan > 1.0f) pan=1.0f;
  if (vol > 4.0f) vol=4.0f;
  if (vol < 0.0f) vol=0.0f;

  if (!src_srate) src_srate=48000;
  if (!dest_srate) dest_srate=48000;

  double vol1=vol,vol2=vol;
  float *dest1=dest[0];
  float *dest2=NULL;
  if (dest_nch > 1)
  {
    dest2=dest[1];
    if (pan < 0.0f)  vol2 *= 1.0f+pan;
    else if (pan > 0.0f) vol1 *= 1.0f-pan;
  }
  

  double rspos=*state;
  double drspos = 1.0;
  if (src_srate != dest_srate) drspos=(double)src_srate/(double)dest_srate;

  for (x = 0; x < dest_len; x ++)
  {
    double ls,rs;
    if (src_srate != dest_srate)
    {
      int ipos = (int)rspos;
      double fracpos=rspos-ipos; 
      if (src_nch == 2)
      {
        ipos+=ipos;
        ls=src[ipos]*(1.0-fracpos) + src[ipos+2]*fracpos;
        rs=src[ipos+1]*(1.0-fracpos) + src[ipos+3]*fracpos;
      }
      else 
      {
        rs=ls=src[ipos]*(1.0-fracpos) + src[ipos+1]*fracpos;
      }
      rspos+=drspos;

    }
    else
    {
      if (src_nch == 2)
      {
        int t=x+x;
        ls=src[t];
        rs=src[t+1];
      }
      else
      {
        rs=ls=src[x];
      }
    }

    ls *= vol1;
    if (ls > 1.0) ls=1.0;
    else if (ls<-1.0) ls=-1.0;

    *dest1++ +=(float) ls;

    if (dest_nch > 1)
    {
      rs *= vol2;
      if (rs > 1.0) rs=1.0;
      else if (rs<-1.0) rs=-1.0;

      *dest2++ += (float) rs;
    }
  }
  *state = rspos - (int)rspos;
}


#endif //_PCMFMTCVT_H_