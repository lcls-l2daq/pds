#ifndef Pds_MonCLIENTMANAGER_HH
#define Pds_MonCLIENTMANAGER_HH

#include "pds/mon/MonPoll.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Semaphore.hh"

namespace Pds {

  class MonFd;
  class MonConsumerClient;
  class MonClient;
  class Task;

  class MonClientManager : public MonPoll, private Routine {
  public:
    MonClientManager(MonConsumerClient& consumer, const char** hosts);
    virtual ~MonClientManager();
    
    void connect(MonClient& client);
    void disconnect(MonClient& client);
    
    unsigned nclients() const;
    const MonClient* client(unsigned c) const;
    MonClient* client(unsigned c);
    MonClient* client(const char* name);
    
  private:
    // Implements MonPoll
    virtual int processMsg();
    virtual int processTmo();
    virtual int processFd();
    virtual int processFd(MonFd& fd);
    
    // Implements Routine
    virtual void routine();
    
  private:
    MonConsumerClient& _consumer;
    unsigned _nclients;
    MonClient** _clients;
    Task*     _task;
    Semaphore _sem;
  };
};

#endif
