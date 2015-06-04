// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#ifndef __ANDOROCCURRENCE_HH
#define __ANDOROCCURRENCE_HH

namespace Pds {
   class AndorOccurrence;
   class AndorManager;
   class GenericPool;
}

class Pds::AndorOccurrence {
  public:
    AndorOccurrence(AndorManager *mgr);
    ~AndorOccurrence();
    void outOfOrder(void);
    void userMessage(const char *msgText);

  private:
    AndorManager* _mgr;
    GenericPool* _outOfOrderPool;
    GenericPool* _userMessagePool;
};

#endif
