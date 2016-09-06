#include "pds/tag/Server.hh"

#include "pds/service/Sockaddr.hh"
#include "pds/service/Task.hh"

#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

using namespace Pds;

//#define DBUG

Tag::Server::Server(unsigned short   port,
                    Tag::ServerImpl& impl) :
  _impl(impl),
  _task(new Task(TaskObject("tagsrv")))
{
  int lfd;
  if ( (lfd = ::socket(AF_INET, SOCK_STREAM, 0))<0 ) {
    perror("Tag::Server socket failed");
    exit(1);
  }
  
  int y=1;
  if(setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1) {
    perror("Tag::Server reuseaddr");
    exit(1);
  }
  
  Ins ins(port);
  Sockaddr saddr(ins);
  if ( ::bind(lfd, saddr.name(), saddr.sizeofName()) < 0) {
    perror("Tag::Server bind");
    exit(1);
  }

  ::listen(lfd, 5);

#ifdef DBUG
  printf("Tag::Server listing on port %u\n",port);
#endif

  pollfd pfd;
  pfd.fd = lfd;
  pfd.events = POLLIN | POLLERR;
  pfd.revents = 0;
  _pfd .push_back(pfd);
  _keys.push_back(Key());
  memset(_members,0,sizeof(_members));
}

void Tag::Server::start()
{
  _task->call(this);
}

void Tag::Server::stop()
{
  _task->destroy();
}

void Tag::Server::routine()
{
  if (::poll(_pfd.data(), _pfd.size(), 1000) > 0) { 
    while (_pfd[0].revents & POLLIN) {
      Sockaddr sa;
      socklen_t sz;
      int fd;
      if ( (fd=::accept(_pfd[0].fd, sa.name(), &sz)) < 0) {
        perror("Tag::Server accept");
        break;
      }
#ifdef DBUG
      printf("Tag::Server accept connection from %x.%d\n",
             sa.get().address(),sa.get().portId());
#endif
      //  Read registration
      Key key(Key::Invalid);
      { uint32_t w;
        if (::recv(fd, &w, sizeof(w), 0) == sizeof(w)) {
#ifdef DBUG
          printf("Tag::Server recv registration %08x\n",w);
#endif
          unsigned group = w&0xff;
          if (group < MAX_GROUPS) {
            unsigned platform = (w>>8)&0xff;
            unsigned m = _members[group+2*platform];
            for(unsigned i=0; i<MAX_GROUP_SIZE; i++)
              if ((m&(1<<i))==0) {
                key = Key( platform, group, i );
                _members[group+2*platform] |= (1<<i);
                break;
              }
          }
        }
      }

      if (!key.valid()) {
        close(fd);
        break;
      }

#ifdef DBUG
      printf("Tag::Server sending key %u.%u.%u\n",
             key.platform(), key.group(), key.member());
#endif

      //  Send key
      ::send(fd, &key, sizeof(Key), 0);

      pollfd pfd;
      pfd.fd = fd;
      pfd.events = POLLIN | POLLERR;
      pfd.revents = 0;
      _pfd .push_back(pfd);
      _keys.push_back(key);
      break;
    }
    for(unsigned i=1; i<_pfd.size(); i++) {
      if (_pfd[i].revents & POLLIN) {
        unsigned index;
        int nb;
        while( (nb=::recv(_pfd[i].fd, &index, sizeof(index), MSG_DONTWAIT))==sizeof(index)) {
          _impl.insert(_keys[i],index);
        }
        if (nb==0) {
          //  Client down
          close(_pfd[i].fd);
          _pfd .erase(_pfd.begin()+i);
          _members[_keys[i].group()] &= ~(1<<_keys[i].member());
          _keys.erase(_keys.begin()+i);
          break;  // can't iterate further
        }
      }
    }
  }
  _task->call(this);
}
