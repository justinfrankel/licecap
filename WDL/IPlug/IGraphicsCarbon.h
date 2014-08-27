#ifndef _IGRAPHICSCARBON_
#define _IGRAPHICSCARBON_

#include <Carbon/Carbon.h>
#include "IGraphicsMac.h"

class IGraphicsCarbon
{
public:
  
  IGraphicsCarbon(IGraphicsMac* pGraphicsMac, WindowRef pWindow, ControlRef pParentControl);
  ~IGraphicsCarbon();
  
  ControlRef GetView() { return mView; }
  CGContextRef GetCGContext() { return mCGC; }
  void OffsetContentRect(CGRect* pR);
  bool Resize(int w, int h);
  
private:
  
  IGraphicsMac* mGraphicsMac;
  bool mIsComposited;
  int mContentXOffset, mContentYOffset;
  RgnHandle mRgn;
  WindowRef mWindow;
  ControlRef mView; // was HIViewRef
  EventLoopTimerRef mTimer;
  EventHandlerRef mControlHandler, mWindowHandler;
  CGContextRef mCGC;
  
public:
  
  static pascal OSStatus CarbonEventHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon);
  static pascal void CarbonTimerHandler(EventLoopTimerRef pTimer, void* pGraphicsCarbon);
};

#endif