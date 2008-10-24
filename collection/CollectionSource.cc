#include "CollectionSource.hh"
#include "CollectionPorts.hh"
#include "Message.hh"
#include "Route.hh"

#include <string.h>
#include <stdio.h>
#include <errno.h>

using namespace Pds;

static const unsigned NoPlatform = 0;
static const unsigned NoTmo = 0;

static const int Connect = 2;

CollectionSource::CollectionSource(unsigned maxpayload, Arp* arp) :
  CollectionServerManager(CollectionPorts::platform(), 
                          Level::Source, NoPlatform, maxpayload, NoTmo, arp),
  _sem(Semaphore::EMPTY)
{
  donottimeout();
}

CollectionSource::~CollectionSource() {}

bool CollectionSource::connect(int interface) 
{
  Route::set(_table, interface);
  interface = Route::interface();
  if (interface) {
    unblock((char*)&Connect);
    _sem.take();
    return true;
  } else {
    return false;
  }
}

int CollectionSource::commit(char* datagram,
                             char* payload,
                             int   payloadSize,
                             const Ins& src) 
{
  if (!payloadSize) {
    int request = *(int*)datagram;
    if (request == Connect) {
      _connected();
      _sem.give();
      return 1;
    }
    return 0;
  } else {
//     const Node& hdr = *(Node*)datagram;
//     const Message& msg = *(Message*)payload;
//     printf("<%s> (oob) from level %d platform %d ip 0x%x pid %d\n",
//            msg.type_name(), hdr.level(), hdr.platform(), hdr.ip(), hdr.pid());
    return 1;
  }
}

int CollectionSource::processTmo() 
{
  // Should never be called (donottimeout)
  return 0;
}
