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
#include "swell-internal.h"

int SWELL_KeyToASCII(int wParam, int lParam, int *newflags)
{
  return 0;
}


static SWELL_CursorResourceIndex *SWELL_curmodule_cursorresource_head;

HCURSOR SWELL_LoadCursor(const char *_idx)
{
#ifdef SWELL_TARGET_GDK

  GdkCursorType def = GDK_LEFT_PTR;
  if (_idx == IDC_NO) def = GDK_PIRATE;
  else if (_idx == IDC_SIZENWSE) def = GDK_BOTTOM_LEFT_CORNER;
  else if (_idx == IDC_SIZENESW) def = GDK_BOTTOM_RIGHT_CORNER;
  else if (_idx == IDC_SIZEALL) def = GDK_FLEUR;
  else if (_idx == IDC_SIZEWE) def =  GDK_RIGHT_SIDE;
  else if (_idx == IDC_SIZENS) def = GDK_TOP_SIDE;
  else if (_idx == IDC_ARROW) def = GDK_LEFT_PTR;
  else if (_idx == IDC_HAND) def = GDK_HAND1;
  else if (_idx == IDC_UPARROW) def = GDK_CENTER_PTR;
  else if (_idx == IDC_IBEAM) def = GDK_XTERM;
  else 
  {
    SWELL_CursorResourceIndex *p = SWELL_curmodule_cursorresource_head;
    while (p)
    {
      if (p->resid == _idx)
      {
        if (p->cachedCursor) return p->cachedCursor;
        // todo: load from p->resname, into p->cachedCursor, p->hotspot
      

      }
      p=p->_next;
    }
  }

  HCURSOR hc= (HCURSOR)gdk_cursor_new_for_display(gdk_display_get_default(),def);

  return hc;
#endif
  return NULL;
}

static HCURSOR m_last_setcursor;

void SWELL_SetCursor(HCURSOR curs)
{
  if (m_last_setcursor == curs) return;

  m_last_setcursor=curs;
#ifdef SWELL_TARGET_GDK
  extern GdkWindow *SWELL_g_focus_oswindow;
  if (SWELL_g_focus_oswindow) 
  {
    gdk_window_set_cursor(SWELL_g_focus_oswindow,(GdkCursor *)curs);
  }
#endif
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
  static HCURSOR last_cursor;
  m_curvis_cnt += (bShow?1:-1);
  if (m_curvis_cnt==-1 && !bShow) 
  {
#ifdef SWELL_TARGET_GDK
    last_cursor = GetCursor();
    SetCursor((HCURSOR)gdk_cursor_new_for_display(gdk_display_get_default(),GDK_BLANK_CURSOR));
#endif

  }
  if (m_curvis_cnt==0 && bShow) 
  {
#ifdef SWELL_TARGET_GDK
    SetCursor(last_cursor);
#endif
  }
  return m_curvis_cnt;
}


BOOL SWELL_SetCursorPos(int X, int Y)
{  

  return false;
}

HCURSOR SWELL_LoadCursorFromFile(const char *fn)
{
#ifdef SWELL_TARGET_GDK
  // todo
#endif

  return NULL;
}

void SWELL_Register_Cursor_Resource(const char *idx, const char *name, int hotspot_x, int hotspot_y)
{
  SWELL_CursorResourceIndex *ri = (SWELL_CursorResourceIndex*)malloc(sizeof(SWELL_CursorResourceIndex));
  ri->hotspot.x = hotspot_x;
  ri->hotspot.y = hotspot_y;
  ri->resname=name;
  ri->cachedCursor=0;
  ri->resid = idx;
  ri->_next = SWELL_curmodule_cursorresource_head;
  SWELL_curmodule_cursorresource_head = ri;
}

#endif
