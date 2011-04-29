#ifndef _WDL_DESTROYCHECK_H_
#define _WDL_DESTROYCHECK_H_

// this is a useful class for verifying that an object (usually "this") hasn't been destroyed:
// to use it you add a WDL_DestroyState as a member of your class, then use the WDL_DestroyCheck 
// helper class (creating it when the pointer is known valid, and checking it later to see if it 
// is still valid).
//
// example:
// class myClass {
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



struct WDL_DestroyStateRec;
struct WDL_DestroyStateNextRec { WDL_DestroyStateRec *next; };
struct WDL_DestroyStateRec { WDL_DestroyStateNextRec n, *prev;  };

class WDL_DestroyState
{
  public:
    WDL_DestroyState() { nn.next=NULL; }

    ~WDL_DestroyState() 
    { 
      while (nn.next)
      {
        WDL_DestroyStateRec *np = nn.next->n.next;
        nn.next->prev=NULL; nn.next->n.next=NULL;
        nn.next=np;
      }
    }

    WDL_DestroyStateNextRec nn;
};

class WDL_DestroyCheck
{
    WDL_DestroyStateRec s;
  public:
    WDL_DestroyCheck(WDL_DestroyState *state)
    { 
      s.n.next=NULL;
      if ((s.prev=&state->nn)) 
      {
	      if ((s.n.next=s.prev->next)) s.n.next->prev = &s.n;
	      s.prev->next=&s;
      }

    }
    ~WDL_DestroyCheck() 
    { 
      if (s.prev)
      {
        s.prev->next = s.n.next; 
        if (s.n.next) s.n.next->prev = s.prev; 
      }
    }

    bool isOK() { return !!s.prev; }
};

#endif
