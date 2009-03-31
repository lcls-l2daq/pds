#ifndef Pds_VmonServerSocket_HH
#define Pds_VmonServerSocket_HH

#include "pds/vmon/VmonSocket.hh"

#include "pds/service/Ins.hh"
#include "pds/service/Sockaddr.hh"

namespace Pds {

  class VmonServerSocket : public VmonSocket {
  public:
    VmonServerSocket(const Ins& mcast,
		     int interface);
    ~VmonServerSocket();

  private:
    Sockaddr _peer;
    Ins      _mcast;
    int      _interface;
  };
};

#endif
