/*
 * ProxyMsg.hh
 *
 *  Created on: Sep 5, 2009
 *      Author: jackp
 */

#ifndef PROXYMSG_HH_
#define PROXYMSG_HH_

#include <netinet/in.h>
#include <arpa/inet.h>
#include "pdsdata/xtc/Src.hh"

struct mcaddress {
    in_addr_t mcaddr;
    uint32_t  mcport;
};

namespace RceFBld {

  class ProxyMsg {
    public:
      ProxyMsg();
      ~ProxyMsg();

    public:
      enum {MaxEventLevelServers=64, ProxyPort=5000};
      bool        byteOrderIsBigEndian;
      uint32_t    numberOfEventLevels;
      mcaddress   mcAddrs[MaxEventLevelServers];
      mcaddress   evrMcAddr;
      uint32_t    payloadSizePerLink;
      uint32_t    numberOfLinks;
      Pds::Src    procInfoSrc;
      Pds::Src    detInfoSrc;
      Pds::TypeId contains;
      in_addr_t   proxyAddress;
      uint32_t    proxyPort;
  };

  class ProxyReplyMsg {

    public:
      ProxyReplyMsg();
      ~ProxyReplyMsg();

    public:
      Pds::Damage   damage;
  };

}

#endif /* PROXYMSG_HH_ */
