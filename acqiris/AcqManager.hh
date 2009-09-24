#ifndef PDSACQMANAGER_HH
#define PDSACQMANAGER_HH

#include "acqiris/aqdrv4/AcqirisD1Import.h"

namespace Pds {

  class Fsm;
  class Appliance;
  class AcqServer;
  class CfgClientNfs;

  class AcqManager {
  public:
    enum MultiModuleNumber {Module0,Module1,Module2,Module3,Module4};
    AcqManager(ViSession instrumentId, AcqServer& server, CfgClientNfs& cfg);
    Appliance& appliance();
    unsigned temperature(MultiModuleNumber module);
    static const char* calibPath();
  private:
    ViSession _instrumentId;
    Fsm& _fsm;
    static const char* _calibPath;
  };
}

#endif
