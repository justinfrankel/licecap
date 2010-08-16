
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


#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include "swell.h"

#define TYPE_PEN 1
#define TYPE_BRUSH 2
#define TYPE_FONT 3
#define TYPE_BITMAP 4

typedef struct 
{
  int type;
  CGColorRef color;
  int wid;
  char *fontface;
  NSImage *bitmapptr;
} GDP_OBJECT;

typedef struct {
  CGContextRef ctx; 
  void *ownedData;
  GDP_OBJECT *curpen;
  GDP_OBJECT *curbrush;
  GDP_OBJECT *curfont;
  CGColorRef curtextcol;
  CGImageRef bitmapimagecache;
  float lastpos_x,lastpos_y;
} GDP_CTX;


static CGColorRef CreateColor(int col)
{
  CGColorSpaceRef cspace=CGColorSpaceCreateDeviceRGB();
  float cols[4]={GetRValue(col)/255.0,GetGValue(col)/255.0,GetBValue(col)/255.0,1.0};
  CGColorRef color=CGColorCreate(cspace,cols);
  CGColorSpaceRelease(cspace);
  return color;
}

HDC WDL_GDP_CreateContext(void *c)
{
  GDP_CTX *ctx=(GDP_CTX *) calloc(1,sizeof(GDP_CTX));
  ctx->ctx=(CGContextRef)c;
  CGAffineTransform f={1,0,0,-1,0,0};
  CGContextSetTextMatrix(ctx->ctx,f);
 // CGContextSelectFont(ctx->ctx,"Arial",12.0,kCGEncodingMacRoman);
  return ctx;
}

HDC WDL_GDP_CreateMemContext(HDC hdc, int w, int h)
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
  
  GDP_CTX *ctx=(GDP_CTX *) calloc(1,sizeof(GDP_CTX));
  ctx->ctx=(CGContextRef)c;
  ctx->ownedData=buf;
  // CGContextSelectFont(ctx->ctx,"Arial",12.0,kCGEncodingMacRoman);
  return ctx;
}

#define INVALIDATE_BITMAPCACHE(x) if ((x)->bitmapimagecache) { CGImageRelease((x)->bitmapimagecache); (x)->bitmapimagecache=0;  }
void WDL_GDP_DeleteContext(HDC ctx)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (ct)
  {
    INVALIDATE_BITMAPCACHE(ct);
    if (ct->ownedData)
    {
      CGContextRelease(ct->ctx);
      free(ct->ownedData);
    }
    if (ct->curtextcol) CGColorRelease(ct->curtextcol);
    free(ctx);
  }
}

HPEN CreatePen(int attr, int wid, int col)
{
  GDP_OBJECT *pen=(GDP_OBJECT *)calloc(sizeof(GDP_OBJECT),1);
  pen->type=TYPE_PEN;
  pen->wid=wid;
  pen->color=CreateColor(col);
  return pen;
}
HBRUSH  CreateSolidBrush(int col)
{
  GDP_OBJECT *brush=(GDP_OBJECT *)calloc(sizeof(GDP_OBJECT),1);
  brush->type=TYPE_BRUSH;
  brush->color=CreateColor(col);
  return brush;
}

HFONT CreateFontIndirect(LOGFONT *lf)
{
  GDP_OBJECT *font=(GDP_OBJECT *)calloc(sizeof(GDP_OBJECT),1);
  font->type=TYPE_FONT;
  font->wid=lf->lfHeight;
  if (!font->wid) font->wid=lf->lfWidth;
  if (font->wid<0)font->wid=-font->wid;
  font->fontface = strdup(lf->lfFaceName);
  return font;
}

