#include "timing.h"

#ifdef TIMING
#ifndef __alpha

#include <stdio.h>

static struct {
	unsigned __int64 st_time;
	__int64 cycles;
	int calls;
} timingInfo[64];

static int timingEnters;

static void rdtsc(__int64 *t)
{
	__asm 
	{
		mov esi, t
		_emit 0xf
		_emit 0x31
		mov [esi], eax
		mov [esi+4], edx
	}
}
	
void _timingInit(void)
{
	memset(timingInfo,0,sizeof(timingInfo));
}

void _timingEnter(int which)
{
 //  if (!timingEnters++) __asm cli
   rdtsc(&timingInfo[which].st_time);
}

void _timingLeave(int which)
{
   __int64 t;
   rdtsc(&t);
//   if (!--timingEnters) __asm sti
   timingInfo[which].cycles += t-timingInfo[which].st_time;
   timingInfo[which].calls++;
}

void _timingPrint(void)
{
	int x;
	FILE *fp = fopen("C:\\timings.txt","wt");
	for (x = 0; x < sizeof(timingInfo)/sizeof(timingInfo[0]); x ++)
	{
		if (timingInfo[x].calls)
			fprintf(fp,"%d: %d calls, %d clocks/call. %.2f seconds of CPU time spent (2.4ghz)\n",
      x,timingInfo[x].calls,(int)(timingInfo[x].cycles/timingInfo[x].calls),
        (double)timingInfo[x].cycles / (2.4*1000.0*1000.0*1000.0)
          
      );
	}
	timingInit();
	fclose(fp);
}

#endif
#endif
