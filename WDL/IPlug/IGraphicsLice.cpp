#include "IGraphicsLice.h"
#include "IControl.h"
#include "math.h"
#include "Log.h"

extern HINSTANCE gHInstance;

struct BitmapStorage
{
  WDL_PtrList<LICE_IBitmap> mBitmaps;

  ~BitmapStorage()
  {
    mBitmaps.Empty(true);
  }
};
static BitmapStorage sRetainedBitmaps;

bool ExistsInStorage(LICE_IBitmap* pBitmap)
{
  int i, n = sRetainedBitmaps.mBitmaps.GetSize();
  for (i = 0; i < n; ++i) {
    if (sRetainedBitmaps.mBitmaps.Get(i) == pBitmap) {
      return true;
    }
  }
  return false;
}

inline LICE_pixel LiceColor(const IColor* pColor) 
{
	return LICE_RGBA(pColor->R, pColor->G, pColor->B, pColor->A);
}

inline float LiceWeight(const IChannelBlend* pBlend)
{
    return (pBlend ? pBlend->mWeight : 1.0f);
}

inline int LiceBlendMode(const IChannelBlend* pBlend)
{
  if (!pBlend) {
      return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
  }
  switch (pBlend->mMethod) {
    case IChannelBlend::kBlendClobber: {
		  return LICE_BLIT_MODE_COPY;
    }
    case IChannelBlend::kBlendAdd: {
		  return LICE_BLIT_MODE_ADD | LICE_BLIT_USE_ALPHA;
    }
    case IChannelBlend::kBlendColorDodge: {
      return LICE_BLIT_MODE_DODGE | LICE_BLIT_USE_ALPHA;
    }
	  case IChannelBlend::kBlendNone:
    default: {
		  return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
    }
	}
}

IGraphicsLice::IGraphicsLice(IPlugBase* pPlug, int w, int h, int refreshFPS)
:	IGraphics(pPlug, w, h, refreshFPS), mDrawBitmap(0), mTmpBitmap(0)
{}

IGraphicsLice::~IGraphicsLice() 
{
	DELETE_NULL(mDrawBitmap);
	DELETE_NULL(mTmpBitmap);
}

IBitmap IGraphicsLice::LoadIBitmap(int ID, const char* name, int nStates)
{
  static WDL_Mutex sMutex;
  static WDL_TypedBuf<int> sCachedIDs;
  static WDL_TypedBuf<IBitmap> sCachedBitmaps;

  WDL_MutexLock cacheLock(&sMutex);
  int i, n = sCachedIDs.GetSize();
  int* pID = sCachedIDs.Get();
  IBitmap* pCachedBitmap = 0;
  for (i = 0; i < n; ++i, ++pID) {
    if (ID == *pID) {
     pCachedBitmap = sCachedBitmaps.Get() + i;
     if (ExistsInStorage((LICE_IBitmap*) pCachedBitmap->mData)) {
       return *pCachedBitmap;
     }
     // Somebody, probably another plugin instance, released the bitmap data
     // out from under the function-local cache, so we need to reload it.
     // We can reuse this pointer to the function-local cache.
     break;
    }
  }

  if (!pCachedBitmap) { // Make a new entry in the function-local cache.
    sCachedIDs.Resize(n + 1);
    sCachedBitmaps.Resize(n + 1);
    *(sCachedIDs.Get() + n) = ID;
    pCachedBitmap = sCachedBitmaps.Get() + n;
  }

  LICE_IBitmap* pLB = OSLoadBitmap(ID, name);
  bool imgResourceFound = (pLB);
  assert(imgResourceFound); // Protect against typos in resource.h and .rc files.
 *pCachedBitmap = IBitmap(pLB, pLB->getWidth(), pLB->getHeight(), nStates);
  RetainBitmap(pCachedBitmap);
  return *pCachedBitmap;
}

void IGraphicsLice::RetainBitmap(IBitmap* pBitmap)
{
  LICE_IBitmap* pLB = (LICE_IBitmap*) pBitmap->mData;
  if (pLB) {
    sRetainedBitmaps.mBitmaps.Add(pLB);
  }
}

