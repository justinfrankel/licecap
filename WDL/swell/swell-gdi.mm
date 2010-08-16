
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
#define SWELL_NSSTRING_DRAWING 0  // face control works?
#define SWELL_ATSUI_DRAWING 1 // faster
#else
#define SWELL_NSSTRING_DRAWING 1
#define SWELL_ATSUI_DRAWING 0
#endif

// unsupported
#define SWELL_HITDRAWING 0
#define SWELL_CGDRAWING 0


static CGColorRef CreateColor(int col, float alpha=1.0f)
{
  static CGColorSpaceRef cspace;
  
  if (!cspace) cspace=CGColorSpaceCreateDeviceRGB();
  
  CGFloat cols[4]={GetRValue(col)/255.0,GetGValue(col)/255.0,GetBValue(col)/255.0,alpha};
  CGColorRef color=CGColorCreate(cspace,cols);
  return color;
}

static WDL_Mutex m_ctxpool_mutex;
static WDL_PtrList<GDP_CTX> m_ctxpool; // not threadsafe but fuck it I dont think anything is yet

GDP_CTX *SWELL_GDP_CTX_NEW()
{
  int ni=m_ctxpool.GetSize();
  GDP_CTX *p=NULL;
  if (ni>0)
  {
    m_ctxpool_mutex.Enter();
    p=m_ctxpool.Get(ni-1);
    if (p)
    { 
      m_ctxpool.Delete(ni-1);
      memset(p,0,sizeof(GDP_CTX));
    }
    m_ctxpool_mutex.Leave();
  }
  if (!p) p=(GDP_CTX *)calloc(sizeof(GDP_CTX),1);
  return p;
}
static void SWELL_GDP_CTX_DELETE(GDP_CTX *p)
{
  if (p && m_ctxpool.GetSize()<1024) 
  {
    m_ctxpool_mutex.Enter();
    m_ctxpool.Add(p);
    m_ctxpool_mutex.Leave();
  }
  else free(p);
}


