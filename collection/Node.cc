#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <netinet/in.h>
#include <netdb.h>

#include "Node.hh"

using namespace Pds;

enum { Transient = 0x8000 };

Node::Node() : _procInfo(Level::Control,0,0) {}

Node::Node(const Node& rhs) :
  _platform (rhs._platform), 
  _group    (rhs._group),
  _uid      (rhs._uid),
  _procInfo (rhs._procInfo)
{}

Node::Node(Level::Type level, uint16_t platform) :
  _platform(platform),
  _group   (0),
  _uid(getuid()),
  _procInfo(level,getpid(),0)
{}

Level::Type Node::level () const {return _procInfo.level();}
unsigned Node::platform () const {return _platform;}
unsigned Node::group    () const {return _group&~Transient;}
bool     Node::transient() const {return _group&Transient; }
int Node::pid() const {return _procInfo.processId();}
int Node::uid() const {return _uid;}
int Node::ip() const {return _procInfo.ipAddr();}
const Ether& Node::ether() const {return _ether;}

int Node::operator == (const Node& rhs) const
{
  // note: don't compare group here, because group can be dynamically changed
  if (_platform == rhs._platform &&
      _uid == rhs._uid &&
      _procInfo == rhs._procInfo) return 1;
  else return 0;
}

void Node::setGroup(uint16_t group)
{
  _group = (_group&Transient) | (group&~Transient);
}

void Node::setTransient(bool t)
{
  if (t)
    _group |= Transient;		      
  else
    _group &= ~Transient;
}

void Node::fixup(int ip, const Ether& ether) 
{
  _procInfo.ipAddr(ip);
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

const ProcInfo& Node::procInfo() const {
  return _procInfo;
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
