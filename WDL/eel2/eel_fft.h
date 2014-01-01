#ifndef __EEL_FFT_H_
#define __EEL_FFT_H_

#include "../fft.h"
#if WDL_FFT_REALSIZE != EEL_F_SIZE
#error EEL_FFT_REALSIZE -- EEL_F_SIZE size mismatch
#endif

#ifndef EEL_FFT_MINBITLEN
#define EEL_FFT_MINBITLEN 4
#endif

#ifndef EEL_FFT_MAXBITLEN
#define EEL_FFT_MAXBITLEN 15
#endif


#ifndef EEL_SLOW_FFT_REORDERING

static int *fft_reorder_table_for_bitsize(int bitsz)
{
  static int s_tab[ (2 << EEL_FFT_MAXBITLEN) + 24*(EEL_FFT_MAXBITLEN-EEL_FFT_MINBITLEN+1) ];
  if (bitsz<=EEL_FFT_MINBITLEN) return s_tab;
  return s_tab + (1<<bitsz) + (bitsz-EEL_FFT_MINBITLEN) * 24;
}
static void fft_make_reorder_table(int bitsz, int *tab)
{
  const int fft_sz=1<<bitsz;
  char flag[1<<EEL_FFT_MAXBITLEN];
  int x;
  int *tabstart = tab;
  memset(flag,0,fft_sz);

  for (x=0;x<fft_sz;x++)
  {
    int fx;
    if (!flag[x] && (fx=WDL_fft_permute(fft_sz,x))!=x)
    {
      flag[x]=1;
      *tab++ = x*2;
      do
      {
        flag[fx]=1;
        *tab++ = fx*2;
        fx = WDL_fft_permute(fft_sz, fx);
      }
      while (fx != x);
      *tab++ = 0; // delimit a run
    }
    else flag[x]=1;
  }
  *tab++ = 0; // doublenull terminated
}

static void fft_reorder_buffer(int bitsz, EEL_F *data, int fwd)
{
  const int *tab=fft_reorder_table_for_bitsize(bitsz);
  if (!fwd)
  {
    while (*tab)
    {
      const int sidx=*tab++;
      EEL_F a=data[sidx];
      EEL_F b=data[sidx+1];
      for (;;)
      {
        EEL_F ta,tb;
        const int idx=*tab++;
        if (!idx) break;
        ta=data[idx];
        tb=data[idx+1];
        data[idx]=a;
        data[idx+1]=b;
        a=ta;
        b=tb;
      }
      data[sidx] = a;
      data[sidx+1] = b;
    }
  }
  else
  {
    while (*tab)
    {
      const int sidx=*tab++;
      int lidx = sidx;
      const EEL_F sta=data[lidx];
      const EEL_F stb=data[lidx+1];
      for (;;)
      {
        const int idx=*tab++;
        if (!idx) break;

        data[lidx]=data[idx];
        data[lidx+1]=data[idx+1];
        lidx=idx;
      }
      data[lidx] = sta;
      data[lidx+1] = stb;
    }
  }
}

#endif

// 0=fw, 1=iv, 2=fwreal, 3=ireal, 4=permutec, 6=permuter
// low bit: is inverse
// second bit: is real
// third bit: is permute
static void FFT(int sizebits, EEL_F *data, int dir)
{
  const int flen=1<<sizebits;

  if (dir >= 4 && dir < 8)
  {
    int x;
    if (dir == 4 || dir == 5)
    {
#ifndef EEL_SLOW_FFT_REORDERING
      // this in-place reordering is about 2x as fast, probably because of both cache effects as well as alloca() being used
      fft_reorder_buffer(sizebits,data,dir==4);
#else
      EEL_F *tmp=(EEL_F*)alloca(sizeof(EEL_F)*flen*2);
    	const int flen2=flen+flen;
	    // reorder entries, now
      memcpy(tmp,data,sizeof(EEL_F)*flen*2);

      if (dir == 4)
      {
 	      for (x = 0; x < flen2; x += 2)
   	    {
   	      int y=WDL_fft_permute(flen,x/2)*2;
          data[x]=tmp[y];
   	      data[x+1]=tmp[y+1];
        }

      }
      else
      {
 	      for (x = 0; x < flen2; x += 2)
   	    {
   	      int y=WDL_fft_permute(flen,x/2)*2;
          data[y]=tmp[x];
   	      data[y+1]=tmp[x+1];
        }
      }
#endif
    }
  }
	else if (dir >= 0 && dir < 2)
	{
    WDL_fft((WDL_FFT_COMPLEX*)data,1<<sizebits,dir&1);
	}
}



