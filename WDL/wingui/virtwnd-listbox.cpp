/*
    WDL - virtwnd-listbox.cpp
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
      

    Implementation for virtual window listboxes.

*/

#include "virtwnd-controls.h"
#include "../lice/lice.h"

WDL_VirtualListBox::WDL_VirtualListBox()
{
  m_cap_startitem=-1;
  m_cap_state=0;
  m_margin_l=m_margin_r=0;
  m_GetItemInfo=0;
  m_CustomDraw=0;
  m_GetItemInfo_ctx=0;
  m_viewoffs=0;
  m_align=-1;
  m_rh=14;
  m_font=0;
  m_clickmsg=0;
  m_dropmsg=0;
  m_dragbeginmsg=0;
}

WDL_VirtualListBox::~WDL_VirtualListBox()
{
}

void WDL_VirtualListBox::OnPaint(LICE_SysBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
  HDC hdc=drawbm->getDC();

  RECT r=m_position;
  r.left+=origin_x;
  r.right+=origin_x;
  r.top+=origin_y;
  r.bottom+=origin_y;

  WDL_VirtualWnd_BGCfg *mainbk=0;
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,(void**)&mainbk) : 0;
  LICE_pixel bgc=WDL_STYLE_GetSysColor(COLOR_BTNFACE);
  bgc=LICE_RGBA_FROMNATIVE(bgc,255);
  if (mainbk && mainbk->bgimage)
  {
    if (mainbk->bgimage->getWidth()>1 && mainbk->bgimage->getHeight()>1)
    {
      WDL_VirtualWnd_ScaledBlitBG(drawbm,mainbk,
                                    r.left,r.top,r.right-r.left,r.bottom-r.top,
                                    cliprect->left,cliprect->top,cliprect->right,cliprect->bottom,
                                    1.0,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
    }
  }
  else
  {
    LICE_FillRect(drawbm,r.left,r.top,r.right-r.left,r.bottom-r.top,bgc,1.0f,LICE_BLIT_MODE_COPY);
  }

  int updownbuttonsize=0; // &1= has top button, &2= has bottom button
  int startpos=0;

  if (m_rh<7) m_rh=7;
  if (num_items*m_rh > m_position.bottom-m_position.top)
  {
    updownbuttonsize = m_rh;
    startpos=m_viewoffs;
    int nivis=(m_position.bottom-m_position.top-updownbuttonsize)/m_rh;
    if (startpos+nivis > num_items) startpos=num_items-nivis;
    if (startpos<0)startpos=0;
  }
  LICE_pixel pencol = WDL_STYLE_GetSysColor(COLOR_3DSHADOW);
  LICE_pixel pencol2 = WDL_STYLE_GetSysColor(COLOR_3DHILIGHT);
  pencol=LICE_RGBA_FROMNATIVE(pencol,255);
  pencol2=LICE_RGBA_FROMNATIVE(pencol2,255);

  int y;
  LICE_pixel tcol=WDL_STYLE_GetSysColor(COLOR_BTNTEXT);
  tcol=LICE_RGBA_FROMNATIVE(tcol,0);

  int endpos=r.bottom;
  int itempos=startpos;
  if (m_font) m_font->SetBkMode(TRANSPARENT);
  if (updownbuttonsize) endpos-=updownbuttonsize;
  for (y = r.top + m_rh; y <= endpos; y += m_rh)
  {
    int ly=y-m_rh;
    LICE_IBitmap *bkbm=0;
    if (m_GetItemInfo && ly >= r.top)
    {
      char buf[64];
      buf[0]=0;
      int color=tcol;

      if (m_GetItemInfo(this,itempos++,buf,sizeof(buf),&color,(void**)&bkbm))
      {
        RECT thisr=r;
        thisr.top = ly+1;
        thisr.bottom = y-1;
        int rev=0;
        int bkbmstate=0;
        if (m_cap_state==1 && m_cap_startitem==itempos-1)
        {
          if (bkbm) bkbmstate=1;
          else color = ((color>>1)&0x7f7f7f7f)+LICE_RGBA(0x7f,0x7f,0x7f,0);
        }
        if (m_cap_state>=0x1000 && m_cap_startitem==itempos-1)
        {
          if (bkbm) bkbmstate=2;
          else
          {
            rev=1;
            LICE_FillRect(drawbm,thisr.left,thisr.top,thisr.right-thisr.left,thisr.bottom-thisr.top,
              color,1.0f,LICE_BLIT_MODE_COPY);
          }
        }
        if (bkbm) //draw image!
        {
          int hh=bkbm->getHeight()/3;
          LICE_ScaledBlit(drawbm,bkbm,
            thisr.left,thisr.top-1,thisr.right-thisr.left,thisr.bottom-thisr.top+2,
            0,bkbmstate*hh,
            bkbm->getWidth(),hh,1.0,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
        }
        if (m_CustomDraw)
          m_CustomDraw(this,itempos-1,&thisr,drawbm);


        if (buf[0])
        {
          thisr.left+=m_margin_l;
          thisr.right-=m_margin_r;
          if (m_font)
          {
            m_font->SetTextColor(rev?bgc:color);
            m_font->DrawText(drawbm,buf,-1,&thisr,DT_SINGLELINE|DT_VCENTER|(m_align<0?DT_LEFT:m_align>0?DT_RIGHT:DT_CENTER)|DT_NOPREFIX);
          }
        }
      }
    }

    if (!bkbm)
    {
      LICE_Line(drawbm,r.left,y,r.right,y,pencol2,1.0f,LICE_BLIT_MODE_COPY,false);
    }
  }
  if (updownbuttonsize)
  {
    LICE_IBitmap *bkbm=0;
    int a=m_GetItemInfo ? m_GetItemInfo(this,0x10000000,NULL,0,NULL,(void**)&bkbm) : 0;

    if (bkbm)
    {
      int hh=bkbm->getHeight()/3;


      int bkbmstate=startpos>0 ? 2 : 1;
      LICE_ScaledBlit(drawbm,bkbm,
        r.left,y-m_rh,(r.right-r.left)/2,m_rh,
        0,bkbmstate*hh,
        bkbm->getWidth()/2,hh,1.0,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);

      bkbmstate=itempos<num_items ? 2 : 1;
      LICE_ScaledBlit(drawbm,bkbm,
        (r.left+r.right)/2,y-m_rh,(r.right-r.left) - (r.right-r.left)/2,m_rh,
        bkbm->getWidth()/2,bkbmstate*hh,
        bkbm->getWidth() - bkbm->getWidth()/2,hh,1.0,LICE_BLIT_USE_ALPHA|LICE_BLIT_MODE_COPY|LICE_BLIT_FILTER_BILINEAR);
      
    }

    if (!a||!bkbm)
    {
      int cx=(r.left+r.right)/2;
      int bs=5;
      int bsh=8;
      LICE_Line(drawbm,cx,y-m_rh+2,cx,y-1,pencol2,1.0f,0,false);
      LICE_Line(drawbm,r.left,y,r.right,y,pencol2,1.0f,0,false);

      y-=m_rh/2+bsh/2;

      bool butaa = true;
      if (itempos<num_items)
      {
        cx=(r.left+r.right)*3/4;

        LICE_Line(drawbm,cx-bs+1,y+2,cx,y+bsh-2,pencol2,1.0f,0,butaa);
        LICE_Line(drawbm,cx,y+bsh-2,cx+bs-1,y+2,pencol2,1.0f,0,butaa);
        LICE_Line(drawbm,cx+bs-1,y+2,cx-bs+1,y+2,pencol2,1.0f,0,butaa);

        LICE_Line(drawbm,cx-bs-1,y+1,cx,y+bsh-1,pencol,1.0f,0,butaa);
        LICE_Line(drawbm,cx,y+bsh-1,cx+bs+1,y+1,pencol,1.0f,0,butaa);
        LICE_Line(drawbm,cx+bs+1,y+1,cx-bs-1,y+1,pencol,1.0f,0,butaa);
      }
      if (startpos>0)
      {
        y-=2;
        cx=(r.left+r.right)/4;
        LICE_Line(drawbm,cx-bs+1,y+bsh,cx,y+3+1,pencol2,1.0f,0,butaa);
        LICE_Line(drawbm,cx,y+3+1,cx+bs-1,y+bsh,pencol2,1.0f,0,butaa);
        LICE_Line(drawbm,cx+bs-1,y+bsh,cx-bs+1,y+bsh,pencol2,1.0f,0,butaa);

        LICE_Line(drawbm,cx-bs-1,y+bsh+1,cx,y+3,pencol,1.0f,0,butaa);
        LICE_Line(drawbm,cx,y+3,cx+bs+1,y+bsh+1,pencol,1.0f,0,butaa);
        LICE_Line(drawbm,cx+bs+1,y+bsh+1, cx-bs-1,y+bsh+1, pencol,1.0f,0,butaa);
      }
    }
  }



  if (!mainbk)
  {
    LICE_Line(drawbm,r.left,r.bottom-1,r.left,r.top,pencol,1.0f,0,false);
    LICE_Line(drawbm,r.left,r.top,r.right-1,r.top,pencol,1.0f,0,false);
    LICE_Line(drawbm,r.right-1,r.top,r.right-1,r.bottom-1,pencol2,1.0f,0,false);
    LICE_Line(drawbm,r.right-1,r.bottom-1,r.left,r.bottom-1,pencol2,1.0f,0,false);
  }


}

int WDL_VirtualListBox::OnMouseDown(int xpos, int ypos)
{
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  int updownbuttonsize=0; // &1= has top button, &2= has bottom button
  int startpos=0;

  int wndheight=m_position.bottom-m_position.top;

  if (m_rh<7) m_rh=7;
  if (num_items*m_rh > wndheight)
  {
    updownbuttonsize = m_rh;
    startpos=m_viewoffs;
    int nivis=(wndheight-updownbuttonsize)/m_rh;
    if (startpos+nivis > num_items) startpos=num_items-nivis;
    if (startpos<0)startpos=0;

    if (ypos >= nivis*m_rh)
    {
      if (ypos < (nivis+1)*m_rh)
      {
        if (xpos < (m_position.right-m_position.left)/2)
        {
          if (m_viewoffs>0)
          {
            m_viewoffs--;
            RequestRedraw(NULL);
          }
        }
        else
        {
          if (m_viewoffs+nivis < num_items)
          {
            m_viewoffs++;
            RequestRedraw(NULL);
          }
        }
      }
      m_cap_state=0;
      m_cap_startitem=-1;
      return 1;
    }
  }

  m_cap_state=0x1000;
  m_cap_startitem=startpos + (ypos)/m_rh;
  RequestRedraw(NULL);

  return 1;
}

bool WDL_VirtualListBox::OnMouseDblClick(int xpos, int ypos)
{
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  int updownbuttonsize=0; // &1= has top button, &2= has bottom button
  int startpos=0;

  int wndheight=m_position.bottom-m_position.top;

  if (m_rh<7) m_rh=7;
  if (num_items*m_rh > wndheight)
  {
    updownbuttonsize = m_rh;
    startpos=m_viewoffs;
    int nivis=(wndheight-updownbuttonsize)/m_rh;
    if (startpos+nivis > num_items) startpos=num_items-nivis;
    if (startpos<0)startpos=0;

    if (ypos >= nivis*m_rh)
    {
      if (ypos < (nivis+1)*m_rh)
      {
        if (xpos < (m_position.right-m_position.left)/2)
        {
          if (m_viewoffs>0)
          {
            m_viewoffs--;
            RequestRedraw(NULL);
          }
        }
        else
        {
          if (m_viewoffs+nivis < num_items)
          {
            m_viewoffs++;
            RequestRedraw(NULL);
          }
        }
      }
      m_cap_state=0;
      m_cap_startitem=-1;
      return true;
    }
  }

  return false;
}

bool WDL_VirtualListBox::OnMouseWheel(int xpos, int ypos, int amt)
{
  // todo: adjust view offset and refresh
  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  int wndheight=m_position.bottom-m_position.top;

  if (m_rh<7) m_rh=7;
  if (num_items*m_rh > wndheight)
  {
    if (amt>0 && m_viewoffs>0) 
    {
      m_viewoffs--;
      RequestRedraw(NULL);
    }
    else if (amt<0)
    {
      int updownbuttonsize = m_rh;
      int nivis=(wndheight-updownbuttonsize)/m_rh;
      if (m_viewoffs+nivis < num_items)
      {
        m_viewoffs++;
        RequestRedraw(NULL);
      }
    }
  }
  return true;
}

void WDL_VirtualListBox::OnMouseMove(int xpos, int ypos)
{
  if (m_cap_state>=0x1000)
  {
    m_cap_state++;
    if (m_cap_state==0x1008)
    {
      if (m_dragbeginmsg)
      {
        SendCommand(m_dragbeginmsg,(INT_PTR)this,m_cap_startitem,this);
      }
    }
  }
  else if (m_cap_state==0)
  {
    int a=IndexFromPt(xpos,ypos);
    if (a>=0)
    {
      m_cap_startitem=a;
      m_cap_state=1;
      RequestRedraw(NULL);
    }
  }
  else if (m_cap_state==1)
  {
    int a=xpos >= 0 && xpos < m_position.right-m_position.left ? IndexFromPt(xpos,ypos) : -1;
    if (a>=0 && a != m_cap_startitem)
    {
      m_cap_startitem=a;
      m_cap_state=1;
      RequestRedraw(NULL);
    }
    else if (a<0)
    {
      m_cap_state=0;
      RequestRedraw(NULL);
    }
  }
}

void WDL_VirtualListBox::OnMouseUp(int xpos, int ypos)
{
  int hit=IndexFromPt(xpos,ypos);
  if (m_cap_state>=0x1000 && m_cap_state<0x1008 && hit==m_cap_startitem && 
      xpos >= 0&& xpos < m_position.right-m_position.left) 
  {
    if (m_clickmsg)
    {
      SendCommand(m_clickmsg,(INT_PTR)this,hit,this);
    }
  }
  else if (m_cap_state>=0x1008)
  {
    // send a message saying drag & drop occurred
    if (m_dropmsg)
      SendCommand(m_dropmsg,(INT_PTR)this,m_cap_startitem,this);
  }

  m_cap_state=0;
  RequestRedraw(NULL);
}

int WDL_VirtualListBox::IndexFromPt(int x, int y)
{

  int num_items = m_GetItemInfo ? m_GetItemInfo(this,-1,NULL,0,NULL,NULL) : 0;
  int updownbuttonsize=0; // &1= has top button, &2= has bottom button
  int startpos=0;

  if (m_rh<7) m_rh=7;
  int nivis=(m_position.bottom-m_position.top)/m_rh;
  if (num_items*m_rh > m_position.bottom-m_position.top)
  {
    updownbuttonsize = m_rh;
    startpos=m_viewoffs;
    nivis=(m_position.bottom-m_position.top-updownbuttonsize)/m_rh;
    if (startpos+nivis > num_items) startpos=num_items-nivis;
    if (startpos<0)startpos=0;
  }

  if (y < 0) return -1;  // not a valid y
  if (y >= nivis*m_rh) return -1;

  return startpos + (y)/m_rh;



}
