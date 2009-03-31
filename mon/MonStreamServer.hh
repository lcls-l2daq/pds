#ifndef Pds_MonStreamServer_hh
#define Pds_MonStreamServer_hh

#include "pds/mon/MonServer.hh"
#include "pds/mon/MonFd.hh"

#include "pds/mon/MonMessage.hh"

namespace Pds {
  class MonStreamSocket;
  class MonStreamServer : public MonServer,
			  public MonFd {
  public:
    MonStreamServer(const Src&,
		    const MonCds&,
		    MonUsage&,
		    MonStreamSocket*);
    virtual ~MonStreamServer();
  private:
    // Implements MonFd
    virtual int fd() const;
    virtual int processIo();  // interface for stream io
  private:
    MonStreamSocket* _socket;
    MonMessage       _request;
  };
};

#endif
    
