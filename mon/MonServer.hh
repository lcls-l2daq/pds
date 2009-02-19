#ifndef Pds_MonSERVER_HH
#define Pds_MonSERVER_HH

#include <sys/uio.h>

#include "pds/mon/MonSocket.hh"
#include "pds/mon/MonFd.hh"
#include "pds/mon/MonMessage.hh"

namespace Pds {

  class MonCds;
  class MonUsage;

  class MonServer : public MonSocket, public MonFd {
  public:
    MonServer(const MonCds& cds, MonUsage& usage, int socket);
    virtual ~MonServer();
    
    void enable();
    void disable();

  private:
    // Implements MonFd
    virtual int fd() const;
    virtual int processIo();
    
  private:
    void adjust();
    unsigned description();
    unsigned payload(unsigned used);
    
  private:
    const MonCds& _cds;
    MonUsage& _usage;
    MonMessage _request;
    MonMessage _reply;
    iovec* _iovreply;
    unsigned _iovcnt;
    int* _signatures;
    unsigned _sigcnt;
    bool _enabled;
  };
};

#endif
