
/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Losers (who use OS X))
   Copyright (C) 2006-2007, Cockos, Inc.

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
  

    This file provides basic win32 GDI-->Quartz translation. It uses features that require OS X 10.4+

*/

#ifndef SWELL_PROVIDED_BY_APP

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include "swell.h"
#include "swell-internal.h"

#include "../mutex.h"

#ifndef __LP64__ 
#define SWELL_ATSUI_TEXT_SUPPORT // faster, but unsupported on 64-bit. will fall back to the NSString drawing method
#endif

static NSString *CStringToNSString(const char *str)
{
  if (!str) str="";
  NSString *ret;
  
  ret=(NSString *)CFStringCreateWithCString(NULL,str,kCFStringEncodingUTF8);
  if (ret) return ret;
  ret=(NSString *)CFStringCreateWithCString(NULL,str,kCFStringEncodingASCII);
  return ret;
}


static CGColorRef CreateColor(int col, float alpha=1.0f)
{
  static CGColorSpaceRef cspace;
  
  if (!cspace) cspace=CGColorSpaceCreateDeviceRGB();
  
  CGFloat cols[4]={GetRValue(col)/255.0,GetGValue(col)/255.0,GetBValue(col)/255.0,alpha};
  CGColorRef color=CGColorCreate(cspace,cols);
  return color;
}


#include "swell-gdi-internalpool.h"


HDC SWELL_CreateGfxContext(void *c)
{
  HDC__ *ctx=SWELL_GDP_CTX_NEW();
  NSGraphicsContext *nsc = (NSGraphicsContext *)c;
//  if (![nsc isFlipped])
//    nsc = [NSGraphicsContext graphicsContextWithGraphicsPort:[nsc graphicsPort] flipped:YES];

  ctx->ctx=(CGContextRef)[nsc graphicsPort];
//  CGAffineTransform f={1,0,0,-1,0,0};
  //CGContextSetTextMatrix(ctx->ctx,f);
  //SetTextColor(ctx,0);
  
 // CGContextSelectFont(ctx->ctx,"Arial",12.0,kCGEncodingMacRoman);
  return ctx;
}

#define ALIGN_EXTRA 63
static void *ALIGN_FBUF(void *inbuf)
{
  const UINT_PTR extra = ALIGN_EXTRA;
  return (void *) (((UINT_PTR)inbuf+extra)&~extra); 
}

HDC SWELL_CreateMemContext(HDC hdc, int w, int h)
{
  void *buf=calloc(w*4*h+ALIGN_EXTRA,1);
  if (!buf) return 0;
  CGColorSpaceRef cs=CGColorSpaceCreateDeviceRGB();
  CGContextRef c=CGBitmapContextCreate(ALIGN_FBUF(buf),w,h,8,w*4,cs, kCGImageAlphaNoneSkipFirst);
  CGColorSpaceRelease(cs);
  if (!c)
  {
    free(buf);
    return 0;
  }


  CGContextTranslateCTM(c,0.0,h);
  CGContextScaleCTM(c,1.0,-1.0);


  HDC__ *ctx=SWELL_GDP_CTX_NEW();
  ctx->ctx=(CGContextRef)c;
  ctx->ownedData=buf;
  // CGContextSelectFont(ctx->ctx,"Arial",12.0,kCGEncodingMacRoman);
  
  SetTextColor(ctx,0);
  return ctx;
}


void SWELL_DeleteGfxContext(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (ct)
  {   
    if (ct->ownedData)
    {
      CGContextRelease(ct->ctx);
      free(ct->ownedData);
    }
    if (ct->curtextcol) [ct->curtextcol release];
    SWELL_GDP_CTX_DELETE(ct);
  }
}
HPEN CreatePen(int attr, int wid, int col)
{
  return CreatePenAlpha(attr,wid,col,1.0f);
}

HBRUSH CreateSolidBrush(int col)
{
  return CreateSolidBrushAlpha(col,1.0f);
}



HPEN CreatePenAlpha(int attr, int wid, int col, float alpha)
{
  HGDIOBJ__ *pen=GDP_OBJECT_NEW();
  pen->type=TYPE_PEN;
  pen->wid=wid<0?0:wid;
  pen->color=CreateColor(col,alpha);
  return pen;
}
HBRUSH  CreateSolidBrushAlpha(int col, float alpha)
{
  HGDIOBJ__ *brush=GDP_OBJECT_NEW();
  brush->type=TYPE_BRUSH;
  brush->color=CreateColor(col,alpha);
  brush->wid=0; 
  return brush;
}


HFONT CreateFontIndirect(LOGFONT *lf)
{
  return CreateFont(lf->lfHeight, lf->lfWidth,lf->lfEscapement, lf->lfOrientation, lf->lfWeight, lf->lfItalic, 
                    lf->lfUnderline, lf->lfStrikeOut, lf->lfCharSet, lf->lfOutPrecision,lf->lfClipPrecision, 
                    lf->lfQuality, lf->lfPitchAndFamily, lf->lfFaceName);
}

void DeleteObject(HGDIOBJ pen)
{
  if (pen)
  {
    HGDIOBJ__ *p=(HGDIOBJ__ *)pen;
    if (p->type == TYPE_PEN || p->type == TYPE_BRUSH || p->type == TYPE_FONT || p->type == TYPE_BITMAP)
    {
      if (p->type == TYPE_PEN || p->type == TYPE_BRUSH)
        if (p->wid<0) return;
      if (p->color) CGColorRelease(p->color);
      if (p->fontdict) [p->fontdict release];
      if (p->font_style) ATSUDisposeStyle(p->font_style);
      if (p->wid && p->bitmapptr) [p->bitmapptr release]; 
      GDP_OBJECT_DELETE(p);
    }
    else free(p);
  }
}


