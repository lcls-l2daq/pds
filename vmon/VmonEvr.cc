#include "pds/vmon/VmonEvr.hh"

#include "pds/vmon/VmonServerManager.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryScalar.hh"
#include "pds/mon/MonDescScalar.hh"
#include "pds/config/EvrConfigType.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/Src.hh"
#include <time.h>

using namespace Pds;

VmonEvr::VmonEvr(const Src& src)
{
  MonGroup* group = new MonGroup("Evr");
  VmonServerManager::instance()->cds().add(group);

  char tmp[32];
  const unsigned maxgroups = EventCodeType::MaxReadoutGroup+1;
  std::vector<std::string> group_names(maxgroups);
  for(unsigned i=0; i<maxgroups; i++) {
    sprintf(tmp,"grp%d",i);
    group_names[i] = std::string(tmp);
  }
  
  MonDescScalar readouts("Readouts",group_names);
  _readouts = new MonEntryScalar(readouts);
  group->add(_readouts);

  std::vector<std::string> queue_names(3);
  queue_names[0]=std::string("wrMaster");
  queue_names[1]=std::string("wrSlave");
  queue_names[2]=std::string("rdSlave");

  MonDescScalar queues("Queues",queue_names);
  _queues = new MonEntryScalar(queues);
  group->add(_queues);
}

VmonEvr::~VmonEvr()
{
}

void VmonEvr::readout(unsigned group)
{
  _readouts->addvalue(1,group);
}

void VmonEvr::queue(int wrM, int rdM,
                    int wrS, int rdS)
{
  _queues->setvalue(wrM-rdM,0);
  _queues->setvalue(wrS-rdM,1);
  _queues->setvalue(rdS-rdM,2);
}

void VmonEvr::reset()
{
  _readouts->reset();
  _queues  ->reset();
}