static EEL_F * fft_func(int dir, EEL_F **blocks, EEL_F *start, EEL_F *length)
{
	const int offs = (int)(*start + 0.0001);
  const int itemSizeShift=(dir&2)?0:1;
	int l=(int)(*length + 0.0001);
	int bitl=0;
	int ilen;
	EEL_F *ptr;
	while (l>1 && bitl < EEL_FFT_MAXBITLEN)
	{
		bitl++;
		l>>=1;
	}
	if (bitl < EEL_FFT_MINBITLEN)  // smallest FFT is 16 item
	{ 
		return start; 
	}
	ilen=1<<bitl;


	// check to make sure we don't cross a boundary
	if (offs/NSEEL_RAM_ITEMSPERBLOCK != (offs + (ilen<<itemSizeShift) - 1)/NSEEL_RAM_ITEMSPERBLOCK) 
	{ 
		return start; 
	}

	ptr=__NSEEL_RAMAlloc(blocks,offs);
	if (!ptr || ptr==&nseel_ramalloc_onfail)
	{ 
		return start; 
	}

	FFT(bitl,ptr,dir);

	return start;
}

static EEL_F * NSEEL_CGEN_CALL  eel_fft(EEL_F **blocks, EEL_F *start, EEL_F *length)
{
  return fft_func(0,blocks,start,length);
}

static EEL_F * NSEEL_CGEN_CALL  eel_ifft(EEL_F **blocks, EEL_F *start, EEL_F *length)
{
  return fft_func(1,blocks,start,length);
}

static EEL_F * NSEEL_CGEN_CALL  eel_fft_permute(EEL_F **blocks, EEL_F *start, EEL_F *length)
{
  return fft_func(4,blocks,start,length);
}

static EEL_F * NSEEL_CGEN_CALL  eel_ifft_permute(EEL_F **blocks, EEL_F *start, EEL_F *length)
{
  return fft_func(5,blocks,start,length);
}

static EEL_F * NSEEL_CGEN_CALL eel_convolve_c(EEL_F **blocks,EEL_F *dest, EEL_F *src, EEL_F *lenptr)
{
	const int dest_offs = (int)(*dest + 0.0001);
	const int src_offs = (int)(*src + 0.0001);
  const int len = ((int)(*lenptr + 0.0001)) * 2;
  EEL_F *srcptr,*destptr;

  if (len < 1 || len > NSEEL_RAM_ITEMSPERBLOCK || dest_offs < 0 || src_offs < 0 || 
      dest_offs >= NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK || src_offs >= NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK) return dest;
  if ((dest_offs&(NSEEL_RAM_ITEMSPERBLOCK-1)) + len > NSEEL_RAM_ITEMSPERBLOCK) return dest;
  if ((src_offs&(NSEEL_RAM_ITEMSPERBLOCK-1)) + len > NSEEL_RAM_ITEMSPERBLOCK) return dest;

  srcptr = __NSEEL_RAMAlloc(blocks,src_offs);
  if (!srcptr || srcptr==&nseel_ramalloc_onfail) return dest;
  destptr = __NSEEL_RAMAlloc(blocks,dest_offs);
  if (!destptr || destptr==&nseel_ramalloc_onfail) return dest;


  WDL_fft_complexmul((WDL_FFT_COMPLEX*)destptr,(WDL_FFT_COMPLEX*)srcptr,(len/2)&~1);

  return dest;
}

static void EEL_fft_register()
{
  WDL_fft_init();
#ifndef EEL_SLOW_FFT_REORDERING
  if (!fft_reorder_table_for_bitsize(EEL_FFT_MINBITLEN)[0])
  {
    int x;
    for (x=EEL_FFT_MINBITLEN;x<=EEL_FFT_MAXBITLEN;x++) fft_make_reorder_table(x,fft_reorder_table_for_bitsize(x));
  }
#endif
  NSEEL_addfunctionex("convolve_c",3,(char *)_asm_generic3parm,(char *)_asm_generic3parm_end-(char *)_asm_generic3parm,NSEEL_PProc_RAM,(void*)&eel_convolve_c);

  NSEEL_addfunctionex("fft",2,(char *)_asm_generic2parm,(char *)_asm_generic2parm_end-(char *)_asm_generic2parm,NSEEL_PProc_RAM,(void*)eel_fft);
  NSEEL_addfunctionex("ifft",2,(char *)_asm_generic2parm,(char *)_asm_generic2parm_end-(char *)_asm_generic2parm,NSEEL_PProc_RAM,(void*)eel_ifft);
  NSEEL_addfunctionex("fft_permute",2,(char *)_asm_generic2parm,(char *)_asm_generic2parm_end-(char *)_asm_generic2parm,NSEEL_PProc_RAM,(void*)eel_fft_permute);
  NSEEL_addfunctionex("fft_ipermute",2,(char *)_asm_generic2parm,(char *)_asm_generic2parm_end-(char *)_asm_generic2parm,NSEEL_PProc_RAM,(void*)eel_ifft_permute);
}


#endif
