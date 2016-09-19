#ifndef Pds_MonReqServer_hh
#define Pds_MonReqServer_hh

#include "pds/utility/Appliance.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/client/Action.hh"
#include "pds/service/Semaphore.hh"

#include "pds/monreq/ConnectionManager.hh"
#include "pds/monreq/ServerConnection.hh"

#include <vector>

namespace Pds {
  
  class MonReqServer : public Appliance, public Routine, public Action {
  public:
    MonReqServer(unsigned int nodenumber, unsigned int platform);
  public:
    ~MonReqServer();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
    InDatagram* fire       (InDatagram*);

    void routine();
  private:
    Task*    _task;
    Task*    _task2;
    int      _id;
    unsigned _platform;

    std::vector<Pds::MonReq::ServerConnection> _servers;
    Pds::MonReq::ConnectionManager _connMgr;
    int      _queued;
    int      _handled;
    Semaphore _sem;

  };
};

#endif