HGDIOBJ SelectObject(HDC ctx, HGDIOBJ pen)
{
  HDC__ *c=(HDC__ *)ctx;
  HGDIOBJ__ *p=(HGDIOBJ__*) pen;
  HGDIOBJ__ **mod=0;
  if (!c||!p) return 0;
  
  if (p == (HGDIOBJ__*)TYPE_PEN) mod=&c->curpen;
  else if (p == (HGDIOBJ__*)TYPE_BRUSH) mod=&c->curbrush;
  else if (p == (HGDIOBJ__*)TYPE_FONT) mod=&c->curfont;

  if (mod)
  {
    HGDIOBJ__ *np=*mod;
    *mod=0;
    return np?np:p;
  }

  
  if (p->type == TYPE_PEN) mod=&c->curpen;
  else if (p->type == TYPE_BRUSH) mod=&c->curbrush;
  else if (p->type == TYPE_FONT) mod=&c->curfont;
  else return 0;
  
  HGDIOBJ__ *op=*mod;
  if (!op) op=(HGDIOBJ__*)p->type;
  if (op != p)
  {
    *mod=p;  
  }
  return op;
}



void SWELL_FillRect(HDC ctx, RECT *r, HBRUSH br)
{
  HDC__ *c=(HDC__ *)ctx;
  HGDIOBJ__ *b=(HGDIOBJ__*) br;
  if (!c || !b || b == (HGDIOBJ__*)TYPE_BRUSH || b->type != TYPE_BRUSH) return;

  if (b->wid<0) return;
  
  CGRect rect=CGRectMake(r->left,r->top,r->right-r->left,r->bottom-r->top);
  CGContextSetFillColorWithColor(c->ctx,b->color);
  CGContextFillRect(c->ctx,rect);	

}

void RoundRect(HDC ctx, int x, int y, int x2, int y2, int xrnd, int yrnd)
{
	xrnd/=3;
	yrnd/=3;
	POINT pts[10]={ // todo: curves between edges
		{x,y+yrnd},
		{x+xrnd,y},
		{x2-xrnd,y},
		{x2,y+yrnd},
		{x2,y2-yrnd},
		{x2-xrnd,y2},
		{x+xrnd,y2},
		{x,y2-yrnd},		
    {x,y+yrnd},
		{x+xrnd,y},
};
	
	WDL_GDP_Polygon(ctx,pts,sizeof(pts)/sizeof(pts[0]));
}

void Ellipse(HDC ctx, int l, int t, int r, int b)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!c) return;
  
  CGRect rect=CGRectMake(l,t,r-l,b-t);
  
  if (c->curbrush && c->curbrush->wid >=0)
  {
    CGContextSetFillColorWithColor(c->ctx,c->curbrush->color);
    CGContextFillEllipseInRect(c->ctx,rect);	
  }
  if (c->curpen && c->curpen->wid >= 0)
  {
    CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
    CGContextStrokeEllipseInRect(c->ctx, rect); //, (float)max(1,c->curpen->wid));
  }
}

void Rectangle(HDC ctx, int l, int t, int r, int b)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!c) return;
  
  CGRect rect=CGRectMake(l,t,r-l,b-t);
  
  if (c->curbrush && c->curbrush->wid >= 0)
  {
    CGContextSetFillColorWithColor(c->ctx,c->curbrush->color);
    CGContextFillRect(c->ctx,rect);	
  }
  if (c->curpen && c->curpen->wid >= 0)
  {
    CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
    CGContextStrokeRectWithWidth(c->ctx, rect, (float)max(1,c->curpen->wid));
  }
}

HGDIOBJ GetStockObject(int wh)
{
  switch (wh)
  {
    case NULL_BRUSH:
    {
      static HGDIOBJ__ br={0,};
      br.type=TYPE_BRUSH;
      br.wid=-1;
      return &br;
    }
    case NULL_PEN:
    {
      static HGDIOBJ__ pen={0,};
      pen.type=TYPE_PEN;
      pen.wid=-1;
      return &pen;
    }
  }
  return 0;
}

void Polygon(HDC ctx, POINT *pts, int npts)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!c) return;
  if (((!c->curbrush||c->curbrush->wid<0) && (!c->curpen||c->curpen->wid<0)) || npts<2) return;

  CGContextBeginPath(c->ctx);
  CGContextMoveToPoint(c->ctx,(float)pts[0].x,(float)pts[0].y);
  int x;
  for (x = 1; x < npts; x ++)
  {
    CGContextAddLineToPoint(c->ctx,(float)pts[x].x,(float)pts[x].y);
  }
  if (c->curbrush && c->curbrush->wid >= 0)
  {
    CGContextSetFillColorWithColor(c->ctx,c->curbrush->color);
  }
  if (c->curpen && c->curpen->wid>=0)
  {
    CGContextSetLineWidth(c->ctx,(float)max(c->curpen->wid,1));
    CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);	
  }
  CGContextDrawPath(c->ctx,c->curpen && c->curpen->wid>=0 && c->curbrush && c->curbrush->wid>=0 ?  kCGPathFillStroke : c->curpen && c->curpen->wid>=0 ? kCGPathStroke : kCGPathFill);
}

void MoveToEx(HDC ctx, int x, int y, POINT *op)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!c) return;
  if (op) 
  { 
    op->x = (int) (c->lastpos_x);
    op->y = (int) (c->lastpos_y);
  }
  c->lastpos_x=(float)x;
  c->lastpos_y=(float)y;
}

void PolyBezierTo(HDC ctx, POINT *pts, int np)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!c||!c->curpen||c->curpen->wid<0||np<3) return;
  
  CGContextSetLineWidth(c->ctx,(float)max(c->curpen->wid,1));
  CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
	
  CGContextBeginPath(c->ctx);
  CGContextMoveToPoint(c->ctx,c->lastpos_x,c->lastpos_y);
  int x; 
  float xp,yp;
  for (x = 0; x < np-2; x += 3)
  {
    CGContextAddCurveToPoint(c->ctx,
      (float)pts[x].x,(float)pts[x].y,
      (float)pts[x+1].x,(float)pts[x+1].y,
      xp=(float)pts[x+2].x,yp=(float)pts[x+2].y);    
  }
  c->lastpos_x=(float)xp;
  c->lastpos_y=(float)yp;
  CGContextStrokePath(c->ctx);
}


