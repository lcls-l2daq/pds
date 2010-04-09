#ifndef Pds_NullServer_hh
#define Pds_NullServer_hh

#include "pds/utility/NetDgServer.hh"

namespace Pds {
  class NullServer : public NetDgServer {
  public:
    NullServer(const Ins& ins,
	       const Src& src,
	       unsigned   maxsiz,
	       unsigned   maxevt) : NetDgServer(ins,src,maxsiz*maxevt) {
      _payload = new char[maxsiz];
    }
    ~NullServer() { delete[] _payload; }
  public:
    int fetch(char* payload, int flags) {
      int error = NetDgServer::fetch(_payload, flags);
      if (error > 0) error = 0;
      return error;
    }
  public:
    const char* payload() const { return _payload; }
  private:
    char* _payload;
  };
};

#endif
