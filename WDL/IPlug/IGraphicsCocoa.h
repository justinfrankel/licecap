#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include <AudioUnit/AudioUnit.h>
#import <AudioUnit/AUCocoaUIView.h>
#include "IGraphicsMac.h"

// Cocoa objects can be supplied by any existing component, 
// so we need to make sure the C++ static lib code gets the 
// IGraphicsCocoa that it expects.
#define IGRAPHICS_COCOA IGraphicsCocoa_v1002

NSString* ToNSString(const char* cStr);

inline CGRect ToCGRect(int h, IRECT* pR)
{
  int B = h - pR->B;
  return CGRectMake(pR->L, B, pR->W(), B + pR->H()); 
}

@interface IGRAPHICS_COCOA : NSView
{
  IGraphicsMac* mGraphics;
  NSTimer* mTimer;
}
- (id) init;
- (id) initWithIGraphics: (IGraphicsMac*) pGraphics;
- (BOOL) isOpaque;
- (BOOL) acceptsFirstResponder;
- (BOOL) acceptsFirstMouse: (NSEvent*) pEvent;
- (void) viewDidMoveToWindow;
- (void) drawRect: (NSRect) rect;
- (void) onTimer: (NSTimer*) pTimer;
- (void) getMouseXY: (NSEvent*) pEvent x: (int*) pX y: (int*) pY;
- (void) mouseDown: (NSEvent*) pEvent;
- (void) mouseUp: (NSEvent*) pEvent;
- (void) mouseDragged: (NSEvent*) pEvent;
- (void) mouseMoved: (NSEvent*) pEvent;
- (void) scrollWheel: (NSEvent*) pEvent;
- (void) killTimer;
@end
