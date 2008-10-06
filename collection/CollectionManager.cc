#include "CollectionManager.hh"
#include "CollectionPorts.hh"
#include "Message.hh"
#include "Route.hh"

#include <string.h>
#include <stdio.h>
#include <errno.h>

using namespace Pds;

CollectionManager::CollectionManager(Level::Type level,
                                     unsigned platform,
                                     unsigned maxpayload,
                                     unsigned timeout,
                                     Arp* arp) :
  CollectionServerManager(CollectionPorts::collection(platform), 
                          level, platform, maxpayload, timeout, arp),
  _sem(Semaphore::EMPTY)
{}

CollectionManager::~CollectionManager() {}

bool CollectionManager::connect(int interface) 
{
  Message msg(Message::Ping);
  Node header(_header);
  Ins dst = CollectionPorts::platform();
  if (interface) {
    Route::set(_table, interface);
    _connected();
  } else {
    // Look for this platform interface by sending a message on all intefaces
    for (unsigned index=0; index<_table.routes(); index++) {
      int interface = _table.interface(index);
      header.fixup(interface, _table.ether(index));
      msg.reply_to(Ins(interface, portId()));
      _client.use(interface);
      _send(msg, dst, header);
    }
    _sem.take();
  }
  return (_ucastServer != 0);
}

int CollectionManager::commit(char* datagram,
                              char* payload,
                              int   payloadSize,
                              const Ins& src) 
{
  if (!payloadSize) {
    _disconnected();
    return 0;
  } else {
    const Node& hdr = *(Node*)datagram;
//     const Message& msg = *(Message*)payload;
//     printf("<%s> (oob) from level %d platform %d ip 0x%x pid %d\n",
//            msg.type_name(), hdr.level(), hdr.platform(), hdr.ip(), hdr.pid());
    Route::set(_table, hdr.ip());
    donottimeout();
    _connected();
    _sem.give();
    return 1;
  }
}

int CollectionManager::processTmo() 
{
  arm(); // Otherwise only OOB server would be re-enabled
  _sem.give();
  return 1;
}
