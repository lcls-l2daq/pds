#ifndef Pds_MonPOLL_HH
#define Pds_MonPOLL_HH

#include <poll.h>

#include "pds/mon/MonLoopback.hh"

namespace Pds {

  class MonFd;

  class MonPoll : public MonLoopback {
  public:
    MonPoll(int timeout);
    virtual ~MonPoll();

    int timeout() const;
    void dotimeout(int timeout);
    void donottimeout();
    void shrink();

    int manage  (MonFd& fd);
    int unmanage(MonFd& fd);
    int poll();

    virtual int processMsg() = 0;
    virtual int processTmo() = 0;
    virtual int processFd() = 0;
    virtual int processFd(MonFd& fd) = 0;

  private:
    void adjust();

  private:
    int _timeout;

    unsigned short _nfds;
    unsigned short _maxfds;

    MonFd** _ofd;
    pollfd* _pfd;
  };
};

#endif
