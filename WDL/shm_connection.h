#ifndef _WDL_SHM_CONNECTION_H_
#define _WDL_SHM_CONNECTION_H_

#include "wdlstring.h"
#include "queue.h"

#ifndef WDL_SHM_IO_BUFFER_SIZE // should never be smaller than 1024
#define WDL_SHM_IO_BUFFER_SIZE 32768
#endif

class WDL_SHM_Connection
{
public:
  WDL_SHM_Connection(const char *uniquestring, int rdIndex/*0 or 1*/);
  ~WDL_SHM_Connection();

  int Status(); // 0 on OK, <0 on error, >0 on disconnected pipe

  bool Send(void *buf, int len, int maxWait=10); // false if timed out

  // returns length, or 0 if no message available.
  // note returned buffer is valid only until next Recv
  int Recv(void **buf, bool remove=true, int maxWait=0); 



private:

  WDL_String m_tempfn;
  HANDLE m_file, m_filemap;
  HANDLE m_events[2]; // 0 = send, 1=recv

  DWORD m_mapsize;

  unsigned char *m_mem; 
  // map of buffer:
  // 2 bytes represent connection status (each)
  // rest divided in half for send/recv
  // first 4 bytes represent "how many bytes in message valid", zeroed to clear

  int m_rdIndex; // read/write

  WDL_Queue m_recvmsg;
  int m_recvmsg_curseq;
};

#endif