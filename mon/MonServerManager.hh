#ifndef Pds_MonSERVERMANAGER_HH
#define Pds_MonSERVERMANAGER_HH

#include "pds/mon/MonPoll.hh"
#include "pds/mon/MonFd.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Semaphore.hh"

#include "pds/mon/MonUsage.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonPort.hh"

namespace Pds {

  class Task;
  class MonServer;

  class MonServerManager : public MonPoll, public MonFd, private Routine {
  public:
    MonServerManager(MonPort::Type type);
    virtual ~MonServerManager();

    int serve();
    int dontserve();

    void enable();
    void disable();

    void update();

    MonCds& cds();
    const MonCds& cds() const;

  private:
    // Implements MonPoll
    virtual int processMsg();
    virtual int processTmo();
    virtual int processFd();
    virtual int processFd(MonFd& fd);

    // Implements MonFd
    virtual int fd() const;
    virtual int processIo();

    // Implements Routine
    virtual void routine();

  private:
    void update_entries();
    void disable_servers();
    void enable_servers();
    void adjust();
    void add(int socket);
    void remove(unsigned short pos);

  private:
    MonCds _cds;
    MonUsage _usage;
    MonSocket _listener;
    unsigned short _maxservers;
    unsigned short _nservers;
    MonServer** _servers;
    Task* _task;
    Semaphore _sem;
    int _result;
  };
};

#endif
