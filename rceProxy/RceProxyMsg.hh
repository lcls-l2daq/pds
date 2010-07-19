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
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Damage.hh"

struct mcaddress {
    in_addr_t mcaddr;
    uint32_t  mcport;
};

namespace RceFBld {

  class ProxyMsg {
    public:
      ProxyMsg() : state(UnMapped) {};
      ~ProxyMsg() {};

    public:
      enum {MaxEventLevelServers=64, ProxyPort=5000};
      enum {ConfigChunkSize=(1<<13)};
      enum {UnMapped, Mapped};
      uint32_t    state;
      uint32_t    byteOrderIsBigEndian;
      uint32_t    numberOfEventLevels;
      mcaddress   mcAddrs[MaxEventLevelServers];
      mcaddress   evrMcAddr;
      Pds::Src    procInfoSrc;
      Pds::Src    detInfoSrc;
      Pds::TypeId contains;
      uint32_t    maxTrafficShapingWidth;
      uint32_t    trafficShapingInitialPhase;
  };

  class ProxyReplyMsg {

    public:
      ProxyReplyMsg() : type(Done), damage(Pds::Damage(0)) {};
      ~ProxyReplyMsg() {};

      enum replyType {Done, SendConfig};
      void setType(replyType t) {type = t;}

    public:
      replyType     type;
      Pds::Damage   damage;
  };

}

#endif /* PROXYMSG_HH_ */
