/*
 * to generate a template language pack, use:
 * build_sample_langpack --template *.rc *.cpp etc > file.langpack
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#define stricmp strcasecmp
#endif

#include "../wdltypes.h"
#include "../assocarray.h"
#include "../ptrlist.h"
#include "../wdlstring.h"
#include "../fnv64.h"

static bool isblank(char c)
{
  return c==' ' || c== '\t' || c=='\r' || c=='\n';
}
static bool isblankorz(char c)
{
  return !c || isblank(c);
}

WDL_StringKeyedArray<char *> section_descs;

WDL_StringKeyedArray< WDL_PtrList<char> * > translations;
WDL_StringKeyedArray< WDL_StringKeyedArray<bool> * > translations_indexed;

void gotString(const char *str, int len, const char *secname)
{
  WDL_PtrList<char> *sec=translations.Get(secname);
  if (!sec)
  {
    sec=new WDL_PtrList<char>;
    translations.Insert(secname,sec);
  }
  WDL_StringKeyedArray<bool> *sec2 = translations_indexed.Get(secname);
  if (!sec2)
  {
    sec2 = new WDL_StringKeyedArray<bool>(true);
    translations_indexed.Insert(secname,sec2);
  }
  if (len > (int)strlen(str)) 
  {
    fprintf(stderr,"gotString got len>strlen(str)\n");
    exit(1);
  }
  char buf[8192];
  if (len > (int) (sizeof(buf)-8))
  {
    fprintf(stderr,"argh, got string longer than 8k, adjust code accordingly or check input.\n");
    exit(1);
  }

  memcpy(buf,str,len);
  buf[len]=0;
  if (sec2->Get(buf)) return; //already in list

  sec2->Insert(buf,true);
  sec->Add(strdup(buf));
}
const char *g_last_file;
int g_last_linecnt;
int length_of_quoted_string(char *p, bool convertRCquotesToSlash)
{
  int l=0;
  while (p[l])
  {
    if (convertRCquotesToSlash && p[l] == '\"' && p[l+1] == '\"')  p[l]='\\';

    if (p[l] == '\"') return l;
    if (p[l] == '\\') 
    {
      l++;
    }
    l++;
  }
  fprintf(stderr,"ERROR: mismatched quotes in file %s:%d, string '%s', check input!\n",g_last_file,g_last_linecnt,p);
  exit(1);
  return -1;
}

static int uint64cmpfunc(WDL_UINT64 *a, WDL_UINT64 *b)
{
  if (*a < *b) return -1;
  if (*a > *b) return 1;
  return 0;
}
#define HACK_WILDCARD_ENTRY 2
static int isLocalizeCall(const char *p)
{
  int rv = 0;
  if (!strncmp(p,"__LOCALIZE",10)) { p+=10; rv = 1; }
  else if (!strncmp(p,"ADD_WILDCARD_ENTRY",18)) { p+=18; rv = HACK_WILDCARD_ENTRY; }
  else return 0;

  if (*p == '_') while (*p == '_' || (*p >= 'A'  && *p <= 'Z')||(*p>='0' && *p <='9')) p++;
  while (*p == ' ') p++;
  return *p == '(' ? rv : 0;
}

WDL_UINT64 outputLine(const char *strv, int casemode)
{
  WDL_UINT64 h = WDL_FNV64_IV;
  const char *p=strv;
  while (*p)
  {
    char c = *p++;
    if (c == '\\')
    {
      if (*p == '\\'||*p == '"' || *p == '\'') h=WDL_FNV64(h,(unsigned char *)p,1);
      else if (*p == 'n') h=WDL_FNV64(h,(unsigned char *)"\n",1);
      else if (*p == 'r') h=WDL_FNV64(h,(unsigned char *)"\r",1);
      else if (*p == 't') h=WDL_FNV64(h,(unsigned char *)"\t",1);
      else if (*p == '0') h=WDL_FNV64(h,(unsigned char *)"",1);
      else if (*p == 'x' && p[1] == 'e' && p[2] == '9') 
      {
        h=WDL_FNV64(h,(unsigned char *)"\xe9",1);
        p+=2;
      }
      else 
      {
        fprintf(stderr,"ERROR: unknown escape seq in '%s' at '%s'\n",strv,p);
      	exit(1);
      }
      p++;
    }
    else h=WDL_FNV64(h,(unsigned char *)&c,1);
  }
  h=WDL_FNV64(h,(unsigned char *)"",1);

  printf("%08X%08X=",(int)(h>>32),(int)(h&0xffffffff));
  int lc = 0;
  while (*strv)
  {
    int c = *strv++;
    if (lc == '%' || lc == '\\') { /* hacky*/ }
    else if (c == '\\' && strv[0] == 'x' && strv[1] == 'e' && strv[2] == '9') 
    {
      strv+=3;
      c = 0xe9;
    }
    else if (casemode == 2)
    {
      switch (tolower(c))
      {
        case 'o': c='0'; break;
        case 'i': c='1'; break;
        case 'e': c='3'; break;
        case 'a': c='4'; break;
        case 's': c='5'; break;
      }
    }
    else if (casemode==-1) c=tolower(c);
    else if (casemode==1) c=toupper(c);
    else if (casemode==4)
    {
      switch (c)
      {
        case 'E': c=0xc494; break;
        case 'A': c=0xc381; break;
        case 'I': c=0xc38f; break;
        case 'N': c=0xc391; break;
        case 'O': c=0xc395; break;
        case 'U': c=0xc39a; break;
        case 'B': c=0xc39f; break;
        case 'C': c=0xc486; break;
        case 'G': c=0xc4a0; break;
        case 'e': c=0xc497; break;
        case 'a': c=0xc3a4; break;
        case 'i': c=0xc4ad; break;
        case 'n': c=0xc584; break;
        case 'o': c=0xc58d; break;
        case 'u': c=0xc5af; break;
        case 'b': c=0xc39e; break;
        case 'c': c=0xc48d; break;
        case 'g': c=0xc49f; break;
      }
      if (c >= 256)
      {
        printf("%c",(c>>8));
        c&=0xff;
      }
    }
    printf("%c",c);
    if (lc == '%' && (c == '.' || c=='l' || (c>='0' && c<='9'))) 
    {
      // ignore .xyz and l between format spec (hacky)
    }
    else if (lc == '%' && c == '%') lc = 0;
    else if (lc == '\\' && c == '\\') lc = 0;
    else lc = c;
  }
  printf("\n");
  return h;
}


