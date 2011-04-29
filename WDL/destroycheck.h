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


struct WDL_DestroyStateRec
{
  WDL_DestroyStateRec *next; // this may sometimes be WDL_DestroyState::next
  WDL_DestroyStateRec *prev; // never use prev->prev, it may not be valid (if prev points to WDL_DestroyState::next instead of a WDL_DestroyStateRec)!
};

class WDL_DestroyState
{
  public:
    WDL_DestroyState() { next=NULL; }

    ~WDL_DestroyState() 
    { 
      while (next)
      {
        WDL_DestroyStateRec *np = next->next;
        next->prev=next->next=NULL;
        next=np;
      }
    }

    WDL_DestroyStateRec *next;
};

class WDL_DestroyCheck
{
    WDL_DestroyStateRec s;
  public:
    WDL_DestroyCheck(WDL_DestroyState *state)
    { 
      s.next=NULL;
      if ((s.prev=(WDL_DestroyStateRec *)&state->next)) 
      {
	if ((s.next=s.prev->next)) s.next->prev = &s;
	s.prev->next=&s;
      }

    }
    ~WDL_DestroyCheck() 
    { 
      if (s.prev)
      {
        s.prev->next = s.next; 
        if (s.next) s.next->prev = s.prev; 
      }
    }

    bool isOK() { return !!s.prev; }
};

#endif
