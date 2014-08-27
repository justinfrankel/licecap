#include "IGraphicsCocoa.h"

inline NSRect ToNSRect(IGraphics* pGraphics, IRECT* pR) 
{
  int B = pGraphics->Height() - pR->B;
  return NSMakeRect(pR->L, B, pR->W(), pR->H()); 
}

inline IRECT ToIRECT(IGraphics* pGraphics, NSRect* pR) 
{
  int x = pR->origin.x, y = pR->origin.y, w = pR->size.width, h = pR->size.height, gh = pGraphics->Height();
  return IRECT(x, gh - (y + h), x + w, gh - y);
}

NSString* ToNSString(const char* cStr)
{
  return [NSString stringWithCString:cStr];
}

inline IMouseMod GetMouseMod(NSEvent* pEvent)
{
  int mods = [pEvent modifierFlags];
  return IMouseMod(true, (mods & NSRightMouseDownMask) || (mods & NSCommandKeyMask), (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));
}

@implementation IGRAPHICS_COCOA

- (id) init
{
  TRACE;
  
  mGraphics = 0;
  mTimer = 0;
  return self;
}

- (id) initWithIGraphics: (IGraphicsMac*) pGraphics
{
  TRACE;
   
  mGraphics = pGraphics;
  NSRect r;
  r.origin.x = r.origin.y = 0.0f;
  r.size.width = (float) pGraphics->Width();
  r.size.height = (float) pGraphics->Height();
  self = [super initWithFrame:r];

  double sec = 1.0 / (double) pGraphics->FPS();
  mTimer = [NSTimer timerWithTimeInterval:sec target:self selector:@selector(onTimer:) userInfo:nil repeats:YES];  
  [[NSRunLoop currentRunLoop] addTimer: mTimer forMode: (NSString*) kCFRunLoopCommonModes];
  
  return self;
}

- (BOOL) isOpaque
{
  return YES;
}

- (BOOL) acceptsFirstResponder
{
  return YES;
}

- (BOOL) acceptsFirstMouse: (NSEvent*) pEvent
{
  return YES;
}

- (void) viewDidMoveToWindow
{
  NSWindow* pWindow = [self window];
  if (pWindow) {
    [pWindow makeFirstResponder: self];
    [pWindow setAcceptsMouseMovedEvents: YES];
  }
}

- (void) drawRect: (NSRect) rect 
{
  mGraphics->Draw(&ToIRECT(mGraphics, &rect));
}

- (void) onTimer: (NSTimer*) pTimer
{
  IRECT r;
  if (pTimer == mTimer && mGraphics && mGraphics->IsDirty(&r)) {
    [self setNeedsDisplayInRect:ToNSRect(mGraphics, &r)];
  }
}

- (void) getMouseXY: (NSEvent*) pEvent x: (int*) pX y: (int*) pY
{
  NSPoint pt = [self convertPoint:[pEvent locationInWindow] fromView:nil];
  *pX = (int) pt.x;
  *pY = mGraphics->Height() - (int) pt.y;
}

- (void) mouseDown: (NSEvent*) pEvent
{
  int x, y;
  [self getMouseXY:pEvent x:&x y:&y];
  if ([pEvent clickCount] > 1) {
    mGraphics->OnMouseDblClick(x, y, &GetMouseMod(pEvent));
  }
  else {
    mGraphics->OnMouseDown(x, y, &GetMouseMod(pEvent));
  }
}

- (void) mouseUp: (NSEvent*) pEvent
{
  int x, y;
  [self getMouseXY:pEvent x:&x y:&y];
  mGraphics->OnMouseUp(x, y, &GetMouseMod(pEvent));
}

- (void) mouseDragged: (NSEvent*) pEvent
{
  int x, y;
  [self getMouseXY:pEvent x:&x y:&y];
  mGraphics->OnMouseDrag(x, y, &GetMouseMod(pEvent));
}

- (void) mouseMoved: (NSEvent*) pEvent
{
  int x, y;
  [self getMouseXY:pEvent x:&x y:&y];
  mGraphics->OnMouseOver(x, y, &GetMouseMod(pEvent));
}

- (void) scrollWheel: (NSEvent*) pEvent
{
  int x, y;
  [self getMouseXY:pEvent x:&x y:&y];
  int d = [pEvent deltaY];
  mGraphics->OnMouseWheel(x, y, &GetMouseMod(pEvent), d);
}

- (void) killTimer
{
  [mTimer invalidate];
  mTimer = 0;
}

- (void) removeFromSuperview
{
  if (mGraphics)
  {
    IGraphics* graphics = mGraphics;
    mGraphics = 0;
    graphics->CloseWindow();
  }
}

@end
