#ifndef PDS_BLDSERVER
#define PDS_BLDSERVER

#include "NetDgServer.hh"

namespace Pds {
  class BldServer : public NetDgServer
  {
  public:
    BldServer(const Ins& ins,
	      const Src& src,
	      unsigned   nbufs);
   ~BldServer();
  public:
    bool        isValued()             const;
  };
};

#endif
