#include "IGraphicsLice.h"
#include "IControl.h"
#include "math.h"
#include "Log.h"

extern HINSTANCE gHInstance;

class BitmapStorage
{
public:

  struct BitmapKey
  {
    int id;
    LICE_IBitmap* bitmap;
  };
  
  WDL_PtrList<BitmapKey> m_bitmaps;
  WDL_Mutex m_mutex;

  LICE_IBitmap* Find(int id)
  {
    WDL_MutexLock lock(&m_mutex);
    int i, n = m_bitmaps.GetSize();
    for (i = 0; i < n; ++i)
    {
      BitmapKey* key = m_bitmaps.Get(i);
      if (key->id == id) return key->bitmap;
    }
    return 0;
  }

  void Add(LICE_IBitmap* bitmap, int id = -1)
  {
    WDL_MutexLock lock(&m_mutex);
    BitmapKey* key = m_bitmaps.Add(new BitmapKey);
    key->id = id;
    key->bitmap = bitmap;
  }

  void Remove(LICE_IBitmap* bitmap)
  {
    WDL_MutexLock lock(&m_mutex);
    int i, n = m_bitmaps.GetSize();
    for (i = 0; i < n; ++i)
    {
      if (m_bitmaps.Get(i)->bitmap == bitmap)
      {
        m_bitmaps.Delete(i, true);
        delete(bitmap);
        break;
      }
    }
  }

  ~BitmapStorage()
  {
    int i, n = m_bitmaps.GetSize();
    for (i = 0; i < n; ++i)
    {
      delete(m_bitmaps.Get(i)->bitmap);
    }
    m_bitmaps.Empty(true);
  }
};

static BitmapStorage s_bitmapCache;

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
  LICE_IBitmap* lb = s_bitmapCache.Find(ID); 
  if (!lb)
  {
    lb = OSLoadBitmap(ID, name);
    bool imgResourceFound = (lb);
    assert(imgResourceFound); // Protect against typos in resource.h and .rc files.
    s_bitmapCache.Add(lb, ID);
  }
  return IBitmap(lb, lb->getWidth(), lb->getHeight(), nStates);
}

void IGraphicsLice::RetainBitmap(IBitmap* pBitmap)
{
  s_bitmapCache.Add((LICE_IBitmap*)pBitmap->mData);
}

void IGraphicsLice::ReleaseBitmap(IBitmap* pBitmap)
{
  s_bitmapCache.Remove((LICE_IBitmap*)pBitmap->mData);
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
  _LICE::LICE_Line(mDrawBitmap, (float)xi, (float)yLo, (float)xi, (float)yHi, LiceColor(pColor), 1.0f, LICE_BLIT_MODE_COPY, false);
  return true;
}

bool IGraphicsLice::DrawHorizontalLine(const IColor* pColor, int yi, int xLo, int xHi)
{
  _LICE::LICE_Line(mDrawBitmap, (float)xLo, (float)yi, (float)xHi, (float)yi, LiceColor(pColor), 1.0f, LICE_BLIT_MODE_COPY, false);
  return true;
}

IBitmap IGraphicsLice::ScaleBitmap(IBitmap* pIBitmap, int destW, int destH)
{
  LICE_IBitmap* pSrc = (LICE_IBitmap*) pIBitmap->mData;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  _LICE::LICE_ScaledBlit(pDest, pSrc, 0, 0, destW, destH, 0.0f, 0.0f, (float) pIBitmap->W, (float) pIBitmap->H, 1.0f, 
    LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR);

  IBitmap bmp(pDest, destW, destH, pIBitmap->N);
  RetainBitmap(&bmp);
  return bmp;
}

IBitmap IGraphicsLice::CropBitmap(IBitmap* pIBitmap, IRECT* pR)
{
  int destW = pR->W(), destH = pR->H();
  LICE_IBitmap* pSrc = (LICE_IBitmap*) pIBitmap->mData;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  _LICE::LICE_Blit(pDest, pSrc, 0, 0, pR->L, pR->T, destW, destH, 1.0f, LICE_BLIT_MODE_COPY);

  IBitmap bmp(pDest, destW, destH, pIBitmap->N);
  RetainBitmap(&bmp);
  return bmp;
}

LICE_pixel* IGraphicsLice::GetBits()
{
  return mDrawBitmap->getBits();
}