#ifndef PDSACQMANAGER_HH
#define PDSACQMANAGER_HH

#include "acqiris/aqdrv4/AcqirisD1Import.h"

namespace Pds {

  class Fsm;
  class Appliance;
  class AcqServer;

  class AcqManager {
  public:
    AcqManager(ViSession instrumentId, AcqServer& server);
    Appliance& appliance();
  private:
    ViSession _instrumentId;
    Fsm& _fsm;
  };
}

#endif