void SWELL_LineTo(HDC ctx, int x, int y)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!c||!c->curpen||c->curpen->wid<0) return;

  float w = (float)max(c->curpen->wid,1);
  CGContextSetLineWidth(c->ctx,w);
  CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
	
  CGContextBeginPath(c->ctx);
  CGContextMoveToPoint(c->ctx,c->lastpos_x + w * 0.5,c->lastpos_y + w*0.5);
  float fx=(float)x,fy=(float)y;
  
  CGContextAddLineToPoint(c->ctx,fx+w*0.5,fy+w*0.5);
  c->lastpos_x=fx;
  c->lastpos_y=fy;
  CGContextStrokePath(c->ctx);
}

void PolyPolyline(HDC ctx, POINT *pts, DWORD *cnts, int nseg)
{
  HDC__ *c=(HDC__ *)ctx;
  if (!c||!c->curpen||c->curpen->wid<0||nseg<1) return;

  float w = (float)max(c->curpen->wid,1);
  CGContextSetLineWidth(c->ctx,w);
  CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
	
  CGContextBeginPath(c->ctx);
  
  while (nseg-->0)
  {
    DWORD cnt=*cnts++;
    if (!cnt) continue;
    if (!--cnt) { pts++; continue; }
    
    CGContextMoveToPoint(c->ctx,(float)pts->x+w*0.5,(float)pts->y+w*0.5);
    pts++;
    
    while (cnt--)
    {
      CGContextAddLineToPoint(c->ctx,(float)pts->x+w*0.5,(float)pts->y+w*0.5);
      pts++;
    }
  }
  CGContextStrokePath(c->ctx);
}
void *SWELL_GetCtxGC(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!ct) return 0;
  return ct->ctx;
}


void SWELL_SetPixel(HDC ctx, int x, int y, int c)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!ct) return;
  CGContextBeginPath(ct->ctx);
  CGContextMoveToPoint(ct->ctx,(float)x-0.5,(float)y-0.5);
  CGContextAddLineToPoint(ct->ctx,(float)x+0.5,(float)y+0.5);
  CGContextSetLineWidth(ct->ctx,(float)1.0);
  CGContextSetRGBStrokeColor(ct->ctx,GetRValue(c)/255.0,GetGValue(c)/255.0,GetBValue(c)/255.0,1.0);
  CGContextStrokePath(ct->ctx);	
}


HFONT CreateFont(int lfHeight, int lfWidth, int lfEscapement, int lfOrientation, int lfWeight, char lfItalic, 
                 char lfUnderline, char lfStrikeOut, char lfCharSet, char lfOutPrecision, char lfClipPrecision, 
                 char lfQuality, char lfPitchAndFamily, const char *lfFaceName)
{
  HGDIOBJ__ *font=GDP_OBJECT_NEW();
  font->type=TYPE_FONT;
  float fontwid=lfHeight;
  
  
  if (!fontwid) fontwid=lfWidth;
  if (fontwid<0)fontwid=-fontwid;
  
  if (fontwid < 2 || fontwid > 8192) fontwid=10;
  
  
  font->font_rotation = lfOrientation/10.0;
  
  
#ifdef SWELL_ATSUI_TEXT_SUPPORT
  ATSUFontID fontid=kATSUInvalidFontID;
  if (lfFaceName && lfFaceName[0])
  {
    ATSUFindFontFromName(lfFaceName,strlen(lfFaceName),kFontFullName /* kFontFamilyName? */ ,(FontPlatformCode)kFontNoPlatform,kFontNoScriptCode,kFontNoLanguageCode,&fontid);
    //    if (fontid==kATSUInvalidFontID) printf("looked up %s and got %d\n",lfFaceName,fontid);
  }
  
  {
    if (ATSUCreateStyle(&font->font_style) == noErr && font->font_style)
    {    
      Fixed fsize=Long2Fix(fontwid);
    
      Boolean isBold=lfWeight >= FW_BOLD;
      Boolean isItal=!!lfItalic;
      Boolean isUnder=!!lfUnderline;
    
      ATSUAttributeTag        theTags[] = { kATSUQDBoldfaceTag, kATSUQDItalicTag, kATSUQDUnderlineTag,kATSUSizeTag,kATSUFontTag };
      ByteCount               theSizes[] = { sizeof(Boolean),sizeof(Boolean),sizeof(Boolean), sizeof(Fixed),sizeof(ATSUFontID)  };
      ATSUAttributeValuePtr   theValues[] =  {&isBold, &isItal, &isUnder,  &fsize, &fontid  } ;
    
      int attrcnt=sizeof(theTags)/sizeof(theTags[0]);
      if (fontid == kATSUInvalidFontID) attrcnt--;    
    
      if (ATSUSetAttributes (font->font_style,                       
                       attrcnt,                       
                       theTags,                      
                       theSizes,                      
                       theValues)!=noErr)
      {
        ATSUDisposeStyle(font->font_style);
        font->font_style=0;
      }
    }
    else
      font->font_style=0;
  }
  
#endif
  
  if (!font->font_style) // fallback to NSString stuff
  { 
    fontwid *= 0.95;
        
    NSString *str=CStringToNSString(lfFaceName);
    NSFont *nsf=[NSFont fontWithName:str size:fontwid];
    if (!nsf) nsf=[NSFont labelFontOfSize:fontwid];
    if (!nsf) nsf=[NSFont systemFontOfSize:fontwid];
    [str release];
    
    font->fontdict=[[NSMutableDictionary alloc] initWithCapacity:8];

    if (nsf) [font->fontdict setObject:nsf forKey:NSFontAttributeName];
    
    if (lfItalic) // italic
    {
      [font->fontdict setObject:[NSNumber numberWithFloat:0.33] forKey:NSObliquenessAttributeName];
    }
    if (lfUnderline)
    {
      [font->fontdict setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSUnderlineStyleAttributeName];
    }
    int weight=lfWeight;
    if (weight>=FW_BOLD)
    {
      float sc = min(fontwid,15.0)*0.55;
      [font->fontdict setObject:[NSNumber numberWithFloat:-sc] forKey:NSStrokeWidthAttributeName];
    }
  }
  
  return font;
}



