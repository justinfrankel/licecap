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

#include "../wdlcstring.h"

int SWELL_KeyToASCII(int wParam, int lParam, int *newflags)
{
  return 0;
}

static void getHotSpotForFile(const char *fn, POINT *pt)
{
  FILE *fp = fopen(fn,"rb");
  if (!fp) return;
  unsigned char buf[32];
  if (fread(buf,1,6,fp)==6 && !buf[0] && !buf[1] && buf[2] == 2 && buf[3] == 0 && buf[4] == 1 && buf[5] == 0)
  {
    fread(buf,1,16,fp);
    pt->x = buf[4]|(buf[5]<<8);
    pt->y = buf[6]|(buf[7]<<8);
  }
  fclose(fp);
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
        char buf[1024];
        GetModuleFileName(NULL,buf,sizeof(buf));
        WDL_remove_filepart(buf);
        snprintf_append(buf,sizeof(buf),"/Resources/%s.cur",p->resname);
        GdkPixbuf *pb = gdk_pixbuf_new_from_file(buf,NULL);
        if (pb) 
        {
          getHotSpotForFile(buf,&p->hotspot);
          GdkCursor *curs = gdk_cursor_new_from_pixbuf(gdk_display_get_default(),pb,p->hotspot.x,p->hotspot.y);
          return (p->cachedCursor = (HCURSOR) curs);
        }
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

static int g_swell_mouse_relmode_curpos_x;
static int g_swell_mouse_relmode_curpos_y;
static bool g_swell_mouse_relmode;
int SWELL_ShowCursor(BOOL bShow)
{
  static HCURSOR last_cursor;
  m_curvis_cnt += (bShow?1:-1);
  if (m_curvis_cnt==-1 && !bShow) 
  {
#ifdef SWELL_TARGET_GDK
    gint x1, y1;
    #if SWELL_TARGET_GDK == 3
    GdkDevice *dev = gdk_device_manager_get_client_pointer (gdk_display_get_device_manager (gdk_display_get_default ()));
    gdk_device_get_position (dev, NULL, &x1, &y1);
    #else
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x1, &y1, NULL);
    #endif
    g_swell_mouse_relmode_curpos_x = x1;
    g_swell_mouse_relmode_curpos_y = y1;
    last_cursor = GetCursor();
    SetCursor((HCURSOR)gdk_cursor_new_for_display(gdk_display_get_default(),GDK_BLANK_CURSOR));
    g_swell_mouse_relmode=true;
#endif

  }
  if (m_curvis_cnt==0 && bShow) 
  {
#ifdef SWELL_TARGET_GDK
    SetCursor(last_cursor);
    g_swell_mouse_relmode=false;
    #if SWELL_TARGET_GDK == 3
    gdk_device_warp(gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_display_get_default())),
                     gdk_screen_get_default(),
                     g_swell_mouse_relmode_curpos_x, g_swell_mouse_relmode_curpos_y);
    #else
    gdk_display_warp_pointer(gdk_display_get_default(),gdk_screen_get_default(), g_swell_mouse_relmode_curpos_x, g_swell_mouse_relmode_curpos_y);
    #endif
#endif
  }
  return m_curvis_cnt;
}

BOOL SWELL_SetCursorPos(int X, int Y)
{  
#ifdef SWELL_TARGET_GDK
  if (g_swell_mouse_relmode) return false;
 
  #if SWELL_TARGET_GDK == 3
  gdk_device_warp(gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gdk_display_get_default())),
                     gdk_screen_get_default(),
                     X, Y);
  #else
  gdk_display_warp_pointer(gdk_display_get_default(),gdk_screen_get_default(), X, Y);
  #endif
  return true;
#else
  return false;
#endif
}

HCURSOR SWELL_LoadCursorFromFile(const char *fn)
{
#ifdef SWELL_TARGET_GDK
  GdkPixbuf *pb = gdk_pixbuf_new_from_file(fn,NULL);
  if (pb) 
  {
    POINT hs = {0,};
    getHotSpotForFile(fn,&hs);
    GdkCursor *curs = gdk_cursor_new_from_pixbuf(gdk_display_get_default(),pb,hs.x,hs.y);
    g_object_unref(pb);
    return (HCURSOR) curs;
  }
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
