#ifndef SWELL_PROVIDED_BY_APP

//#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include "swell.h"
#include "swell-internal.h"

char *lstrcpyn(char *dest, const char *src, int l)
{
  if (l<1) return dest;

  char *dsrc=dest;
  while (--l > 0)
  {
    char p=*src++;
    if (!p) break;
    *dest++=p;
  }
  *dest++=0;

  return dsrc;
}

@implementation SWELL_DataHold
-(id) initWithVal:(void *)val
{
  if ((self = [super init]))
  {
    m_data=val;
  }
  return self;
}
-(void *) getValue
{
  return m_data;
}
@end

void *SWELL_CStringToCFString(const char *str)
{
  if (!str) str="";
  void *ret;
  
  ret=(void *)CFStringCreateWithCString(NULL,str,kCFStringEncodingUTF8);
  if (ret) return ret;
  ret=(void*)CFStringCreateWithCString(NULL,str,kCFStringEncodingASCII);
  return ret;
}

DWORD GetModuleFileName(HINSTANCE ignored, char *fn, DWORD nSize)
{
  *fn=0;
#ifdef SWELL_TARGET_OSX
  CFBundleRef bund=CFBundleGetMainBundle();
  if (bund) 
  {
    CFURLRef url=CFBundleCopyBundleURL(bund);
    if (url)
    {
      char buf[8192];
      if (CFURLGetFileSystemRepresentation(url,true,(UInt8*)buf,sizeof(buf)))
      {
        lstrcpyn(fn,buf,nSize);
      }
      CFRelease(url);
    }
  }
#endif
  return strlen(fn);
}


#endif
