#include "swell.h"


#ifndef SWELL_PROVIDED_BY_APP

// only add this file to your project if you are an application that wishes to publish the SWELL API to its modules/plugins
// the modules should be compiled using SWELL_PROVIDED_BY_APP and include swell-modstub.mm

#undef _WDL_SWELL_H_API_DEFINED_
#undef SWELL_API_DEFINE
#define SWELL_API_DEFINE(ret, func, parms) {#func, (void *)func },
static struct api_ent
{
  const char *name;
  void *func;
}
api_table[]=
{
#include "swell.h"
};

static int compfunc(const void *a, const void *b)
{
  return strcmp(((struct api_ent*)a)->name,((struct api_ent*)b)->name);
}

extern "C" {
__attribute__ ((visibility ("default"))) void *SWELLAPI_GetFunc(const char *name)
{
  if (!name) return (void *)0x100; // version
  static int a; 
  if (!a)
  { 
    a=1;
    qsort(api_table,sizeof(api_table)/sizeof(api_table[0]),sizeof(api_table[0]),compfunc);
  }
  struct api_ent find={name,NULL};
  struct api_ent *res=(struct api_ent *)bsearch(&find,api_table,sizeof(api_table)/sizeof(api_table[0]),sizeof(api_table[0]),compfunc);
  if (res) return res->func;
  return NULL;
}
};

#endif