BOOL GetTextMetrics(HDC ctx, TEXTMETRIC *tm)
{
  
  HDC__ *ct=(HDC__ *)ctx;
  if (tm) // give some sane defaults
  {
    tm->tmInternalLeading=3;
    tm->tmAscent=12;
    tm->tmDescent=4;
    tm->tmHeight=16;
    tm->tmAveCharWidth = 10;
  }
  if (!ct||!tm) return 0;
  
#ifdef SWELL_ATSUI_TEXT_SUPPORT
  if (ct->curfont && ct->curfont->font_style)
  {
    ATSUTextMeasurement ascent=Long2Fix(10);
    ATSUTextMeasurement descent=Long2Fix(3);
    ATSUTextMeasurement sz=Long2Fix(0);
    ATSUTextMeasurement width =Long2Fix(12);
    ATSUGetAttribute(ct->curfont->font_style,  kATSUAscentTag, sizeof(ATSUTextMeasurement), &ascent,NULL);
    ATSUGetAttribute(ct->curfont->font_style,  kATSUDescentTag, sizeof(ATSUTextMeasurement), &descent,NULL);
    ATSUGetAttribute(ct->curfont->font_style,  kATSUSizeTag, sizeof(ATSUTextMeasurement), &sz,NULL);
    ATSUGetAttribute(ct->curfont->font_style, kATSULineWidthTag, sizeof(ATSUTextMeasurement),&width,NULL);
    
    float asc=Fix2X(ascent);
    float desc=Fix2X(descent);
    float size = Fix2X(sz);
    
    if (size < (asc+desc)*0.2) size=asc+desc;
            
    tm->tmAscent = (int)ceil(asc);
    tm->tmDescent = (int)ceil(desc);
    tm->tmInternalLeading=(int)ceil(asc+desc-size);
    if (tm->tmInternalLeading<0)tm->tmInternalLeading=0;
    tm->tmHeight=(int) ceil(asc+desc);
    tm->tmAveCharWidth = (int) (ceil(asc+desc)*0.65); // (int)ceil(Fix2X(width));
    
    return 1;
  }
#endif
  
  
  // fallback: NSString drawing
  
  NSFont *curfont=NULL;
  
  if (ct->curfont && ct->curfont->fontdict)
  {
    curfont=[ct->curfont->fontdict objectForKey:NSFontAttributeName];
  }
  if (!curfont) 
  {
    curfont = [NSFont systemFontOfSize:10]; 
  }
  
  
  float asc=([curfont ascender]);
  float desc=([curfont descender]);
  //float leading=[curfont leading];
  float ch=[curfont capHeight];
  
  NSRect br=[curfont boundingRectForFont];
 
  tm->tmAscent = (int)(asc + 0.5);
  tm->tmDescent = (int)(-desc + 0.5);
  tm->tmHeight=tm->tmAscent + tm->tmDescent;
  tm->tmInternalLeading=(int)(asc - ch + 0.5); 
  if (tm->tmInternalLeading<0) tm->tmInternalLeading=0;

  //float widsc=1.0;
//  if (![curfont isFixedPitch]) widsc = 0.75;
  //tm->tmAveCharWidth = (int) ceil(br.size.width*widsc);
  tm->tmAveCharWidth = (int) (ceil(asc-desc)*0.55); // (int)ceil(Fix2X(width));
  
  //  tm->tmAscent += tm->tmDescent;
  return 1;
}



#ifdef SWELL_ATSUI_TEXT_SUPPORT

static int DrawTextATSUI(HDC ctx, CFStringRef strin, RECT *r, int align, bool *err)
{
  HDC__ *ct=(HDC__ *)ctx;
  HGDIOBJ__ *font=ct->curfont; // caller must specify a valid font
  
  UniChar strbuf[4096];
  int strbuf_len;
 
  {
    strbuf[0]=0;
    CFRange r = {0,CFStringGetLength(strin)};
    if (r.length > 4095) r.length=4095;
    strbuf_len=CFStringGetBytes(strin,r,kCFStringEncodingUTF16,' ',false,(UInt8*)strbuf,sizeof(strbuf)-2,NULL);
    if (strbuf_len<0)strbuf_len=0;
    else if (strbuf_len>4095) strbuf_len=4095;
    strbuf[strbuf_len]=0;
  }
  
  {
    RGBColor tcolor={GetRValue(ct->cur_text_color_int)*256,GetGValue(ct->cur_text_color_int)*256,GetBValue(ct->cur_text_color_int)*256};
    ATSUAttributeTag        theTags[] = { kATSUColorTag,   };
    ByteCount               theSizes[] = { sizeof(RGBColor),  };
    ATSUAttributeValuePtr   theValues[] =  {&tcolor,  } ;
        
    // error check this? we can live with the wrong color maybe?
    ATSUSetAttributes(font->font_style,  sizeof(theTags)/sizeof(theTags[0]), theTags, theSizes, theValues);
  }
  
  UniCharCount runLengths[1]={kATSUToTextEnd};
  ATSUTextLayout layout; 
  if (ATSUCreateTextLayoutWithTextPtr(strbuf, kATSUFromTextBeginning, kATSUToTextEnd, strbuf_len, 1, runLengths, &font->font_style, &layout)!=noErr)
  {
    *err=true;
    return 0;
  }
  
  {
    Fixed frot = X2Fix(font->font_rotation);
    
    ATSULineTruncation tv = (align & DT_END_ELLIPSIS) ? kATSUTruncateEnd : kATSUTruncateNone;
    ATSUAttributeTag        theTags[] = { kATSUCGContextTag, kATSULineTruncationTag, kATSULineRotationTag };
    ByteCount               theSizes[] = { sizeof (CGContextRef), sizeof(ATSULineTruncation), sizeof(Fixed)};
    ATSUAttributeValuePtr   theValues[] =  { &ct->ctx, &tv, &frot } ;
    
    
    if (ATSUSetLayoutControls (layout,
                           
                           sizeof(theTags)/sizeof(theTags[0]),
                           
                           theTags,
                           
                           theSizes,
                           
                           theValues)!=noErr)
    {
      *err=true;
      ATSUDisposeTextLayout(layout);   
      return 0;
    }
  }
  
  
  ATSUTextMeasurement leftFixed, rightFixed, ascentFixed, descentFixed;

  if (ATSUGetUnjustifiedBounds(layout, kATSUFromTextBeginning, kATSUToTextEnd, &leftFixed, &rightFixed, &ascentFixed, &descentFixed)!=noErr) 
  {
    *err=true;
    ATSUDisposeTextLayout(layout);   
    return 0;
  }

  int w=Fix2Long(rightFixed);
  int descent=Fix2Long(descentFixed);
  int h=descent + Fix2Long(ascentFixed);
  if (align&DT_CALCRECT)
  {
    ATSUDisposeTextLayout(layout);   
    r->right=r->left+w;
    r->bottom=r->top+h;
    return h;  
  }
  CGContextSaveGState(ct->ctx);    

  if (!(align & DT_NOCLIP))
    CGContextClipToRect(ct->ctx,CGRectMake(r->left,r->top,r->right-r->left,r->bottom-r->top));

  int l=r->left, t=r->top;
    
  if (fabs(font->font_rotation)<45.0)
  {
    if (align & DT_RIGHT) l = r->right-w;
    else if (align & DT_CENTER) l = (r->right+r->left)/2 - w/2;      
  }
  else l+=Fix2Long(ascentFixed); // 90 degree special case (we should generalize this to be correct throughout the rotation range, but oh well)
    
  if (align & DT_BOTTOM) t = r->bottom-h;
  else if (align & DT_VCENTER) t = (r->bottom+r->top)/2 - h/2;
    
  CGContextTranslateCTM(ct->ctx,0,t);
  CGContextScaleCTM(ct->ctx,1,-1);
  CGContextTranslateCTM(ct->ctx,0,-t-h);
    
  if (ct->curbkmode == OPAQUE)
  {      
    CGRect bgr = CGRectMake(l, t, w, h);
    CGColorSpaceRef csr = CGColorSpaceCreateDeviceRGB();
    float col[] = { (float)GetRValue(ct->curbkcol)/255.0f,  (float)GetGValue(ct->curbkcol)/255.0f, (float)GetBValue(ct->curbkcol)/255.0f, 1.0f };
    CGColorRef bgc = CGColorCreate(csr, col);
    CGContextSetFillColorWithColor(ct->ctx, bgc);
    CGContextFillRect(ct->ctx, bgr);
    CGColorRelease(bgc);	
    CGColorSpaceRelease(csr);
  }
 
  if (ATSUDrawText(layout,kATSUFromTextBeginning,kATSUToTextEnd,Long2Fix(l),Long2Fix(t+descent))!=noErr)
    *err=true;
  
  CGContextRestoreGState(ct->ctx);    
  
  ATSUDisposeTextLayout(layout);   
  
  return h;
}

