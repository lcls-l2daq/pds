#include "pds/vmon/VmonPoll.hh"

#include "pds/mon/MonFd.hh"
#include "pds/mon/MonLoopback.hh"

#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"

#include <errno.h>
#include <string.h>

using namespace Pds;


VmonPoll::VmonPoll() :
  _task    (new Task(TaskObject("vmonMgr"))),
  _loopback(new MonLoopback),
  _ofd     (0)
{
  _pfd[0].fd = _loopback->socket();
  _pfd[0].events = POLLIN;
  _pfd[0].revents = 0;
}


VmonPoll::~VmonPoll()
{
  _task->destroy();
}


void VmonPoll::manage(MonFd& fd)
{
  _ofd = &fd;
  _pfd[1].fd = fd.fd();
  _pfd[1].events = POLLIN;
  _pfd[1].revents = 0;
  _task->call(this);
}

void VmonPoll::unmanage(MonFd& fd)
{
  int msg=0;
  _loopback->write(&msg,sizeof(msg));
}

int VmonPoll::poll()
{
  if (::poll(_pfd, 2, -1) > 0) {
    if (_pfd[1].revents & (POLLIN | POLLERR))
      return _ofd->processIo();
    if (_pfd[0].revents & (POLLIN | POLLERR)) {
      int cmd;
      _loopback->read(&cmd,sizeof(cmd));
      _ofd = 0;
      return 0;
    }
  }
  else {
    //  known valid returns from poll are : EINTR - interrupted by a signal
    printf("VmonPoll::poll failed : reason %s\n",strerror(errno));
  }
  return 1;
}

void VmonPoll::routine()
{
  while(poll());
}


