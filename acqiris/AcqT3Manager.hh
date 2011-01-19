#ifndef PDSACQT3MANAGER_HH
#define PDSACQT3MANAGER_HH

#include "acqiris/aqdrv4/AcqirisImport.h"

namespace Pds {

  class Fsm;
  class Appliance;
  class AcqServer;
  class CfgClientNfs;

  class AcqT3Manager {
  public:
    AcqT3Manager(ViSession instrumentId, AcqServer& server, CfgClientNfs& cfg);
    ~AcqT3Manager();
    Appliance& appliance();
  private:
    ViSession _instrumentId;
    Fsm& _fsm;
  };
}

#endif
