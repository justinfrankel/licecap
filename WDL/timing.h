/*
  WDL - timing.h

  this is based on some public domain Pentium RDTSC timing code from usenet in 1996.

  To enable this, your app must #define TIMING, include timing.h, call timingInit(), then timingEnter(x)/timingLeave(x) a bunch
  of times (where x is 0-64), then timingPrint at the end. 

  on timingPrint(), C:\\timings.txt will be overwritten.

*/

#ifndef _TIMING_H_
#define _TIMING_H_


//#define TIMING



#if defined(TIMING)
#ifdef __cplusplus
extern "C" {
#endif
void _timingEnter(int);
void _timingLeave(int);
#ifdef __cplusplus
}
#endif
#define timingLeave(x) _timingLeave(x)
#define timingEnter(x) _timingEnter(x)
#else
#define timingLeave(x)
#define timingEnter(x)
#endif

#define timingPrint()
#define timingInit()

#endif
