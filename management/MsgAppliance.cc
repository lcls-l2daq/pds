#include "pds/management/MsgAppliance.hh"
#include "pds/utility/Transition.hh"
#include "pdsdata/xtc/TransitionId.hh"

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

using namespace Pds;

MsgAppliance::MsgAppliance()
{
  char buff[128];
  sprintf(buff,"pcds_node:%d",getppid());
  _fd = ::open(buff,O_NONBLOCK | O_WRONLY);
  if (_fd < 0)
    printf("Error opening message stream %s : reason %s\n",buff,strerror(errno));
  else {
    printf("Opened message stream %s on fd %d\n",buff,_fd);
    ::write(_fd,"Ready",5);
  }
}

MsgAppliance::~MsgAppliance()
{
  if (_fd >= 0)
    ::close(_fd);
}

Transition* MsgAppliance::transitions(Transition* tr) 
{ 
  if (_fd >= 0) {
    const char* msg = TransitionId::name(tr->id());
    ::write(_fd,msg,strlen(msg));
  }
  return tr;
}

InDatagram* MsgAppliance::occurrences(InDatagram* dg) { return dg; }
InDatagram* MsgAppliance::events     (InDatagram* dg) { return dg; }
