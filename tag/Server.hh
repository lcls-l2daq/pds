#ifndef PdsTag_Server_hh
#define PdsTag_Server_hh

#include "pds/service/Routine.hh"

#include "pds/tag/Key.hh"

#include <vector>
#include <poll.h>

namespace Pds {
  class Task;
  namespace Tag {
    class ServerImpl {
    public:
      virtual ~ServerImpl() {}
      virtual void insert(Key, unsigned buffer) = 0;
    };

    class Server : public Routine {
    public:
      Server(unsigned short port,
             ServerImpl&);
    public:
      void start();
      void stop ();
    public:
      void routine();
    private:
      ServerImpl&           _impl;
      Task*                 _task;
      uint32_t              _members[16];
      std::vector<Key>      _keys;
      std::vector<pollfd >  _pfd;
    };
  };
};

#endif
      
