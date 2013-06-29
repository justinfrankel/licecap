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
  

    This file provides basic APIs for browsing for files, directories, and messageboxes.

    These APIs don't all match the Windows equivelents, but are close enough to make it not too much trouble.

  */


#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "../wdlcstring.h"
#import <Cocoa/Cocoa.h>
static NSMutableArray *extensionsFromList(const char *extlist)
{
	NSMutableArray *fileTypes = [[NSMutableArray alloc] initWithCapacity:30];
	while (*extlist)
	{
		extlist += strlen(extlist)+1;
		if (!*extlist) break; 
		while (*extlist)
		{
			while (*extlist && *extlist != '.') extlist++;
			if (!*extlist) break;
			extlist++;
			char tmp[32];
			lstrcpyn_safe(tmp,extlist,sizeof(tmp));
			if (strstr(tmp,";")) strstr(tmp,";")[0]=0;
			if (tmp[0] && tmp[0]!='*')
			{
				NSString *s=(NSString *)SWELL_CStringToCFString(tmp);
				[fileTypes addObject:s];
				[s release];
			}
			while (*extlist && *extlist != ';') extlist++;
			if (*extlist == ';') extlist++;
		}
		extlist++;
	}
	
	return fileTypes;
}

static const char *BFSF_Templ_dlgid;
static DLGPROC BFSF_Templ_dlgproc;
static struct SWELL_DialogResourceIndex *BFSF_Templ_reshead;
void BrowseFile_SetTemplate(const char *dlgid, DLGPROC dlgProc, struct SWELL_DialogResourceIndex *reshead)
{
  BFSF_Templ_reshead=reshead;
  BFSF_Templ_dlgid=dlgid;
  BFSF_Templ_dlgproc=dlgProc;
}

// return true
bool BrowseForSaveFile(const char *text, const char *initialdir, const char *initialfile, const char *extlist,
                       char *fn, int fnsize)
{
	NSSavePanel *panel = [NSSavePanel savePanel];
  NSString *title=(NSString *)SWELL_CStringToCFString(text); 
	[panel setTitle:title];
  [panel setAccessoryView:nil];
  HWND oh=NULL;
  if (BFSF_Templ_dlgproc && BFSF_Templ_dlgid) // create a child dialog and set it to the panel
  {
    oh=SWELL_CreateDialog(BFSF_Templ_reshead, BFSF_Templ_dlgid, (HWND)panel, BFSF_Templ_dlgproc, 0);
    BFSF_Templ_dlgproc=0;
    BFSF_Templ_dlgid=0;
  }
	
	NSMutableArray *fileTypes = extensionsFromList(extlist);	
	
	[panel setAllowedFileTypes:fileTypes];
	NSString *ifn=0;
	NSString *idir=0;
	
	if (initialfile && *initialfile)
	{
		char s[2048];
		lstrcpyn_safe(s,initialfile,sizeof(s));
		char *p=s;
		while (*p) p++;
		while (p >= s && *p != '/') p--;
    if (p>=s)
    {
      *p=0;
      ifn=(NSString *)SWELL_CStringToCFString(p+1);
      idir=(NSString *)SWELL_CStringToCFString(s[0]?s:"/");
    }
    else 
      ifn=(NSString *)SWELL_CStringToCFString(s);
	}
	if (!idir && initialdir && *initialdir)
	{
		idir=(NSString *)SWELL_CStringToCFString(initialdir);
	}
	
  HMENU hm=SWELL_GetDefaultModalWindowMenu();
  if (hm) hm=SWELL_DuplicateMenu(hm);
  SWELL_SetCurrentMenu(hm);
	int result = [panel runModalForDirectory:idir file:ifn];
  SWELL_SetCurrentMenu(GetMenu(GetFocus()));
  if (hm) DestroyMenu(hm);
  
  if (oh) SendMessage(oh,WM_DESTROY,0,0);
  [panel setAccessoryView:nil];
	[title release];
	[fileTypes release];
	
	if (result == NSOKButton)
	{
		char buf[2048];
		buf[0]=0;
		buf[sizeof(buf)-1]=0;
		SWELL_CFStringToCString([panel filename],buf,(sizeof(buf)-1));
		if (buf[0])
		{
			lstrcpyn_safe(fn,buf,fnsize);
			return true;
		}
	}
	
	return false;
	
}

