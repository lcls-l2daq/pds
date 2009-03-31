#include "pds/management/VmonServerAppliance.hh"
#include "pds/collection/Route.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/Transition.hh"
#include "pds/vmon/VmonServerManager.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/Ins.hh"
#include "pds/mon/MonServer.hh"
#include "pds/vmon/VmonServerSocket.hh"


#include <errno.h>
#include <string.h>

using namespace Pds;

VmonServerAppliance::VmonServerAppliance(const Src& src) :
  _src      (src),
  _partition(-1)
{
}


VmonServerAppliance::~VmonServerAppliance()
{
}


Transition* VmonServerAppliance::transitions(Transition* tr)
{
  if (tr->id()==TransitionId::Map) {

    const Allocate& allocate = reinterpret_cast<const Allocate&>(*tr);
    _partition = allocate.allocation().partitionid();
    Ins vmon(StreamPorts::vmon(_partition));

    VmonServerManager::instance()->listen(_src, vmon);
  }
  else if (tr->id()==TransitionId::Unmap) {
    VmonServerManager::instance()->listen();
  }

  return tr;
}

InDatagram* VmonServerAppliance::occurrences(InDatagram* dg) 
{ return dg; }

InDatagram* VmonServerAppliance::events     (InDatagram* dg)
{ return dg; }
    