WDL_StringKeyedArray<int> g_resdefs;
const char *getResourceDefinesFromHeader(const char *fn)
{
  g_resdefs.DeleteAll();

  FILE *fp=fopen(fn,"rb");
  if (!fp) return "error opening header";
  for (;;)
  {
    char buf[8192];
    g_last_linecnt++;
    if (!fgets(buf,sizeof(buf),fp)) break;
    char *p = buf;
    while (*p) p++;
    while (p>buf && (p[-1] == '\r'|| p[-1] == '\n' || p[-1] == ' ')) p--;
    *p=0;
    
    if (!strncmp(buf,"#define",7))
    {
      p=buf;
      while (*p && *p != ' '&& *p != '\t') p++;
      while (*p == ' ' || *p == '\t') p++;
      char *n1 = p;
      while (*p && *p != ' '&& *p != '\t') p++;
      if (*p) *p++=0;
      while (*p == ' ' || *p == '\t') p++;
      int a = atoi(p);
      if (a && *n1)
      {
        g_resdefs.Insert(n1,a);
      }
    }
  }

  fclose(fp);
  return NULL;
}

void processRCfile(FILE *fp, const char *dirprefix)
{
  char sname[512];
  sname[0]=0;
  int depth=0;
  for (;;)
  {
    char buf[8192];
    g_last_linecnt++;
    if (!fgets(buf,sizeof(buf),fp)) break;
    char *p = buf;
    while (*p) p++;
    while (p>buf && (p[-1] == '\r'|| p[-1] == '\n' || p[-1] == ' ')) p--;
    *p=0;

    p=buf;
    if (sname[0]) while (*p == ' ' || *p == '\t') p++;
    char *first_tok = p;
    if (!strncmp(first_tok,"CLASS",5) && isblank(first_tok[5]) && first_tok[6]=='\"') continue;

    while (*p && *p != '\t' && *p != ' ') p++;
    if (*p) *p++=0;
    while (*p == '\t' || *p == ' ') p++;
    char *second_tok = p;

    if ((!strncmp(second_tok,"DIALOG",6) && isblankorz(second_tok[6])) ||
        (!strncmp(second_tok,"DIALOGEX",8) && isblankorz(second_tok[8]))||
        (!strncmp(second_tok,"MENU",4) && isblankorz(second_tok[4])))
    {
      if (sname[0])
      {
        fprintf(stderr,"got %s inside a block\n",second_tok);
        exit(1);
      }
      int sec = g_resdefs.Get(first_tok);
      if (!sec) 
      {
        fprintf(stderr, "unknown dialog %s\n",first_tok);
        exit(1);
      }
      sprintf(sname,"%s%s%s_%d",dirprefix?dirprefix:"",dirprefix?"_":"",second_tok[0] == 'M' ? "MENU" : "DLG",sec);
      section_descs.Insert(sname,strdup(first_tok));
    }
    else if (sname[0] && *second_tok && *first_tok)
    {
      if (*second_tok == '"')
      {
        int l = length_of_quoted_string(second_tok+1,true);
        if (l>0)
        {
          gotString(second_tok+1,l,sname);
          
          // OSX menu support: store a 2nd string w/o \tshortcuts, strip '&' too
          // note: relies on length_of_quoted_string() pre-conversion above
          if (depth && strstr(sname, "MENU_"))
          {
            int j=0;
            char* m=second_tok+1;
            for(;;)
            {
              if (!*m || (*m == '\\' && *(m+1)=='t') || (*m=='\"' && *(m-1)!='\\')) { buf[j]=0; break; }
              if (*m != '&') buf[j++] = *m;
              m++;
            }
            if (j!=l) gotString(buf,j,sname);
          }
        }
      }
    }
    else if (!strcmp(first_tok,"BEGIN")) 
    {
      depth++;
    }
    else if (!strcmp(first_tok,"END")) 
    {
      depth--;
      if (depth<0)
      {
        fprintf(stderr,"extra END\n");
        exit(1);
      }
      if (!depth) sname[0]=0;
    }

  }
  if (depth!=0)
  {
    fprintf(stderr,"missing some ENDs at end of rc file\n");
    exit(1);
  }
}

