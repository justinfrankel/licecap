#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "../wdltypes.h"
#include "../ptrlist.h"
#include "../wdlstring.h"
#include "../assocarray.h"
#include "ns-eel.h"

// todo for fun:
// add argc/argv support
// add time(), gets(), sleep()

// eel_files.h:
// add fopen(), fgets(), fgetc(), fseek(), ftell(), and fclose() 

int g_verbose;

static void writeToStandardError(const char *fmt, ...)
{
  va_list arglist;
  va_start(arglist, fmt);
  vfprintf(stderr,fmt,arglist);
  fprintf(stderr,"\n");
  fflush(stderr);
  va_end(arglist);
}


class sInst {
  public:
    enum { MAX_USER_STRINGS=16384, STRING_INDEX_BASE=90000 };

    sInst(const char *code);

    ~sInst() 
    { 
      if (m_code) NSEEL_code_free(m_code);
      if (m_vm) NSEEL_VM_free(m_vm);

      int x;
      for (x=0;x<MAX_USER_STRINGS;x++) delete m_rw_strings[x];
      m_strings.Empty(true,free);
    }

    NSEEL_VMCTX m_vm;
    NSEEL_CODEHANDLE m_code; // init, timer, message code, oscmsg code

    WDL_FastString *m_rw_strings[MAX_USER_STRINGS];
    WDL_PtrList<char> m_strings;
    WDL_StringKeyedArray<EEL_F *> m_namedvars;

    static int varEnumProc(const char *name, EEL_F *val, void *ctx)
    {
      ((sInst *)ctx)->m_namedvars.AddUnsorted(name,val);
      return 1;
    }

    EEL_F *GetNamedVar(const char *s, bool createIfNotExists)
    {
      if (!*s) return NULL;
      EEL_F *r = m_namedvars.Get(s);
      if (r || !createIfNotExists) return r;
      r=NSEEL_VM_regvar(m_vm,s);
      if (r) m_namedvars.Insert(s,r);
      return r;
    }
    EEL_F *GetVarForFormat(int formatidx) { return NULL; } // must use %{xyz}s syntax

    int AddString(const char *str)
    {
      const int n = m_strings.GetSize();
      int x;
      for (x=0;x<n && strcmp(m_strings.Get(x),str);x++);
      if (x==n) m_strings.Add(strdup(str));
      return x+STRING_INDEX_BASE;
    }

    const char *GetStringForIndex(EEL_F val, WDL_FastString **isWriteableAs=NULL)
    {
      int idx = (int) (val+0.5);
      if (idx>=0 && idx < MAX_USER_STRINGS)
      {
        if (isWriteableAs)
        {
          if (!m_rw_strings[idx]) m_rw_strings[idx] = new WDL_FastString;
          *isWriteableAs = m_rw_strings[idx];
        }
        return m_rw_strings[idx]?m_rw_strings[idx]->Get():"";
      }

      if (isWriteableAs) *isWriteableAs=NULL;

      return m_strings.Get(idx - STRING_INDEX_BASE);
    }


};

#define EEL_STRING_GETNAMEDVAR(x,y) ((sInst*)(opaque))->GetNamedVar(x,y)
#define EEL_STRING_GETFMTVAR(x) ((sInst*)(opaque))->GetVarForFormat(x)
#define EEL_STRING_GET_FOR_INDEX(x, wr) ((sInst*)(opaque))->GetStringForIndex(x, wr)
#define EEL_STRING_ADDTOTABLE(x)  ((sInst*)(opaque))->AddString(x.Get())

#define EEL_STRING_DEBUGOUT writeToStandardError // no parameters, since it takes varargs
#define EEL_STRING_STDOUT_WRITE(x) { printf("%s",x); fflush(stdout); }
#include "eel_strings.h"

sInst::sInst(const char *code)
{ 
  memset(m_rw_strings,0,sizeof(m_rw_strings));
  m_code = NULL;
  m_vm = NSEEL_VM_alloc();
  if (!m_vm) fprintf(stderr,"NSEEL_VM_alloc(): failed\n");
  NSEEL_VM_SetCustomFuncThis(m_vm,this);
  if (m_vm) 
  {
    WDL_FastString pc;
    eel_preprocess_strings(this,pc,code);
    m_code = NSEEL_code_compile_ex(m_vm,pc.Get(),0,0);
    char *err;
    if (!m_code && (err=NSEEL_code_getcodeerror(m_vm)))
    {
      fprintf(stderr,"NSEEL_code_compile: %s\n",err);
    }
  }
  if (m_code)
  {
    m_namedvars.DeleteAll();
    NSEEL_VM_enumallvars(m_vm,varEnumProc, this);
    m_namedvars.Resort();
  }
}
    


void NSEEL_HOSTSTUB_EnterMutex() { }
void NSEEL_HOSTSTUB_LeaveMutex() { }

int main(int argc, char **argv)
{
  FILE *fp = stdin;
  int argpos = 1;
  while (argpos < argc && argv[argpos][0] == '-' && argv[argpos][1])
  {
    if (!strcmp(argv[argpos],"-v")) g_verbose++;
    else
    {
      printf("Usage: %s [-v] [scriptfile]\n",argv[0]);
      return -1;
    }
    argpos++;
  }
  if (argpos < argc)
  {
    fp = strcmp(argv[argpos],"-") ? fopen(argv[argpos],"r") : stdin;
    if (!fp)
    {
      printf("Error opening %s\n",argv[argpos]);
      return -1;
    }
    argpos++;
  }

  if (NSEEL_init())
  {
    printf("NSEEL_init(): error initializing\n");
    return -1;
  }
  EEL_string_register();

  WDL_FastString code;
  char line[4096];
  for (;;)
  {
    line[0]=0;
    fgets(line,sizeof(line),fp);
    if (!line[0]) break;
    code.Append(line);
  }
  if (fp != stdin) fclose(fp);

  sInst inst(code.Get());
  if (inst.m_code)
  {
    const int argv_offs = 1<<22;
    WDL_FastString fs,t;
    fs.SetFormatted(64,"argc=0; argv=%d;\n",argv_offs);
    int x;
    for (x=0;x<argc;x++)
    {
      if (x==0 || x >= argpos)
      {
        t.Set(argv[x]);
        fs.AppendFormatted(64,"argv[argc]=%d; argc+=1;\n",inst.AddString(t));
      }
    }
    NSEEL_CODEHANDLE code=NSEEL_code_compile(inst.m_vm,fs.Get(),0);
    if (code)
    {
      NSEEL_code_execute(code);
      NSEEL_code_free(code);
    }

    NSEEL_code_execute(inst.m_code);
  }

  return 0;
}
