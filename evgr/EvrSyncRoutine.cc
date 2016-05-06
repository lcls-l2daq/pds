#include "pds/evgr/EvrSyncRoutine.hh"

#include "pds/utility/StreamPorts.hh"
#include "pds/utility/Mtu.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/evgr/EvrSyncCallback.hh"
#include "pds/collection/Route.hh"

#include <poll.h>

using namespace Pds;

EvrSyncRoutine::EvrSyncRoutine(unsigned         partition,
                               EvrSyncCallback& sync) :
  _group (StreamPorts::evr(partition)),
  _server((unsigned)-1,
          _group,
          sizeof(EvrDatagram),
          Mtu::Size),
  _loopback(sizeof(EvrDatagram)),
  _sync  (sync),
  _sem   (Semaphore::EMPTY)
{
  _server.join(_group,Ins(Route::interface()));
}

EvrSyncRoutine::~EvrSyncRoutine()
{
  EvrDatagram dgram;
  _loopback.unblock(reinterpret_cast<char*>(&dgram));
  _sem.take();
}

void EvrSyncRoutine::routine()
{
  const int bsiz = 256;
  char* payload = new char[bsiz];

  int nfds = 2;

  pollfd* pfd = new pollfd[2];
  pfd[0].fd = _server.socket();
  pfd[0].events = POLLIN | POLLERR | POLLHUP;
  pfd[0].revents = 0;
  pfd[1].fd = _loopback.fd();
  pfd[1].events = POLLIN | POLLERR | POLLHUP;
  pfd[1].revents = 0;
      
  while(::poll(pfd, nfds, -1) > 0) {
    if (pfd[0].revents & (POLLIN | POLLERR)) {
      int len = _server.fetch( payload, 0 );
      if (len ==0) {
        const EvrDatagram* dg = reinterpret_cast<const EvrDatagram*>(_server.datagram());
            
        _sync.initialize(dg->seq.stamp().fiducials(),
                         dg->seq.service()==TransitionId::Enable);
#ifdef DBG
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        printf("sync mcast seq %d.%09d (%x : %d.%09d)\n",
               int(ts.tv_sec), int(ts.tv_nsec),
               dg->seq.stamp().fiducials(),
               dg->seq.clock().seconds(),
               dg->seq.clock().nanoseconds());
#endif
      }
    }
    if (pfd[1].revents & (POLLIN | POLLERR)) {
      _sem.give();
      break;
    }
    pfd[0].revents = 0;
    pfd[1].revents = 0;
  }
}
