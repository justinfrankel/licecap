
/*
 *  capturewindow.mm
 *  licecap
 *
 *  Created by Justin on 11/7/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "../WDL/swell/swell.h"
#include "../WDL/lice/lice.h"

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

bool GetScreenDataOld(int xpos, int ypos, LICE_IBitmap *bmOut)
{
#if 0
  NSRect rect=NSMakeRect(xpos,ypos,bmOut->getWidth(),bmOut->getHeight());
  NSRect rect0=NSMakeRect(0,0,rect.size.width,rect.size.height);
  NSWindow *w = [[NSWindow alloc] initWithContentRect:rect
                                            styleMask:NSBorderlessWindowMask backing:NSBackingStoreNonretained defer:NO];
  [w setBackgroundColor:[NSColor clearColor]];
  [w setLevel:NSScreenSaverWindowLevel + 1];
  [w setHasShadow:NO];
  [w setAlphaValue:0.0f];
  [w orderFront:w];
  [w setContentView:[[[NSView alloc] initWithFrame:rect] autorelease]];
  
  [[w contentView] lockFocus];
  NSBitmapImageRep *r = [[NSBitmapImageRep alloc] initWithFocusedViewRect:[[w contentView] bounds]];  
  [[w contentView] unlockFocus];
  [w orderOut:w];
  [w close];

  NSImage *img=[[NSImage alloc] initWithSize:rect0.size];
  [img addRepresentation:r];
  [r release];
  
#else
	PicHandle picHandle;
	GDHandle mainDevice;
	Rect rect;

  int scrh=CGDisplayPixelsHigh(CGMainDisplayID());

  
	rect.left=xpos;
  rect.top=scrh-ypos -bmOut->getHeight();
  rect.right = xpos+bmOut->getWidth();
  rect.bottom =rect.top +bmOut->getHeight();
	
	// Get the main screen. I may want to add support for multiple screens later
	mainDevice = GetMainDevice();
	
	// Capture the screen into the PicHandle.
	picHandle = OpenPicture(&rect);
	CopyBits((BitMap *)*(**mainDevice).gdPMap, (BitMap *)*(**mainDevice).gdPMap,
           &rect, &rect, srcCopy, 0l);
	ClosePicture();
	
	// Convert the PicHandle into an NSImage
	// First lock the PicHandle so it doesn't move in memory while we copy
	HLock((Handle)picHandle);
	NSImageRep *imageRep = [NSPICTImageRep imageRepWithData:[NSData dataWithBytes:(*picHandle)
                                                             length:GetHandleSize((Handle)picHandle)]];
	HUnlock((Handle)picHandle);
	
	// We can release the PicHandle now that we're done with it
	KillPicture(picHandle);
	
	// Create an image with the representation
	NSImage *img = [[NSImage alloc] initWithSize:[imageRep size]];
	[img addRepresentation:imageRep];  
  
  
#endif
  HDC hdc= bmOut->getDC();
  if (hdc)
  {
    void *ctx=SWELL_GetCtxGC(hdc);
    if (ctx)
    {
      NSRect rect0=NSMakeRect(0,0,bmOut->getWidth(),bmOut->getHeight());
      [NSGraphicsContext saveGraphicsState];
      NSGraphicsContext *gc=[NSGraphicsContext graphicsContextWithGraphicsPort:ctx flipped:NO];
      [NSGraphicsContext setCurrentContext:gc];
      [img drawInRect:rect0 fromRect:rect0 operation:NSCompositeCopy fraction:1.0];
      [NSGraphicsContext restoreGraphicsState];    
    }
  }
  
  
  [img release];
  return true;
  
  
}

static bool hasGLfailed=false;

bool GetScreenData(int xpos, int ypos, LICE_IBitmap *bmOut)
{ 
  if (hasGLfailed) return GetScreenDataOld(xpos,ypos,bmOut);
  CGLContextObj    glContextObj;
  CGLPixelFormatObj pixelFormatObj ;
  long numPixelFormats ;
  CGLPixelFormatAttribute attribs[4] =
  {
    kCGLPFAFullScreen,
    kCGLPFADisplayMask,
    (CGLPixelFormatAttribute)CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID()),
  } ;
  
  
  
  /* Build a full-screen GL context */
  CGLChoosePixelFormat( attribs, &pixelFormatObj, &numPixelFormats );
  if ( pixelFormatObj == NULL )  
  {
    hasGLfailed=true;
    return GetScreenDataOld(xpos,ypos,bmOut);
  }  
  CGLCreateContext( pixelFormatObj, NULL, &glContextObj ) ;
  CGLDestroyPixelFormat( pixelFormatObj ) ;
  if ( glContextObj == NULL ) 
  {
    hasGLfailed=true;
    return GetScreenDataOld(xpos,ypos,bmOut);
  }  
  
  CGLSetCurrentContext( glContextObj ) ;
  CGLSetFullScreen( glContextObj ) ;
  
  glReadBuffer(GL_FRONT);
  
  
  /* Read framebuffer into our bitmap */
  glFinish(); /* Finish all OpenGL commands */
  glPixelStorei(GL_PACK_ALIGNMENT, 4); /* Force 4-byte alignment */
  glPixelStorei(GL_PACK_ROW_LENGTH, bmOut->getRowSpan());
  glPixelStorei(GL_PACK_SKIP_ROWS, 0);
  glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
  
  /*
   * Fetch the data in XRGB format, matching the bitmap context.
   */
  glReadPixels(xpos,ypos, bmOut->getWidth(), bmOut->getHeight(),
               GL_BGRA,
#ifdef __BIG_ENDIAN__
               GL_UNSIGNED_INT_8_8_8_8_REV, // for PPC
#else
               GL_UNSIGNED_INT_8_8_8_8, // for Intel! http://lists.apple.com/archives/quartz-dev/2006/May/msg00100.html
#endif
               bmOut->getBits());
  
  
  
  /* Get rid of GL context */
  CGLSetCurrentContext( NULL );
  CGLClearDrawable( glContextObj ); // disassociate from full screen
  CGLDestroyContext( glContextObj ); // and destroy the context
  
  return true;
}

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
