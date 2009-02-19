#ifndef Pds_MonCLIENT_HH
#define Pds_MonCLIENT_HH

#include <sys/uio.h>

#include "pds/mon/MonSocket.hh"
#include "pds/mon/MonFd.hh"
#include "pds/service/Ins.hh"

#include "pds/mon/MonMessage.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonPort.hh"
#include "pds/mon/MonUsage.hh"

namespace Pds {

  class MonConsumerClient;

  class MonClient : public MonSocket, public MonFd {
  public:
    MonClient(MonConsumerClient& consumer, 
	      MonPort::Type type, 
	      unsigned id,
	      const char* host);
    virtual ~MonClient();

    int use(int signature);
    int dontuse(int signature);
    int askdesc();
    int askload();

    MonCds& cds();
    const MonCds& cds() const;
    const Ins& dst() const;
    unsigned id() const;
    bool needspayload() const;
  
  private:
    // Implements Server
    virtual int fd() const;
    virtual int processIo();

  private:
    void adjustload();
    void payload();
    void adjustdesc();
    void description();

  private:
    unsigned _id;
    MonConsumerClient& _consumer;
    MonCds _cds;
    Ins _dst;
    MonMessage _request;
    MonMessage _reply;
    iovec _iovreq[2];
    iovec* _iovload;
    unsigned _iovcnt;
    char* _desc;
    unsigned short _descsize;
    unsigned short _maxdescsize;
    MonUsage _usage;
  };
};

#endif
