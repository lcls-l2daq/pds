#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <netinet/in.h>
#include <netdb.h>

#include "Node.hh"

using namespace Pds;

enum { Channel=0, Module=4, Trigger=7, Group=8, Transient=15 };
enum { Channel_mask = (1<<Module   )-(1<<Channel  ) };
enum { Module_mask  = (1<<Trigger  )-(1<<Module   ) };
enum { Trigger_mask = (1<<Group    )-(1<<Trigger  ) };
enum { Group_mask   = (1<<Transient)-(1<<Group    ) };
enum { Transient_mask= (1<<16)-(1<<Transient) };

Node::Node() : _procInfo(Level::Control,0,0) {}

Node::Node(const Node& rhs) :
  _platform (rhs._platform), 
  _group    (rhs._group),
  _uid      (rhs._uid),
  _procInfo (rhs._procInfo),
  _ether    (rhs._ether)
{}

Node::Node(Level::Type level, uint16_t platform) :
  _platform(platform),
  _group   (0),
  _uid(getuid()),
  _procInfo(level,getpid(),0)
{ memset(&_ether, 0, sizeof(_ether)); }

Level::Type Node::level () const {return _procInfo.level();}
unsigned Node::platform () const {return _platform;}
unsigned Node::group    () const {return (_group&Group_mask)>>Group;}
bool     Node::transient() const {return (_group&Transient_mask)>>Transient;}
bool     Node::triggered() const {return (_group&Trigger_mask);}
unsigned Node::evr_module () const { return (_group&Module_mask )>>Module;}
unsigned Node::evr_channel() const { return (_group&Channel_mask)>>Channel;}

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
  _group = (_group&~Group_mask) | ((group<<Group)&Group_mask);
}

void Node::setTransient(bool t)
{
  if (t)
    _group |= Transient_mask;		      
  else
    _group &= ~Transient_mask;
}

void Node::setTrigger(unsigned module,
		      unsigned channel)
{
  _group = (_group&~(Module_mask|Channel_mask)) |
    ((module <<Module )&Module_mask) |
    ((channel<<Channel)&Channel_mask) |
    Trigger_mask;
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
