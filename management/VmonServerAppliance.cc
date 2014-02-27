#include "pds/management/VmonServerAppliance.hh"
#include "pds/collection/Route.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/Transition.hh"
#include "pds/vmon/VmonServerManager.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/Ins.hh"
#include "pds/mon/MonServer.hh"
#include "pds/vmon/VmonServerSocket.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <errno.h>
#include <string.h>

using namespace Pds;

VmonServerAppliance::VmonServerAppliance(const Src& src,
                                         const char* name) :
  _src      (src),
  _partition(-1)
{
  if (name)
    _name = std::string(name);
  else {
    char buff[128];
    if (src.level()==Level::Source) {
      const DetInfo& info = static_cast<const DetInfo&>(src);
      sprintf(buff,"%s.%d.%s.%d",
              DetInfo::name(info.detector()),
              info.detId(),
              DetInfo::name(info.device()),
              info.devId());
    }
    else {
      const ProcInfo& info = static_cast<const ProcInfo&>(src);
      sprintf(buff,"%10.10s[%d.%d.%d.%d : %d]",
              Level::name(info.level()),
              (info.ipAddr()>>24)&0xff,
              (info.ipAddr()>>16)&0xff,
              (info.ipAddr()>> 8)&0xff,
              (info.ipAddr()>> 0)&0xff,
              info.processId());
    }
    _name = std::string(buff);
  }
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

    VmonServerManager::instance(_name.c_str())->listen(_src, vmon);
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
    
