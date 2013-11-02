// $Id$
// Author: Jana Thayer <jana@slac.stanford.edu>

#ifndef __RAYONIXOCCURRENCE_HH
#define __RAYONIXOCCURRENCE_HH

namespace Pds {
   class RayonixOccurrence;
   class RayonixManager;
   class GenericPool;
}

class Pds::RayonixOccurrence {
  public:
    RayonixOccurrence(RayonixManager *mgr);
    ~RayonixOccurrence();
    void outOfOrder(void);
    void userMessage(char *msgText);

  private:
    RayonixManager* _mgr;
    GenericPool* _outOfOrderPool;
    GenericPool* _userMessagePool;
};

#endif
