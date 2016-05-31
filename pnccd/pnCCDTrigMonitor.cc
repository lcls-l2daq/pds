#include "pds/pnccd/pnCCDTrigMonitor.hh"
#include "pds/pnccd/pnCCDServer.hh"
#include "pds/service/Semaphore.hh"
#include <stdio.h>

#define TRIG_OFF 0
#define TRIG_ON  1

using namespace Pds;

pnCCDTrigMonitor::pnCCDTrigMonitor(const char* pvname, pnCCDServer* server, ca_client_context* context) :
  Pds_Epics::EpicsCA (pvname,this),
  _server(server),
  _sem(new Semaphore(Semaphore::FULL)),
  _wait(new Semaphore(Semaphore::EMPTY)),
  _expect_off(false),
  _context(context)
{
}

pnCCDTrigMonitor::~pnCCDTrigMonitor()
{
  ca_context_destroy();
  delete _sem;
  delete _wait;
}

void pnCCDTrigMonitor::updated()
{
  // check that data from PV makes sense
  _sem->take();
  if (check()) {
    dbr_enum_t val = *(dbr_enum_t*)data();
    if (val == TRIG_ON && _expect_off) {
      printf("pnCCDTrigMonitor::updated self trigger mode activated unexpectedly!\n");
      _server->flagTriggerError();
    }   
  } else printf("pnCCDTrigMonitor::updated channel access data is not of the expected type\n");
  _sem->give();
}

void pnCCDTrigMonitor::disable(bool writePV)
{
  if (writePV) {
    if (check()) {
      put(TRIG_OFF, true);
    } else printf("pnCCDTrigMonitor::disable channel access data is not of the expected type\n");
  }
  _expect_off = true;
}

void pnCCDTrigMonitor::enable(bool writePV)
{
  _expect_off = false;
  if (writePV) {
    if (check()) {
      put(TRIG_ON, true);
    } else printf("pnCCDTrigMonitor::enable channel access data is not of the expected type\n");
  }
}

bool pnCCDTrigMonitor::check() {
  if (ca_current_context() == NULL) ca_attach_context(_context);
  return (_channel.nelements() == 1) && (_channel.type() == DBR_TIME_SHORT);
}

void pnCCDTrigMonitor::putStatus(bool status) {
  if(!status)
    printf("pnCCDTrigMonitor::putStatus channel access put returned a failed status code\n");
  _wait->give();
}

void pnCCDTrigMonitor::put(dbr_enum_t value, bool wait) {
  _sem->take();
  *(dbr_enum_t*) data() = value;
  _channel.put_cb();
  ca_flush_io();
  _sem->give();
  if (wait)
    _wait->take();
}

#undef TRIG_OFF
#undef TRIG_ON
