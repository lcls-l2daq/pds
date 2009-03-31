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

#include "pdsdata/xtc/Src.hh"

namespace Pds {

  class MonConsumerClient;

  class MonClient : public MonFd {
  public:
    MonClient(MonConsumerClient& consumer, 
	      MonCds* cds,
	      MonSocket& socket,
	      const Src& src);
    virtual ~MonClient();

    MonSocket& socket();
    const Src& src() const;

    int use    (int signature);
    int dontuse(int signature);
    int use_all();
    int askdesc();
    int askload();

    void read_description(int size);
    void read_payload();

    MonCds& cds();
    const MonCds& cds() const;
    const Ins& dst() const;
    void dst(const Ins&);
    bool needspayload() const;
  
  private:
    // Implements Server
    virtual int fd() const;
    virtual int processIo();

  private:
    void adjustload ();
    void payload    ();
    void adjustdesc ();

  private:
    Src _src;
    MonConsumerClient& _consumer;
    MonCds* _cds;
    MonSocket& _socket;
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
