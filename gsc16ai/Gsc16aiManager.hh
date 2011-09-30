#ifndef GSC16AIMANAGER_HH
#define GSC16AIMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"
#include "Gsc16aiOccurrence.hh"

namespace Pds {
  class Gsc16aiServer;
  class Gsc16aiManager;
  class Gsc16aiOccurrence;
  class CfgClientNfs;
}

class Pds::Gsc16aiManager {
  public:
    Gsc16aiManager(Gsc16aiServer* server,
                   CfgClientNfs* cfg);
    Appliance& appliance(void);

  private:
    Fsm&                _fsm;
    static const char*  _calibPath;
    Gsc16aiOccurrence*  _occSend;
};

#endif
