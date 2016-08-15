#ifndef MonTag_Server_hh
#define MonTag_Server_hh

#include "pds/service/Routine.hh"

#include <vector>

namespace Pds {
  class Task;

  namespace MonTag {
    class Group;
    class Node;

    class Server : public Pds::Routine {
    public:
      Server(unsigned short port);
      ~Server();
    public:
      unsigned next();
    public:
      void     routine();
    private:
      int   _socket;
      Task* _task;
      std::vector<Group*> _groups;
      std::vector<Node* > _nodes;
    };
  };
};

#endif
