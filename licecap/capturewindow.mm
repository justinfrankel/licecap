
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
#ifdef MAC_OS_X_VERSION_10_12 // newer SDKs drop support for these APIs
  return false;
#else
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
  #endif
  
}

static CGEventRef eventCallback(CGEventTapProxy proxy,
                                CGEventType type,
                                CGEventRef event,
                                void *refcon)
{
  if (type == kCGEventLeftMouseDown) { *(int *)refcon |= 0x101; }
  else if (type == kCGEventLeftMouseUp) { *(int *)refcon  &= ~0x001; }
  else if (type == kCGEventRightMouseDown) { *(int *)refcon |= 0x102; }
  else if (type == kCGEventRightMouseUp) { *(int *)refcon  &= ~0x002; }
  else if (type == kCGEventOtherMouseDown) { *(int *)refcon |= 0x104; }
  else if (type == kCGEventOtherMouseUp) { *(int *)refcon  &= ~0x004; }
  
  return event;
}


void DoMouseCursor(LICE_IBitmap *bmOut, int xoffs, int yoffs)
{
  POINT pt;
  GetCursorPos(&pt);
  pt.x += xoffs;
  pt.y += yoffs;
  
  NSCursor *c = NULL;
  NSImage *img = NULL;
  NSPoint hs;
  bool img_needrelease=false;
  
  if ([NSCursor respondsToSelector:@selector(currentSystemCursor)])
  {
    c = (NSCursor *)[NSCursor currentSystemCursor];
  }
  else
  {
    /* //todo: support cursors on 10.4-10.5
    PixMapHandle h=nil;
    Point pt;
    QDGetCursorData(true,&h,&pt);
    if (h)
    {      
      hs.x = pt.h;
      hs.y = pt.v;
      img = [[NSImage alloc] initWithData:[NSData dataWithBytes:*h length:GetHandleSize((Handle)h)]];
      img_needrelease=true;
    }
     */
  }
  
  if (!c && !img) c=[NSCursor arrowCursor]; // currentCursor but thats useless too
  
  if (c)
  {
    img = [c image];
    hs = [c hotSpot];
  }

  if (!img) return;
       
  extern int g_prefs;
  if (g_prefs&4)
  {
    static int clicked;
    static bool triedreg;
    if (!triedreg)
    {
      triedreg=true;
      CFMachPortRef r=CGEventTapCreate(kCGHIDEventTap,kCGHeadInsertEventTap,kCGEventTapOptionListenOnly,
                                       CGEventMaskBit(kCGEventLeftMouseDown)|
                                       CGEventMaskBit(kCGEventRightMouseDown)|
                                       CGEventMaskBit(kCGEventOtherMouseDown)|
                                       CGEventMaskBit(kCGEventLeftMouseUp)|
                                       CGEventMaskBit(kCGEventRightMouseUp)|
                                       CGEventMaskBit(kCGEventOtherMouseUp)                                  
                                       ,
                       eventCallback,&clicked);
      if (r)
      {
        CFRunLoopAddSource(CFRunLoopGetCurrent(),CFMachPortCreateRunLoopSource(NULL,r,0),kCFRunLoopCommonModes);
      }
    }
    
    if (clicked)
    {
      clicked &= 0xf;
      LICE_Circle(bmOut, pt.x+1, pt.y+1, 10.0f, LICE_RGBA(0,0,0,255), 1.0f, LICE_BLIT_MODE_COPY, true);
      LICE_Circle(bmOut, pt.x+1, pt.y+1, 9.0f, LICE_RGBA(255,255,255,255), 1.0f, LICE_BLIT_MODE_COPY, true);
    }
  }
  
  if (img)
  {
    HDC hdc= bmOut->getDC();
    if (hdc)
    {
      void *ctx=SWELL_GetCtxGC(hdc);
      if (ctx)
      {
        NSRect rect0;
        rect0.size = [img size];
        rect0.origin.x = pt.x - hs.x;
        rect0.origin.y = pt.y + hs.y - rect0.size.height;
        [NSGraphicsContext saveGraphicsState];
        NSGraphicsContext *gc=[NSGraphicsContext graphicsContextWithGraphicsPort:ctx flipped:NO];
        [NSGraphicsContext setCurrentContext:gc];
        [img drawInRect:rect0 fromRect:NSMakeRect(0,0,rect0.size.width,rect0.size.height) operation:NSCompositeSourceOver fraction:1.0];
        [NSGraphicsContext restoreGraphicsState];    
      }
    }
  }
  if (img_needrelease) [img release];
}


