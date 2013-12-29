#include "timing.h"

#ifdef TIMING

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

static struct {
	__int64 st_time;
	__int64 cycles,mint,maxt;
  int foo;
	int calls;
} timingInfo[64];


static void rdtsc(__int64 *t)
{
#ifdef _WIN64
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);
  *t = now.QuadPart;
#elif !defined(_WIN32)
  struct timeval tm={0,};
  gettimeofday(&tm,NULL);
  *t = ((__int64)tm.tv_sec)*1000000 + (__int64)tm.tv_usec;
#else
	__asm 
	{
		mov esi, t
		_emit 0xf
		_emit 0x31
		mov [esi], eax
		mov [esi+4], edx
	}
#endif
}
	
void _timingInit(void)
{
	memset(timingInfo,0,sizeof(timingInfo));
}

void _timingEnter(int which)
{
  rdtsc(&timingInfo[which].st_time);
}

void _timingLeave(int which)
{
  __int64 t;
  rdtsc(&t);
  t -= timingInfo[which].st_time;
  if (!timingInfo[which].mint || t < timingInfo[which].mint) timingInfo[which].mint=t?t:1;
  if (t > timingInfo[which].maxt) timingInfo[which].maxt=t;
  timingInfo[which].cycles += t;
  timingInfo[which].calls += 1;
}

void _timingPrint(void)
{
	int x,p=0;
	for (x = 0; x < sizeof(timingInfo)/sizeof(timingInfo[0]); x ++)
	{
    char buf[512];
		if (timingInfo[x].calls)
    {
      p++;
			sprintf(buf,"%d: %d calls, %.4f clocks/call (min=%.4f, max=%.4f). %.8f thingies of CPU time spent\n",
      x,timingInfo[x].calls,(timingInfo[x].cycles/(double)timingInfo[x].calls),
      (double)timingInfo[x].mint,(double)timingInfo[x].maxt,
        (double)timingInfo[x].cycles 
#if defined(_WIN64) || !defined(_WIN32)
          / 1000000.0
#else
        / (2.4*1000.0*1000.0*1000.0)
#endif
          
      );
#ifdef _WIN32
      OutputDebugString(buf);
#else
      printf("%s",buf);
#endif
    }
	}
#ifdef _WIN32
  if (!p) OutputDebugString("no calls to timing lib\n");
#else
  if (!p) printf("no calls to timing lib\n");
#endif
  
	timingInit();
}

#endif
