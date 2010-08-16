/*
  WDL - rpool.h
  Copyright (C) 2006 and later, Cockos Incorporated

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  

  This file defines a template for a class that stores a list of objects, and allows the caller
  to periodically get an object, do something with it, and add it back into the pool.

  When the caller does this, it can set ownership of the object, and an expiration for that ownership.


  The PTYPE1 and PTYPE2entries for the template are there to store additional information (for use with poollist.h)


  This is pretty esoteric. But we use it for some things.

*/


#ifndef _WDL_RPOOL_H_
#define _WDL_RPOOL_H_

// resource pool (time based)

#include "ptrlist.h"
#include "mutex.h"


template<class RTYPE, class PTYPE1, class PTYPE2> class WDL_ResourcePool
{
  public:
    WDL_ResourcePool()
    {
      user1=0;
      user2=0;
      m_rlist=NULL;
    }
    ~WDL_ResourcePool()
    {
      while (m_rlist)
      {
        RP_ENT *tp=m_rlist;
        m_rlist=m_rlist->next;
        delete tp->m_res;
        delete tp;
      }
    }

    void AddResource(RTYPE *item, void *own, unsigned int until)
    {
      WDL_MutexLock lock(&m_mutex);
      RP_ENT *t=new RP_ENT(item,own,until);
      t->next=m_rlist;
      m_rlist=t;
    }

    RTYPE *GetResource(void *own, unsigned int now)
    {
      WDL_MutexLock lock(&m_mutex);
      RP_ENT *ent=m_rlist;
      RP_ENT *lastent=NULL;
      RP_ENT *bestent=NULL;
      RP_ENT *bestlastent=NULL;
      int cnt=0;
      while (ent)
      {
        cnt++;
        if (ent->m_ownerptr == own)
        {
          RTYPE *t=ent->m_res;
          if (lastent) lastent->next = ent->next;
          else m_rlist = ent->next;

          delete ent;

          return t;
        }
        if (ent->m_owneduntil < now)
        {
          bestent=ent;
          bestlastent=lastent;
        }
        lastent=ent;
        ent=ent->next;
      }

      if (!bestent) 
      {
        return 0;
      }

      RTYPE *t=bestent->m_res;
      if (bestlastent) bestlastent->next = bestent->next;
      else m_rlist = bestent->next;
      delete bestent;
      return t;
    }

    PTYPE1 user1;
    PTYPE2 user2;

private:
  class RP_ENT 
  {
    public:
      RP_ENT(RTYPE *res, void *own, unsigned int til) :
          m_res(res), m_ownerptr(own), m_owneduntil(til), next(NULL) { }
      ~RP_ENT() { }

      RTYPE *m_res;
      unsigned int m_owneduntil;
      void *m_ownerptr;

      RP_ENT *next;
  };

  WDL_Mutex m_mutex;
  RP_ENT *m_rlist;



};



#endif