void IGraphicsLice::ReleaseBitmap(IBitmap* pBitmap)
{
  LICE_IBitmap* pLB = (LICE_IBitmap*) pBitmap->mData;
  if (pLB) {
    int i, n = sRetainedBitmaps.mBitmaps.GetSize();
    for (i = 0; i < n; ++i) {
      if (sRetainedBitmaps.mBitmaps.Get(i) == pLB) {
        sRetainedBitmaps.mBitmaps.Delete(i, true);
        break;
      }
    }
  }
}

void IGraphicsLice::PrepDraw()
{
  mDrawBitmap = new LICE_SysBitmap(Width(), Height());
  mTmpBitmap = new LICE_MemBitmap();      
}

bool IGraphicsLice::DrawBitmap(IBitmap* pIBitmap, IRECT* pDest, int srcX, int srcY, const IChannelBlend* pBlend)
{
  LICE_IBitmap* pLB = (LICE_IBitmap*) pIBitmap->mData;
  IRECT r = pDest->Intersect(&mDrawRECT);
  srcX += r.L - pDest->L;
  srcY += r.T - pDest->T;
  _LICE::LICE_Blit(mDrawBitmap, pLB, r.L, r.T, srcX, srcY, r.W(), r.H(), LiceWeight(pBlend), LiceBlendMode(pBlend));
	return true;
}

bool IGraphicsLice::DrawRotatedBitmap(IBitmap* pIBitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg,
    const IChannelBlend* pBlend)
{
	LICE_IBitmap* pLB = (LICE_IBitmap*) pIBitmap->mData;

	//double dA = angle * PI / 180.0;
	// Can't figure out what LICE_RotatedBlit is doing for irregular bitmaps exactly.
	//double w = (double) bitmap.W;
	//double h = (double) bitmap.H;
	//double sinA = fabs(sin(dA));
	//double cosA = fabs(cos(dA));
	//int W = int(h * sinA + w * cosA);
	//int H = int(h * cosA + w * sinA);

	int W = pIBitmap->W;
	int H = pIBitmap->H;
	int destX = destCtrX - W / 2;
	int destY = destCtrY - H / 2;

  _LICE::LICE_RotatedBlit(mDrawBitmap, pLB, destX, destY, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) angle, 
		false, LiceWeight(pBlend), LiceBlendMode(pBlend) | LICE_BLIT_FILTER_BILINEAR, 0.0f, (float) yOffsetZeroDeg);

	return true;
}

bool IGraphicsLice::DrawRotatedMask(IBitmap* pIBase, IBitmap* pIMask, IBitmap* pITop, int x, int y, double angle,
    const IChannelBlend* pBlend)    
{
	LICE_IBitmap* pBase = (LICE_IBitmap*) pIBase->mData;
	LICE_IBitmap* pMask = (LICE_IBitmap*) pIMask->mData;
	LICE_IBitmap* pTop = (LICE_IBitmap*) pITop->mData;

	double dA = angle * PI / 180.0;
	int W = pIBase->W;
	int H = pIBase->H;
//	RECT srcR = { 0, 0, W, H };
	float xOffs = (W % 2 ? -0.5f : 0.0f);

	if (!mTmpBitmap) {
		mTmpBitmap = new LICE_MemBitmap();
	}
  _LICE::LICE_Copy(mTmpBitmap, pBase);
	_LICE::LICE_ClearRect(mTmpBitmap, 0, 0, W, H, LICE_RGBA(255, 255, 255, 0));

	_LICE::LICE_RotatedBlit(mTmpBitmap, pMask, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) dA, 
		true, 1.0f, LICE_BLIT_MODE_ADD | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);
	_LICE::LICE_RotatedBlit(mTmpBitmap, pTop, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) dA,
		true, 1.0f, LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);

  IRECT r = IRECT(x, y, x + W, y + H).Intersect(&mDrawRECT);
  _LICE::LICE_Blit(mDrawBitmap, mTmpBitmap, r.L, r.T, r.L - x, r.T - y, r.R - r.L, r.B - r.T,
    LiceWeight(pBlend), LiceBlendMode(pBlend));
