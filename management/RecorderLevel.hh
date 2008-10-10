#ifndef PDS_EVENTLEVEL_HH
#define PDS_EVENTLEVEL_HH

#include "PartitionMember.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/OutletWireInsList.hh"

namespace Pds {

class EventCallback;
class EventStreams;
class Arp;

class RecorderLevel: public PartitionMember {
public:
  RecorderLevel(unsigned partition,
		EventCallback& callback,
		Arp* arp);
  virtual ~RecorderLevel();
  
  bool attach();
  void detach();
private:  
  // Implements PartitionMember
  Message& reply     (Message::Type);
  void     allocated (const Allocate&, unsigned);
  void     post      (const Transition&);
  void     post      (const InDatagram&);

private:
  EventCallback& _callback;         // object to notify
  EventStreams*  _streams;          // appliance streams
  Message        _reply;
};

}

#endif
