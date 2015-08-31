/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Linux?
   Copyright (C) 2006-2007, Cockos, Inc.

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
  

    This file provides basic key and mouse cursor querying, as well as a key to windows key translation function.

  */

#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-dlggen.h"

int SWELL_KeyToASCII(int wParam, int lParam, int *newflags)
{
  return 0;
}


SWELL_CursorResourceIndex *SWELL_curmodule_cursorresource_head;

HCURSOR SWELL_LoadCursor(const char *_idx)
{
  return NULL;
}

static HCURSOR m_last_setcursor;

void SWELL_SetCursor(HCURSOR curs)
{
  m_last_setcursor=curs;
  // todo
}

HCURSOR SWELL_GetCursor()
{
  return m_last_setcursor;
}
HCURSOR SWELL_GetLastSetCursor()
{
  return m_last_setcursor;
}




static int m_curvis_cnt;
bool SWELL_IsCursorVisible()
{
  return m_curvis_cnt>=0;
}
int SWELL_ShowCursor(BOOL bShow)
{
  m_curvis_cnt += (bShow?1:-1);
  if (m_curvis_cnt==-1 && !bShow) 
  {
  }
  if (m_curvis_cnt==0 && bShow) 
  {
  }
  return m_curvis_cnt;
}


BOOL SWELL_SetCursorPos(int X, int Y)
{  

  return false;
}

HCURSOR SWELL_LoadCursorFromFile(const char *fn)
{
  return NULL;
}

#endif
