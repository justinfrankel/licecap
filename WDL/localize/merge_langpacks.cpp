#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#define stricmp strcasecmp
#include <ctype.h>
#endif

#include "../assocarray.h"
#include "../wdlstring.h"
#include "../ptrlist.h"
#include "../wdlcstring.h"




// call at start of file with format_flag=-1
// expect possible blank (\r or \n) lines, too, ignore them
static void fgets_as_utf8(char *linebuf, int linebuf_size, FILE *fp, int *format_flag) 
{
  // format flag: 0=utf8, 1=utf16le, 2=utf16be, 3=ansi
  linebuf[0]=0;
  if (*format_flag>0)
  {
    int sz=0;
    while (sz < linebuf_size-8)
    {
      int a = fgetc(fp);
      int b = *format_flag==3 ? 0 : fgetc(fp);
      if (a<0 || b<0) break;
      if (*format_flag==2) a = (a<<8)+b;
      else a += b<<8;

      
    again:
      if (a >= 0xD800 && a < 0xDC00) // UTF-16 supplementary planes
      {
        int aa = fgetc(fp);
        int bb = fgetc(fp);
        if (aa < 0 || bb < 0) break;

        if (*format_flag==2) aa = (aa<<8)+bb;
        else aa += bb<<8;

        if (aa >= 0xDC00 && aa < 0xE000) 
        {
          a = 0x10000 + ((a&0x3FF)<<10) + (aa&0x3FF);
        }
        else
        {
          a=aa;
          goto again;
        }
      }


      if (a < 0x80) linebuf[sz++] = a;
      else 
      {
        if (a<0x800)  // 11 bits (5+6 bits)
        {
          linebuf[sz++] = 0xC0 + ((a>>6)&31);
        }
        else 
        {
          if (a < 0x10000) // 16 bits (4+6+6 bits)
          {
            linebuf[sz++] = 0xE0 + ((a>>12)&15); // 4 bits
          }
          else // 21 bits yow
          {
            linebuf[sz++] = 0xF0 + ((a>>18)&7); // 3 bits
            linebuf[sz++] = 0x80 + ((a>>12)&63); // 6 bits
          }
          linebuf[sz++] = 0x80 + ((a>>6)&63); // 6 bits
        }
        linebuf[sz++] = 0x80 + (a&63); // 6 bits
      }

      if (a == '\n') break;
    }
    linebuf[sz]=0;      
  }
  else
  {
    fgets(linebuf,linebuf_size,fp);
  }

  if (linebuf[0] && *format_flag<0)
  {
    unsigned char *p=(unsigned char *)linebuf;
    if (p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBf)
    {
      *format_flag=0;
      memmove(linebuf,linebuf+3,strlen(linebuf+3)+1); // skip UTF-8 BOM
    }
    else if ((p[0] == 0xFF && p[1] == 0xFE) || (p[0] == 0xFE && p[1] == 0xFF))
    {
      fseek(fp,2,SEEK_SET);
      *format_flag=p[0] == 0xFE ? 2 : 1;
      fgets_as_utf8(linebuf,linebuf_size,fp,format_flag);
      return;
    }
    else 
    {
      for (;;)
      {
        const unsigned char *str=(unsigned char *)linebuf;
        while (*str)
        {
          unsigned char c = *str++;
          if (c >= 0xC0)
          {
            if (c <= 0xDF && str[0] >=0x80 && str[0] <= 0xBF) str++;
            else if (c <= 0xEF && str[0] >=0x80 && str[0] <= 0xBF && str[1] >=0x80 && str[1] <= 0xBF) str+=2;
            else if (c <= 0xF7 && 
                     str[0] >=0x80 && str[0] <= 0xBF && 
                     str[1] >=0x80 && str[1] <= 0xBF && 
                     str[2] >=0x80 && str[2] <= 0xBF) str+=3;
            else break;
          }
          else if (c >= 128) break;
        }
        if (*str) break;

        linebuf[0]=0;
        fgets(linebuf,linebuf_size,fp);
        if (!linebuf[0]) break;
      }
      *format_flag = linebuf[0] ? 3 : 0; // if scanned the whole file without an invalid UTF8 pair, then UTF-8 (0), otherwise ansi (3)
      fseek(fp,0,SEEK_SET);
      fgets_as_utf8(linebuf,linebuf_size,fp,format_flag);
      return;
    }
  }
}


