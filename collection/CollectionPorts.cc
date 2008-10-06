#include "CollectionPorts.hh"

static const int McastAddrCollection = 0xefff1000;
static const unsigned short PlatformPort   = 10000;
static const unsigned short CollectionPort = 10001;


Pds::Ins Pds::CollectionPorts::platform() 
{
  return Ins(McastAddrCollection, PlatformPort);
}

Pds::Ins Pds::CollectionPorts::collection(unsigned char platform) 
{
  return Ins(McastAddrCollection | platform, CollectionPort);
}
