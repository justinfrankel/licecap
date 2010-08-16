#define HAVE_PROTOTYPES
#define HAVE_STDLIB_H
#ifndef _WIN32
#define NEED_SYS_TYPES_H
#define HAVE_STDDEF_H
#else
#include <windows.h>
#define XMD_H
#undef FAR
#define HAVE_BOOLEAN
typedef short INT16;
#endif

