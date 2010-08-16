/*
    WDL - virtwnd-controls.h
    Copyright (C) 2006 and later Cockos Incorporated

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
      
*/



#ifndef _WDL_VIRTWND_CONTROLS_H_
#define _WDL_VIRTWND_CONTROLS_H_

#include "virtwnd.h"
#include "virtwnd-skin.h"

#include "../lice/lice_text.h"

// an app should implement these
extern int WDL_STYLE_WantGlobalButtonBorders();
extern bool WDL_STYLE_WantGlobalButtonBackground(int *col);
extern int WDL_STYLE_GetSysColor(int);
extern void WDL_STYLE_ScaleImageCoords(int *x, int *y);
extern bool WDL_Style_WantTextShadows(int *col);

// this is the default, you can override per painter if you want
extern bool WDL_STYLE_GetBackgroundGradient(double *gradstart, double *gradslope); // return values 0.0-1.0 for each, return false if no gradient desired

// for slider
extern LICE_IBitmap *WDL_STYLE_GetSliderBitmap2(bool vert);
extern bool WDL_STYLE_AllowSliderMouseWheel();
extern int WDL_STYLE_GetSliderDynamicCenterPos();






// virtwnd-iconbutton.cpp
class WDL_VirtualIconButton : public WDL_VWnd
{
  public:
    WDL_VirtualIconButton();
    ~WDL_VirtualIconButton();
    void SetEnabled(bool en) {m_en=en; }
    bool GetEnabled() { return m_en; }
    void OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);
    void OnPaintOver(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);
    void SetIcon(WDL_VirtualIconButton_SkinConfig *cfg) { if (m_iconCfg!=cfg) { m_iconCfg=cfg; RequestRedraw(NULL); } }
    void SetIsButton(bool isbutton) { m_is_button=isbutton; }

    int OnMouseDown(int xpos, int ypos); // return -1 to eat, >0 to capture
    void OnMouseMove(int xpos, int ypos);
    void OnMouseUp(int xpos, int ypos);
    bool OnMouseDblClick(int xpos, int ypos);

    void SetBGCol1Callback(int msg) { m_bgcol1_msg=msg; }

    bool WantsPaintOver();
    void GetPositionPaintOverExtent(RECT *r);

    void SetForceBorder(bool fb) { m_forceborder=fb; }

    // only used if no icon config set
    void SetTextLabel(const char *text, char align=0, LICE_IFont *font=NULL);
    void SetCheckState(char state); // -1 = no checkbox, 0=unchecked, 1=checked
    char GetCheckState() { return m_checkstate; }
    
  private:

    WDL_VirtualIconButton_SkinConfig *m_iconCfg;
    int m_bgcol1_msg;
    bool m_is_button,m_forceborder;
    char m_pressed;
    bool m_en;
    char m_textalign;
    char m_checkstate;

    WDL_String m_textlbl;
    LICE_IFont *m_textfont;
};



class WDL_VirtualStaticText : public WDL_VWnd
{
  public:
    WDL_VirtualStaticText();
    ~WDL_VirtualStaticText();
    void SetWantSingleClick(bool ws) {m_wantsingle=ws; }
    void OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);
    void SetFont(LICE_IFont *font) { m_font=font; }
    LICE_IFont *GetFont() { return m_font; }
    void SetAlign(int align) { m_align=align; } // -1=left,0=center,1=right
    void SetText(const char *text);
    void SetBorder(bool bor) { m_wantborder=bor; }
    const char *GetText() { return m_text.Get(); }
    void SetColor(int fg=-1, int bg=-1, bool tint=false) { m_fg=fg; m_bg=bg; m_dotint=tint; }
    void SetMargins(int l, int r) { m_margin_l=l; m_margin_r=r; }
    void SetBkImage(WDL_VirtualWnd_BGCfg *bm) { m_bkbm=bm; }

    bool OnMouseDblClick(int xpos, int ypos);
    int OnMouseDown(int xpos, int ypos);

  private:
    WDL_VirtualWnd_BGCfg *m_bkbm;
    int m_align;
    bool m_dotint;
    int m_fg,m_bg;
    int m_margin_r, m_margin_l;
    bool m_wantborder;
    bool m_wantsingle;
    LICE_IFont *m_font;
    WDL_String m_text;
};

