#ifndef Pds_IocNode_hh
#define Pds_IocNode_hh

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/ioc/IocConnection.hh"

#include <string>

namespace Pds {
  class Src;
  class IocControl;

  class IocNode {
  public:
    IocNode(std::string host_and_port,
            std::string config,
            std::string alias,
            std::string detector,
            std::string device,
            std::string pvname,
            std::string flags);
  public:
    Node        node () const;
    const Src&  src  () const;
    const char* alias() const;
    IocConnection *get_connection(IocControl *ctrl) { 
        return IocConnection::get_connection(_host, _host_ip, _port, ctrl); 
    };
    void write_config(IocConnection *c);
  private:
    std::string _host;
    std::string _config;
    uint16_t    _port;
    std::string _alias;
    std::string _detector;
    std::string _device;
    std::string _pvname;
    std::string _flags;

    uint32_t    _host_ip;
    uint32_t    _src[2];
  };

};

#endif
