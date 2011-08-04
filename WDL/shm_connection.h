#ifndef _WDL_SHM_CONNECTION_H_
#define _WDL_SHM_CONNECTION_H_

#ifdef _WIN32
#include <windows.h>
#else
#include "swell/swell.h"
#include "swell/swell-internal.h"
#endif

#include <time.h>

#include "wdlstring.h"
#include "wdltypes.h"
#include "queue.h"

class WDL_SHM_Connection
{
public:
  WDL_SHM_Connection(bool whichChan, // a true con connects to a false con -- note on SHM false should be created FIRST.
                     const char *uniquestring, // identify 
                     int shmsize=262144, // bytes, whoever opens first decides
                     int timeout_sec=0
                     );

  ~WDL_SHM_Connection();

  int Run(); // call as often as possible, returns <0 error, >0 if did something

  bool WantSendKeepAlive(); // called when it needs a keepalive to be sent (may be never, or whatever interval it decides)
  
 // wait for this if you want to see when data comes in
  HANDLE GetWaitEvent() 
  { 
    #ifdef _WIN32
    return m_events[m_whichChan]; 
    #else
    return m_waitevt;
    #endif
  }
  
  // receiving and sending data
  WDL_Queue recv_queue;
  WDL_Queue send_queue;


private:


  int m_timeout_sec;
  time_t m_last_recvt;
  int m_timeout_cnt;

  WDL_String m_tempfn;

#ifdef _WIN32

  // todo: abstract for OS X?
  HANDLE m_file, m_filemap; 
  HANDLE m_events[2]; // [m_whichChan] set when the other side did something useful

  unsigned char *m_mem; 
  int m_whichChan; // which channel we read from
#else
  time_t m_next_keepalive;
  
  int m_sockbufsize;

  int m_rdbufsize;
  char *m_rdbuf; // m_rdbufsize

  int m_listen_socket;
  int m_socket;

  bool m_isConnected;

  HANDLE m_waitevt;
  void *m_sockaddr;
#endif

};

#endif
