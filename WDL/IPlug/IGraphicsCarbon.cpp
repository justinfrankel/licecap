#include "IGraphicsCarbon.h"

IRECT GetRegionRect(EventRef pEvent, int gfxW, int gfxH)
{
  RgnHandle pRgn = 0;
  if (GetEventParameter(pEvent, kEventParamRgnHandle, typeQDRgnHandle, 0, sizeof(RgnHandle), 0, &pRgn) == noErr && pRgn) {
    Rect rct;
    GetRegionBounds(pRgn, &rct);
    return IRECT(rct.left, rct.top, rct.right, rct.bottom); 
  }
  return IRECT(0, 0, gfxW, gfxH);
}

IRECT GetControlRect(EventRef pEvent, int gfxW, int gfxH)
{
  Rect rct;
  if (GetEventParameter(pEvent, kEventParamCurrentBounds, typeQDRectangle, 0, sizeof(Rect), 0, &rct) == noErr) {
    int w = rct.right - rct.left;
    int h = rct.bottom - rct.top;
    if (w > 0 && h > 0) {
      return IRECT(0, 0, w, h);
    }
  }  
  return IRECT(0, 0, gfxW, gfxH);
}

// static 
pascal OSStatus IGraphicsCarbon::CarbonEventHandler(EventHandlerCallRef pHandlerCall, EventRef pEvent, void* pGraphicsCarbon)
{
  IGraphicsCarbon* _this = (IGraphicsCarbon*) pGraphicsCarbon;
  IGraphicsMac* pGraphicsMac = _this->mGraphicsMac;
  UInt32 eventClass = GetEventClass(pEvent);
  UInt32 eventKind = GetEventKind(pEvent);
  switch (eventClass) {      
    case kEventClassControl: {
      switch (eventKind) {          
        case kEventControlDraw: {
          
          int gfxW = pGraphicsMac->Width(), gfxH = pGraphicsMac->Height();
          IRECT r = GetRegionRect(pEvent, gfxW, gfxH);  
          
          CGrafPtr port = 0;
          if (_this->mIsComposited) {
            GetEventParameter(pEvent, kEventParamCGContextRef, typeCGContextRef, 0, sizeof(CGContextRef), 0, &(_this->mCGC));         
            CGContextTranslateCTM(_this->mCGC, 0, gfxH);
            CGContextScaleCTM(_this->mCGC, 1.0, -1.0);     
            pGraphicsMac->Draw(&r);
          }
          else {
            #pragma REMINDER("not swapping gfx ports in non-composited mode")
            
            int ctlH = r.H();
            _this->mContentYOffset = ctlH - gfxH;
            
            //Rect rct;          
            //GetWindowBounds(_this->mWindow, kWindowContentRgn, &rct);
            //int wndH = rct.bottom - rct.top;
            //if (wndH > gfxH) {
            //  int yOffs = wndH - gfxH;
            //  _this->mContentYOffset = yOffs;
            // }
            
            GetEventParameter(pEvent, kEventParamGrafPort, typeGrafPtr, 0, sizeof(CGrafPtr), 0, &port);
            QDBeginCGContext(port, &(_this->mCGC));        
            // Old-style controls drawing, ask the plugin what's dirty rather than relying on the OS.
            r.R = r.T = r.R = r.B = 0;
            _this->mGraphicsMac->IsDirty(&r);         
          }       
          
          pGraphicsMac->Draw(&r);
            
          if (port) {
            CGContextFlush(_this->mCGC);
            QDEndCGContext(port, &(_this->mCGC));
          }
                     
          return noErr;
        }  
        case kEventControlBoundsChanged: {        
          int gfxW = pGraphicsMac->Width(), gfxH = pGraphicsMac->Height();
          IRECT r = GetControlRect(pEvent, gfxW, gfxH);
          //pGraphicsMac->GetPlug()->UserResizedWindow(&r);
          return noErr;
        }        
        case kEventControlDispose: {
          // kComponentCloseSelect call should already have done this for us (and deleted mGraphicsMac, for that matter).
          // pGraphicsMac->CloseWindow();
          return noErr;
        }
      }
      break;
    }
    case kEventClassMouse: {
      HIPoint hp;
      GetEventParameter(pEvent, kEventParamWindowMouseLocation, typeHIPoint, 0, sizeof(HIPoint), 0, &hp);
      HIPointConvert(&hp, kHICoordSpaceWindow, _this->mWindow, kHICoordSpaceView, _this->mView);
      int x = (int) hp.x;
      int y = (int) hp.y;

      UInt32 mods;
      GetEventParameter(pEvent, kEventParamKeyModifiers, typeUInt32, 0, sizeof(UInt32), 0, &mods);
      IMouseMod mmod(true, (mods & cmdKey), (mods & shiftKey), (mods & controlKey), (mods & optionKey));
      
      switch (eventKind) {
        case kEventMouseDown: {
          CallNextEventHandler(pHandlerCall, pEvent);   // Activates the window, if inactive.
          
          UInt32 clickCount = 0;
          GetEventParameter(pEvent, kEventParamClickCount, typeUInt32, 0, sizeof(UInt32), 0, &clickCount);
          if (clickCount > 1) {
            pGraphicsMac->OnMouseDblClick(x, y, &mmod);
          }
          else {          
            pGraphicsMac->OnMouseDown(x, y, &mmod);
          }
          return noErr;
        }
        case kEventMouseUp: {
          pGraphicsMac->OnMouseUp(x, y, &mmod);
          return noErr;
        }
        case kEventMouseMoved: {
          pGraphicsMac->OnMouseOver(x, y, &mmod);
          return noErr;
        }
        case kEventMouseDragged: {
          pGraphicsMac->OnMouseDrag(x, y, &mmod);
          return noErr; 
        }
        case kEventMouseWheelMoved: {
          EventMouseWheelAxis axis;
          GetEventParameter(pEvent, kEventParamMouseWheelAxis, typeMouseWheelAxis, 0, sizeof(EventMouseWheelAxis), 0, &axis);
          if (axis == kEventMouseWheelAxisY) {
            int d;
            GetEventParameter(pEvent, kEventParamMouseWheelDelta, typeSInt32, 0, sizeof(SInt32), 0, &d);
            pGraphicsMac->OnMouseWheel(x, y, &mmod, d);
            return noErr;
          }
        }   
      }
      break;    
    }
  }
  return eventNotHandledErr;
}    