HDC SWELL_CreateGfxContext(void *c)
{
  GDP_CTX *ctx=SWELL_GDP_CTX_NEW();
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

HDC SWELL_CreateMemContext(HDC hdc, int w, int h)
{
  // we could use CGLayer here, but it's 10.4+ and seems to be slower than this
//  if (w&1) w++;
  void *buf=calloc(w*4,h);
  if (!buf) return 0;
  CGColorSpaceRef cs=CGColorSpaceCreateDeviceRGB();
  CGContextRef c=CGBitmapContextCreate(buf,w,h,8,w*4,cs, kCGImageAlphaNoneSkipFirst);
  CGColorSpaceRelease(cs);
  if (!c)
  {
    free(buf);
    return 0;
  }


  CGContextTranslateCTM(c,0.0,h);
  CGContextScaleCTM(c,1.0,-1.0);


  GDP_CTX *ctx=SWELL_GDP_CTX_NEW();
  ctx->ctx=(CGContextRef)c;
  ctx->ownedData=buf;
  // CGContextSelectFont(ctx->ctx,"Arial",12.0,kCGEncodingMacRoman);
  
  SetTextColor(ctx,0);
  return ctx;
}


void SWELL_DeleteGfxContext(HDC ctx)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (ct)
  {   
    if (ct->ownedData)
    {
      CGContextRelease(ct->ctx);
      free(ct->ownedData);
    }
    if (ct->curtextcol) [ct->curtextcol release];
    if (ct->curcgtextcol) CGColorRelease(ct->curcgtextcol);
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

static WDL_PtrList<GDP_OBJECT> m_objpool;
static GDP_OBJECT *GDP_OBJECT_NEW()
{
  int ni=m_objpool.GetSize();
  GDP_OBJECT *p=NULL;
  if (ni>0)
  {
    m_ctxpool_mutex.Enter();
    p=m_objpool.Get(ni-1);
    if (p)
    {
      m_objpool.Delete(ni-1);
      memset(p,0,sizeof(GDP_OBJECT));
    }
    m_ctxpool_mutex.Leave();
  }
  if (!p) p=(GDP_OBJECT *)calloc(sizeof(GDP_OBJECT),1);    
  return p;
}
static void GDP_OBJECT_DELETE(GDP_OBJECT *p)
{
  if (p && m_objpool.GetSize()<1024) 
  {
    m_ctxpool_mutex.Enter();
    m_objpool.Add(p);
    m_ctxpool_mutex.Leave();
  }
  else free(p);
}

HPEN CreatePenAlpha(int attr, int wid, int col, float alpha)
{
  GDP_OBJECT *pen=GDP_OBJECT_NEW();
  pen->type=TYPE_PEN;
  pen->wid=wid<0?0:wid;
  pen->color=CreateColor(col,alpha);
  return pen;
}
HBRUSH  CreateSolidBrushAlpha(int col, float alpha)
{
  GDP_OBJECT *brush=GDP_OBJECT_NEW();
  brush->type=TYPE_BRUSH;
  brush->color=CreateColor(col,alpha);
  brush->wid=0; 
  return brush;
}

#define FONTSCALE 0.9
HFONT CreateFont(long lfHeight, long lfWidth, long lfEscapement, long lfOrientation, long lfWeight, char lfItalic, 
  char lfUnderline, char lfStrikeOut, char lfCharSet, char lfOutPrecision, char lfClipPrecision, 
         char lfQuality, char lfPitchAndFamily, const char *lfFaceName)
{
  GDP_OBJECT *font=GDP_OBJECT_NEW();
  font->type=TYPE_FONT;
  float fontwid=lfHeight;
  
  
  if (!fontwid) fontwid=lfWidth;
  if (fontwid<0)fontwid=-fontwid;
  
  if (fontwid < 2 || fontwid > 8192) fontwid=10;
 
  
  font->font_rotation = lfOrientation/10.0;
#if SWELL_ATSUI_DRAWING
  // need to use lfFaceName FMGetATSFontRefFromFont(
  ATSUCreateStyle(&font->font_style);

  ATSUFontID fontid=kATSUInvalidFontID;
  if (lfFaceName && lfFaceName[0])
  {
    ATSUFindFontFromName(lfFaceName,strlen(lfFaceName),kFontFullName /* kFontFamilyName? */ ,(FontPlatformCode)kFontNoPlatform,kFontNoScriptCode,kFontNoLanguageCode,&fontid);
//    if (fontid==kATSUInvalidFontID) printf("looked up %s and got %d\n",lfFaceName,fontid); // todo remove me
  }
  
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
    
    
    ATSUSetAttributes (font->font_style,                       
                       attrcnt,                       
                       theTags,                      
                       theSizes,                      
                       theValues);
  }
  
#endif
  
#if SWELL_NSSTRING_DRAWING
  
  fontwid *= FONTSCALE;
  NSString *str=(NSString *)SWELL_CStringToCFString(lfFaceName);
  NSFont *nsf=[NSFont fontWithName:str size:fontwid];
  [str release];
  if (!nsf) nsf=[NSFont labelFontOfSize:fontwid];
  if (!nsf) nsf=[NSFont systemFontOfSize:fontwid];

  font->fontparagraphinfo = [[NSMutableParagraphStyle alloc] init];
  font->fontdict=[NSMutableDictionary dictionaryWithObjectsAndKeys:font->fontparagraphinfo,NSParagraphStyleAttributeName,nsf,NSFontAttributeName, NULL];
  [font->fontparagraphinfo release];
  [font->fontdict retain];
  
  if (lfItalic) // italic
  {
    [font->fontdict setObject:[NSNumber numberWithFloat:0.33] forKey:NSObliquenessAttributeName];
  }
  if (lfUnderline)
  {
    [font->fontdict setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSUnderlineStyleAttributeName];
  }
  int weight=lfWeight;
  if (weight>FW_SEMIBOLD)
  {
    double sc=40.0*(weight-FW_SEMIBOLD)/(1000-FW_SEMIBOLD);
    if(sc>0.0)[font->fontdict setObject:[NSNumber numberWithFloat:-sc] forKey:NSStrokeWidthAttributeName];
  }
#endif
  
  return font;
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
    GDP_OBJECT *p=(GDP_OBJECT *)pen;
    if (p->type == TYPE_PEN || p->type == TYPE_BRUSH || p->type == TYPE_FONT || p->type == TYPE_BITMAP)
    {
      if (p->type == TYPE_PEN || p->type == TYPE_BRUSH)
        if (p->wid<0) return;
      if (p->color) CGColorRelease(p->color);
      if (p->fontdict)
        [(NSMutableDictionary*)p->fontdict release];
      if (p->font_style) ATSUDisposeStyle(p->font_style);
      if (p->wid && p->bitmapptr) [p->bitmapptr release]; 
      GDP_OBJECT_DELETE(p);
    }
    else free(p);
  }
}


HGDIOBJ SelectObject(HDC ctx, HGDIOBJ pen)
{
  GDP_CTX *c=(GDP_CTX *)ctx;
  GDP_OBJECT *p=(GDP_OBJECT*) pen;
  GDP_OBJECT **mod=0;
  if (!c||!p) return 0;
  
  if (p == (GDP_OBJECT*)TYPE_PEN) mod=&c->curpen;
  else if (p == (GDP_OBJECT*)TYPE_BRUSH) mod=&c->curbrush;
  else if (p == (GDP_OBJECT*)TYPE_FONT) mod=&c->curfont;

  if (mod)
  {
    GDP_OBJECT *np=*mod;
    *mod=0;
    return np?np:p;
  }

  
  if (p->type == TYPE_PEN) mod=&c->curpen;
  else if (p->type == TYPE_BRUSH) mod=&c->curbrush;
  else if (p->type == TYPE_FONT) mod=&c->curfont;
  else return 0;
  
  GDP_OBJECT *op=*mod;
  if (!op) op=(GDP_OBJECT*)p->type;
  if (op != p)
  {
    *mod=p;
  
    if (p->type == TYPE_FONT)
    {
//      CGContextSelectFont(c->ctx,p->fontface,(float)p->wid,kCGEncodingMacRoman);
    }
  }
  return op;
}



