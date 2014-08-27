#include "IGraphics.h"
#include "IControl.h"

#define DEFAULT_FPS 24

// If not dirty for this many timer ticks, we call OnGUIIDle.
// Only looked at if USE_IDLE_CALLS is defined.
#define IDLE_TICKS 20

IGraphics::IGraphics(IPlugBase* pPlug, int w, int h, int refreshFPS)
:	mPlug(pPlug), mWidth(w), mHeight(h), mIdleTicks(0), 
  mMouseCapture(-1), mMouseOver(-1), mMouseX(0), mMouseY(0), mHandleMouseOver(false), mStrict(true), mDisplayControlValue(false)
{
	mFPS = (refreshFPS > 0 ? refreshFPS : DEFAULT_FPS);
}

IGraphics::~IGraphics()
{
    mControls.Empty(true);
}

void IGraphics::Resize(int w, int h)
{
  // The OS implementation class has to do all the work, then call up to here.
  mWidth = w;
  mHeight = h;
  ReleaseMouseCapture();
  mControls.Empty(true);
  mPlug->ResizeGraphics(w, h);
}

void IGraphics::AttachBackground(int ID, const char* name)
{
  IBitmap bg = LoadIBitmap(ID, name);
  IControl* pBG = new IBitmapControl(mPlug, 0, 0, -1, &bg, IChannelBlend::kBlendClobber);
  mControls.Insert(0, pBG);
}

int IGraphics::AttachControl(IControl* pControl)
{
	mControls.Add(pControl);
  return mControls.GetSize() - 1;
}

void IGraphics::HideControl(int paramIdx, bool hide)
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
	for (i = 0; i < n; ++i, ++ppControl) {
		IControl* pControl = *ppControl;
		if (pControl->ParamIdx() == paramIdx) {
      pControl->Hide(hide);
    }
    // Could be more than one, don't break until we check them all.
  }
}

void IGraphics::GrayOutControl(int paramIdx, bool gray)
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
	for (i = 0; i < n; ++i, ++ppControl) {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx) {
      pControl->GrayOut(gray);
    }
    // Could be more than one, don't break until we check them all.
  }
}

void IGraphics::ClampControl(int paramIdx, double lo, double hi, bool normalized)
{
  if (!normalized) {
    IParam* pParam = mPlug->GetParam(paramIdx);
    lo = pParam->GetNormalized(lo);
    hi = pParam->GetNormalized(hi);
  }  
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
	for (i = 0; i < n; ++i, ++ppControl) {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx) {
      pControl->Clamp(lo, hi);
    }
    // Could be more than one, don't break until we check them all.
  }
}

void IGraphics::SetParameterFromPlug(int paramIdx, double value, bool normalized)
{
  if (!normalized) {
    IParam* pParam = mPlug->GetParam(paramIdx);
    value = pParam->GetNormalized(value);
  }  
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
	for (i = 0; i < n; ++i, ++ppControl) {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx) {
      //WDL_MutexLock lock(&mMutex);
      pControl->SetValueFromPlug(value);
      // Could be more than one, don't break until we check them all.
    }
  }
}

void IGraphics::SetControlFromPlug(int controlIdx, double normalizedValue)
{
  if (controlIdx >= 0 && controlIdx < mControls.GetSize()) {
    //WDL_MutexLock lock(&mMutex);
    mControls.Get(controlIdx)->SetValueFromPlug(normalizedValue);
  }
}

void IGraphics::SetAllControlsDirty()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
	for (i = 0; i < n; ++i, ++ppControl) {
    IControl* pControl = *ppControl;
    pControl->SetDirty(false);
  }
}

void IGraphics::SetParameterFromGUI(int paramIdx, double normalizedValue)
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
	for (i = 0; i < n; ++i, ++ppControl) {
    IControl* pControl = *ppControl;
    if (pControl->ParamIdx() == paramIdx) {
      pControl->SetValueFromUserInput(normalizedValue);
      // Could be more than one, don't break until we check them all.
    }
	}
}

bool IGraphics::DrawBitmap(IBitmap* pBitmap, IRECT* pR, int bmpState, const IChannelBlend* pBlend)
{
	int srcY = 0;
	if (pBitmap->N > 1 && bmpState > 1) {
		srcY = int(0.5 + (double) pBitmap->H * (double) (bmpState - 1) / (double) pBitmap->N);
	}
	return DrawBitmap(pBitmap, pR, 0, srcY, pBlend);    
}

bool IGraphics::DrawRect(const IColor* pColor, IRECT* pR)
{
  bool rc = DrawHorizontalLine(pColor, pR->T, pR->L, pR->R);
  rc &= DrawHorizontalLine(pColor, pR->B, pR->L, pR->R);
  rc &= DrawVerticalLine(pColor, pR->L, pR->T, pR->B);
  rc &= DrawVerticalLine(pColor, pR->R, pR->T, pR->B);
  return rc;
}

