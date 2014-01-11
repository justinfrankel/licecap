#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "../wdltypes.h"
#include "../ptrlist.h"
#include "../wdlstring.h"
#include "../assocarray.h"
#include "ns-eel.h"

// todo for fun:
// add argc/argv support
// add time(), gets(), sleep()

int g_verbose, g_interactive;

static void writeToStandardError(const char *fmt, ...)
{
  va_list arglist;
  va_start(arglist, fmt);
  vfprintf(stderr,fmt,arglist);
  fprintf(stderr,"\n");
  fflush(stderr);
  va_end(arglist);
}


class eel_string_context_state;
class eel_net_state;

class sInst {
  public:
    enum 
    { 
      MAX_FILE_HANDLES=512,
      FILE_HANDLE_INDEX_BASE=1000000
    };

    sInst();
    ~sInst();

    int runcode(const char *code, bool showerr, bool canfree);

    NSEEL_VMCTX m_vm;

    WDL_PtrList<void> m_code_freelist;

    FILE *m_handles[MAX_FILE_HANDLES];
    EEL_F OpenFile(const char *fn, const char *mode)
    {
      if (!*fn || !*mode) return -1.0;
      if (!strcmp(fn,"stdin")) return 0;
      if (!strcmp(fn,"stdout")) return 1;
      if (!strcmp(fn,"stderr")) return 2;

      int x;
      for (x=0;x<MAX_FILE_HANDLES && m_handles[x];x++);
      if (x>= MAX_FILE_HANDLES) return -1.0;
      FILE *fp = fopen(fn,mode);
      if (!fp) return -1.0;
      m_handles[x]=fp;
      return x + FILE_HANDLE_INDEX_BASE;
    }
    EEL_F CloseFile(int fp_idx)
    {
      fp_idx-=FILE_HANDLE_INDEX_BASE;
      if (fp_idx>=0 && fp_idx<MAX_FILE_HANDLES && m_handles[fp_idx])
      {
        fclose(m_handles[fp_idx]);
        m_handles[fp_idx]=0;
        return 0.0;
      }
      return -1.0;
    }
    FILE *GetFileFP(int fp_idx)
    {
      if (fp_idx==0) return stdin;
      if (fp_idx==1) return stdout;
      if (fp_idx==2) return stderr;
      fp_idx-=FILE_HANDLE_INDEX_BASE;
      if (fp_idx>=0 && fp_idx<MAX_FILE_HANDLES) return m_handles[fp_idx];
      return NULL;
    }

    eel_string_context_state *m_string_context;
    eel_net_state *m_net_state;
};

//#define EEL_STRINGS_MUTABLE_LITERALS
//#define EEL_STRING_WANT_MUTEX

#define EEL_STRING_MAX_USER_STRINGS 32768
#define EEL_STRING_LITERAL_BASE 2000000

#define EEL_STRING_GET_CONTEXT_POINTER(opaque) (((sInst *)opaque)->m_string_context)
#define EEL_STRING_DEBUGOUT writeToStandardError // no parameters, since it takes varargs
#define EEL_STRING_STDOUT_WRITE(x,len) { fwrite(x,len,1,stdout); fflush(stdout); }
#include "eel_strings.h"

#define EEL_FILE_OPEN(fn,mode) ((sInst*)opaque)->OpenFile(fn,mode)
#define EEL_FILE_GETFP(fp) ((sInst*)opaque)->GetFileFP(fp)
#define EEL_FILE_CLOSE(fpindex) ((sInst*)opaque)->CloseFile(fpindex)

#define EEL_NET_GET_CONTEXT(opaque) (((sInst *)opaque)->m_net_state)

#include "eel_files.h"

#include "eel_fft.h"

#include "eel_mdct.h"

#include "eel_misc.h"

#include "eel_net.h"

sInst::sInst()
{
  memset(m_handles,0,sizeof(m_handles));
  m_vm = NSEEL_VM_alloc();
  if (!m_vm) fprintf(stderr,"NSEEL_VM_alloc(): failed\n");
  NSEEL_VM_SetCustomFuncThis(m_vm,this);

  m_string_context = new eel_string_context_state;
  eel_string_initvm(m_vm);
  m_net_state = new eel_net_state(4096,NULL);
}

sInst::~sInst() 
{
  m_code_freelist.Empty((void (*)(void *))NSEEL_code_free);
  if (m_vm) NSEEL_VM_free(m_vm);

  int x;
  for (x=0;x<MAX_FILE_HANDLES;x++) 
  {
    if (m_handles[x]) fclose(m_handles[x]); 
    m_handles[x]=0;
  }
  delete m_string_context;
  delete m_net_state;
}