void SWELL_FillRect(HDC ctx, RECT *r, HBRUSH br)
{
  GDP_CTX *c=(GDP_CTX *)ctx;
  GDP_OBJECT *b=(GDP_OBJECT*) br;
  if (!c || !b || b == (GDP_OBJECT*)TYPE_BRUSH || b->type != TYPE_BRUSH) return;

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
  GDP_CTX *c=(GDP_CTX *)ctx;
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
  GDP_CTX *c=(GDP_CTX *)ctx;
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
      static GDP_OBJECT br={0,};
      br.type=TYPE_BRUSH;
      br.wid=-1;
      return &br;
    }
    case NULL_PEN:
    {
      static GDP_OBJECT pen={0,};
      pen.type=TYPE_PEN;
      pen.wid=-1;
      return &pen;
    }
  }
  return 0;
}

void Polygon(HDC ctx, POINT *pts, int npts)
{
  GDP_CTX *c=(GDP_CTX *)ctx;
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
  GDP_CTX *c=(GDP_CTX *)ctx;
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
  GDP_CTX *c=(GDP_CTX *)ctx;
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
  GDP_CTX *c=(GDP_CTX *)ctx;
  if (!c||!c->curpen||c->curpen->wid<0) return;

  CGContextSetLineWidth(c->ctx,(float)max(c->curpen->wid,1));
  CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
	
  CGContextBeginPath(c->ctx);
  CGContextMoveToPoint(c->ctx,c->lastpos_x,c->lastpos_y);
  float fx=(float)x,fy=(float)y;
  
  CGContextAddLineToPoint(c->ctx,fx,fy);
  c->lastpos_x=fx;
  c->lastpos_y=fy;
  CGContextStrokePath(c->ctx);
}

void PolyPolyline(HDC ctx, POINT *pts, DWORD *cnts, int nseg)
{
  GDP_CTX *c=(GDP_CTX *)ctx;
  if (!c||!c->curpen||c->curpen->wid<0||nseg<1) return;

  CGContextSetLineWidth(c->ctx,(float)max(c->curpen->wid,1));
  CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
	
  CGContextBeginPath(c->ctx);
  
  while (nseg-->0)
  {
    DWORD cnt=*cnts++;
    if (!cnt) continue;
    if (!--cnt) { pts++; continue; }
    
    CGContextMoveToPoint(c->ctx,(float)pts->x,(float)pts->y);
    pts++;
    
    while (cnt--)
    {
      CGContextAddLineToPoint(c->ctx,(float)pts->x,(float)pts->y);
      pts++;
    }
  }
  CGContextStrokePath(c->ctx);
}
void *SWELL_GetCtxGC(HDC ctx)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return 0;
  return ct->ctx;
}


void SWELL_SetPixel(HDC ctx, int x, int y, int c)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return;
  CGContextBeginPath(ct->ctx);
  CGContextMoveToPoint(ct->ctx,(float)x,(float)y);
  CGContextAddLineToPoint(ct->ctx,(float)x+0.5,(float)y+0.5);
  CGContextSetLineWidth(ct->ctx,(float)1.5);
  CGContextSetRGBStrokeColor(ct->ctx,GetRValue(c)/255.0,GetGValue(c)/255.0,GetBValue(c)/255.0,1.0);
  CGContextStrokePath(ct->ctx);	
}


#if SWELL_NSSTRING_DRAWING



BOOL GetTextMetrics(HDC ctx, TEXTMETRIC *tm)
{
  // what the fuck.
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (tm) // give some sane defaults
  {
    tm->tmInternalLeading=3;
    tm->tmAscent=12;
    tm->tmDescent=4;
    tm->tmHeight=16;
    tm->tmAveCharWidth = 10;
  }
  if (!ct||!tm) return 0;
  
  NSFont *curfont=NULL;
  
  if (ct->curfont && ct->curfont->fontdict)
  {
    curfont=[ct->curfont->fontdict objectForKey:NSFontAttributeName];
  }
  if (!curfont) curfont = [NSFont systemFontOfSize:10]; 
  
  
  float asc=[curfont ascender];
  float desc=-[curfont descender];
  float leading=[curfont leading];
  float ch=[curfont capHeight];
  
  tm->tmAscent = (int)ceil(asc);
  tm->tmDescent = (int)ceil(desc);
  tm->tmInternalLeading=(int)(asc - ch);
  tm->tmHeight=(int) ceil(asc+desc+leading);
  tm->tmAveCharWidth = (int) ceil([curfont boundingRectForFont].size.width);
  
  
  //  tm->tmAscent += tm->tmDescent;
  return 1;
}


