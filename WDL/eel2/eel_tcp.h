#ifndef _EEL_TCP_H_
#define _EEL_TCP_H_

// tcp_listen_begin(port) returns 0 on OK
// x = tcp_listen(port) poll this, returns connection id >=0 or -1
// tcp_listen_end(port); 

// connection = tcp_connect(host, port)
// tcp_set_block(connection, block?)
// tcp_close(connection)

// tcp_send(connection, string[, length]) // can return 0 if block, -1 if error, otherwise returns length sent
// tcp_recv(connection, string[, maxlength]) // 0 on nothing, -1 on error, otherwise returns length recv'd

static EEL_F NSEEL_CGEN_CALL _eel_tcp_connect(void *opaque, EEL_F *dest, EEL_F *port)
{
  if (opaque)
  {
    EEL_STRING_MUTEXLOCK_SCOPE
    const char *fn = EEL_STRING_GET_FOR_INDEX(*dest,NULL);
    if (!fn) return -1.0;
    int port = (int) (*port+0.5);
    struct hostent *he = gethostbyname(fn);
    if (!he) return -1.0;
    
  }
  return -1.0;
}

void EEL_tcp_initsocketlib() // call per thread
{
#ifdef _WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif
}

void EEL_tcp_register()
{
/*
  NSEEL_addfunctionex("tcp_listen",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_tcp_listen);
  NSEEL_addfunctionex("tcp_listen_begin",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_tcp_listen_begin);
  NSEEL_addfunctionex("tcp_listen_end",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_tcp_listen_end);
  */

  NSEEL_addfunctionex("tcp_connect",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_tcp_connect);
  NSEEL_addfunctionex("tcp_set_block",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_tcp_set_block);
  NSEEL_addfunctionex("tcp_close",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_tcp_close);

  NSEEL_addfunc_varparm("tcp_send",2,NSEEL_PProc_THIS,(void *)&_eel_tcp_send);
  NSEEL_addfunc_varparm("tcp_recv",2,NSEEL_PProc_THIS,(void *)&_eel_tcp_recv);

}

#endif
