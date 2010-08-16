#ifndef _IGRAPHICSLICE_
#define _IGRAPHICSLICE_

#include "IControl.h"
#include "../lice/lice.h"

// Specialty stuff for calling in to Reaper for Lice functionality.
#ifdef REAPER_SPECIAL
  #include "../IPlugExt/ReaperExt.h"
  #define _LICE ReaperExt
#else
  #define _LICE
#endif

class IGraphicsLice : public IGraphics
{
public:

	IGraphicsLice(IPlugBase* pPlug, int w, int h, int refreshFPS = 0);
	~IGraphicsLice();

  void PrepDraw();

	bool DrawBitmap(IBitmap* pBitmap, IRECT* pDest, int srcX, int srcY,
    const IChannelBlend* pBlend = 0);
	virtual bool DrawRotatedBitmap(IBitmap* pBitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg = 0,
    const IChannelBlend* pBlend = 0); 
	virtual bool DrawRotatedMask(IBitmap* pBase, IBitmap* pMask, IBitmap* pTop, int x, int y, double angle,
    const IChannelBlend* pBlend = 0); 
	bool DrawPoint(const IColor* pColor, float x, float y, 
		const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool ForcePixel(const IColor* pColor, int x, int y);
	bool DrawLine(const IColor* pColor, float x1, float y1, float x2, float y2,
		const IChannelBlend* pBlend = 0, bool antiAlias = false);
	bool DrawArc(const IColor* pColor, float cx, float cy, float r, float minAngle, float maxAngle, 
		const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool DrawCircle(const IColor* pColor, float cx, float cy, float r,
  const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool FillIRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend = 0);
  IColor GetPoint(int x, int y);
  void* GetData() { return GetBits(); }
  bool DrawVerticalLine(const IColor* pColor, int xi, int yLo, int yHi);
  bool DrawHorizontalLine(const IColor* pColor, int yi, int xLo, int xHi);

  // The backing store for any bitmaps created by LoadIBitmap, ScaleBitmap, or CropBitmap,
  // or any bitmaps passed to RetainBitmap, will be retained across all plugin instances 
  // until explicitly released, or until the dll completely unloads.
    
  IBitmap LoadIBitmap(int ID, const char* name, int nStates = 1);

  // Specialty use...
  IBitmap ScaleBitmap(IBitmap* pBitmap, int destW, int destH);
  IBitmap CropBitmap(IBitmap* pBitmap, IRECT* pR);
  void RetainBitmap(IBitmap* pBitmap);
  void ReleaseBitmap(IBitmap* pBitmap);
  LICE_pixel* GetBits();

protected:
  virtual LICE_IBitmap* OSLoadBitmap(int ID, const char* name) = 0;
	LICE_SysBitmap* mDrawBitmap;
  
private:
	LICE_MemBitmap* mTmpBitmap;
};

////////////////////////////////////////

#endif