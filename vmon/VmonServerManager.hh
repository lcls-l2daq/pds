#ifndef Pds_VmonServerManager_hh
#define Pds_VmonServerManager_hh

#include "pds/mon/MonFd.hh"
#include "pds/vmon/VmonPoll.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonUsage.hh"

namespace Pds {

  class Ins;
  class Src;
  class MonServer;
  class VmonServerSocket;

  class VmonServerManager : public VmonPoll,
			    public MonFd {
  public:
    static VmonServerManager* instance(const char* =0);
  public:
    MonCds& cds();
    void listen(const Src&, const Ins&);
    void listen();
  public:
    virtual int fd() const;
    virtual int processIo();
  private:
    VmonServerManager(const char*);
    ~VmonServerManager();
  private:
    MonCds     _cds;
    MonUsage   _usage;
    MonServer* _server;
    VmonServerSocket* _socket;
  };

};

#endif