int DrawText(HDC ctx, const char *buf, int buflen, RECT *r, int align)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
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
  
  // NSString, attributes based text drawing
  
  NSMutableDictionary *dict=NULL;
  NSMutableParagraphStyle *parinfo=NULL;
  
  if (ct->curfont) 
  {
    dict=ct->curfont->fontdict;
    parinfo=ct->curfont->fontparagraphinfo;
  }
  
  if (!dict||!parinfo) 
  {
    static NSMutableParagraphStyle *dp;
    static NSMutableDictionary *dd;
    if (!dd) 
    {
      dp=[[NSMutableParagraphStyle alloc] init];
      dd=[NSMutableDictionary dictionaryWithObjectsAndKeys:dp,NSParagraphStyleAttributeName,[NSFont systemFontOfSize:10],NSFontAttributeName, NULL];
      [dd retain];
    }
    parinfo=dp;
    dict=dd;
    
    if (!dd) return 0;
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
  
  
  NSString *str=(NSString*)SWELL_CStringToCFString(tmp);
  
  
  if (parinfo)
  {
    [parinfo setAlignment:((align&DT_RIGHT)?NSRightTextAlignment : (align&DT_CENTER) ? NSCenterTextAlignment : NSLeftTextAlignment)];
    [parinfo setLineBreakMode:((align&DT_END_ELLIPSIS)? NSLineBreakByTruncatingTail:NSLineBreakByClipping)];
  }
  
  
  
  NSGraphicsContext *gc=NULL,*oldgc=NULL;
  if (ct->ctx != [[NSGraphicsContext currentContext] graphicsPort])
  {
    gc=[NSGraphicsContext graphicsContextWithGraphicsPort:ct->ctx flipped:YES];
    oldgc=[NSGraphicsContext currentContext];
    [NSGraphicsContext setCurrentContext:gc];
  }
  
  NSSize sz={0,0};//[as size];
  NSRect rsz=[str boundingRectWithSize:sz options:NSStringDrawingUsesDeviceMetrics attributes:dict];
  sz=rsz.size;
  
  int ret=10;
  if (align & DT_CALCRECT)
  {
    r->right=r->left+ceil(sz.width);
    r->bottom=r->top+ceil(sz.height)+1;
    ret=ceil(sz.height);
  }
  else
  {
    ret=ceil(sz.height);
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
  	drawr.origin.y+=sz.height;
    
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
    [str drawWithRect:drawr options:NSStringDrawingUsesDeviceMetrics attributes:dict];
    
  }
  
  if (gc)
  {
    [NSGraphicsContext setCurrentContext:oldgc];
    //      [gc release];
  }
  
  [str release];
  
  return ret;
}
#endif // NSSTRING_DRAWING
  
#if SWELL_HITDRAWING

int DrawText(HDC ctx, const char *buf, int buflen, RECT *r, int align)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return 0;

  CFStringRef label=(CFStringRef)SWELL_CStringToCFString(buf); 
  HIRect hiBounds = { {r->left, r->top}, {r->right-r->left, r->bottom-r->top} };
  HIThemeTextInfo textInfo = {0, kThemeStateActive, kThemeCurrentPortFont, kHIThemeTextHorizontalFlushLeft, 
	  kHIThemeTextVerticalFlushTop, kHIThemeTextBoxOptionStronglyVertical, kHIThemeTextTruncationEnd, 1, false};
  
//  if (ct->curfont && ct->curfont->wid <= 12) 
    textInfo.fontID=kThemeMiniSystemFont;
  //else if (ct->curfont->wid < 14) textInfo.fontID=kThemeLabelFont; 
  //else if (ct->curfont->wid <= 16) textInfo.fontID=kThemeSmallSystemFont;
//  else textInfo.fontID=kThemeSystemFont;
  
  if (align & DT_CENTER) textInfo.horizontalFlushness=kHIThemeTextHorizontalFlushCenter;
  else if (align&DT_RIGHT) textInfo.horizontalFlushness=kHIThemeTextHorizontalFlushRight;
  if (align & DT_VCENTER) textInfo.verticalFlushness=kHIThemeTextVerticalFlushCenter;
  else if (align&DT_BOTTOM) textInfo.verticalFlushness=kHIThemeTextVerticalFlushBottom;
	
  float h;
  if (align & DT_CALCRECT)
  {
    float w=r->right-r->left;
    h=r->bottom-r->top;
    HIThemeGetTextDimensions(label,0,&textInfo,&w,&h,NULL);
    r->right=r->left+(int)w;
    r->bottom=r->top+(int)h;
  }
  else
  {
    if (ct->curcgtextcol) CGContextSetFillColorWithColor(ct->ctx,ct->curcgtextcol);
    
    if (!(align&DT_SINGLELINE))
    {
      textInfo.truncationMaxLines=30;
    }
    HIThemeDrawTextBox(label, &hiBounds, &textInfo, ct->ctx, kHIThemeOrientationNormal);
    float w=r->right-r->left;
    h=r->bottom-r->top;
    HIThemeGetTextDimensions(label,0,&textInfo,&w,&h,NULL);
    return (int)ceil(h);
  }
  CFRelease(label);
  
  return h;
}

#endif//SWELL_HITDRAWING
  
  

#if SWELL_ATSUI_DRAWING



