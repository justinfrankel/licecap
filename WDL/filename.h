/*
    WDL - filename.h
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

#ifndef _WDL_FILENAME_H_
#define _WDL_FILENAME_H_

#include "wdltypes.h"

static WDL_STATICFUNC_UNUSED char WDL_filename_filterchar(char p, char repl='_', bool filterSlashes=true)
{
  if (p == '?'  || 
      p == '*'  ||
      p == ':'  ||
      p == '\"' ||
      p == '|'  ||
      p == '<'  || 
      p == '>') 
  {
    return repl;
  }

  if (filterSlashes && (p == '/' || p == '\\' ))
  {
    return repl;
  }

  return p;
}

// used for filename portion, typically (not whole filenames)
static WDL_STATICFUNC_UNUSED void WDL_filename_filterstr(char *str, char repl='_', bool filterSlashes=true)
{
  const char *rd = str;
  char *wr = str;
  while (*rd)
  {
    char r=WDL_filename_filterchar(*rd++,repl,filterSlashes);
    if (r) *wr++ = r;
  }
  *wr=0;
}



#endif // _WDL_FILENAME_H_
