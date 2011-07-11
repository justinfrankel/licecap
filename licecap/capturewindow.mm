
/*
 *  capturewindow.mm
 *  licecap
 *
 *  Created by Justin on 11/7/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "../WDL/swell/swell.h"

void DrawTransparentRectInCurrentContext(RECT r)
{
  [[NSColor clearColor] set];
  NSRectFill(NSMakeRect(r.left,r.top,r.right-r.left,r.bottom-r.top));
}

void SetNSWindowOpaque(HWND h, bool op)
{
  NSWindow *w = (NSWindow *)h;
  if ([w isKindOfClass:[NSView class]]) w = [(NSView *)w window];
  else if (![w isKindOfClass:[NSWindow class]]) return;
  
  if (w) [w setOpaque:op];
}

void RefreshWindowShadows(HWND h)
{
  NSWindow *w = (NSWindow *)h;
  if ([w isKindOfClass:[NSView class]]) w = [(NSView *)w window];
  else if (![w isKindOfClass:[NSWindow class]]) return;
  [w display];
  if ([w hasShadow])
  {
    [w setHasShadow:NO];
    [w setHasShadow:YES];
  }
}
