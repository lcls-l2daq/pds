#include "pds/pvdaq/Manager.hh"
#include "pds/pvdaq/Server.hh"

#include "pds/pvdaq/Manager.hh"
#include "pds/client/Fsm.hh"

using namespace Pds::PvDaq;

Manager::Manager(Server& server) : _fsm(*new Pds::Fsm())
{
  _fsm.callback(Pds::TransitionId::Configure,&server);
  _fsm.callback(Pds::TransitionId::Enable   ,&server);
  _fsm.callback(Pds::TransitionId::Disable  ,&server);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

