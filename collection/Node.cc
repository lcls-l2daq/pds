#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <netinet/in.h>
#include <netdb.h>

#include "Node.hh"

using namespace Pds;

Node::Node() {}

Node::Node(const Node& rhs) :
  _level(rhs._level), 
  _partition(rhs._partition), 
  _pid(rhs._pid),
  _uid(rhs._uid),
  _ip(rhs._ip)
{}

Node::Node(Level::Type level, unsigned partition) :
  _level(level), 
  _partition(partition),
  _pid(getpid()),
  _uid(getuid()),
  _ip(0)
{}

Level::Type Node::level() const {return _level;}
unsigned Node::partition() const {return _partition;}

int Node::pid() const {return _pid;}
int Node::uid() const {return _uid;}
int Node::ip() const {return _ip;}
const Ether& Node::ether() const {return _ether;}

int Node::operator == (const Node& rhs) const
{
  if (_level == rhs._level &&
      _partition == rhs._partition &&
      _pid == rhs._pid &&
      _uid == rhs._uid &&
      _ip == rhs._ip) return 1;
  else return 0;
}

void Node::fixup(int ip, const Ether& ether) 
{
  _ip = ip; 
  _ether = ether;
}

int Node::user_name(int uid, char* buf, int bufsiz)
{
  const unsigned ressiz = 1024;
  char pwbuf[ressiz];
  struct passwd pw;
  struct passwd* ppw=&pw;
  int result = getpwuid_r(uid, &pw, pwbuf, ressiz, &ppw);
  if (result == 0) {
    strncpy(buf, pw.pw_name, bufsiz);
  } else {
    snprintf(buf, bufsiz, "%d", uid);
  }
  return result;
}

int Node::ip_name(int ip, char* buf, int bufsiz)
{
  struct in_addr ipaddr;
  ipaddr.s_addr = htonl(ip);
  struct hostent* host = 
    gethostbyaddr((const char*)&ipaddr, sizeof(ipaddr), AF_INET);
  if (host) {
    strncpy(buf, host->h_name, bufsiz);
    return 0;
  } else {
    snprintf(buf, bufsiz, "%d.%d.%d.%d", 
	     (ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);
    return 1;
  }
}