void DeleteObject(HGDIOBJ pen)
{
  if (pen)
  {
    GDP_OBJECT *p=(GDP_OBJECT *)pen;
    if (p->type == TYPE_PEN || p->type == TYPE_BRUSH || p->type == TYPE_FONT || p->type == TYPE_BITMAP)
    {
      if (p->color) CGColorRelease(p->color);
      free(p->fontface);
      if (p->wid && p->bitmapptr) [p->bitmapptr release]; 
    }
    free(p);
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



void FillRect(HDC ctx, RECT *r, HBRUSH br)
{
  GDP_CTX *c=(GDP_CTX *)ctx;
  GDP_OBJECT *b=(GDP_OBJECT*) br;
  if (!c || !b || b == (GDP_OBJECT*)TYPE_BRUSH || b->type != TYPE_BRUSH) return;

  INVALIDATE_BITMAPCACHE(c);
  
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

void Rectangle(HDC ctx, int l, int t, int r, int b)
{
  GDP_CTX *c=(GDP_CTX *)ctx;
  if (!c) return;
  
  CGRect rect=CGRectMake(l,t,r-l,b-t);
  INVALIDATE_BITMAPCACHE(c);
  
  if (c->curbrush)
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
  if (!c->curbrush && !c->curpen || npts<2) return;
  INVALIDATE_BITMAPCACHE(c);

  CGContextBeginPath(c->ctx);
  CGContextMoveToPoint(c->ctx,(float)pts[0].x,(float)pts[0].y);
  int x;
  for (x = 1; x < npts; x ++)
  {
    CGContextAddLineToPoint(c->ctx,(float)pts[x].x,(float)pts[x].y);
  }
  if (c->curbrush)
  {
    CGContextSetFillColorWithColor(c->ctx,c->curbrush->color);
  }
  if (c->curpen && c->curpen->wid>=0)
  {
    CGContextSetLineWidth(c->ctx,(float)max(c->curpen->wid,1));
    CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);	
  }
  CGContextDrawPath(c->ctx,c->curpen && c->curpen->wid>=0 && c->curbrush ?  kCGPathFillStroke : c->curpen && c->curpen->wid>=0 ? kCGPathStroke : kCGPathFill);
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
  INVALIDATE_BITMAPCACHE(c);
  
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


void LineTo(HDC ctx, int x, int y)
{
  GDP_CTX *c=(GDP_CTX *)ctx;
  if (!c||!c->curpen||c->curpen->wid<0) return;
  INVALIDATE_BITMAPCACHE(c);

  CGContextSetLineWidth(c->ctx,(float)max(c->curpen->wid,1));
  CGContextSetStrokeColorWithColor(c->ctx,c->curpen->color);
	
  CGContextBeginPath(c->ctx);
  CGContextMoveToPoint(c->ctx,c->lastpos_x,c->lastpos_y);
  CGContextAddLineToPoint(c->ctx,(float)x,(float)y);
  c->lastpos_x=(float)x;
  c->lastpos_y=(float)y;
  CGContextStrokePath(c->ctx);
}

void PolyPolyline(HDC ctx, POINT *pts, DWORD *cnts, int nseg)
{
  GDP_CTX *c=(GDP_CTX *)ctx;
  if (!c||!c->curpen||c->curpen->wid<0||nseg<1) return;
  INVALIDATE_BITMAPCACHE(c);

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

void SWELL_SyncCtxFrameBuffer(HDC ctx)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return;
  INVALIDATE_BITMAPCACHE(ct);
}

void SetPixel(HDC ctx, int x, int y, int c)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return;
  INVALIDATE_BITMAPCACHE(ct);
  CGContextBeginPath(ct->ctx);
  CGContextMoveToPoint(ct->ctx,(float)x,(float)y);
  CGContextAddLineToPoint(ct->ctx,(float)x+0.5,(float)y+0.5);
  CGContextSetLineWidth(ct->ctx,(float)1.5);
  CGContextSetRGBStrokeColor(ct->ctx,GetRValue(c)/255.0,GetGValue(c)/255.0,GetBValue(c)/255.0,1.0);
  CGContextStrokePath(ct->ctx);	
}

void DrawText(HDC ctx, const char *buf, int buflen, RECT *r, int align)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return;
  INVALIDATE_BITMAPCACHE(ct);
  
#if 1
  CFStringRef label=(CFStringRef)SWELL_CStringToCFString(buf); 
  HIRect hiBounds = { {r->left, r->top}, {r->right-r->left, r->bottom-r->top} };
  HIThemeTextInfo textInfo = {0, kThemeStateActive, kThemeCurrentPortFont, kHIThemeTextHorizontalFlushLeft, 
	  kHIThemeTextVerticalFlushTop, kHIThemeTextBoxOptionStronglyVertical, kHIThemeTextTruncationEnd, 1, false};

  if (ct->curfont && ct->curfont->wid <= 12) textInfo.fontID=kThemeMiniSystemFont;
  //else if (ct->curfont->wid < 14) textInfo.fontID=kThemeLabelFont; 
  //else if (ct->curfont->wid <= 16) textInfo.fontID=kThemeSmallSystemFont;
  else textInfo.fontID=kThemeSystemFont;
  
  if (align & DT_CENTER) textInfo.horizontalFlushness=kHIThemeTextHorizontalFlushCenter;
  else if (align&DT_RIGHT) textInfo.horizontalFlushness=kHIThemeTextHorizontalFlushRight;
  if (align & DT_VCENTER) textInfo.verticalFlushness=kHIThemeTextVerticalFlushCenter;
  else if (align&DT_BOTTOM) textInfo.verticalFlushness=kHIThemeTextVerticalFlushBottom;
	
  if (align & DT_CALCRECT)
  {
    float w=r->right-r->left,h=r->bottom-r->top;
    HIThemeGetTextDimensions(label,0,&textInfo,&w,&h,NULL);
    r->right=r->left+(int)w;
    r->bottom=r->top+(int)h;
  }
  else
  {
    if (ct->curtextcol) CGContextSetFillColorWithColor(ct->ctx,ct->curtextcol);

    HIThemeDrawTextBox(label, &hiBounds, &textInfo, ct->ctx, kHIThemeOrientationNormal);
  }
  CFRelease(label);
   
#else
  
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
	return;
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
  if (ct->curtextcol) CGContextSetFillColorWithColor(ct->ctx,ct->curtextcol);
  CGContextSetTextDrawingMode(ct->ctx,kCGTextFill);
  CGContextShowTextAtPoint(ct->ctx,xpos,ypos,buf,strlen(buf));
  CGContextRestoreGState(ct->ctx);
#endif
}

void SetBkColor(HDC ctx, int col)
{
}

void SetBkMode(HDC ctx, int col)
{
}

void SetTextColor(HDC ctx, int col)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (!ct) return;
  if (ct->curtextcol) CGColorRelease(ct->curtextcol);
  ct->curtextcol=CreateColor(col);
}