#endif

int DrawText(HDC ctx, const char *buf, int buflen, RECT *r, int align)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!ct) return 0;
  
  char tmp[4096];
  const char *p=buf;
  char *op=tmp;
  while (*p && (op-tmp)<sizeof(tmp)-1 && (buflen<0 || (p-buf)<buflen))
  {
    if (*p == '&' && !(align&DT_NOPREFIX)) p++; 
    else if (*p == '\r')  p++; 
    else if (*p == '\n' && (align&DT_SINGLELINE)) { *op++ = ' '; p++; }
    else *op++=*p++;
  }
  *op=0;
  
  if (!tmp[0]) return 0; // dont draw empty strings
  
  NSString *str=CStringToNSString(tmp);
  
  if (!str) return 0;
  
#ifdef SWELL_ATSUI_TEXT_SUPPORT
  if (ct->curfont && ct->curfont->font_style)
  {
    bool err=false;
    int ret =  DrawTextATSUI(ctx,(CFStringRef)str,r,align,&err);
    if (!err) 
    {
      [str release];
      return ret;
    }
  }
#endif  
  
  // NSString, attributes based text drawing
  
  NSMutableDictionary *dict=NULL;
  if (ct->curfont) dict=ct->curfont->fontdict;  
  if (!dict) 
  {
    static NSMutableDictionary *dd;
    if (!dd) 
    {
      dd=[NSMutableDictionary dictionaryWithObjectsAndKeys:[NSFont systemFontOfSize:10],NSFontAttributeName, NULL];
      [dd retain];
    }
    dict=dd;
    if (!dd) 
    {
      [str release];
      return 0;
    }
  }
  
  if (ct->curbkmode==OPAQUE)
  {
    NSColor *bkcol=[NSColor colorWithCalibratedRed:GetRValue(ct->curbkcol)/255.0f green:GetGValue(ct->curbkcol)/255.0f blue:GetBValue(ct->curbkcol)/255.0f alpha:1.0f];
    [dict setObject:bkcol forKey:NSBackgroundColorAttributeName];
  }   
  else
  {
    [dict removeObjectForKey:NSBackgroundColorAttributeName];
  }
  if (ct->curtextcol)
    [dict setObject:ct->curtextcol forKey:NSForegroundColorAttributeName];
  
  
  NSTextAlignment amode=((align&DT_RIGHT)?NSRightTextAlignment : (align&DT_CENTER) ? NSCenterTextAlignment : NSLeftTextAlignment);
  NSLineBreakMode lbmode = ((align&DT_END_ELLIPSIS)? NSLineBreakByTruncatingTail:NSLineBreakByClipping);
  
  NSMutableParagraphStyle *parinfo = [dict objectForKey:NSParagraphStyleAttributeName];
  if (!parinfo || [parinfo alignment] != amode || [parinfo lineBreakMode] != lbmode)
  {
    parinfo = [[NSMutableParagraphStyle alloc] init];
    [parinfo setAlignment:amode];
    [parinfo setLineBreakMode:lbmode];
    [dict setObject:[parinfo autorelease] forKey:NSParagraphStyleAttributeName];
  }

  
  
  NSGraphicsContext *gc=NULL,*oldgc=NULL;
  if (ct->ctx != [[NSGraphicsContext currentContext] graphicsPort])
  {
    gc=[NSGraphicsContext graphicsContextWithGraphicsPort:ct->ctx flipped:YES];
    oldgc=[NSGraphicsContext currentContext];
    [NSGraphicsContext setCurrentContext:gc];
  }
  
  NSStringDrawingOptions opt = NSStringDrawingUsesFontLeading;
  NSSize sz={0,0};//[as size];
  NSRect rsz=[str boundingRectWithSize:sz options:opt attributes:dict];
  TEXTMETRIC tm;
  GetTextMetrics(ctx,&tm);
        
  sz=rsz.size;
  
  
  int ret=ceil(sz.height);
  if (align & DT_CALCRECT)
  {
    r->right=r->left+ceil(sz.width)*1.02;
    r->bottom=r->top+ceil(sz.height)*1.02;
  }
  else
  {
    NSRect drawr=NSMakeRect(r->left,r->top,r->right-r->left,r->bottom-r->top);
    if (align&DT_BOTTOM)
    {
      float dy=(drawr.size.height-sz.height);
      drawr.origin.y += dy;
      drawr.size.height -= dy;      
    }
    else if (align&DT_VCENTER)
    {
      float dy=((int)(drawr.size.height-sz.height ))/2;
      drawr.origin.y += dy;
      drawr.size.height -= dy*2.0;
    }
    else
    {
      drawr.size.height=sz.height;
    }
  	drawr.origin.y+=tm.tmAscent;
    
    if (align & DT_NOCLIP) // no clip, grow drawr if necessary (preserving alignment)
    {
      if (drawr.size.width < sz.width)
      {
        if (align&DT_RIGHT) drawr.origin.x -= (sz.width-drawr.size.width);
        else if (align&DT_CENTER) drawr.origin.x -= (sz.width-drawr.size.width)/2;
        drawr.size.width=sz.width;
      }
      if (drawr.size.height < sz.height)
      {
        if (align&DT_BOTTOM) drawr.origin.y -= (sz.height-drawr.size.height);
        else if (align&DT_VCENTER) drawr.origin.y -= (sz.height-drawr.size.height)/2;
        drawr.size.height=sz.height;
      }
    }
    [str drawWithRect:drawr options:opt attributes:dict];
    
  }
  
  if (gc)
  {
    [NSGraphicsContext setCurrentContext:oldgc];
    //      [gc release];
  }
  
  [str release];
  
  return ret;
}
    
  