int sInst::runcode(const char *codeptr, bool showerr, bool canfree)
{
  if (m_vm) 
  {
    NSEEL_CODEHANDLE code = NSEEL_code_compile_ex(m_vm,codeptr,1,canfree ? 0 : NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
    if (code) m_string_context->update_named_vars(m_vm);

    char *err;
    if (!code && (err=NSEEL_code_getcodeerror(m_vm)))
    {
      if (NSEEL_code_geterror_flag(m_vm)&1) return 1;
      if (showerr) fprintf(stderr,"NSEEL_code_compile: %s\n",err);
      return -1;
    }
    else
    {
      if (code)
      {
        NSEEL_code_execute(code);
        if (canfree) NSEEL_code_free(code);
        else m_code_freelist.Add((void*)code);
      }
      return 0;
    }
  }
  return -1;
}
    


void NSEEL_HOSTSTUB_EnterMutex() { }
void NSEEL_HOSTSTUB_LeaveMutex() { }


int main(int argc, char **argv)
{
  FILE *fp = stdin;
  int argpos = 1;
  const char *scriptfn = argv[0];
  while (argpos < argc && argv[argpos][0] == '-' && argv[argpos][1])
  {
    if (!strcmp(argv[argpos],"-v")) g_verbose++;
    else if (!strcmp(argv[argpos],"-i")) g_interactive++;
    else
    {
      printf("Usage: %s [-v] [-i | scriptfile | -]\n",argv[0]);
      return -1;
    }
    argpos++;
  }
  if (argpos < argc && !g_interactive)
  {
    fp = strcmp(argv[argpos],"-") ? fopen(scriptfn = argv[argpos],"r") : stdin;
    if (!fp)
    {
      printf("Error opening %s\n",argv[argpos]);
      return -1;
    }
    argpos++;
  }
  else
  {
#ifndef _WIN32
    if (!g_interactive && isatty(0)) 
#else
    if (1)
#endif
       g_interactive=1;
  }

  if (NSEEL_init())
  {
    printf("NSEEL_init(): error initializing\n");
    return -1;
  }
  EEL_string_register();
  EEL_file_register();
  EEL_fft_register();
  EEL_mdct_register();
  EEL_misc_register();
  EEL_tcp_register();
  EEL_tcp_initsocketlib();

  WDL_FastString code,t;

  sInst inst;
  {
    const int argv_offs = 1<<22;
    code.SetFormatted(64,"argc=0; argv=%d;\n",argv_offs);
    int x;
    for (x=argpos-1;x<argc;x++)
    {
      code.AppendFormatted(64,"argv[argc]=%d; argc+=1;\n",
          inst.m_string_context->AddString(new WDL_FastString(x<argpos ? scriptfn : argv[x])));
    }
    inst.runcode(code.Get(),true,true);
  }

  if (g_interactive)
  {
    printf("EEL interactive mode, type quit to quit, abort to abort multiline entry\n");
    EEL_F *resultVar = NSEEL_VM_regvar(inst.m_vm,"__result");
    code.Set("");
    char line[4096];
    for (;;)
    {
      if (!code.Get()[0]) printf("EEL> ");
      else printf("> ");
      fflush(stdout);
      line[0]=0;
      fgets(line,sizeof(line),fp);
      if (!line[0]) break;
      code.Append(line);
      while (line[0] && (
               line[strlen(line)-1] == '\r' ||
               line[strlen(line)-1] == '\n' ||
               line[strlen(line)-1] == '\t' ||
               line[strlen(line)-1] == ' '
              )) line[strlen(line)-1]=0;

      if (!strcmp(line,"quit")) break;
      if (!strcmp(line,"abort")) code.Set("");

      t.Set("__result = (");
      t.Append(code.Get());
      t.Append(");");
      int res=inst.runcode(t.Get(),false,true);
      if (!res)
      {
        if (resultVar) printf("=%g ",*resultVar);
        code.Set("");
      }
      else
      {
        res=inst.runcode(code.Get(),true, false);
        if (res<=0) code.Set("");
        // res>0 means need more lines
      }
    }
  }
  else
  {
    code.Set("");
    char line[4096];
    for (;;)
    {
      line[0]=0;
      fgets(line,sizeof(line),fp);
      if (!line[0]) break;
      code.Append(line);
    }
    if (fp != stdin) fclose(fp);

    inst.runcode(code.Get(),true,true);
    
  }

  return 0;
}
