#include "CollectionPorts.hh"

static const int McastAddrCollection = 0xefff1000;
static const unsigned short PlatformPort   = 10000;
static const unsigned short CollectionPort = 10001;


using namespace Pds;

Ins PdsCollectionPorts::platform() 
{
  return Ins(McastAddrCollection, PlatformPort);
}

Ins PdsCollectionPorts::collection(unsigned char partition) 
{
  return Ins(McastAddrCollection | partition, CollectionPort);
}
