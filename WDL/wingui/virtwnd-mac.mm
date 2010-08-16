/*
    WDL - virtwnd-mac.mm
    Copyright (C) 2006 and later Cockos Incorporated

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
      

    Implementation of a couple functions to interface with Cocoa (since these use objective C they need to be here)

*/

#include "virtwnd.h"

int WDL_VirtualWnd_ChildList::Mac_SendCommand(HWND hwnd, int msg, int parm1, int parm2, WDL_VirtualWnd *src)
{
  id v=(id)hwnd;
  if (!v) return 0;
  return (int) [v virtWndCommand:msg p1:parm1 p2:parm2 sender:src];
}

void WDL_VirtualWnd_ChildList::Mac_Invalidate(HWND hwnd, RECT *r)
{
  NSView *v=(NSView *)hwnd;
  if (!v) return;
  [v setNeedsDisplayInRect:NSMakeRect(r->left,r->top,r->right-r->left,r->bottom-r->top)]; 
}
