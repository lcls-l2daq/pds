#ifndef Pds_MonSERVER_HH
#define Pds_MonSERVER_HH

#include <sys/uio.h>

#include "pds/mon/MonSocket.hh"
#include "pds/mon/MonMessage.hh"

namespace Pds {

  class MonCds;
  class MonUsage;

  class MonServer {
  public:
    MonServer(const Src&, 
	      const MonCds& cds, 
	      MonUsage& usage, 
	      MonSocket& socket);
    virtual ~MonServer();

    MonSocket& socket();

    bool enabled() const;

    void enable();
    void disable();

    void description();
    void payload();
    void payload(unsigned size);

  private:
    void adjust();
    void reply (MonMessage::Type,int);
    
  private:
    const MonCds& _cds;
    MonUsage&  _usage;
    MonSocket& _socket;
    MonMessage _reply;
    iovec* _iovreply;
    unsigned _iovcnt;
    int* _signatures;
    unsigned _sigcnt;
    bool _enabled;
  };
};

#endif
