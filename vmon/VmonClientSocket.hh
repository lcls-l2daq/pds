#ifndef Pds_VmonClientSocket_HH
#define Pds_VmonClientSocket_HH

#include "pds/vmon/VmonSocket.hh"
#include "pds/service/Sockaddr.hh"

namespace Pds {

  class Ins;

  class VmonClientSocket : public VmonSocket {
  public:
    VmonClientSocket();
    ~VmonClientSocket();
    
    int set_dst(const Ins&,
		int interface);

  private:
    Sockaddr      _addr;
    Sockaddr      _peer;
  };
};

#endif
