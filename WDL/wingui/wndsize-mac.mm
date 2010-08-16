/*
    WDL - wndsize-mac.mm
    Copyright (C) 2004 and later Cockos Incorporated

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

    Code to enable wndsize.h on OS X
      
*/


#include "wndsize.h"
#import <Cocoa/Cocoa.h>

#if 0
HWND WDL_WndSizer::NSView_GetSubViewByTag(HWND hwnd, int tag)
{
  NSView *v=(NSView *)hwnd;
  if (!v) return 0;
  return (HWND) [v viewWithTag:tag];
}

void WDL_WndSizer::NSView_GetBounds(HWND hwnd, RECT *r)
{
  NSView *v=(NSView *)hwnd;
  if (!v) 
  {
    r->left=r->top=r->right=r->bottom=0;
    return;
  }
  NSRect b=[v bounds];
  r->left=r->top=0;
  r->right=(int)(ceil(b.size.width));
  r->bottom=(int)(ceil(b.size.height));
}

void WDL_WndSizer::NSView_GetFrame(HWND hwnd, RECT *r)
{
  NSView *v=(NSView *)hwnd;
  if (!v) 
  {
    r->left=r->top=r->right=r->bottom=0;
    return;
  }
  NSRect b=[v frame];
  r->left=(int)ceil(b.origin.x);
  r->left=(int)ceil(b.origin.y);
  r->right=(int)(ceil(b.size.width+b.origin.x));
  r->bottom=(int)(ceil(b.size.height+b.origin.y));
}

void WDL_WndSizer::NSView_SetFrame(HWND hwnd, RECT *r)
{
  NSView *v=(NSView *)hwnd;
  if (!v) return;
  
  [v setFrame:NSMakeRect(r->left,r->top,r->right-r->left,r->bottom-r->top)];
}

#endif
