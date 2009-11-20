#ifndef Pds_VmonClientManager_hh
#define Pds_VmonClientManager_hh

#include "pds/management/CollectionObserver.hh"
#include "pds/mon/MonFd.hh"

#include "pds/vmon/VmonPoll.hh"
#include "pdsdata/xtc/TransitionId.hh"

#include <list>
using std::list;

namespace Pds {

  class MonClient;
  class MonConsumerClient;
  class VmonClientSocket;

  class VmonClientManager : public CollectionObserver,
			    public MonFd {
  public:
    VmonClientManager(unsigned char platform,
		      const char*   partition,
		      MonConsumerClient& consumer);
    ~VmonClientManager();
  public:
    void request_payload();
  public:
    void connect(int partition);
    void map();
    void disconnect();
  public:
    // CollectionObserver interface
    virtual void allocated(const Allocation&) {}
    virtual void dissolved() {}
    virtual void post(const Transition&);
    virtual void post(const InDatagram&);
  public:
    // MonFd interface
    virtual int fd() const;
    virtual int processIo();
  private:
    virtual void add(MonClient&) = 0;
  protected:
    virtual void clear();
    MonClient* lookup(const Src&);
  private:
    MonConsumerClient& _consumer;
    VmonClientSocket*  _socket;
    list<MonClient*>   _client_list;
    int                _partition;
    enum { Disconnected, Connected, Mapped } _state;
    VmonPoll           _poll;
  };

};

#endif
    