void processCPPfile(FILE *fp)
{
  char clocsec[512];
  clocsec[0]=0;
  for (;;)
  {
    char buf[8192];
    g_last_linecnt++;
    if (!fgets(buf,sizeof(buf),fp)) break;
    char *p = buf;
    while (*p) p++;
    while (p>buf && (p[-1] == '\r'|| p[-1] == '\n' || p[-1] == ' ')) p--;
    *p=0;
    char *commentp =strstr(buf,"//");
    p=buf;
    while (*p && (!commentp || p < commentp)) // ignore __LOCALIZE after //
    {
      int hm;
      if ((p==buf || (!isalnum(p[-1]) && p[-1] != '_')) && (hm=isLocalizeCall(p)))
      {
        while (*p != '(') p++;
        p++;
        while (isblank(*p)) p++;
        if (*p++ != '"')
        {
          fprintf(stderr,"Error: missing \" on '%s'\n",buf);
          exit(1);
        }
        int l = length_of_quoted_string(p,false);
        char *sp = p;
        p+=l+1;
        while (isblank(*p)) p++;
        if (*p++ != ',')
        {
          fprintf(stderr,"Error: missing , on '%s'\n",buf);
          exit(1);
        }
        while (isblank(*p)) p++;
        if (*p++ != '"')
        {
          fprintf(stderr,"Error: missing second \" on '%s'\n",buf);
          exit(1);
        }
        int l2 = length_of_quoted_string(p,false);

        if (hm == HACK_WILDCARD_ENTRY)
        {
          gotString(p,l2,"render_wildcard");
          p += l2;
        }
        else
        {
          char sec[512];
          memcpy(sec,p,l2);
          sec[l2]=0;
          p+=l2;
          gotString(sp,l,sec);
        }
      }
      p++;
    }

    if (clocsec[0])
    {
      p=buf;
      while (*p)
      {
	      if (*p == '"')
	      {
	        int l = length_of_quoted_string(p+1,false);
          if (l >= 7 && !strncmp(p+1,"MM_CTX_",7))
          {
            // ignore MM_CTX_* since these are internal strings
          }
          else
          {
  	        gotString(p+1,l,clocsec);
          }
	        p += l+2;
	      }
	      else
	      {
    	  if (*p == '\\') p++;
          if (*p == '/' && p[1] == '/') break; // comment terminates
    	  p++;
        }
      }
    }

    char *p2;
    if (commentp && (p2=strstr(commentp,"!WANT_LOCALIZE_STRINGS_BEGIN:"))) strcpy(clocsec,strstr(p2,":")+1);
    else if (commentp && (strstr(commentp,"!WANT_LOCALIZE_STRINGS_END"))) clocsec[0]=0;
  }
}



