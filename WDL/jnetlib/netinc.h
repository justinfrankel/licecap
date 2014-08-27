/*
** JNetLib
** Copyright (C) 2000-2001 Nullsoft, Inc.
** Author: Justin Frankel
** File: netinc.h - network includes and portability defines (used internally)
** License: see jnetlib.h
*/

#ifndef _NETINC_H_
#define _NETINC_H_

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <time.h>
#define ERRNO (WSAGetLastError())
#define SET_SOCK_BLOCK(s,block) { unsigned long __i=block?0:1; ioctlsocket(s,FIONBIO,&__i); }
#ifndef EWOULDBLOCK
  #define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
  #define EINPROGRESS WSAEWOULDBLOCK
#endif
typedef int socklen_t;

#else

#ifndef THREAD_SAFE
#define THREAD_SAFE
#endif
#ifndef _REENTRANT
#define _REENTRANT
#endif
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#define ERRNO errno
#define closesocket(s) close(s)
#define SET_SOCK_BLOCK(s,block) { int __flags; if ((__flags = fcntl(s, F_GETFL, 0)) != -1) { if (!block) __flags |= O_NONBLOCK; else __flags &= ~O_NONBLOCK; fcntl(s, F_SETFL, __flags);  } }
typedef int SOCKET;
#define INVALID_SOCKET (-1)

#ifndef stricmp
#define stricmp(x,y) strcasecmp(x,y)
#endif
#ifndef strnicmp
#define strnicmp(x,y,z) strncasecmp(x,y,z)
#endif

#endif // !_WIN32

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifndef INADDR_ANY
#define INADDR_ANY 0
#endif

#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#endif //_NETINC_H_
