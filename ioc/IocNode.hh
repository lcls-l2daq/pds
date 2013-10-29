#ifndef Pds_IocNode_hh
#define Pds_IocNode_hh

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <string>

namespace Pds {
  class Src;

  class IocNode {
  public:
    IocNode(unsigned host_ip, 
	    unsigned host_port,
	    unsigned phy_id,
	    std::string alias);
  public:
    Node        node () const;
    const Src&  src  () const;
    const char* alias() const;
  private:
    uint32_t    _host_ip;
    uint32_t    _host_port;
    uint32_t    _src[2];
    std::string _alias;
  };

};

#endif
