#include "pds/ioc/IocNode.hh"
#include "pdsdata/xtc/Level.hh"

class MyInfo : public Pds::Src {
public:
  MyInfo(unsigned phy) : Src(Pds::Level::Source)
  {_phy=phy;}
};

using namespace Pds;

IocNode::IocNode(unsigned host_ip,
		 unsigned host_port,
		 unsigned phy_id,
		 std::string alias) :
  _host_ip  (host_ip),
  _host_port(host_port),
  _alias    (alias)
{
  new(&_src[0]) MyInfo(phy_id);
}

Node IocNode::node() const
{
  Node n(Level::Segment,0);
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