BOOL GetTextMetrics(HDC ctx, TEXTMETRIC *tm)
{
  // what the fuck.
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (tm) // give some sane defaults
  {
    tm->tmInternalLeading=3;
    tm->tmAscent=12;
    tm->tmDescent=4;
    tm->tmHeight=16;
    tm->tmAveCharWidth=12;
  }
  if (!ct||!tm||!ct->curfont||!ct->curfont->font_style) return 0;
  

  ATSUTextMeasurement ascent=Long2Fix(10);
  ATSUTextMeasurement descent=Long2Fix(3);
  ATSUTextMeasurement leading=Long2Fix(3);
  ATSUTextMeasurement width =Long2Fix(12);
  ATSUGetAttribute(ct->curfont->font_style,  kATSUAscentTag, sizeof(ATSUTextMeasurement), &ascent,NULL);
  ATSUGetAttribute(ct->curfont->font_style,  kATSUDescentTag, sizeof(ATSUTextMeasurement), &descent,NULL);
  ATSUGetAttribute(ct->curfont->font_style,  kATSULeadingTag, sizeof(ATSUTextMeasurement), &leading,NULL);
  ATSUGetAttribute(ct->curfont->font_style, kATSULineWidthTag, sizeof(ATSUTextMeasurement),&width,NULL);
  
  float asc=Fix2X(ascent);
  float desc=Fix2X(descent);
  float lead = Fix2X(leading);
  
  tm->tmAscent = (int)ceil(asc);
  tm->tmDescent = (int)ceil(desc);
  tm->tmInternalLeading=(int)(lead);
  tm->tmHeight=(int) ceil(asc+desc+lead);
  tm->tmAveCharWidth = (int) (ceil(asc+desc+lead)*0.75); // (int)ceil(Fix2X(width));
  
  return 1;
}



int DrawText(HDC ctx, const char *buf, int buflen, RECT *r, int align)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
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
  
  static HFONT m_bkfont;
  GDP_OBJECT *font=ct->curfont ? ct->curfont : (GDP_OBJECT*)m_bkfont;
  if (!font)
  {
    font=(GDP_OBJECT*) (m_bkfont=CreateFont(10,0,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,"Arial"));
    if (!font) return 0;
  }
  
  UniChar strbuf[4096];
  strbuf[0]=0;
  int l=0;
  CFStringRef str=CFStringCreateWithCString(NULL,tmp,kCFStringEncodingUTF8);
  if (!str) str=CFStringCreateWithCString(NULL,tmp,kCFStringEncodingASCII);
  
  if (str)
  {
    CFRange r = {0,CFStringGetLength(str)};
    if (r.length > 4095) r.length=4095;
    l=CFStringGetBytes(str,r,kCFStringEncodingUTF16,' ',false,(UInt8*)strbuf,sizeof(strbuf)-2,NULL);
    if (l<0)l=0;
    else if (l>4095) l=4095;
    strbuf[l]=0;
    CFRelease(str);
  }
  
  {
    RGBColor tcolor={GetRValue(ct->cur_text_color_int)*256,GetGValue(ct->cur_text_color_int)*256,GetBValue(ct->cur_text_color_int)*256};
    ATSUAttributeTag        theTags[] = { kATSUColorTag,   };
    ByteCount               theSizes[] = { sizeof(RGBColor),  };
    ATSUAttributeValuePtr   theValues[] =  {&tcolor,  } ;
    
    
    ATSUSetAttributes(font->font_style,  sizeof(theTags)/sizeof(theTags[0]), theTags, theSizes, theValues);
  }
  
  UniCharCount runLengths[1]={kATSUToTextEnd};
  ATSUTextLayout layout; 
  ATSUCreateTextLayoutWithTextPtr(strbuf, kATSUFromTextBeginning, kATSUToTextEnd, l, 1, runLengths, &font->font_style, &layout);
  
 
  {
    Fixed frot = X2Fix(font->font_rotation);
     
    ATSULineTruncation tv = (align & DT_END_ELLIPSIS) ? kATSUTruncateEnd : kATSUTruncateNone;
    ATSUAttributeTag        theTags[] = { kATSUCGContextTag, kATSULineTruncationTag, kATSULineRotationTag };
    ByteCount               theSizes[] = { sizeof (CGContextRef), sizeof(ATSULineTruncation), sizeof(Fixed)};
    ATSUAttributeValuePtr   theValues[] =  { &ct->ctx, &tv, &frot } ;
    
    
    ATSUSetLayoutControls (layout,
                           
                           sizeof(theTags)/sizeof(theTags[0]),
                           
                           theTags,
                           
                           theSizes,
                           
                           theValues);
  }

  
  ATSUTextMeasurement leftFixed, rightFixed, ascentFixed, descentFixed;
  
  ATSUGetUnjustifiedBounds(layout, kATSUFromTextBeginning, kATSUToTextEnd, &leftFixed, &rightFixed, &ascentFixed, &descentFixed);
  
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
    
  //if (!(align & DT_NOCLIP))
  {
    CGContextSaveGState(ct->ctx);    
  }
 
  if (!(align & DT_NOCLIP))
    CGContextClipToRect(ct->ctx,CGRectMake(r->left,r->top,r->right-r->left,r->bottom-r->top));
  
  {
    
    int l=r->left, t=r->top;
    
    if (fabs(font->font_rotation)<45.0)
    {
      if (align & DT_RIGHT)
        l = r->right-w;
      else if (align & DT_CENTER)
        l = (r->right+r->left)/2 - w/2;      
    }
    else l+=Fix2Long(ascentFixed); // 90 degree special case (we should generalize this to be correct throughout the rotation range, but oh well)
    
    if (align & DT_BOTTOM)
      t = r->bottom-h;
    else if (align & DT_VCENTER)
      t = (r->bottom+r->top)/2 - h/2;
    
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
        
    ATSUDrawText(layout,kATSUFromTextBeginning,kATSUToTextEnd,Long2Fix(l),Long2Fix(t+descent));
  }

