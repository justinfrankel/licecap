#ifndef _WDL_DESTROYCHECK_H_
#define _WDL_DESTROYCHECK_H_

// this is a useful class for verifying that an object (usually "this") hasn't been destroyed:
// to use it you add a WDL_DestroyState as a member of your class, then use the WDL_DestroyCheck 
// helper class (creating it when the pointer is known valid, and checking it later to see if it 
// is still valid).
//
// example:
// class myClass // {
//   WDL_DestroyState dest;
//   ...
// };
//
// calling code (on myClass *classInstnace):
//  WDL_DestroyCheck chk(&classInstance->dest);
//  somefunction();
//  if (!chk.isOK()) printf("classInstance got deleted!\n");
//
// NOTE: only use this when these objects will be accessed from the same thread -- it will fail miserably
// in a multithreaded environment

class WDL_DestroyState
{
    int *b;
  public:
    WDL_DestroyState() : b(NULL) {  }
    ~WDL_DestroyState() { if (b && !(*b&=0x7fffffff)) free(b); }
    int *getB() { if (!b && (b=(int *)malloc(sizeof(int)))) *b = 0x80000000; return b; }
};

class WDL_DestroyCheck
{
    int *m_s;
  public:
    WDL_DestroyCheck(WDL_DestroyState *s) { if ((m_s = s->getB())) ++*m_s; }
    ~WDL_DestroyCheck() { if (m_s && !--*m_s) free(m_s); }
    bool isOK() { return m_s && (*m_s&0x80000000); }
};

#endif
