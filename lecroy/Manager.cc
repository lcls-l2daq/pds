#include "pds/lecroy/Manager.hh"
#include "pds/lecroy/Server.hh"
#include "pds/client/Fsm.hh"

using namespace Pds::LeCroy;

Manager::Manager(Server& server) : _fsm(*new Pds::Fsm())
{
  _fsm.callback(Pds::TransitionId::Configure,&server);
  _fsm.callback(Pds::TransitionId::Enable   ,&server);
  _fsm.callback(Pds::TransitionId::Disable  ,&server);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