//  if (!(align & DT_NOCLIP))
  {
    CGContextRestoreGState(ct->ctx);    
  }
  
  ATSUDisposeTextLayout(layout);   
  
  return h;
}

#endif//SWELL_ATSUI_DRAWING
  


#if SWELL_CGDRAWING
int DrawText(HDC ctx, const char *buf, int buflen, RECT *r, int align)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return 0;
   
  //NSString *label=(NSString *)SWELL_CStringToCFString(buf); 
  //NSRect r2 = NSMakeRect(r->left,r->top,r->right-r->left,r->bottom-r->top);
  //[label drawWithRect:r2 options:NSStringDrawingUsesLineFragmentOrigin attributes:nil];
  //[label release];
  
  float xpos=(float)r->left;
  float ypos=(float)r->top;
  
  if (align & DT_CALCRECT)
  {
#if 0
    CGContextSaveGState(ct->ctx);
	CGContextSetTextDrawingMode(ct->ctx,kCGTextClip);
    CGContextShowTextAtPoint(ct->ctx,(float)r->left,(float)r->top,buf,strlen(buf));
	CGRect orect=CGContextGetClipBoundingBox(ct->ctx);
	r->right = r->left + (int) ceil(orect.size.width);
	r->bottom= r->top + (int) ceil(orect.size.height);
    CGContextRestoreGState(ct->ctx);
	// measure
#endif
	return 0;
  }
	
#if 0
//  if (align & (DT_VCENTER|DT_BOTTOM|DT_CENTER|DT_RIGHT))
  {
    CGContextSaveGState(ct->ctx);
 	CGContextSetTextDrawingMode(ct->ctx,kCGTextClip);
	CGContextClipToRect(ct->ctx,CGRectMake(0,0,0,0)); // tested with this in case the kCGTextClip adds rather than intersects
    CGContextShowTextAtPoint(ct->ctx,xpos,ypos,buf,strlen(buf));
	CGRect orect=CGContextGetClipBoundingBox(ct->ctx);
 	printf("text '%s'@%f, %f, measured to %f,%f,%f,%f\n",buf,xpos,ypos,orect.origin.x,orect.origin.y,orect.size.width,orect.size.height);
	CGContextRestoreGState(ct->ctx);

/*	if (align&DT_VCENTER)
	{
		ypos = r->top + (r->bottom-r->top - (orect.size.height))*0.5;
	}
	else if (align&DT_BOTTOM)
	{
	}
	else */ // top
	{
		float yoffs = orect.origin.y-r->top;
	//	ypos = r->top - yoffs;
	}
	/*
	if (align&DT_CENTER)
	{
		xpos = r->left + (r->right-r->left - (orect.size.width))*0.5;
	}
	else if (align&DT_RIGHT)
	{
	}
	else */ // left
 	{
//		xpos = r->left + (r->left - orect.origin.x);
	}
	
  }
#endif
    
  CGRect cr=CGRectMake((float)r->left,(float)r->top,(float)(r->right-r->left),(float)(r->bottom-r->top));
  CGContextSaveGState(ct->ctx);
//  CGContextClipToRect(ct->ctx,cr);

  if (ct->curcgtextcol) CGContextSetFillColorWithColor(ct->ctx,ct->curcgtextcol);
  CGContextSetTextDrawingMode(ct->ctx,kCGTextFill);
  CGContextShowTextAtPoint(ct->ctx,xpos,ypos,buf,strlen(buf));
  CGContextRestoreGState(ct->ctx);

  return 30;
}
#endif // CG drawing

void SetBkColor(HDC ctx, int col)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return;
  ct->curbkcol=col;
}

void SetBkMode(HDC ctx, int col)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return;
  ct->curbkmode=col;
}

void SetTextColor(HDC ctx, int col)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return;
  ct->cur_text_color_int = col;
  
#if SWELL_NSSTRING_DRAWING
  [ct->curtextcol release];    
  ct->curtextcol=[NSColor colorWithCalibratedRed:GetRValue(col)/255.0f green:GetGValue(col)/255.0f blue:GetBValue(col)/255.0f alpha:1.0f];
  [ct->curtextcol retain];
#else

  if (ct->curcgtextcol) CGColorRelease(ct->curcgtextcol);
  ct->curcgtextcol = CreateColor(col,1.0);
  
#endif
}

HICON LoadNamedImage(const char *name, bool alphaFromMask)
{
  int needfree=0;
  NSImage *img=0;
  NSString *str=(NSString *)SWELL_CStringToCFString(name); 
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
  GDP_OBJECT *i=GDP_OBJECT_NEW();
  i->type=TYPE_BITMAP;
  i->wid=needfree;
  i->bitmapptr = img;
  return i;
}

