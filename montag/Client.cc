#include "pds/montag/Client.hh"
#include "pds/montag/Group.hh"

#include "pds/service/Sockaddr.hh"

#include <stdlib.h>
#include <stdio.h>

using namespace Pds::MonTag;

Client::Client(const Pds::Ins&  svc,
               const char*      group) 
{
  _socket = ::socket(AF_INET, SOCK_STREAM, 0);

  Sockaddr addr(svc);

  if (::connect(_socket, addr.name(), addr.sizeofName())) {
    perror("Connecting to MonTag::Server");
    exit(1);
  }

  unsigned name_len = Group::NAME_LEN;
  char name[name_len];
  snprintf(name, name_len, group);
  ::send(_socket, name, name_len, 0);
}

void     Client::request      (unsigned buffer)
{
  if (::send(_socket, &buffer, sizeof(buffer), MSG_NOSIGNAL)<0)
    perror("MonTag::Client::request");
}

