#include "pds/montag/Server.hh"
#include "pds/montag/Group.hh"
#include "pds/montag/Node.hh"

#include "pds/service/Sockaddr.hh"
#include "pds/service/Task.hh"

#include <stdlib.h>
#include <poll.h>
#include <unistd.h>

using namespace Pds::MonTag;

Server::Server(unsigned short port) :
  _task(new Task(TaskObject("MonTag")))
{
  _socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (_socket == -1) {
    perror("Opening listen socket");
    exit(1);
  }

  Sockaddr listen_server(port);

  int y=1;
  if(setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1) {
    perror("set reuseaddr");
    exit (1);
  }
        
  if (bind(_socket, listen_server.name(), listen_server.sizeofName()) < 0) {
    perror("tcp bind error: \n");
    exit (1);
  }

  listen(_socket, 10);

  _task->call(this);
}

Server::~Server()
{
  _task->destroy();
}

void Server::routine()
{
  unsigned nfd = _nodes.size()+1;
  pollfd pfd[nfd];
  pfd[0].fd = _socket;
  pfd[0].events = POLLIN | POLLERR;
  for(unsigned i=0; i<nfd-1; i++) {
    pfd[i+1].fd     = _nodes[i]->fd();
    pfd[i+1].events = POLLIN | POLLERR;
  }

  if (::poll(pfd, nfd, 1000)>0) {

    for(unsigned i=0; i<nfd-1; i++) {
      if (pfd[i+1].revents & POLLIN) {
        Group& group = _nodes[i]->group();
        unsigned buffer;
        if (::read(pfd[i+1].fd, &buffer, sizeof(buffer)) == sizeof(buffer))
          group.push( _nodes[i]->member(), buffer);
        else {
          // connection terminated
          delete _nodes[i];
          _nodes.erase(_nodes.begin()+i);
          if (group.empty()) {
            for(unsigned j=0; j<_groups.size(); j++)
              if (_groups[j]==&group) 
                _groups[j]=0;
            delete &group;
          }
          break; // no longer safe to iterate
        }
      }
    }
        

    //
    //  A new connection attempt
    //    Receive the group name and register if new
    //    Add the node to the list for that group
    //    Return the key for decoding tag requests
    //
    if (pfd[0].revents & POLLIN) { // a new connection attempt
      Sockaddr  addr;
      socklen_t addrlen = addr.sizeofName();
      int fd = ::accept(_socket, addr.name(), &addrlen);
      if (fd == -1) {
        perror("Accepting mon node");
      }
      else {
        //  Receive the group name
        unsigned name_len = Group::NAME_LEN;
        char name[name_len];
        ::read(fd, name, name_len);

        //  Find the group or create, if new
        std::string sname(name);
        Group* group = 0;
        for(unsigned i=0; i<_groups.size(); i++)
          if (_groups[i] && _groups[i]->name() == sname) {
            group = _groups[i];
            break;
          }
        if (group == 0) {
          for(unsigned i=0; i<_groups.size(); i++)
            if (_groups[i]==0) {
              group = new Group(sname,8*i);
              _groups[i] = group;
            }
          if (group == 0) {
            group = new Group(sname,8*_groups.size());
            _groups.push_back(group);
          }
        }
      
        //  Register the node with the group
        int member = group->insert();
        if (member < 0) {
          Ins svc(addr.get());
          printf("Reject connection from %x.%d\n",
                 svc.address(),
                 svc.portId());
        }
        else {
          Node* node = new Node(fd, member, *group);
          _nodes.push_back(node);
          Tag tag(group->tag(member));
          ::send(fd, &tag, sizeof(tag),0);
       }
      }
    }
  }

  _task->call(this);
}
        
unsigned Server::next()
{
  unsigned v=0;
  for(unsigned i=0; i<_groups.size(); i++)
    if (_groups[i])
      v |= _groups[i]->pop();
  return v;
}  