void SetBkColor(HDC ctx, int col)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!ct) return;
  ct->curbkcol=col;
}

void SetBkMode(HDC ctx, int col)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!ct) return;
  ct->curbkmode=col;
}

void SetTextColor(HDC ctx, int col)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (!ct) return;
  ct->cur_text_color_int = col;
  
  if (ct->curtextcol) [ct->curtextcol release];    
  ct->curtextcol=[NSColor colorWithCalibratedRed:GetRValue(col)/255.0f green:GetGValue(col)/255.0f blue:GetBValue(col)/255.0f alpha:1.0f];
  [ct->curtextcol retain];
}



HICON LoadNamedImage(const char *name, bool alphaFromMask)
{
  int needfree=0;
  NSImage *img=0;
  NSString *str=CStringToNSString(name); 
  if (strstr(name,"/"))
  {
    img=[[NSImage alloc] initWithContentsOfFile:str];
    if (img) needfree=1;
  }
  if (!img) img=[NSImage imageNamed:str];
  [str release];
  if (!img) 
  {
    return 0;
  }
    
  if (alphaFromMask)
  {
    NSSize sz=[img size];
    NSImage *newImage=[[NSImage alloc] initWithSize:sz];
    [newImage lockFocus];
    
    [img setFlipped:YES];
    [img drawInRect:NSMakeRect(0,0,sz.width,sz.height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
    int y;
    CGContextRef myContext = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
    for (y=0; y< sz.height; y ++)
    {
      int x;
      for (x = 0; x < sz.width; x ++)
      {
        NSColor *col=NSReadPixel(NSMakePoint(x,y));
        if (col && [col numberOfComponents]<=4)
        {
          CGFloat comp[4];
          [col getComponents:comp]; // this relies on the format being RGB
          if (comp[0] == 1.0 && comp[1] == 0.0 && comp[2] == 1.0 && comp[3]==1.0)
            //fabs(comp[0]-1.0) < 0.0001 && fabs(comp[1]-.0) < 0.0001 && fabs(comp[2]-1.0) < 0.0001)
          {
            CGContextClearRect(myContext,CGRectMake(x,y,1,1));
          }
        }
      }
    }
    [newImage unlockFocus];
    
    if (needfree) [img release];
    needfree=1;
    img=newImage;    
  }
  HGDIOBJ__ *i=GDP_OBJECT_NEW();
  i->type=TYPE_BITMAP;
  i->wid=needfree;
  i->bitmapptr = img;
  return i;
}

void DrawImageInRect(HDC ctx, HICON img, RECT *r)
{
  HGDIOBJ__ *i = (HGDIOBJ__ *)img;
  if (!ctx || !i || i->type != TYPE_BITMAP||!i->bitmapptr) return;
  HDC__ *ct=(HDC__*)ctx;
  //CGContextDrawImage(ct->ctx,CGRectMake(r->left,r->top,r->right-r->left,r->bottom-r->top),(CGImage*)i->bitmapptr);
  // probably a better way since this ignores the ctx
  [NSGraphicsContext saveGraphicsState];
  NSGraphicsContext *gc=[NSGraphicsContext graphicsContextWithGraphicsPort:ct->ctx flipped:NO];
  [NSGraphicsContext setCurrentContext:gc];
  NSImage *nsi=i->bitmapptr;
  NSRect rr=NSMakeRect(r->left,r->top,r->right-r->left,r->bottom-r->top);
  [nsi setFlipped:YES];
  [nsi drawInRect:rr fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
  [nsi setFlipped:NO];
  [NSGraphicsContext restoreGraphicsState];
//  [gc release];
}


BOOL GetObject(HICON icon, int bmsz, void *_bm)
{
  memset(_bm,0,bmsz);
  if (bmsz != sizeof(BITMAP)) return false;
  BITMAP *bm=(BITMAP *)_bm;
  HGDIOBJ__ *i = (HGDIOBJ__ *)icon;
  if (!i || i->type != TYPE_BITMAP) return false;
  NSImage *img = i->bitmapptr;
  if (!img) return false;
  bm->bmWidth = (int) ([img size].width+0.5);
  bm->bmHeight = (int) ([img size].height+0.5);
  return true;
}


void *GetNSImageFromHICON(HICON ico)
{
  HGDIOBJ__ *i = (HGDIOBJ__ *)ico;
  if (!i || i->type != TYPE_BITMAP) return 0;
  return i->bitmapptr;
}

#if 0
static int ColorFromNSColor(NSColor *color, int valifnul)
{
  if (!color) return valifnul;
  float r,g,b;
  NSColor *color2=[color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
  if (!color2) 
  {
    NSLog(@"error converting colorspace from: %@\n",[color colorSpaceName]);
    return valifnul;
  }
  
  [color2 getRed:&r green:&g blue:&b alpha:NULL];
  return RGB((int)(r*255.0),(int)(g*255.0),(int)(b*255.0));
}
#else
#define ColorFromNSColor(a,b) (b)
#endif
int GetSysColor(int idx)
{
 // NSColors that seem to be valid: textBackgroundColor, selectedTextBackgroundColor, textColor, selectedTextColor
  
  switch (idx)
  {
    case COLOR_WINDOW: return ColorFromNSColor([NSColor controlColor],RGB(192,192,192));
    case COLOR_3DFACE: 
    case COLOR_BTNFACE: return ColorFromNSColor([NSColor controlColor],RGB(192,192,192));
    case COLOR_SCROLLBAR: return ColorFromNSColor([NSColor controlColor],RGB(32,32,32));
    case COLOR_3DSHADOW: return ColorFromNSColor([NSColor selectedTextBackgroundColor],RGB(96,96,96));
    case COLOR_3DHILIGHT: return ColorFromNSColor([NSColor selectedTextBackgroundColor],RGB(224,224,224));
    case COLOR_BTNTEXT: return ColorFromNSColor([NSColor selectedTextBackgroundColor],RGB(0,0,0));
    case COLOR_3DDKSHADOW: return (ColorFromNSColor([NSColor selectedTextBackgroundColor],RGB(96,96,96))>>1)&0x7f7f7f;
    case COLOR_INFOBK: return RGB(255,240,200);
    case COLOR_INFOTEXT: return RGB(0,0,0);
      
  }
  return 0;
}


void BitBlt(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int mode)
{
  StretchBlt(hdcOut,x,y,w,h,hdcIn,xin,yin,w,h,mode);
}

void StretchBlt(HDC hdcOut, int x, int y, int destw, int desth, HDC hdcIn, int xin, int yin, int w, int h, int mode)
{
  if (!hdcOut || !hdcIn||w<1||h<1) return;
  HDC__ *src=(HDC__*)hdcIn;
  HDC__ *dest=(HDC__*)hdcOut;
  if (!src->ownedData || !src->ctx || !dest->ctx) return;

  if (w<1||h<1) return;
  
  int sw=  CGBitmapContextGetWidth(src->ctx);
  int sh= CGBitmapContextGetHeight(src->ctx);
  
  int preclip_w=w;
  int preclip_h=h;
  
  if (xin<0) 
  { 
    x-=(xin*destw)/w;
    w+=xin; 
    xin=0; 
  }
  if (yin<0) 
  { 
    y-=(yin*desth)/h;
    h+=yin; 
    yin=0; 
  }
  if (xin+w > sw) w=sw-xin;
  if (yin+h > sh) h=sh-yin;
  
  if (w<1||h<1) return;

  if (destw==preclip_w) destw=w; // no scaling, keep width the same
  else if (w != preclip_w) destw = (w*destw)/preclip_w;
  
  if (desth == preclip_h) desth=h;
  else if (h != preclip_h) desth = (h*desth)/preclip_h;
  
  CGImageRef img = NULL, subimg = NULL;
  NSBitmapImageRep *rep = NULL;
  
  static char is105;
  if (!is105)
  {
    SInt32 v=0x1040;
    Gestalt(gestaltSystemVersion,&v);
    is105 = v>=0x1050 ? 1 : -1;    
  }
  
  bool want_subimage=false;
  
  bool use_alphachannel = mode == SRCCOPY_USEALPHACHAN;
  
  if (is105>0)
  {
    // [NSBitmapImageRep CGImage] is not available pre-10.5
    unsigned char *p = (unsigned char *)ALIGN_FBUF(src->ownedData);
    p += (xin + sw*yin)*4;
    rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:&p pixelsWide:w pixelsHigh:h
                                               bitsPerSample:8 samplesPerPixel:(3+use_alphachannel) hasAlpha:use_alphachannel isPlanar:FALSE
                                              colorSpaceName:NSDeviceRGBColorSpace bitmapFormat:(NSBitmapFormat)((use_alphachannel ? NSAlphaNonpremultipliedBitmapFormat : 0) |NSAlphaFirstBitmapFormat) bytesPerRow:sw*4 bitsPerPixel:32];
    img=(CGImageRef)[rep CGImage];
    if (img) CGImageRetain(img);
  }
  else
  {
    if (1) // this seems to be WAY better on 10.4 than the alternative (below)
    {
      static CGColorSpaceRef cs;
      if (!cs) cs = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
      unsigned char *p = (unsigned char *)ALIGN_FBUF(src->ownedData);
      p += (xin + sw*yin)*4;
      
      CGDataProviderRef provider = CGDataProviderCreateWithData(NULL,p,4*sw*h,NULL);
      img = CGImageCreate(w,h,8,32,4*sw,cs,use_alphachannel?kCGImageAlphaFirst:kCGImageAlphaNoneSkipFirst,provider,NULL,NO,kCGRenderingIntentDefault);
      //    CGColorSpaceRelease(cs);
      CGDataProviderRelease(provider);
    }
    else 
    {
      // causes lots of kernel messages, so avoid if possible, also doesnt support alpha bleh
      img = CGBitmapContextCreateImage(src->ctx);
      want_subimage = true;
    }
  }
  
  if (!img) return;
  
  if (want_subimage && (w != sw || h != sh))
  {
    subimg = CGImageCreateWithImageInRect(img,CGRectMake(xin,yin,w,h));
    if (!subimg) 
    {
      CGImageRelease(img);
      return;
    }
  }
  
  CGContextRef output = (CGContextRef)dest->ctx; 
  
  
  CGContextSaveGState(output);
  CGContextScaleCTM(output,1.0,-1.0);  
  CGContextDrawImage(output,CGRectMake(x,-desth-y,destw,desth),subimg ? subimg : img);    
  CGContextRestoreGState(output);
  
  if (subimg) CGImageRelease(subimg);
  CGImageRelease(img);   
  if (rep) [rep release];
  
}

void SWELL_PushClipRegion(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (ct && ct->ctx) CGContextSaveGState(ct->ctx);
}

void SWELL_SetClipRegion(HDC ctx, RECT *r)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (ct && ct->ctx) CGContextClipToRect(ct->ctx,CGRectMake(r->left,r->top,r->right-r->left,r->bottom-r->top));

}

void SWELL_PopClipRegion(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (ct && ct->ctx) CGContextRestoreGState(ct->ctx);
}

void *SWELL_GetCtxFrameBuffer(HDC ctx)
{
  HDC__ *ct=(HDC__ *)ctx;
  if (ct) return ALIGN_FBUF(ct->ownedData);
  return 0;
}


HDC GetDC(HWND h)
{
  if (h && [(id)h isKindOfClass:[NSWindow class]])
  {
    if ([(id)h respondsToSelector:@selector(getSwellPaintInfo:)]) 
    {
      PAINTSTRUCT ps={0,}; 
      [(id)h getSwellPaintInfo:(PAINTSTRUCT *)&ps];
      if (ps.hdc) 
      {
        if ((ps.hdc)->ctx) CGContextSaveGState((ps.hdc)->ctx);
        return ps.hdc;
      }
    }
    h=(HWND)[(id)h contentView];
  }
  
  if (h && [(id)h isKindOfClass:[NSView class]])
  {
    if ([(id)h respondsToSelector:@selector(getSwellPaintInfo:)]) 
    {
      PAINTSTRUCT ps={0,}; 
      [(id)h getSwellPaintInfo:(PAINTSTRUCT *)&ps];
      if (ps.hdc) 
      {
        if (((HDC__*)ps.hdc)->ctx) CGContextSaveGState((ps.hdc)->ctx);
        return ps.hdc;
      }
    }
    
    if ([(NSView*)h lockFocusIfCanDraw])
    {
      HDC ret= SWELL_CreateGfxContext([NSGraphicsContext currentContext]);
      if (ret)
      {
         if ((ret)->ctx) CGContextSaveGState((ret)->ctx);
      }
      return ret;
    }
  }
  return 0;
}

HDC GetWindowDC(HWND h)
{
  HDC ret=GetDC(h);
  if (ret)
  {
    NSView *v=NULL;
    if ([(id)h isKindOfClass:[NSWindow class]]) v=[(id)h contentView];
    else if ([(id)h isKindOfClass:[NSView class]]) v=(NSView *)h;
    
    if (v)
    {
      NSRect b=[v bounds];
      float xsc=b.origin.x;
      float ysc=b.origin.y;
      if ((xsc || ysc) && (ret)->ctx) CGContextTranslateCTM((ret)->ctx,xsc,ysc);
    }
  }
  return ret;
}

void ReleaseDC(HWND h, HDC hdc)
{
  if (hdc)
  {
    if ((hdc)->ctx) CGContextRestoreGState((hdc)->ctx);
  }
  if (h && [(id)h isKindOfClass:[NSWindow class]])
  {
    if ([(id)h respondsToSelector:@selector(getSwellPaintInfo:)]) 
    {
      PAINTSTRUCT ps={0,}; 
      [(id)h getSwellPaintInfo:(PAINTSTRUCT *)&ps];
      if (ps.hdc && ps.hdc==hdc) return;
    }
    h=(HWND)[(id)h contentView];
  }
  bool isView=h && [(id)h isKindOfClass:[NSView class]];
  if (isView)
  {
    if ([(id)h respondsToSelector:@selector(getSwellPaintInfo:)]) 
    {
      PAINTSTRUCT ps={0,}; 
      [(id)h getSwellPaintInfo:(PAINTSTRUCT *)&ps];
      if (ps.hdc && ps.hdc==hdc) return;
    }
  }    
    
  if (hdc) SWELL_DeleteGfxContext(hdc);
  if (isView && hdc)
  {
    [(NSView *)h unlockFocus];
//    if ([(NSView *)h window]) [[(NSView *)h window] flushWindow];
  }
}

void SWELL_FillDialogBackground(HDC hdc, RECT *r, int level)
{
  CGContextRef ctx=(CGContextRef)SWELL_GetCtxGC(hdc);
  if (ctx)
  {
  // level 0 for now = this
    HIThemeSetFill(kThemeBrushDialogBackgroundActive,NULL,ctx,kHIThemeOrientationNormal);
    CGRect rect=CGRectMake(r->left,r->top,r->right-r->left,r->bottom-r->top);
    CGContextFillRect(ctx,rect);	         
  }
}


HBITMAP CreateBitmap(int width, int height, int numplanes, int bitsperpixel, unsigned char* bits)
{
  int spp = bitsperpixel/8;
  Boolean hasa = (bitsperpixel == 32);
  Boolean hasp = (numplanes > 1); // won't actually work yet for planar data
  NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:0 pixelsWide:width pixelsHigh:height 
                                                               bitsPerSample:8 samplesPerPixel:spp
                                                               hasAlpha:hasa isPlanar:hasp
                                                               colorSpaceName:NSDeviceRGBColorSpace
                                                                bitmapFormat:NSAlphaFirstBitmapFormat 
                                                                 bytesPerRow:0 bitsPerPixel:0];    
  if (!rep) return 0;
  unsigned char* p = [rep bitmapData];
  int pspan = [rep bytesPerRow]; // might not be the same as width
  
  int y;
  for (y=0;y<height;y ++)
  {
    memcpy(p,bits,width*4);
    p+=pspan;
    bits += width*4;
  }

  NSImage* img = [[NSImage alloc] init];
  [img addRepresentation:rep]; 
  [rep release];
  
  HGDIOBJ__* obj = GDP_OBJECT_NEW();
  obj->type = TYPE_BITMAP;
  obj->wid = 1; // need free
  obj->bitmapptr = img;
  return obj;
}



#endif
