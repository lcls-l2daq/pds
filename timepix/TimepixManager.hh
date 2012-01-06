#ifndef TIMEPIXMANAGER_HH
#define TIMEPIXMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"
// #include "TimepixOccurrence.hh"

namespace Pds {
  class TimepixServer;
  class TimepixManager;
  //class TimepixOccurrence;
  class CfgClientNfs;
}

class Pds::TimepixManager {
  public:
    TimepixManager(TimepixServer* server,
                   CfgClientNfs* cfg);
    Appliance& appliance(void);

  private:
    Fsm&                _fsm;
    static const char*  _calibPath;
    // TimepixOccurrence*  _occSend;
};

#endif