class WDL_VirtualComboBox : public WDL_VWnd
{
  public:
    WDL_VirtualComboBox();
    ~WDL_VirtualComboBox();
    void OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);
    void SetFont(LICE_IFont *font) { m_font=font; }
    LICE_IFont *GetFont() { return m_font; }
    void SetAlign(int align) { m_align=align; } // -1=left,0=center,1=right

    int GetCurSel() { if (m_items.Get(m_curitem)) return m_curitem; return -1; }
    void SetCurSel(int sel) { if (!m_items.Get(sel)) sel=-1; if (m_curitem != sel) { m_curitem=sel; RequestRedraw(NULL); } }

    int GetCount() { return m_items.GetSize(); }
    void Empty() { m_items.Empty(true,free); m_itemdatas.Empty(); }

    int AddItem(const char *str, void *data=NULL) { m_items.Add(strdup(str)); m_itemdatas.Add(data); return m_items.GetSize()-1; }
    const char *GetItem(int item) { return m_items.Get(item); }
    void *GetItemData(int item) { return m_itemdatas.Get(item); }

    int OnMouseDown(int xpos, int ypos);

  private:
    int m_align;
    int m_curitem;
    LICE_IFont *m_font;

    WDL_PtrList<char> m_items;
    WDL_PtrList<void> m_itemdatas;
};



class WDL_VirtualSlider : public WDL_VWnd
{
  public:
    WDL_VirtualSlider();
    ~WDL_VirtualSlider();

    void SetBGCol1Callback(int msg) { m_bgcol1_msg=msg; }
    void SetScrollMessage(int msg) { m_scrollmsg=msg; }
    void SetRange(int minr, int maxr, int center) { m_minr=minr; m_maxr=maxr; m_center=center; }
    void GetRange(int *minr, int *maxr, int *center) { if (minr) *minr=m_minr; if (maxr) *maxr=m_maxr; if (center) *center=m_center; }
    int GetSliderPosition();
    void SetSliderPosition(int pos);
    bool GetIsVert();
    void SetNotifyOnClick(bool en) { m_sendmsgonclick=en; }

    void GetButtonSize(int *w, int *h);

    void OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);

    int OnMouseDown(int xpos, int ypos);
    void OnMouseMove(int xpos, int ypos);
    void OnMouseUp(int xpos, int ypos);
    bool OnMouseDblClick(int xpos, int ypos);
    bool OnMouseWheel(int xpos, int ypos, int amt);

    void SetSkinImageInfo(WDL_VirtualSlider_SkinConfig *cfg) { m_skininfo=cfg; }

    // override
    virtual void GetPositionPaintExtent(RECT *r);

  private:
    WDL_VirtualSlider_SkinConfig *m_skininfo;

    int m_bgcol1_msg,m_scrollmsg;
    void OnMoveOrUp(int xpos, int ypos, int isup);
    int m_minr, m_maxr, m_center, m_pos;

    int m_tl_extra, m_br_extra;

    bool m_captured;
    bool m_needflush;
    bool m_sendmsgonclick;
    
};


class WDL_VirtualListBox : public WDL_VWnd
{
  public:
    WDL_VirtualListBox();
    ~WDL_VirtualListBox();
    void OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);
    void SetFont(LICE_IFont *font) { m_font=font; }
    LICE_IFont *GetFont() { return m_font; }
    void SetAlign(int align) { m_align=align; } // -1=left,0=center,1=right
    void SetRowHeight(int rh) { m_rh=rh; }
    void SetMargins(int l, int r) { m_margin_l=l; m_margin_r=r; }

    void SetDroppedMessage(int msg) { m_dropmsg=msg; }
    void SetClickedMessage(int msg) { m_clickmsg=msg; }
    void SetDragBeginMessage(int msg) { m_dragbeginmsg=msg; }
    int IndexFromPt(int x, int y);

    // idx<0 means return count of items
    int (*m_GetItemInfo)(WDL_VirtualListBox *sender, int idx, char *nameout, int namelen, int *color, void **bkbg); // bkbg=LICE_IBitmap* if idx>=0, otherwise WDL_VirtualWnd_BGCfg
    void (*m_CustomDraw)(WDL_VirtualListBox *sender, int idx, RECT *r, LICE_SysBitmap *drawbm);
    void *m_GetItemInfo_ctx;

    int OnMouseDown(int xpos, int ypos);
    bool OnMouseDblClick(int xpos, int ypos);
    bool OnMouseWheel(int xpos, int ypos, int amt);
    void OnMouseMove(int xpos, int ypos);
    void OnMouseUp(int xpos, int ypos);

  private:
    int m_cap_state;
    int m_cap_startitem;
    int m_clickmsg,m_dropmsg,m_dragbeginmsg;
    int m_viewoffs;
    int m_align;
    int m_margin_r, m_margin_l;
    int m_rh;
    LICE_IFont *m_font;
};


#endif