#ifndef _LOG_
#define _LOG_

#include "stdarg.h"
#include "Containers.h"

#if defined _WIN32
  #define SYS_THREAD_ID (int) GetCurrentThreadId()
#elif defined __APPLE__
  #define SYS_THREAD_ID (int) pthread_self()
#else 
  #error "No OS defined!"
#endif

#if defined TRACER_BUILD
  #define TRACE Trace(TRACELOC, "");
#else
  #define TRACE
#endif

#define TRACELOC __FUNCTION__,__LINE__
void Trace(const char* funcName, int line, const char* fmtStr, ...);

// To trace some arbitrary data:                 Trace(TRACELOC, "%s:%d", myStr, myInt);
// To simply create a trace entry in the log:    TRACE;
// No need to wrap tracer calls in #ifdef TRACER_BUILD because Trace is a no-op unless TRACER_BUILD is defined.

const char* VSTOpcodeStr(int opCode);
const char* AUSelectStr(int select);
const char* AUPropertyStr(int propID);
const char* AUScopeStr(int scope);

struct Timer
{
	int mT;
	Timer();

	// Returns true every sec seconds.
	bool Every(double sec);
};

// Not yet ported to WDL.
// Snarf the whole file into a StrVector.
//StrVector ReadFileIntoStr(WDL_String* pFileName);
//int WriteBufferToFile(const DBuffer& buf, WDL_String* pFileName);
// Split line into fields.  Whitespace is treated as contiguous.
//StrVector SplitStr(WDL_String* pFileName, char separator = ' ');
void ToLower(char* cDest, const char* cSrc);

const char* CurrentTime();
void CompileTimestamp(const char* Mmm_dd_yyyy, const char* hh_mm_ss, WDL_String* pStr);
const char* AppendTimestamp(const char* Mmm_dd_yyyy, const char* hh_mm_ss, const char* cStr);
#define APPEND_TIMESTAMP(str) AppendTimestamp(__DATE__, __TIME__, str)

#endif