void DrawImageInRect(HDC ctx, HICON img, RECT *r)
{
  GDP_OBJECT *i = (GDP_OBJECT *)img;
  if (!ctx || !i || i->type != TYPE_BITMAP||!i->bitmapptr) return;
  GDP_CTX *ct=(GDP_CTX*)ctx;
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
  GDP_OBJECT *i = (GDP_OBJECT *)icon;
  if (!i || i->type != TYPE_BITMAP) return false;
  NSImage *img = i->bitmapptr;
  if (!img) return false;
  bm->bmWidth = (int) ([img size].width+0.5);
  bm->bmHeight = (int) ([img size].height+0.5);
  return true;
}


void *GetNSImageFromHICON(HICON ico)
{
  GDP_OBJECT *i = (GDP_OBJECT *)ico;
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

void BitBltAlphaFromMem(HDC hdcOut, int x, int y, int w, int h, void *inbufptr, int inbuf_span, int inbuf_h, int xin, int yin, int mode, bool useAlphaChannel, float opacity)
{
  if (opacity<0.0001) return;
  
  if (!hdcOut ||w<1||h<1) return;
  GDP_CTX *dest=(GDP_CTX*)hdcOut;
  if (!inbufptr || !inbuf_span || !inbuf_h || !dest->ctx) return;
  
  if (inbuf_span < 0)
  {
    inbufptr = ((char *)inbufptr) - 4*inbuf_span*(inbuf_h-1);
  }
  int srcWidth = inbuf_span < 0 ? -inbuf_span : inbuf_span;
  int srcHeight = inbuf_h;
  if (xin<0){ x-=xin; w+=xin; xin=0; }
  if (yin<0){ y-=yin; h+=yin; yin=0; }
  if (xin+w>srcWidth) w=srcWidth-xin;
  if (yin+h>srcHeight) h=srcHeight-yin;
  if (w<1||h<1) return;

  void *buf=calloc(w*4,h);
  if (!buf) return;
  CGColorSpaceRef cs=CGColorSpaceCreateDeviceRGB();
  CGContextRef newtmpctx=CGBitmapContextCreate(buf,w,h,8,w*4,cs, kCGImageAlphaPremultipliedFirst);
  CGColorSpaceRelease(cs);
  if (!newtmpctx)
  {
    free(buf);
    return;
  }

  unsigned char *outbuf = (unsigned char *)buf;
  unsigned char *inbuf = ((unsigned char *)inbufptr) + xin*4 + yin*4*inbuf_span;
  int a;
  int maxop=255;
  if (useAlphaChannel) maxop=256;
  int opac=(int)(opacity*maxop + 0.5);
  if (opac<0)opac=0;
  else if (opac>maxop) opac=maxop;
  
  for (a=0;a<h;a++)
  {
    int b;
    if (useAlphaChannel && opac!=256)
    {
      for (b=0;b<w;b++)
      {
        int v=*outbuf++ = (*inbuf++ * (int)opac)>>8;
        *outbuf++ = (*inbuf++*v)>>8;
        *outbuf++ = (*inbuf++*v)>>8;
        *outbuf++ = (*inbuf++*v)>>8;
      }
    }
    else if (useAlphaChannel)
    {
      for (b=0;b<w;b++)
      {
        int v=*outbuf++ = *inbuf++;
        *outbuf++ = (*inbuf++*v)>>8;
        *outbuf++ = (*inbuf++*v)>>8;
        *outbuf++ = (*inbuf++*v)>>8;
      }
    }
    else if (opac<255)
    {
      for (b=0;b<w;b++)
      {
        *outbuf++ = opac; inbuf++;
        *outbuf++ = (*inbuf++*opac)>>8;
        *outbuf++ = (*inbuf++*opac)>>8;
        *outbuf++ = (*inbuf++*opac)>>8;
      }
    }
    else
    {
      for (b=0;b<w;b++)
      {
        *outbuf++ = 255; inbuf++;
        *outbuf++ = *inbuf++;
        *outbuf++ = *inbuf++;
        *outbuf++ = *inbuf++;
      }
    }
    inbuf += inbuf_span*4 - w*4;
  }

  CGImage *img=CGBitmapContextCreateImage(newtmpctx);
  if (img)
  {
    CGContextSaveGState(dest->ctx);
    CGContextScaleCTM(dest->ctx,1.0,-1.0);  
    CGContextDrawImage(dest->ctx,CGRectMake(x,-h-y,w,h),img);    
    CGContextRestoreGState(dest->ctx);
    CGImageRelease(img);
  }
  CGContextRelease(newtmpctx);
  free(buf);

}

void BitBltAlpha(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int mode, bool useAlphaChannel, float opacity)
{
  if (!useAlphaChannel&&opacity>=0.9999)
  {
    BitBlt(hdcOut,x,y,w,h,hdcIn,xin,yin,mode);
    return;
  }

  if (!hdcOut || !hdcIn||w<1||h<1) return;
  GDP_CTX *src=(GDP_CTX*)hdcIn;
  GDP_CTX *dest=(GDP_CTX*)hdcOut;
  if (!src->ownedData || !src->ctx || !dest->ctx) return;
  
  BitBltAlphaFromMem(hdcOut,x,y,w,h,src->ownedData,CGBitmapContextGetWidth(src->ctx), CGBitmapContextGetHeight(src->ctx),xin,yin,mode,useAlphaChannel,opacity);
  
}

void BitBlt(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int mode)
{
  if (!hdcOut || !hdcIn||w<1||h<1) return;
  GDP_CTX *src=(GDP_CTX*)hdcIn;
  GDP_CTX *dest=(GDP_CTX*)hdcOut;
  if (!src->ownedData || !src->ctx || !dest->ctx) return;
  
  
  CGImageRef img=CGBitmapContextCreateImage(src->ctx);

  if (!img) return;
  
  CGContextRef output = (CGContextRef)dest->ctx; 
  
  if (xin || yin || w != CGImageGetWidth(img) || h != CGImageGetHeight(img))
  {
    CGImageRef img2 = CGImageCreateWithImageInRect(img,CGRectMake(xin,yin,w,h));
    CGImageRelease(img);
    if (!img2) return;
    img=img2;
    w=CGImageGetWidth(img);
    h=CGImageGetHeight(img);
  }
  
  CGContextSaveGState(output);
  CGContextScaleCTM(output,1.0,-1.0);  
  CGContextDrawImage(output,CGRectMake(x,-h-y,w,h),img);    
  CGContextRestoreGState(output);
  
  CGImageRelease(img);   
}

void StretchBlt(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int srcw, int srch, int mode)
{
  if (!hdcOut || !hdcIn||srcw<1||srch<1||w<1||h<1) return;
  GDP_CTX *src=(GDP_CTX*)hdcIn;
  GDP_CTX *dest=(GDP_CTX*)hdcOut;
  if (!src->ownedData || !src->ctx || !dest->ctx) return;

  CGImageRef img = CGBitmapContextCreateImage(src->ctx);
  if (!img) return;
  
  if (xin || yin || srcw != CGImageGetWidth(img) || srch != CGImageGetHeight(img))
  {
    CGImageRef img2 = CGImageCreateWithImageInRect(img,CGRectMake(xin,yin,srcw,srch));
    CGImageRelease(img);
    if (!img2) return;
    img=img2;
    w=CGImageGetWidth(img);
    h=CGImageGetHeight(img);
  }
    
  CGContextSaveGState(dest->ctx);
  CGContextScaleCTM(dest->ctx,1.0,-1.0);  
  CGContextDrawImage(dest->ctx,CGRectMake(x,-h-y,w,h),img);    
  CGContextRestoreGState(dest->ctx);
  
  CGImageRelease(img);
}

void SWELL_PushClipRegion(HDC ctx)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (ct && ct->ctx) CGContextSaveGState(ct->ctx);
}

