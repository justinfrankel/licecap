#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "../wdltypes.h"
#include "../wdlstring.h"

#ifdef _WIN32
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

typedef struct compileContext {
  int l_stats[4];
} compileContext;
#define onCompileNewLine(a,b,c) {}
#include "tmp.c"

int ppOut(char *input)
{
      if (input[0])
      { 
 	compileContext ctx = { 0,};
        char *exp=preprocessCode(&ctx,input);
        if (!exp)
        {
           return -1;
        }
        int cc=0;
        while (*exp)
        {
          if (*exp != ' ' && *exp != '\t' && *exp != '\r' && *exp != '\n') {
 
            printf("%c",*exp);
            if (cc++ >= 60) { cc=0; printf("\n"); }
          }
          exp++;
        }
        printf("\n");
      }
  return 0;
}
int main(int argc, char **argv)
{
  if (argc != 2) { fprintf(stderr,"usage: preproc_test filename\n"); return 0; }
  FILE *fp =fopen(argv[1],"rb");
  if (!fp)  { fprintf(stderr,"error opening '%s'\n",argv[1]); return 1; }

  int insec=0;
  WDL_FastString cursec;
  for (;;)
  {
    char buf[4096];
    buf[0]=0;
    fgets(buf,sizeof(buf),fp);
    if (!buf[0]) break;
    char *p=buf;
    while (*p == ' ' || *p == '\t') p++;
    if (p[0] == '@') 
    {
      insec=1;
      if (ppOut((char*)cursec.Get())) { fprintf(stderr,"Error preprocessing %s!\n",argv[1]); return -1; }
      cursec.Set("");
    }
    else if (insec)
    {
      cursec.Append(buf);
      continue;
    }
    printf("%s",buf);
  }
  if (ppOut((char*)cursec.Get())) { fprintf(stderr,"Error preprocessing %s!\n",argv[1]); return -1; }

  fclose(fp);
  return 0;
}
