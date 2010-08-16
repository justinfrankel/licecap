/*
** JNetLib
** Copyright (C) 2000-2001 Nullsoft, Inc.
** Author: Justin Frankel
** File: listen.h - JNL interface for opening a TCP listen
** License: see jnetlib.h
**
** Usage:
**   1. create a JNL_Listen object with the port and (optionally) the interface
**      to listen on.
**   2. call get_connect() to get any new connections (optionally specifying what
**      buffer sizes the connection should be created with)
**   3. check is_error() to see if an error has occured
**   4. call port() if you forget what port the listener is on.
**
*/

#ifndef _LISTEN_H_
#define _LISTEN_H_
#include "connection.h"

class JNL_Listen
{
  public:
    JNL_Listen(short port, unsigned long which_interface=0);
    ~JNL_Listen();

    JNL_Connection *get_connect(int sendbufsize=8192, int recvbufsize=8192);
    short port(void) { return m_port; }
    int is_error(void) { return (m_socket<0); }

  protected:
    int m_socket;
    short m_port;
};

#endif //_LISTEN_H_
