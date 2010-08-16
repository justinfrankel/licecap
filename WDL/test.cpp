#include <windows.h>
#include <stdio.h>
#include "denormal.h"



double blah(double a)
{
  return denormal_filter_double(a);
}

#if 1

int main()
{
  DWORD d=timeGetTime();
  int x;
  double a[1024] = { 1.00000000001 };
  for (x=0;x<1000000000;x++)
  {
   a[x&1023]=blah(a[x&511]*a[0] - 0.0000000000000001);
  }

  printf("%f\n",a[3]);
  printf("%d ms\n",timeGetTime()-d);
  return 0;
}
#endif