bool GetScreenDataGL(int xpos, int ypos, LICE_IBitmap *bmOut)
{ 
  static bool hasGLfailed=false;
  if (hasGLfailed) return GetScreenDataOld(xpos,ypos,bmOut);
  CGLContextObj    glContextObj;
  CGLPixelFormatObj pixelFormatObj ;
  GLint numPixelFormats ;
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


bool GetScreenData(int xpos, int ypos, LICE_IBitmap *bmOut)
{
  static bool hasNewFailed=false;
  if (hasNewFailed) return GetScreenDataGL(xpos,ypos,bmOut);

  int use_h=bmOut->getHeight();
  int use_w=bmOut->getWidth();
  
  if (!CGDisplayCreateImageForRect)
  {
    hasNewFailed=true;
    return GetScreenDataGL(xpos,ypos,bmOut);
  }
  
  CGImageRef r=CGDisplayCreateImageForRect(kCGDirectMainDisplay,CGRectMake(xpos,CGDisplayPixelsHigh(CGMainDisplayID()) - ypos - use_h,use_w,use_h));
  if (!r) 
  {
    hasNewFailed=true;
    return GetScreenDataGL(xpos,ypos,bmOut);
  }
  static CGColorSpaceRef cs;
  if (!cs) cs=CGColorSpaceCreateDeviceRGB();
  CGContextRef ctx=CGBitmapContextCreate(bmOut->getBits(),use_w,use_h,8,4*bmOut->getRowSpan(),cs,kCGImageAlphaNoneSkipFirst|kCGBitmapByteOrder32Host);
  if (!ctx)
  {
    hasNewFailed=true;
    CGImageRelease(r);
    return GetScreenDataGL(xpos,ypos,bmOut);
  }
  CGContextScaleCTM(ctx,1.0,-1.0);
  CGContextDrawImage(ctx,CGRectMake(0,-use_h,use_w,use_h),r);
  
  CGContextRelease(ctx);
  CGImageRelease(r);
                        
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

@interface tmpNSWindow
-(NSUInteger) styleMask;
-(void) setStyleMask:(NSUInteger)mask;
@end

void SWELL_SetWindowResizeable(HWND h, bool allow)
{
  NSWindow *w = [(NSView *)h window];
  if ([w respondsToSelector:@selector(setStyleMask:)])
  {
    NSUInteger a = [(tmpNSWindow*)w styleMask];
    NSUInteger oa=a;
    if (allow) a |= NSResizableWindowMask;
    else a &= ~NSResizableWindowMask;
    if (a != oa) [(tmpNSWindow*)w setStyleMask:a];
  }
}

extern "C" {
  extern int _CGSDefaultConnection()  __attribute__((weak_import));
  extern CGError CGSGetScreenRectForWindow(int cid, int wid, CGRect *outRect)  __attribute__((weak_import));
};

bool GetAsyncNSWindowRect(HWND hwnd, RECT *r)
{
  if (!_CGSDefaultConnection || !CGSGetScreenRectForWindow) return false;

  int idx;
  if ([(id)hwnd isKindOfClass:[NSView class]]) idx = [[(NSView *)hwnd window] windowNumber];
  else if ([(id)hwnd isKindOfClass:[NSWindow class]]) idx = [(NSWindow *)hwnd windowNumber];
  else return false;


  CGRect rr;
  CGSGetScreenRectForWindow(_CGSDefaultConnection(),idx,&rr);

  int scrh=CGDisplayPixelsHigh(CGMainDisplayID());

  r->left = (int)rr.origin.x;
  r->top = scrh - (int)rr.origin.y;
  r->right = (int)rr.origin.x + rr.size.width;
  r->bottom = scrh - ((int)rr.origin.y + rr.size.height);
  return true;

}

