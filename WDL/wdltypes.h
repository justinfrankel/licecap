#ifndef _WDLTYPES_
#define _WDLTYPES_

#ifdef _MSC_VER

typedef __int64 WDL_INT64;
typedef unsigned __int64 WDL_UINT64;

#else

typedef long long WDL_INT64;
typedef unsigned long long WDL_UINT64;

#endif

#endif