// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#ifndef __TIMEPIXOCCURRENCE_HH
#define __TIMEPIXOCCURRENCE_HH

namespace Pds {
   class TimepixOccurrence;
   class TimepixManager;
   class GenericPool;
}

class Pds::TimepixOccurrence {
  public:
    TimepixOccurrence(TimepixManager *mgr);
    ~TimepixOccurrence();
    void outOfOrder(void);
    void userMessage(char *msgText);

  private:
    TimepixManager* _mgr;
    GenericPool* _outOfOrderPool;
    GenericPool* _userMessagePool;
};

#endif
