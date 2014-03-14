#ifndef UDPCAMMANAGER_HH
#define UDPCAMMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"
#include "UdpCamOccurrence.hh"

namespace Pds {
  class UdpCamServer;
  class UdpCamManager;
  class CfgClientNfs;
}

class Pds::UdpCamManager {
  public:
    UdpCamManager(UdpCamServer* server,
                  CfgClientNfs* cfg);
    Appliance& appliance(void);
    void setOccSend(UdpCamOccurrence* occSend);

  private:
    Fsm&                _fsm;
    static const char*  _calibPath;
    UdpCamOccurrence*   _occSend;
};

#endif
