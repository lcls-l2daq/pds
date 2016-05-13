/*
 * pnCCDManager.hh
 *
 *  Created on: May 11, 2016
 *      Author: ddamiani
 */

#ifndef PNCCDTRIGMONITOR_HH
#define PNCCDTRIGMONITOR_HH

#include "pds/epicstools/EpicsCA.hh"

namespace Pds {
  class Semaphore;
  class pnCCDServer;
  class pnCCDTrigMonitor;
}

class Pds::pnCCDTrigMonitor : public Pds_Epics::EpicsCA,
                              public Pds_Epics::PVMonitorCb {
  public:
    pnCCDTrigMonitor( const char* pvname, pnCCDServer* server, ca_client_context* context);
    ~pnCCDTrigMonitor();
    void enable(bool writePV);
    void disable(bool writePV);
    void updated();
    void putStatus(bool status);
    bool check();
    void put(dbr_enum_t value, bool wait);

  private:
    pnCCDServer*       _server;
    Semaphore*         _sem;
    Semaphore*         _wait;
    bool               _expect_off;
    ca_client_context* _context;
};

#endif
