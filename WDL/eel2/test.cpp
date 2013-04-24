#include "ns-eel.h"
#include <string.h>

void NSEEL_HOSTSTUB_EnterMutex()
{
}
void NSEEL_HOSTSTUB_LeaveMutex()
{
}

int main()
{
  printf("init\n");
  if (NSEEL_init())
  {
    printf("Error initializing EEL\n");
    return -1;
  }

  printf("alloc\n");
  NSEEL_VMCTX vm = NSEEL_VM_alloc();
  if (!vm) 
  {
    printf("Error creating VM\n");
    return -1;
  }

  printf("reg\n");
  double *var_ret = NSEEL_VM_regvar(vm,"ret");

  if (var_ret) *var_ret=1.0;

  printf("compile\n");
  char buf[1024];
  gets(buf);
  
  
  // note that you shouldnt pass a readonly string directly, since it may need to 
  // fudge with the string during the compilation (it will always restore it to the 
  // original value though).
  NSEEL_CODEHANDLE ch = NSEEL_code_compile(vm,buf,0);

  if (ch)
  {
    int n;
    for (n = 0; n < 10; n ++)
    {
      NSEEL_code_execute(ch);
      printf("pass(%d), ret=%f\n",n+1,var_ret ? *var_ret : 0.0);
    }

    NSEEL_code_free(ch);
  }
  else 
  {
    char *err = NSEEL_code_getcodeerror(vm);

    printf("Error compiling code at '%s'\n",err?err:"????");
  }

  NSEEL_VM_free(vm);


  return 0;
}