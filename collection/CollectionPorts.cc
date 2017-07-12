#include "CollectionPorts.hh"

static const int McastAddrCollection = 0xeffe1000;
static const unsigned short PlatformPort   = 12000;
static const unsigned short CollectionPort = 12001;
static const unsigned short TagServerPort  = 12002;


Pds::Ins Pds::CollectionPorts::platform() 
{
  return Ins(McastAddrCollection, PlatformPort);
}

Pds::Ins Pds::CollectionPorts::collection(unsigned char platform) 
{
  return Ins(McastAddrCollection | platform, CollectionPort);
}

Pds::Ins Pds::CollectionPorts::tagserver (unsigned char platform) 
{
  return Ins(TagServerPort);
}
