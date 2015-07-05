#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-internal.h"

bool IsRightClickEmulateEnabled()
{
  return false;
}

void SWELL_EnableRightClickEmulate(BOOL enable)
{
}
HANDLE SWELL_CreateProcess(const char *exe, int nparams, const char **params)
{
  return 0; // todo
}


#endif