BOOL GetTextMetrics(HDC ctx, TEXTMETRIC *tm)
{
  GDP_CTX *ct=(GDP_CTX *)ctx;
  if (tm)
  {
    tm->tmInternalLeading=3;
    tm->tmAscent=12;
  }
  if (!ct||!tm||!ct->curfont) return 0;
  tm->tmInternalLeading=ct->curfont->wid/5;
  tm->tmAscent=ct->curfont->wid;
  return 1;
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
          float comp[4];
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
  GDP_OBJECT *i=(GDP_OBJECT *)calloc(1,sizeof(GDP_OBJECT));
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
  INVALIDATE_BITMAPCACHE(ct);
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

void *GetNSImageFromHICON(HICON ico)
{
  GDP_OBJECT *i = (GDP_OBJECT *)ico;
  if (!i || i->type != TYPE_BITMAP) return 0;
  return i->bitmapptr;
}

static int ColorFromNSColor(NSColor *color, int valifnul)
{
  if (!color) return valifnul;
  float r,g,b;
  [color getRed:&r green:&g blue:&b alpha:NULL];
  return RGB((int)(r*255.0),(int)(g*255.0),(int)(b*255.0));
}
#define ColorFromNSColor(a,b) (b)

int GetSysColor(int idx)
{

  switch (idx)
  {
    case COLOR_WINDOW: return ColorFromNSColor([NSColor windowBackgroundColor],RGB(192,192,192));
    case COLOR_3DFACE: 
    case COLOR_BTNFACE: return ColorFromNSColor([NSColor controlColor],RGB(192,192,192));
    case COLOR_SCROLLBAR: return ColorFromNSColor([NSColor scrollbarColor],RGB(32,32,32));
    case COLOR_3DSHADOW: return ColorFromNSColor([NSColor shadowColor],RGB(32,32,32));
    case COLOR_3DHILIGHT: return ColorFromNSColor([NSColor highlightColor],RGB(224,224,224));
    case COLOR_BTNTEXT: return ColorFromNSColor([NSColor controlTextColor],RGB(0,0,0));
    case COLOR_3DDKSHADOW: return (ColorFromNSColor([NSColor shadowColor],RGB(32,32,32))>>1)&0x7f7f7f;
    case COLOR_INFOBK: return RGB(255,240,200);
    case COLOR_INFOTEXT: return RGB(0,0,0);
      
  }
  return 0;
}

void BitBlt(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int mode)
{
  if (!hdcOut || !hdcIn) return;
  GDP_CTX *src=(GDP_CTX*)hdcIn;
  GDP_CTX *dest=(GDP_CTX*)hdcOut;
  if (!src->ownedData || !src->ctx || !dest->ctx) return;
  
  if (!src->bitmapimagecache) 
    src->bitmapimagecache=CGBitmapContextCreateImage(src->ctx);
  
  CGImageRef img=src->bitmapimagecache;
  if (!img) return;
  
  CGContextSaveGState(dest->ctx);
  CGContextClipToRect(dest->ctx,CGRectMake(x,y,w,h));
  
  CGContextDrawImage(dest->ctx,CGRectMake(x-xin,y-yin,CGImageGetWidth(img),CGImageGetHeight(img)),img);
  CGContextRestoreGState(dest->ctx);
  
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
