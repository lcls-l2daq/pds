#include "pds/tag/Client.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/Ins.hh"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

//#define DBUG

using namespace Pds::Tag;

Client::Client(const Ins& server,
               unsigned   platform,
               unsigned   group) :
  _fd(-1)
{
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("Tag::Client ::socket");
    exit(1);
  }

  Pds::Sockaddr sa(server);
  if (::connect(fd, sa.name(), sa.sizeofName()) < 0) {
    perror("Tag::Client ::connect");
    exit(1);
  }

#ifdef DBUG
  printf("Tag::Client connected to %x.%d\n",
         sa.get().address(), sa.get().portId());
#endif

  uint32_t w = 0;
  w |= ((platform&0xff)<<8) | ((group&0xff)<<0);
#ifdef DBUG
  printf("Tag::Client sending %08x\n",w);
#endif
  ::send(fd, &w, sizeof(w), 0);
  
  if (::recv(fd, &_key, sizeof(_key), 0) != sizeof(_key)) {
    perror("Tag::Client ::recv");
    exit(1);
  }

#ifdef DBUG
  printf("Tag::Client received key %u.%u.%u\n",
         _key.platform(),
         _key.group(),
         _key.member());
#endif

  _fd = fd;
}

Client::~Client() { if (_fd>=0) close(_fd); }

const Key& Client::key() const { return _key; }

void Client::request(unsigned buffer)
{
#ifdef DBUG
  printf("Tag::Client requesting %u\n",buffer);
#endif

  ::send(_fd, &buffer, sizeof(buffer), 0);
}
