#ifndef RAYONIXMANAGER_HH
#define RAYONIXMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"
#include "RayonixOccurrence.hh"

namespace Pds {
  class RayonixServer;
  class RayonixManager;
  class CfgClientNfs;
}

class Pds::RayonixManager {
public:
  RayonixManager(RayonixServer* server,
                 CfgClientNfs* cfg);
  ~RayonixManager();

  Appliance& appliance(void); 

  void setOccSend(RayonixOccurrence* occSend);

private:
  Fsm&                _fsm;
  RayonixOccurrence*  _occSend;
};

#endif
