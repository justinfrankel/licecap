#ifndef _WDL_VIRTWND_SKIN_H_
#define _WDL_VIRTWND_SKIN_H_

class LICE_IBitmap;

typedef struct // if set these override the default virtualwnd styles for this object
{
  LICE_IBitmap *bgimage;
  int bgimage_lt[2],bgimage_rb[2]; // size of 
  int bgimage_lt_out[2],bgimage_rb_out[2]; // size of outside area (like shadows)
  int bgimage_noalphaflags; // 4x4 flags of "no alpha", so 65535 is image has no alpha whatsoever
} WDL_VirtualWnd_BGCfg;

void WDL_VirtualWnd_PreprocessBGConfig(WDL_VirtualWnd_BGCfg *a);

// used by elements to draw a WDL_VirtualWnd_BGCfg
void WDL_VirtualWnd_ScaledBlitBG(LICE_IBitmap *dest, 
                                 WDL_VirtualWnd_BGCfg *src,
                                 int destx, int desty, int destw, int desth,
                                 int clipx, int clipy, int clipw, int cliph,
                                 float alpha, int mode);
int WDL_VirtualWnd_ScaledBG_GetPix(WDL_VirtualWnd_BGCfg *src,
                                   int ww, int wh,
                                   int x, int y);


typedef struct // if set these override the default virtualwnd styles for this object
{
  WDL_VirtualWnd_BGCfg bgimagecfg[2];
  LICE_IBitmap *thumbimage[2]; // h,v 
  int thumbimage_lt[2],thumbimage_rb[2];
  unsigned int zeroline_color; // needs alpha channel set!
} WDL_VirtualSlider_SkinConfig;

void WDL_VirtualSlider_PreprocessSkinConfig(WDL_VirtualSlider_SkinConfig *a);

typedef struct
{
  LICE_IBitmap *image; // 2x width, second half is "mouseover" image. or straight image if image_issingle set
  LICE_IBitmap *olimage; // drawn in second pass

  bool image_ltrb_used,image_issingle;
  int image_ltrb[4]; // extents outside the rect
} WDL_VirtualIconButton_SkinConfig;

void WDL_VirtualIconButton_PreprocessSkinConfig(WDL_VirtualIconButton_SkinConfig *a);



#endif