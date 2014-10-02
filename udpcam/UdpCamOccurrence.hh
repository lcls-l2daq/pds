// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#ifndef __UDPCAMOCCURRENCE_HH
#define __UDPCAMOCCURRENCE_HH

namespace Pds {
   class UdpCamOccurrence;
   class UdpCamManager;
   class GenericPool;
}

class Pds::UdpCamOccurrence {
  public:
    UdpCamOccurrence(UdpCamManager *mgr);
    ~UdpCamOccurrence();
    void outOfOrder(void);
    void userMessage(const char *msgText);

  private:
    UdpCamManager* _mgr;
    GenericPool* _outOfOrderPool;
    GenericPool* _userMessagePool;
};

#endif
