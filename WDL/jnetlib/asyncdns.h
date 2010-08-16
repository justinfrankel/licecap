/*
** JNetLib
** Copyright (C) 2000-2001 Nullsoft, Inc.
** Author: Justin Frankel
** File: asyncdns.h - JNL portable asynchronous DNS interface
** License: see jnetlib.h
**
** Usage:
**   1. Create JNL_AsyncDNS object, optionally with the number of cache entries.
**   2. call resolve() to resolve a hostname into an address. The return value of 
**      resolve is 0 on success (host successfully resolved), 1 on wait (meaning
**      try calling resolve() with the same hostname in a few hundred milliseconds 
**      or so), or -1 on error (i.e. the host can't resolve).
**   3. call reverse() to do reverse dns (ala resolve()).
**   4. enjoy.
*/

#ifndef _ASYNCDNS_H_
#define _ASYNCDNS_H_

class JNL_AsyncDNS
{
public:
  JNL_AsyncDNS(int max_cache_entries=64);
  ~JNL_AsyncDNS();

  int resolve(char *hostname, unsigned long *addr); // return 0 on success, 1 on wait, -1 on unresolvable
  int reverse(unsigned long addr, char *hostname); // return 0 on success, 1 on wait, -1 on unresolvable. hostname must be at least 256 bytes.

private:
  typedef struct 
  {
    int last_used; // timestamp.
    char resolved;
    char mode; // 1=reverse
    char hostname[256];
    unsigned long addr;
  } 
  cache_entry;

  cache_entry *m_cache;
  int m_cache_size;
  volatile int m_thread_kill;
#ifdef _WIN32
  HANDLE m_thread;
  static unsigned long WINAPI _threadfunc(LPVOID _d);
#else
  pthread_t m_thread;
  static unsigned int _threadfunc(void *_d);
#endif
  void makesurethreadisrunning(void);

};

#endif //_ASYNCDNS_H_