bool IGraphics::DrawVerticalLine(const IColor* pColor, IRECT* pR, float x)
{
  x = BOUNDED(x, 0.0f, 1.0f);
  int xi = pR->L + int(x * (float) (pR->R - pR->L));
  return DrawVerticalLine(pColor, xi, pR->T, pR->B);
}

bool IGraphics::DrawHorizontalLine(const IColor* pColor, IRECT* pR, float y)
{
  y = BOUNDED(y, 0.0f, 1.0f);
  int yi = pR->B - int(y * (float) (pR->B - pR->T));
  return DrawHorizontalLine(pColor, yi, pR->L, pR->R);
}

bool IGraphics::DrawVerticalLine(const IColor* pColor, int xi, int yLo, int yHi)
{
  IChannelBlend clobber(IChannelBlend::kBlendClobber);
  return DrawLine(pColor, (float) xi, (float) yLo, (float) xi, (float) yHi, &clobber);   
}
 
bool IGraphics::DrawHorizontalLine(const IColor* pColor, int yi, int xLo, int xHi)
{
  IChannelBlend clobber(IChannelBlend::kBlendClobber);
  return DrawLine(pColor, (float) xLo, (float) yi, (float) xHi, (float) yi, &clobber);  
}

bool IGraphics::DrawRadialLine(const IColor* pColor, float cx, float cy, float angle, float rMin, float rMax, 
  const IChannelBlend* pBlend, bool antiAlias)
{
  float sinV = sin(angle);
  float cosV = cos(angle);
  float xLo = cx + rMin * sinV;
  float xHi = cx + rMax * sinV;
  float yLo = cy - rMin * cosV;
  float yHi = cy - rMax * cosV;
  return DrawLine(pColor, xLo, yLo, xHi, yHi, pBlend, antiAlias);
}

bool IGraphics::IsDirty(IRECT* pR)
{
  bool dirty = false;
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
	for (i = 0; i < n; ++i, ++ppControl) {
    IControl* pControl = *ppControl;
    if (pControl->IsDirty()) {
      *pR = pR->Union(pControl->GetRECT());
      dirty = true;
    }
  }
  
#ifdef USE_IDLE_CALLS
  if (dirty) {
    mIdleTicks = 0;
  }
  else
  if (++mIdleTicks > IDLE_TICKS) {
    OnGUIIdle();
    mIdleTicks = 0;
  }
#endif
    
  return dirty;
}

void IGraphics::DisplayControlValue(IControl* pControl)
{
//  char str[32];
//  int paramIdx = pControl->ParamIdx();
//  if (paramIdx >= 0) {
//    IParam* pParam = mPlug->GetParam(paramIdx);
//    pParam->GetDisplayForHost(str);
//    IRECT r = *(pControl->GetRECT());
//    r.L = r.MW() - 10;
//    r.R = r.L + 20;
//    r.T = r.MH() - 5;
//    r.B = r.T + 10;    
//    DrawIText(&IText(), str, &r);
//  }  
}  
                         
// The OS is announcing what needs to be redrawn,
// which may be a larger area than what is strictly dirty.
bool IGraphics::Draw(IRECT* pR)
{
//  #pragma REMINDER("Mutex set while drawing")
//  WDL_MutexLock lock(&mMutex);
  
  int i, j, n = mControls.GetSize();
  if (!n) {
    return true;
  }

  if (mStrict) {
    mDrawRECT = *pR;
    int n = mControls.GetSize();
    IControl** ppControl = mControls.GetList();
    for (int i = 0; i < n; ++i, ++ppControl) {
      IControl* pControl = *ppControl;
      if (!(pControl->IsHidden()) && pR->Intersects(pControl->GetRECT())) {
        pControl->Draw(this);
//        if (mDisplayControlValue && i == mMouseCapture) {
//          DisplayControlValue(pControl);
//        }        
      }
      pControl->SetClean();
    }
  }
  else {
    IControl* pBG = mControls.Get(0);
    if (pBG->IsDirty()) { // Special case when everything needs to be drawn.
      mDrawRECT = *(pBG->GetRECT());
      for (int j = 0; j < n; ++j) {
        IControl* pControl2 = mControls.Get(j);
        if (!j || !(pControl2->IsHidden())) {
          pControl2->Draw(this);
          pControl2->SetClean();
        }
      }
    }
    else {
      for (i = 1; i < n; ++i) {
        IControl* pControl = mControls.Get(i);
        if (pControl->IsDirty()) {
          mDrawRECT = *(pControl->GetRECT()); 
          for (j = 0; j < n; ++j) {
            IControl* pControl2 = mControls.Get(j);
            if ((i == j) || (!(pControl2->IsHidden()) && pControl2->GetRECT()->Intersects(&mDrawRECT))) {
              pControl2->Draw(this);
            }
          }
          pControl->SetClean();
        }
      }
    }
  }

  return DrawScreen(pR);

}