void SWELL_SetClipRegion(HDC ctx, RECT *r)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (ct && ct->ctx) CGContextClipToRect(ct->ctx,CGRectMake(r->left,r->top,r->right-r->left,r->bottom-r->top));

}

void SWELL_PopClipRegion(HDC ctx)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (ct && ct->ctx) CGContextRestoreGState(ct->ctx);
}

void *SWELL_GetCtxFrameBuffer(HDC ctx)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (ct) return ct->ownedData;
  return 0;
}


HDC GetDC(HWND h)
{
  if (h && [(id)h isKindOfClass:[NSWindow class]])
  {
  // todo: eventually this can go away (once we move over to fully view based dialogs)
    if ([(id)h respondsToSelector:@selector(getSwellPaintInfo:)]) 
    {
      PAINTSTRUCT ps={0,}; 
      [(id)h getSwellPaintInfo:(PAINTSTRUCT *)&ps];
      if (ps.hdc) 
      {
        if (((GDP_CTX*)ps.hdc)->ctx) CGContextSaveGState(((GDP_CTX*)ps.hdc)->ctx);
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
        if (((GDP_CTX*)ps.hdc)->ctx) CGContextSaveGState(((GDP_CTX*)ps.hdc)->ctx);
        return ps.hdc;
      }
    }
    
    if ([(NSView*)h lockFocusIfCanDraw])
    {
      HDC ret= SWELL_CreateGfxContext([NSGraphicsContext currentContext]);
      if (ret)
      {
         if (((GDP_CTX*)ret)->ctx) CGContextSaveGState(((GDP_CTX*)ret)->ctx);
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
      if ((xsc || ysc) && ((GDP_CTX*)ret)->ctx) CGContextTranslateCTM(((GDP_CTX*)ret)->ctx,xsc,ysc);
    }
  }
  return ret;
}

void ReleaseDC(HWND h, HDC hdc)
{
  if (hdc)
  {
    if (((GDP_CTX*)hdc)->ctx) CGContextRestoreGState(((GDP_CTX*)hdc)->ctx);
  }
  if (h && [(id)h isKindOfClass:[NSWindow class]])
  {
    // todo: eventually this can go away (once we move over to fully view based stuff)
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


#endif
