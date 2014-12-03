#include "pds/ioc/IocNode.hh"
#include "pdsdata/xtc/Level.hh"
#include<netdb.h>
#include<arpa/inet.h>
#include <stdlib.h>

class MyInfo : public Pds::Src {
public:
  MyInfo(unsigned phy) : Src(Pds::Level::Source)
  {_phy=phy;}
};

using namespace Pds;

IocNode::IocNode(std::string host_and_port, std::string config, std::string alias,
                 std::string detector, std::string device, std::string pvname,
                 std::string flags) :
  _config   (config + "\n"),
  _alias    (alias),
  _detector (detector),
  _device   (device),
  _pvname   (pvname),
  _flags    (flags)
{
    size_t pos;

    if ((pos = host_and_port.find(':')) == std::string::npos) {
        _host = host_and_port;
        _port = 12350;
    } else {
        _host = host_and_port.substr(0, pos);
        _port = atoi(host_and_port.substr(pos + 1, std::string::npos).c_str());
    }

    struct hostent *h = gethostbyname(_host.c_str());
    if (h && h->h_addrtype == AF_INET)
        _host_ip = ntohl(*(uint32_t *)h->h_addr_list[0]);
    else
        _host_ip = 0;

    std::string name = detector + "|" + device;
    new(&_src[0]) DetInfo(name.c_str());

    _conn = NULL;
}

Node IocNode::node() const
{
  Node n(Level::Segment,src().phy());
  n.fixup(_host_ip,Ether());
  return n;
}

const Src& IocNode::src() const
{
  return *reinterpret_cast<const Src*>(&_src[0]);
}

const char* IocNode::alias() const
{
  return _alias.c_str();
}

void IocNode::addPV(std::string alias, std::string line)
{
    _extra_aliases.push_back(alias);
    _extra_lines.push_back(line);
}

void IocNode::write_config(IocConnection *c)
{
    _conn = c;
    _idx = c->getIndex();
    c->transmit(_config);
    c->transmit("record " + _alias + "\n");

    for (unsigned int i = 0; i < _extra_aliases.size(); i++) {
        c->transmit(_extra_lines[i] + "\n");
        c->transmit("record " + _extra_aliases[i] + "\n");
    }
}
