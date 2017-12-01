#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "../wdlcstring.h"
#include "../wdlstring.h"

#include "ns-eel-int.h"

void NSEEL_HOSTSTUB_EnterMutex() { }
void NSEEL_HOSTSTUB_LeaveMutex() { }


int main(int argc, char **argv)
{
  NSEEL_VMCTX vm = NSEEL_VM_alloc();
  WDL_FastString code;
  int pos=1;
  for (;;)
  {
    char line[4096];
    for (;;)
    {
      if (argc > 1) {
        if (pos >= argc) return 0;
        code.Append(argv[pos++]);
        code.Append("\n");
      } 
      else
      {
        if (!code.Get()[0]) printf("EEL> ");
        else printf("> ");
        fflush(stdout);
        line[0]=0;
        if (argc > 0)
        fgets(line,sizeof(line),stdin);
        if (!line[0]) break;
        code.Append(line);
      }
      while (line[0] && (
               line[strlen(line)-1] == '\r' ||
               line[strlen(line)-1] == '\n' ||
               line[strlen(line)-1] == '\t' ||
               line[strlen(line)-1] == ' '
              )) line[strlen(line)-1]=0;

      if (!strcmp(line,"quit")) break;
      if (!strcmp(line,"abort")) code.Set("");

      NSEEL_CODEHANDLE ch = NSEEL_code_compile(vm,code.Get(),0);
      if (!ch) continue;
      codeHandleType *p = (codeHandleType*)ch;
      if (p->code) 
      {
        char buf[512];
        buf[0]=0;
        GetTempPath(sizeof(buf)-64,buf);
        lstrcatn(buf,"jsfx-out",sizeof(buf));
        FILE *fp = fopen(buf,"wb");
        if (fp)
        {
          fwrite(p->code,1,p->code_size,fp);
          fclose(fp);
          char buf2[2048];
          snprintf(buf2,sizeof(buf2),"disasm \"%s\"",buf);
          system(buf2);
        }
      }
      code.Set("");

      NSEEL_code_free(ch);
    }
  }

  return 0;
}