//	ReaperExt::LICE_Blit(mDrawBitmap, mTmpBitmap, x, y, &srcR, LiceWeight(pBlend), LiceBlendMode(pBlend));
	return true;
}

bool IGraphicsLice::DrawPoint(const IColor* pColor, float x, float y, 
		const IChannelBlend* pBlend, bool antiAlias)
{
  float weight = (pBlend ? pBlend->mWeight : 1.0f);
  _LICE::LICE_PutPixel(mDrawBitmap, int(x + 0.5f), int(y + 0.5f), LiceColor(pColor), weight, LiceBlendMode(pBlend));
	return true;
}

bool IGraphicsLice::ForcePixel(const IColor* pColor, int x, int y)
{
  LICE_pixel* px = mDrawBitmap->getBits();
  px += x + y * mDrawBitmap->getRowSpan();
  *px = LiceColor(pColor);
  return true;
}

bool IGraphicsLice::DrawLine(const IColor* pColor, float x1, float y1, float x2, float y2,
  const IChannelBlend* pBlend, bool antiAlias)
{
  _LICE::LICE_Line(mDrawBitmap, x1, y1, x2, y2, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
	return true;
}

bool IGraphicsLice::DrawArc(const IColor* pColor, float cx, float cy, float r, float minAngle, float maxAngle, 
	const IChannelBlend* pBlend, bool antiAlias)
{
 _LICE::LICE_Arc(mDrawBitmap, cx, cy, r, minAngle, maxAngle, LiceColor(pColor), 
        LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
	return true;
}

bool IGraphicsLice::DrawCircle(const IColor* pColor, float cx, float cy, float r,
	const IChannelBlend* pBlend, bool antiAlias)
{
  _LICE::LICE_Circle(mDrawBitmap, cx, cy, r, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
	return true;
}

bool IGraphicsLice::FillIRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend)
{
  _LICE::LICE_FillRect(mDrawBitmap, pR->L, pR->T, pR->W(), pR->H(), LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend));
    return true;
}

IColor IGraphicsLice::GetPoint(int x, int y)
{
  LICE_pixel pix = _LICE::LICE_GetPixel(mDrawBitmap, x, y);
  return IColor(LICE_GETA(pix), LICE_GETR(pix), LICE_GETG(pix), LICE_GETB(pix));
}

bool IGraphicsLice::DrawVerticalLine(const IColor* pColor, int xi, int yLo, int yHi)
{
  int W = Width(), H = Height();
  xi = BOUNDED(xi, 0, W - 1);
  yLo = BOUNDED(yLo, 0, H - 1);
  yHi = BOUNDED(yHi, 0, H - 1);

  LICE_pixel px = LiceColor(pColor);
  LICE_pixel* pPx = mDrawBitmap->getBits() + yLo * W + xi;
  for (int i = yLo; i <= yHi; ++i, pPx += W) {
    *pPx = px;
  }
  return true;
}

bool IGraphicsLice::DrawHorizontalLine(const IColor* pColor, int yi, int xLo, int xHi)
{
  int W = Width(), H = Height();
  yi = BOUNDED(yi, 0, H - 1);
  xLo = BOUNDED(xLo, 0, W - 1);
  xHi = BOUNDED(xHi, 0, W - 1);
    
  LICE_pixel px = LiceColor(pColor);
  LICE_pixel* pPx = mDrawBitmap->getBits() + yi * W + xLo;
  for (int i = xLo; i <= xHi; ++i, ++pPx) {
    *pPx = px;
  }
  return true;
}
 
struct ScaledBitmapCacheKey
{
  LICE_IBitmap* mSrcBitmap;
  int mW, mH;
};

IBitmap IGraphicsLice::ScaleBitmap(IBitmap* pIBitmap, int destW, int destH)
{
  static WDL_Mutex sMutex;
  static WDL_TypedBuf<ScaledBitmapCacheKey> sCachedKeys;
  static WDL_TypedBuf<IBitmap> sCachedBitmaps;

  WDL_MutexLock cacheLock(&sMutex);
  int i, n = sCachedKeys.GetSize();
  ScaledBitmapCacheKey* pKey = sCachedKeys.Get();
  IBitmap* pCachedBitmap = 0;
  for (i = 0; i < n; ++i, ++pKey) {
    if ((LICE_IBitmap*) pIBitmap->mData == pKey->mSrcBitmap && destW == pKey->mW && destH == pKey->mH) {
      pCachedBitmap = sCachedBitmaps.Get() + i;
      if (ExistsInStorage((LICE_IBitmap*) pCachedBitmap->mData)) {
       return *pCachedBitmap;
     }
     // Somebody, probably another plugin instance, released the bitmap data
     // out from under the function-local cache, so we need to reload it.
     // We can reuse this pointer to the function-local cache.
     break;
    }
  }

  if (!pCachedBitmap) { // Make a new entry in the function-local cache.
    sCachedKeys.Resize(n + 1);
    sCachedBitmaps.Resize(n + 1);
    pKey = sCachedKeys.Get() + n;
    pKey->mSrcBitmap = (LICE_IBitmap*) pIBitmap->mData;
    pKey->mW = destW;
    pKey->mH = destH;
    pCachedBitmap = sCachedBitmaps.Get() + n;
  }

  LICE_IBitmap* pSrc = (LICE_IBitmap*) pIBitmap->mData;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  _LICE::LICE_ScaledBlit(pDest, pSrc, 0, 0, destW, destH, 0.0f, 0.0f, (float) pIBitmap->W, (float) pIBitmap->H, 1.0f, 
    LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR);
  *pCachedBitmap = IBitmap(pDest, destW, destH, pIBitmap->N);
  RetainBitmap(pCachedBitmap);
  return *pCachedBitmap;
}

struct CroppedBitmapCacheKey
{
  LICE_IBitmap* mSrcBitmap;
  IRECT mR;
};

IBitmap IGraphicsLice::CropBitmap(IBitmap* pIBitmap, IRECT* pR)
{
  static WDL_Mutex sMutex;
  static WDL_TypedBuf<CroppedBitmapCacheKey> sCachedKeys;
  static WDL_TypedBuf<IBitmap> sCachedBitmaps;

  WDL_MutexLock cacheLock(&sMutex);
  int i, n = sCachedKeys.GetSize();
  CroppedBitmapCacheKey* pKey = sCachedKeys.Get();
  IBitmap* pCachedBitmap = 0;
  for (i = 0; i < n; ++i, ++pKey) {
    if ((LICE_IBitmap*) pIBitmap->mData == pKey->mSrcBitmap && *pR == pKey->mR) {
      pCachedBitmap = sCachedBitmaps.Get() + i;
      if (ExistsInStorage((LICE_IBitmap*) pCachedBitmap->mData)) {
       return *pCachedBitmap;
     }
     // Somebody, probably another plugin instance, released the bitmap data
     // out from under the function-local cache, so we need to reload it.
     // We can reuse this pointer to the function-local cache.
     break;
    }
  }

  if (!pCachedBitmap) { // Make a new entry in the function-local cache.
    sCachedKeys.Resize(n + 1);
    sCachedBitmaps.Resize(n + 1);
    pKey = sCachedKeys.Get() + n;
    pKey->mSrcBitmap = (LICE_IBitmap*) pIBitmap->mData;
    pKey->mR = *pR;
    pCachedBitmap = sCachedBitmaps.Get() + n;
  }

  int destW = pR->W(), destH = pR->H();
  LICE_IBitmap* pSrc = (LICE_IBitmap*) pIBitmap->mData;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  _LICE::LICE_Blit(pDest, pSrc, 0, 0, pR->L, pR->T, destW, destH, 1.0f, LICE_BLIT_MODE_COPY);
  *pCachedBitmap = IBitmap(pDest, destW, destH, pIBitmap->N);
  RetainBitmap(pCachedBitmap);
  return *pCachedBitmap;
}

LICE_pixel* IGraphicsLice::GetBits()
{
  return mDrawBitmap->getBits();
}