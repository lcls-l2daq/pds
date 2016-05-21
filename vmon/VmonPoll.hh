#ifndef Pds_VmonPoll_hh
#define Pds_VmonPoll_hh

#include "pds/service/Routine.hh"
#include "pds/service/Semaphore.hh"

#include <poll.h>

namespace Pds {

  class MonFd;
  class Task;
  class MonSocket;

  class VmonPoll : private Routine {
  public:
    VmonPoll();
    ~VmonPoll();
  public:
    void manage  (MonFd&);
    void unmanage(MonFd&);
  private:
    int poll();
    void routine();
  private:
    Task*      _task;
    MonSocket* _loopback;
    MonFd*     _ofd;
    pollfd     _pfd[2];
    Semaphore  _sem;
  };

};

#endif