int main(int argc, char **argv)
{
  int x;
  int casemode=0;
  for (x=1;x<argc;x++)
  {
    if (argv[x][0] == '-')
    {
      if (!strcmp(argv[x],"--lower")) casemode = -1;
      else if (!strcmp(argv[x],"--leet")) casemode = 2;
      else if (!strcmp(argv[x],"--utf8test")) casemode = 4;
      else if (!strcmp(argv[x],"--upper")) casemode = 1;
      else if (!strcmp(argv[x],"--template")) casemode = 3;
      else
      {
        printf("Usage: build_sample_langpack [--leet|--lower|--upper|--template] file.rc file.cpp ...\n");
        exit(1);
      }
      continue;
    }
    FILE *fp = fopen(argv[x],"rb");
    if (!fp)
    {
      fprintf(stderr,"Error opening %s\n",argv[x]);
      return 1;
    }
    g_last_file = argv[x];
    g_last_linecnt=1;
    int alen =strlen(argv[x]);
    if (alen>3 && !stricmp(argv[x]+alen-3,".rc"))
    {
      WDL_String s(argv[x]);
      WDL_String dpre;
      char *p=s.Get();
      while (*p) p++;
      while (p>=s.Get() && *p != '\\' && *p != '/') p--;
      *++p=0;
      if (p>s.Get())
      {
        p-=2;
        while (p>=s.Get() && *p != '\\' && *p != '/') p--;
        dpre.Set(++p); // get dir name portion
        if (dpre.GetLength()) dpre.Get()[dpre.GetLength()-1]=0;
      }
      if (!strcmp(dpre.Get(),"jesusonic")) dpre.Set("jsfx");

      s.Append("resource.h");
      const char *err=getResourceDefinesFromHeader(s.Get());     
      if (err)
      {
        fprintf(stderr,"Error reading %s: %s\n",s.Get(),err);
        exit(1);
      }
      processRCfile(fp,dpre.Get()[0]?dpre.Get():NULL);
    }
    else 
    {
      processCPPfile(fp);
    }
      
    fclose(fp);
  }
  if (casemode==4) printf("\xef\xbb\xbf");
  printf("#NAME:%s\n",
       casemode==-1 ? "English (lower case, demo)" : 
       casemode==1 ? "English (upper case, demo)" : 
       casemode==2 ? "English (leet-speak, demo)" : 
       casemode==4 ? "UTF-8 English test (demo)" : 
       casemode==3 ? "Template (edit-me)" : 
       "English (sample language pack)");
  if (casemode==3) 
  {
    printf("; NOTE: this is the best starting point for making a new langpack.\n"
           "; As you translate a string, remove the ; from the beginning of the\n"
           "; line. If the line begins with ;^, then it is an optional string,\n"
           "; and you should only modify that line if the definition in [common]\n"
           "; is not accurate for that context.\n"
           "; You can enlarge windows using 5CA1E00000000000=scale, for example:\n"
           "; [DLG_218] ; IDD_LOCKSETTINGS\n"
           "; 5CA1E00000000000=1.2\n"
           "; This makes the above dialog 1.2x wider than default.\n\n");
  }
  
  WDL_StringKeyedArray<bool> common_found;
  {
    if (!translations_indexed.GetSize())
    {
      fprintf(stderr,"no sections!\n");
      exit(1);
    }
    else if (translations_indexed.GetSize() > 4096)
    {
      fprintf(stderr,"too many translation sections, check input or adjust code here\n");
      exit(1);
    }
    fprintf(stderr,"%d sections\n",translations_indexed.GetSize());

    int pos[4096]={0,};
    printf("[common]\n");
    WDL_FastString matchlist;
    WDL_AssocArray<WDL_UINT64, bool> ids(uint64cmpfunc); 
    int minpos = 0;
    for (;;)
    {
      int matchcnt=0;
      matchlist.Set("");
      const char *str=NULL;
      for(x=minpos;x<translations_indexed.GetSize();x++)
      {
        const char *secname;
        WDL_StringKeyedArray<bool> *l = translations_indexed.Enumerate(x,&secname);
	      int sz=l->GetSize();
        if (!str)
	      {
          if (x>minpos) 
          {
            memset(pos,0,sizeof(pos)); // start over
            minpos=x;
          }
          while (!str && pos[x]<sz)
          {
            l->Enumerate(pos[x]++,&str); 
            if (common_found.Get(str)) str=NULL; // skip if we've already analyzed this string
          }
          if (str) matchlist.Set(secname); 
        }
	      else 
        {
          while (pos[x] < sz)
	        {
      	    const char *tv=NULL;
	          l->Enumerate(pos[x],&tv);
      	    int c = strcmp(tv,str);
      	    if (c>0) break; 
      	    pos[x]++;
      	    if (!c)
	          {
              matchlist.Append(", ");
	            matchlist.Append(secname);
	            matchcnt++;
	            break;
	          }
	        }
        }
	    }
      if (matchcnt>0)
      {
        common_found.Insert(str,true);
        //printf("; used by: %s\n",matchlist.Get());
        if (casemode==3) printf(";");
        WDL_UINT64 a = outputLine(str,casemode);
        if (ids.Get(a))
        {
          fprintf(stderr,"duplicate hash for strings in common section, hope this is OK.. 64 bit hash fail!\n");
          exit(1);
        }
//        printf("\n");
        ids.Insert(a,true);
      }
      if (minpos == x-1) break;
    }
    printf("\n");
  }

  for(x=0;x<translations.GetSize();x++)
  {
    const char *nm=NULL;
    WDL_PtrList<char> *p = translations.Enumerate(x,&nm);
    if (x) printf("\n");
    char *secinfo = section_descs.Get(nm);
    printf("[%s]%s%s\n",nm,secinfo?" ; ":"", secinfo?secinfo:"");
    int a;
    int y;
    WDL_AssocArray<WDL_UINT64, bool> ids(uint64cmpfunc); 
    for (a=0;a<2;a++)
    {
      for (y=0;y<p->GetSize();y++)
      {
        char *strv=p->Get(y);
        if ((common_found.Get(strv)?1:0) != a) continue;

        if (a) printf(";^");
        else if (casemode==3) printf(";");

        WDL_UINT64 a = outputLine(strv,casemode);
        if (ids.Get(a))
        {
          fprintf(stderr,"duplicate hash for strings in section, hope this is OK.. 64 bit hash fail!\n");
          exit(1);
        }
        ids.Insert(a,true);
      }
    }
  }
  return 0;
}
