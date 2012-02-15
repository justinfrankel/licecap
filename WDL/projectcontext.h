#ifndef _PROJECTCONTEXT_H_
#define _PROJECTCONTEXT_H_

#include "wdltypes.h"

class WDL_String;
class WDL_FastString;
class WDL_HeapBuf;

#ifndef _REAPER_PLUGIN_PROJECTSTATECONTEXT_DEFINED_
#define _WDL_PROJECTSTATECONTEXT_DEFINED_

class ProjectStateContext // this is also defined in reaper_plugin.h (keep them identical, thx)
{
public:
  virtual ~ProjectStateContext(){};

  virtual void WDL_VARARG_WARN(printf,2,3) AddLine(const char *fmt, ...) = 0;
  virtual int GetLine(char *buf, int buflen)=0; // returns -1 on eof

  virtual WDL_INT64 GetOutputSize()=0;

  virtual int GetTempFlag()=0;
  virtual void SetTempFlag(int flag)=0;
};

#endif

ProjectStateContext *ProjectCreateFileRead(const char *fn);
ProjectStateContext *ProjectCreateFileWrite(const char *fn);
ProjectStateContext *ProjectCreateMemCtx(WDL_HeapBuf *hb); // read or write, be sure to delete it before accessing hb



// helper functions
class LineParser;
bool ProjectContext_EatCurrentBlock(ProjectStateContext *ctx); // returns TRUE if got valid >, otherwise it means eof...
bool ProjectContext_GetNextLine(ProjectStateContext *ctx, LineParser *lpOut); // true if lpOut is valid


int cfg_decode_binary(ProjectStateContext *ctx, WDL_HeapBuf *hb); // 0 on success, doesnt clear hb
void cfg_encode_binary(ProjectStateContext *ctx, const void *ptr, int len);

int cfg_decode_textblock(ProjectStateContext *ctx, WDL_String *str); // 0 on success, appends to str
int cfg_decode_textblock(ProjectStateContext *ctx, WDL_FastString *str); // 0 on success, appends to str
void cfg_encode_textblock(ProjectStateContext *ctx, const char *text);

void makeEscapedConfigString(const char *in, WDL_String *out);

#endif//_PROJECTCONTEXT_H_
