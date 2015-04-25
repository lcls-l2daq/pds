#ifndef PDSACQMANAGER_HH
#define PDSACQMANAGER_HH

#include "acqiris/aqdrv4/AcqirisD1Import.h"

namespace Pds {

  class Fsm;
  class Appliance;
  class AcqServer;
  class CfgClientNfs;
  class Semaphore;

  class AcqManager {
  public:
    enum MultiModuleNumber {Module0,Module1,Module2,Module3,Module4};
    enum AcqFlag {AcqFlagVerbose=1, AcqFlagZealous=2, AcqFlagShutdown=4, AcqFlagDebug=8};
    enum {AcqTemperaturePeriod=10};
    AcqManager(ViSession instrumentId, AcqServer& server, CfgClientNfs& cfg, Semaphore&, char *pvPrefix, unsigned pvPeriod, char *pAcqFlag);
    ~AcqManager();
    Appliance& appliance();
    unsigned temperature(MultiModuleNumber module);
    static const char* calibPath();
  private:
    ViSession _instrumentId;
    Fsm& _fsm;
    static const char* _calibPath;
    char* _pAcqFlag;
    bool _verbose;
  public:
    bool _zealous;
  };
}

#endif
