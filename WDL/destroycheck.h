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

class WDL_DestroyState
{
  public:
   struct buff
   {
      buff() { refcnt=1; isOK=true; }
      ~buff() { }
      void Release() { if (--refcnt<1) delete this; }
      int refcnt;
      bool isOK; 
    };
    WDL_DestroyState() { b=new buff; }
    ~WDL_DestroyState() { b->isOK=false; b->Release(); }
    buff *b;
};

class WDL_DestroyCheck
{
    WDL_DestroyState::buff *m_s;
  public:
    WDL_DestroyCheck(WDL_DestroyState *s) { m_s = s->b; m_s->refcnt++; }
    ~WDL_DestroyCheck() { m_s->Release();  }
    bool isOK() { return m_s->isOK; }
};

#endif
