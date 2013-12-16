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


class sInst {
  public:
    enum 
    { 
      MAX_USER_STRINGS=16384, 
      STRING_INDEX_BASE=90000,
      MAX_FILE_HANDLES=512,
      FILE_HANDLE_INDEX_BASE=1000000
    };

    sInst() 
    {
      memset(m_handles,0,sizeof(m_handles));
      memset(m_rw_strings,0,sizeof(m_rw_strings));
      m_vm = NSEEL_VM_alloc();
      if (!m_vm) fprintf(stderr,"NSEEL_VM_alloc(): failed\n");
      NSEEL_VM_SetCustomFuncThis(m_vm,this);
    }

    ~sInst() 
    {
      m_code_freelist.Empty((void (*)(void *))NSEEL_code_free);
      if (m_vm) NSEEL_VM_free(m_vm);

      int x;
      for (x=0;x<MAX_USER_STRINGS;x++) delete m_rw_strings[x];
      for (x=0;x<MAX_FILE_HANDLES;x++) 
      {
        if (m_handles[x]) fclose(m_handles[x]); 
        m_handles[x]=0;
      }
      m_strings.Empty(true);
    }
    int runcode(const char *code, bool showerr, bool canfree);

    NSEEL_VMCTX m_vm;

    WDL_FastString *m_rw_strings[MAX_USER_STRINGS];
    WDL_PtrList<WDL_FastString> m_strings;
    WDL_StringKeyedArray<EEL_F *> m_namedvars;
    WDL_PtrList<void> m_code_freelist;
    WDL_FastString m_pc;

    static int varEnumProc(const char *name, EEL_F *val, void *ctx)
    {
      if (!((sInst *)ctx)->m_namedvars.Get(name)) ((sInst *)ctx)->m_namedvars.Insert(name,val);
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

    int AddString(const WDL_FastString &s)
    {
      WDL_FastString *ns = new WDL_FastString;
      *ns = s;
      m_strings.Add(ns);
      return m_strings.GetSize()-1+STRING_INDEX_BASE;
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

      WDL_FastString *s = m_strings.Get(idx - STRING_INDEX_BASE);
      if (isWriteableAs) *isWriteableAs=s;
      return s ? s->Get() : NULL;
    }

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

};

#define EEL_STRING_GETNAMEDVAR(x,y) ((sInst*)(opaque))->GetNamedVar(x,y)
#define EEL_STRING_GETFMTVAR(x) ((sInst*)(opaque))->GetVarForFormat(x)
#define EEL_STRING_GET_FOR_INDEX(x, wr) ((sInst*)(opaque))->GetStringForIndex(x, wr)
#define EEL_STRING_ADDTOTABLE(x)  ((sInst*)(opaque))->AddString(x)

#define EEL_STRING_DEBUGOUT writeToStandardError // no parameters, since it takes varargs
#define EEL_STRING_STDOUT_WRITE(x,len) { fwrite(x,len,1,stdout); fflush(stdout); }
#include "eel_strings.h"

#define EEL_FILE_OPEN(fn,mode) ((sInst*)opaque)->OpenFile(fn,mode)
#define EEL_FILE_GETFP(fp) ((sInst*)opaque)->GetFileFP(fp)
#define EEL_FILE_CLOSE(fpindex) ((sInst*)opaque)->CloseFile(fpindex)

#include "eel_files.h"

int sInst::runcode(const char *code, bool showerr, bool canfree)
{
  if (m_vm) 
  {
    m_pc.Set("");
    eel_preprocess_strings(this,m_pc,code);
    NSEEL_CODEHANDLE code = NSEEL_code_compile_ex(m_vm,m_pc.Get(),1,canfree ? 0 : NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
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
        NSEEL_VM_enumallvars(m_vm,varEnumProc, this);
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
    fp = strcmp(argv[argpos],"-") ? fopen(argv[argpos],"r") : stdin;
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

  WDL_FastString code,t;

  sInst inst;
  {
    const int argv_offs = 1<<22;
    code.SetFormatted(64,"argc=0; argv=%d;\n",argv_offs);
    int x;
    for (x=0;x<argc;x++)
    {
      if (x==0 || x >= argpos)
      {
        t.Set(argv[x]);
        code.AppendFormatted(64,"argv[argc]=%d; argc+=1;\n",inst.AddString(t));
      }
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
