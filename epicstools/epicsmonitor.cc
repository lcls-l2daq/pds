#include "pds/epicstools/EpicsCA.hh"
#include <stdio.h>
#include <unistd.h>

class MyCb : public Pds_Epics::PVMonitorCb {
public:
  MyCb() {}
  ~MyCb() {}
public:
  void updated() { printf("updated\n"); }
};

int main(int argc, char* argv[])
{
  const char* pvname = argv[1];

  //  EPICS thread initialization
  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ), 
           "control calling ca_context_create" );

  Pds_Epics::EpicsCA* ca = new Pds_Epics::EpicsCA(pvname,new MyCb);
  
  while(1)
    sleep(1);

  ca_context_destroy();

  return 0;
}