bool read_ini(const char *fn, WDL_StringKeyedArray< WDL_StringKeyedArray<char *> * > *sections, WDL_String *mod_name, int options)
{
  FILE *fp = fopen(fn,"r");
  if (!fp) return false;

  WDL_StringKeyedArray<char *> *cursec=NULL;

  bool warn=false;
  int char_fmt=-1;
  char linebuf[16384];
  int linecnt=0;
  for (;;)
  {
    linebuf[0]=0;
    fgets_as_utf8(linebuf,sizeof(linebuf),fp,&char_fmt);
    if (!linebuf[0]) break;
    if (char_fmt==3 && !warn)
    {
      fprintf(stderr,"Warning: '%s' was not saved with UTF-8 or UTF-16 encoding, the merged Langpack might contain garbled characters.\n", fn);
      warn=true;
    }
  
    char *p=linebuf;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    char *lbstart = p;
    while (*p) p++;
    p--;
    while (p >= lbstart)
    {
      if (options&1) { if (*p == '\n' || *p == '\r') p--; else break; }
      else { if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p--; else break; }
    }
    p++;
    *p=0;

    if (!linecnt && !strncmp(lbstart,"#NAME:",6))
    {
      mod_name->Set(lbstart+6);
    }

    linecnt++;

    if (!*lbstart || *lbstart == '#') continue;

    if (*lbstart == '[') 
    {
      if (cursec) cursec->Resort();

      lbstart++;
      {
        char *tmp = lbstart;
        while (*tmp && *tmp != ']') tmp++;
        *tmp=0;
      }

      cursec = sections->Get(lbstart);
      if (!cursec)
      {
        cursec = new WDL_StringKeyedArray<char *>(false,WDL_StringKeyedArray<char *>::freecharptr);
        sections->Insert(lbstart,cursec);
      }
    }
    else if (cursec) 
    {
      char *eq = strstr(lbstart,"=");
      if (eq)
      {
        *eq++ = 0;
        cursec->AddUnsorted(lbstart,strdup(eq));
      }
    }
  }
  if (cursec)
    cursec->Resort();

  fclose(fp);
  return true;
}


char *GotKey(WDL_StringKeyedArray<char *> *cursec_modptr, WDL_StringKeyedArray<bool> *cursec_hadvals, char *key) // key[19], can be mangled
{
  char *match=NULL;
  if (cursec_modptr)
  {
    int start_idx=2;
    match = cursec_modptr->Get(key+start_idx);
    if (!match)
    {
      start_idx=1;
      key[1]=';';
      match = cursec_modptr->Get(key+start_idx);
    }
    if (!match)
    {
      start_idx=0;
      key[0]=';';
      key[1]='^';
      match = cursec_modptr->Get(key+start_idx);
    }
    if (match)
    {
      printf("%s=%s\n",key+start_idx,match);
      cursec_hadvals->Insert(key+2,true);
    }
  }
  return match;
}





