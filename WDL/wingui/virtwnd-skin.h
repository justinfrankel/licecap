#ifndef _WDL_VIRTWND_SKIN_H_
#define _WDL_VIRTWND_SKIN_H_

class LICE_IBitmap;

typedef struct // if set these override the default virtualwnd styles for this object
{
  LICE_IBitmap *bgimage[2]; // h, v
  LICE_IBitmap *thumbimage[2]; // h,v 
  int thumbimage_lt[2],thumbimage_rb[2];
} WDL_VirtualSlider_SkinConfig;

void WDL_VirtualSlider_PreprocessSkinConfig(WDL_VirtualSlider_SkinConfig *a);

typedef struct
{
  HICON hIcon;
  LICE_IBitmap *image; // 2x width, second half is "mouseover" image
} WDL_VirtualIconButton_SkinConfig;

#endif