// static 
pascal void IGraphicsCarbon::CarbonTimerHandler(EventLoopTimerRef pTimer, void* pGraphicsCarbon)
{
  IGraphicsCarbon* _this = (IGraphicsCarbon*) pGraphicsCarbon;
  IRECT r;
  if (_this->mGraphicsMac->IsDirty(&r)) {
    if (_this->mIsComposited) {
      HIViewSetNeedsDisplayInRect(_this->mView, &CGRectMake(r.L, r.T, r.W(), r.H()), true);
    }
    else {
      int h = _this->mGraphicsMac->Height();
      SetRectRgn(_this->mRgn, r.L, h - r.B, r.R, h - r.T);
      UpdateControls(_this->mWindow, 0);// _this->mRgn);
    }
  } 
}

void ResizeWindow(WindowRef pWindow, int w, int h)
{
  Rect gr;  // Screen.
  GetWindowBounds(pWindow, kWindowContentRgn, &gr);
  gr.right = gr.left + w;
  gr.bottom = gr.top + h;
  SetWindowBounds(pWindow, kWindowContentRgn, &gr); 
}

IGraphicsCarbon::IGraphicsCarbon(IGraphicsMac* pGraphicsMac, WindowRef pWindow, ControlRef pParentControl)
: mGraphicsMac(pGraphicsMac), mWindow(pWindow), mView(0), mTimer(0), mControlHandler(0), mWindowHandler(0), mCGC(0),
  mContentXOffset(0), mContentYOffset(0)
{ 
  TRACE;
  
  Rect r;   // Client.
  r.left = r.top = 0;
  r.right = pGraphicsMac->Width();
  r.bottom = pGraphicsMac->Height();   
  //ResizeWindow(pWindow, r.right, r.bottom);

  WindowAttributes winAttrs = 0;
  GetWindowAttributes(pWindow, &winAttrs);
  mIsComposited = (winAttrs & kWindowCompositingAttribute);
  mRgn = NewRgn();  
  
  UInt32 features =  kControlSupportsFocus | kControlHandlesTracking | kControlSupportsEmbedding;
  if (mIsComposited) {
    features |= kHIViewIsOpaque | kHIViewFeatureDoesNotUseSpecialParts;
  }
  CreateUserPaneControl(pWindow, &r, features, &mView);    
  
  const EventTypeSpec controlEvents[] = {	
    //{ kEventClassControl, kEventControlInitialize },
    //{kEventClassControl, kEventControlGetOptimalBounds},    
    //{ kEventClassControl, kEventControlHitTest },
    { kEventClassControl, kEventControlClick },
    //{ kEventClassKeyboard, kEventRawKeyDown },
    { kEventClassControl, kEventControlDraw },
    { kEventClassControl, kEventControlDispose },
    { kEventClassControl, kEventControlBoundsChanged }
  };
  InstallControlEventHandler(mView, CarbonEventHandler, GetEventTypeCount(controlEvents), controlEvents, this, &mControlHandler);
  
  const EventTypeSpec windowEvents[] = {
    { kEventClassMouse, kEventMouseDown },
    { kEventClassMouse, kEventMouseUp },
    { kEventClassMouse, kEventMouseMoved },
    { kEventClassMouse, kEventMouseDragged },
    { kEventClassMouse, kEventMouseWheelMoved }
  };
  InstallWindowEventHandler(mWindow, CarbonEventHandler, GetEventTypeCount(windowEvents), windowEvents, this, &mWindowHandler);  
  
  double t = 2.0*kEventDurationSecond/(double)pGraphicsMac->FPS();
  OSStatus s = InstallEventLoopTimer(GetMainEventLoop(), 0.0, t, CarbonTimerHandler, this, &mTimer);
  
  if (mIsComposited) {
    if (!pParentControl) {
      HIViewRef hvRoot = HIViewGetRoot(pWindow);
      s = HIViewFindByID(hvRoot, kHIViewWindowContentID, &pParentControl); 
    }  
    s = HIViewAddSubview(pParentControl, mView);
  }
  else {
    if (!pParentControl) {
      if (GetRootControl(pWindow, &pParentControl) != noErr) {
        CreateRootControl(pWindow, &pParentControl);
      }
    }
    s = EmbedControl(mView, pParentControl); 
  }

  if (s == noErr) {
    SizeControl(mView, r.right, r.bottom);  // offset?
  }
}

IGraphicsCarbon::~IGraphicsCarbon()
{
  // Called from IGraphicsMac::CloseWindow.
  RemoveEventLoopTimer(mTimer);
  RemoveEventHandler(mControlHandler);
  RemoveEventHandler(mWindowHandler);
  mTimer = 0;
  mView = 0;
  DisposeRgn(mRgn);
}

void IGraphicsCarbon::OffsetContentRect(CGRect* pR)
{
  *pR = CGRectOffset(*pR, (float) mContentXOffset, (float) mContentYOffset);
}

bool IGraphicsCarbon::Resize(int w, int h)
{
  if (mWindow && mView) {
    ResizeWindow(mWindow, w, h);
    return (HIViewSetFrame(mView, &CGRectMake(0, 0, w, h)) == noErr);
  }
  return false;
}