void IGraphics::SetStrictDrawing(bool strict)
{
  mStrict = strict;
  SetAllControlsDirty();
}

void IGraphics::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	ReleaseMouseCapture();
  int c = GetMouseControlIdx(x, y);
	if (c >= 0) {
		mMouseCapture = c;
		mMouseX = x;
		mMouseY = y;
    mDisplayControlValue = (pMod->R);
    IControl* pControl = mControls.Get(c);
		pControl->OnMouseDown(x, y, pMod);
    int paramIdx = pControl->ParamIdx();
    if (paramIdx >= 0) {
      mPlug->BeginInformHostOfParamChange(paramIdx);
    }    
  }
}

void IGraphics::OnMouseUp(int x, int y, IMouseMod* pMod)
{
	int c = GetMouseControlIdx(x, y);
	mMouseCapture = mMouseX = mMouseY = -1;
  mDisplayControlValue = false;
	if (c >= 0) {
    IControl* pControl = mControls.Get(c);
		pControl->OnMouseUp(x, y, pMod);
    int paramIdx = pControl->ParamIdx();
    if (paramIdx >= 0) {
      mPlug->EndInformHostOfParamChange(paramIdx);
    }    
	}
}

bool IGraphics::OnMouseOver(int x, int y, IMouseMod* pMod)
{
  if (mHandleMouseOver) {
    int c = GetMouseControlIdx(x, y);
    if (c >= 0) {
	    mMouseX = x;
	    mMouseY = y;
	    mControls.Get(c)->OnMouseOver(x, y, pMod);
      if (mMouseOver >= 0 && mMouseOver != c) {
        mControls.Get(mMouseOver)->OnMouseOut();
      }
      mMouseOver = c;
    }
  }
  return mHandleMouseOver;
}

void IGraphics::OnMouseOut()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
	for (i = 0; i < n; ++i, ++ppControl) {
		IControl* pControl = *ppControl;
		pControl->OnMouseOut();
	}
  mMouseOver = -1;
}

void IGraphics::OnMouseDrag(int x, int y, IMouseMod* pMod)
{
  int c = mMouseCapture;
  if (c >= 0) {
	  int dX = x - mMouseX;
	  int dY = y - mMouseY;
    if (dX != 0 || dY != 0) {
      mMouseX = x;
      mMouseY = y;
      mControls.Get(c)->OnMouseDrag(x, y, dX, dY, pMod);
    }
  }
}

bool IGraphics::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
	ReleaseMouseCapture();
  bool newCapture = false;
	int c = GetMouseControlIdx(x, y);
	if (c >= 0) {
    IControl* pControl = mControls.Get(c);
    if (pControl->MouseDblAsSingleClick()) {
      mMouseCapture = c;
      mMouseX = x;
      mMouseY = y;
      pControl->OnMouseDown(x, y, pMod);
      newCapture = true;
    }
    else {
		  pControl->OnMouseDblClick(x, y, pMod);
    }
	}
  return newCapture;
}

void IGraphics::OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
{	
	int c = GetMouseControlIdx(x, y);
	if (c >= 0) {
		mControls.Get(c)->OnMouseWheel(x, y, pMod, d);
	}
}

void IGraphics::ReleaseMouseCapture()
{
	mMouseCapture = mMouseX = mMouseY = -1;
}

void IGraphics::OnKeyDown(int x, int y, int key)
{
	int c = GetMouseControlIdx(x, y);
	if (c >= 0) {
		mControls.Get(c)->OnKeyDown(x, y, key);
	}
}

int IGraphics::GetMouseControlIdx(int x, int y)
{
	if (mMouseCapture >= 0) {
		return mMouseCapture;
	}
	// The BG is a control and will catch everything, so assume the programmer
	// attached the controls from back to front, and return the frontmost match.
  int i = mControls.GetSize() - 1;
  IControl** ppControl = mControls.GetList() + i;
	for (/* */; i >= 0; --i, --ppControl) {
    IControl* pControl = *ppControl;
    if (!pControl->IsHidden() && !pControl->IsGrayed() && pControl->IsHit(x, y)) {
      return i;
    }
	}
	return -1;
}

void IGraphics::OnGUIIdle()
{
  int i, n = mControls.GetSize();
  IControl** ppControl = mControls.GetList();
	for (i = 0; i < n; ++i, ++ppControl) {
		IControl* pControl = *ppControl;
		pControl->OnGUIIdle();
	}
}