bool BrowseForDirectory(const char *text, const char *initialdir, char *fn, int fnsize)
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
  NSString *title=(NSString *)SWELL_CStringToCFString(text); 
	[panel setTitle:title];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseFiles:NO];
	[panel setCanCreateDirectories:YES];
	[panel setCanChooseDirectories:YES];
	[panel setResolvesAliases:YES];

  HWND oh=NULL;
  if (BFSF_Templ_dlgproc && BFSF_Templ_dlgid) // create a child dialog and set it to the panel
  {
    oh=SWELL_CreateDialog(BFSF_Templ_reshead, BFSF_Templ_dlgid, (HWND)panel, BFSF_Templ_dlgproc, 0);
    BFSF_Templ_dlgproc=0;
    BFSF_Templ_dlgid=0;
  }
	
	NSString *idir=0;
	
	if (initialdir && *initialdir)
	{
		idir=(NSString *)SWELL_CStringToCFString(initialdir);
	}
	
  HMENU hm=SWELL_GetDefaultModalWindowMenu();
  if (hm) hm=SWELL_DuplicateMenu(hm);
  SWELL_SetCurrentMenu(hm);
	int result = [panel runModalForDirectory:idir file:nil types:nil];
  SWELL_SetCurrentMenu(GetMenu(GetFocus()));
  if (hm) DestroyMenu(hm);
	
  if (oh) SendMessage(oh,WM_DESTROY,0,0);
  [panel setAccessoryView:nil];
  
	if (idir) [idir release];
	
	[title release];
	
	if (result != NSOKButton) return 0;
	
	NSArray *filesToOpen = [panel filenames];
	int count = [filesToOpen count];
		
	if (!count) return 0;
		
  NSString *aFile = [filesToOpen objectAtIndex:0];
  if (!aFile) return 0;
  SWELL_CFStringToCString(aFile,fn,(fnsize-1));
  fn[fnsize-1]=0;
  return 1;
}