int main(int argc, char **argv)
{
  int i;
  int options = 0;
  for (i=3; i<argc; i++)
  {
    if (!strcmp(argv[i],"-p")) options|=1;
    else if (!strcmp(argv[i],"-c")) options|=2;
    else { options=0; break; }
  }  
  if (argc<3 || (argc>3 && !options))
  {
    fprintf(stderr,"Usage: merge_langpacks new_template.txt my_language.txt [options] > new_language.txt\n");
    fprintf(stderr,"Options:\n");
    fprintf(stderr,"  -p: preserve trailing spaces/tabs\n");
    fprintf(stderr,"  -c: comment obsolete sections/strings\n");
    return 1;
  }
  FILE *reffp = fopen(argv[1],"r");
  if (!reffp)
  {
    fprintf(stderr,"Error reading '%s', invalid template langpack.\n",argv[1]);
    return 1;
  }
  WDL_StringKeyedArray<WDL_StringKeyedArray<char *> * > mods;
  WDL_String mod_name;
  if (!read_ini(argv[2],&mods,&mod_name,options)||!mods.GetSize())
  {
    fprintf(stderr,"Error reading '%s', invalid langpack.\n",argv[2]);
    return 1;
  }

  int char_fmt=-1;

  //printf("%c%c%c",0xEF,0xBB,0xBF); // UTF-8 BOM
  WDL_String cursec;
  WDL_StringKeyedArray<char *> *cursec_modptr=NULL;
  WDL_StringKeyedArray<bool> cursec_hadvals(false);
  WDL_PtrList<char> cursec_added;

  bool warn=false;
  char key[2+16+1];
  int line_count=0;
  for (;;)
  {
    char buf[16384];
    buf[0]=0;
    fgets_as_utf8(buf,sizeof(buf),reffp,&char_fmt);
    if (char_fmt==3 && !warn)
    {
      fprintf(stderr,"Warning: '%s' was not saved with UTF-8 or UTF-16 encoding, the merged Langpack might contain garbled characters.\n", argv[1]);
      warn=true;
    }

    bool wasZ = !buf[0];

    // remove any trailing newlines
    char *p=buf;
    while (*p) p++;
    while (p>buf && (p[-1] == '\r'||p[-1] == '\n')) p--;
    *p=0;

    // skip any leading space
    p=buf;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    line_count++;
    if (line_count==1 && mod_name.Get()[0])
    {
      printf("#NAME:%s\n",mod_name.Get());
      if (!strncmp(p,"#NAME:",6)) continue; // ignore template #NAME: line, use modded
    }

    char *np;
    if (wasZ || (p[0] == '[' && (np = strstr(p,"]")) && np > p+1)) 
    {
      if (cursec.Get()[0])
      {
        // finish current section
        if (cursec_added.GetSize())
        {
          printf("######## added in template (section: %s):\n",cursec.Get());
          int x;
          for(x=0;x<cursec_added.GetSize();x++)
          {
            printf("%s\n",cursec_added.Get(x));
          }
          printf("\n");
        }

        if (cursec_modptr)
        {
          // print any non-comment lines in section that were not in template
          int x;
          int oc=0;
          for (x=0;x<cursec_modptr->GetSize();x++)
          {
            const char *nm=NULL;
            const char *val= cursec_modptr->Enumerate(x,&nm);
            if (!nm || !val) break;

            if (nm[0] == ';') continue;

            if (!cursec_hadvals.Get(nm))
            {
              if (!oc++) printf("######## not in template, possibly obsolete (section: %s):\n",cursec.Get());
              printf("%s%s=%s\n", (options&2) ? ";" : "", nm, val);
            }
          }
          if (oc) printf("\n");

        }

        mods.Delete(cursec.Get());
        cursec_hadvals.DeleteAll();
        cursec_added.Empty(true,free);
        cursec_modptr=NULL;
      }

      if (wasZ) break;

      printf("%s\n",buf);
      cursec.Set(p+1, (int)(np-p-1));
      cursec_modptr = mods.Get(cursec.Get());

      // if any, preserve the scaling at the top of the section
      strcpy(key+2,"5CA1E00000000000");
      GotKey(cursec_modptr, &cursec_hadvals, key);
      // done, scaling lines are added by hand (not part of templates)

      continue;
    }


    if (!cursec.Get()[0]) printf("%s\n",buf);
    else
    {
      char *eq = strstr(p,"=");
      if (!eq) printf("%s\n",buf);
      else
      {
        if (p[0] == ';' && *++p == '^') p++;
        if (eq-p==16)
        {
          lstrcpyn_safe(key+2, p, 17);
          if (!GotKey(cursec_modptr, &cursec_hadvals, key))
          {
            cursec_added.Add(strdup(buf));
          }
        }
        else
        {
          printf("%s\n",buf);
        }
      }
    }
  }

  fclose(reffp);

  int x;
  for (x=0;x<mods.GetSize();x++)
  {
    const char *secname=NULL;
    cursec_modptr = mods.Enumerate(x,&secname);
    if (!cursec_modptr || !secname) break;
    printf("\n[%s]\n",secname);
    printf("######## possibly obsolete section!\n");
    int y;
    for (y=0;y<cursec_modptr->GetSize();y++)
    {
      const char *nm=NULL;
      const char *v=cursec_modptr->Enumerate(y,&nm);
      if (!nm || !v) break;
      printf("%s%s=%s\n", (options&2) && nm[0]!=';' ? ";" : "", nm, v);
    }
  }

  return 0;
}

