#ifndef _WDL_SHM_CONNECTION_H_
#define _WDL_SHM_CONNECTION_H_

#include "wdlstring.h"
#include "queue.h"

class WDL_SHM_Connection
{
public:
  WDL_SHM_Connection(bool whichChan, // a true con connects to a false con
                     const char *uniquestring, // identify 
                     int shmsize=262144, // bytes, whoever opens first decides
                     int timeout_sec=0
                     );

  ~WDL_SHM_Connection();

  int Run(); // call as often as possible, returns <0 error, >0 if did something
  HANDLE GetWaitEvent() { return m_events[m_whichChan]; } // wait for this if you want to see when data comes in
  
  // receiving and sending data
  WDL_Queue recv_queue;
  WDL_Queue send_queue;


private:

  WDL_String m_tempfn;

  int m_timeout_sec;
  time_t m_last_recvt;

  // todo: abstract for OS X?
  HANDLE m_file, m_filemap; 
  HANDLE m_events[2]; // [m_whichChan] set when the other side did something useful

  unsigned char *m_mem; 
  int m_whichChan; // which channel we read from

};

#endif