char *BrowseForFiles(const char *text, const char *initialdir, 
                     const char *initialfile, bool allowmul, const char *extlist)
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
  NSString *title=(NSString *)SWELL_CStringToCFString(text); 
  HWND oh=NULL;
  if (BFSF_Templ_dlgproc && BFSF_Templ_dlgid) // create a child dialog and set it to the panel
  {
    oh=SWELL_CreateDialog(BFSF_Templ_reshead, BFSF_Templ_dlgid, (HWND)panel, BFSF_Templ_dlgproc, 0);
    BFSF_Templ_dlgproc=0;
    BFSF_Templ_dlgid=0;
  }

	[panel setTitle:title];
	[panel setAllowsMultipleSelection:(allowmul?YES:NO)];
	[panel setCanChooseFiles:YES];
	[panel setCanChooseDirectories:NO];
	[panel setResolvesAliases:YES];
	
	NSMutableArray *fileTypes = extensionsFromList(extlist);	
	
	NSString *ifn=0;
	NSString *idir=0;
	
	if (initialfile && *initialfile)
	{
		char s[2048];
		lstrcpyn_safe(s,initialfile,sizeof(s));
		char *p=s;
		while (*p) p++;
		while (p >= s && *p != '/') p--;
    if (p>=s)
    {
  		*p=0;
	  	ifn=(NSString *)SWELL_CStringToCFString(p+1);
      idir=(NSString *)SWELL_CStringToCFString(s[0]?s:"/");
    }
    else 
	  	ifn=(NSString *)SWELL_CStringToCFString(s);
    
	}
	if (!idir && initialdir && *initialdir)
	{
		idir=(NSString *)SWELL_CStringToCFString(initialdir);
	}
	
  HMENU hm=SWELL_GetDefaultModalWindowMenu();
  if (hm) hm=SWELL_DuplicateMenu(hm);
  SWELL_SetCurrentMenu(hm);
  
	int result = [panel runModalForDirectory:idir file:ifn types:fileTypes];

  SWELL_SetCurrentMenu(GetMenu(GetFocus()));
  if (hm) DestroyMenu(hm);
	
  if (oh) SendMessage(oh,WM_DESTROY,0,0);
  [panel setAccessoryView:nil];
  
	if (ifn) [ifn release];
	if (idir) [idir release];
	
	[fileTypes release];
	[title release];
	
	if (result != NSOKButton) return 0;
	
	NSArray *filesToOpen = [panel filenames];
	int i, count = [filesToOpen count];
		
	if (!count) return 0;
		
	if (count==1||!allowmul)
	{
		NSString *aFile = [filesToOpen objectAtIndex:0];
		if (!aFile) return 0;
		char fn[2048];
		SWELL_CFStringToCString(aFile,fn,(sizeof(fn)-1));
		fn[sizeof(fn)-1]=0;
		char *ret=(char *)malloc(strlen(fn)+2);
		memcpy(ret,fn,strlen(fn));
		ret[strlen(fn)]=0;
		ret[strlen(fn)+1]=0;
		return ret;
	}
		
	int rsize=1;
	char *ret=0;
	for (i=0; i<count; i++) 
	{
		NSString *aFile = [filesToOpen objectAtIndex:i];
		if (!aFile) continue;
		char fn[2048];
		SWELL_CFStringToCString(aFile,fn,(sizeof(fn)-1));
		fn[sizeof(fn)-1]=0;
		
		int tlen=strlen(fn)+1;
		ret=(char *)realloc(ret,rsize+tlen+1);
		if (!ret) return 0;
    
		if (rsize==1) ret[0]=0;
		strcpy(ret+rsize,fn);
		rsize+=tlen;
		ret[rsize]=0;
	}	
	return ret;
}




int MessageBox(HWND hwndParent, const char *text, const char *caption, int type)
{
  
  int ret=0;

  NSString *tit=(NSString *)SWELL_CStringToCFString(caption?caption:""); 
  NSString *text2=(NSString *)SWELL_CStringToCFString(text?text:"");
  
  if (type == MB_OK)
  {
    NSRunAlertPanel(tit,@"%@",@"OK",@"",@"",text2);
    ret=IDOK;
  }	
  else if (type == MB_OKCANCEL)
  {
    ret=NSRunAlertPanel(tit,@"%@",@"OK",@"Cancel",@"",text2);
    if (ret) ret=IDOK;
    else ret=IDCANCEL;
  }
  else if (type == MB_YESNO)
  {
    ret=NSRunAlertPanel(tit,@"%@",@"Yes",@"No",@"",text2);
  //  printf("ret=%d\n",ret);
    if (ret) ret=IDYES;
    else ret=IDNO;
  }
  else if (type == MB_RETRYCANCEL)
  {
    ret=NSRunAlertPanel(tit,@"%@",@"Retry",@"Cancel",@"",text2);
//    printf("ret=%d\n",ret);
    if (ret) ret=IDRETRY;
    else ret=IDCANCEL;
  }
  else if (type == MB_YESNOCANCEL)
  {
    ret=NSRunAlertPanel(tit,@"%@",@"Yes",@"Cancel",@"No",text2);
    if (ret == 1) ret=IDYES;
    else if (ret==-1) ret=IDNO;
    else ret=IDCANCEL;
  }
  
  [text2 release];
  [tit release];
  
  return ret; 
}

#endif
