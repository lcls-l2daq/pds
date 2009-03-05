#ifndef PDS_ObserverLevel_HH
#define PDS_ObserverLevel_HH

#include "PartitionMember.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/OutletWireInsList.hh"

namespace Pds {

class EventCallback;
class EventStreams;
class Arp;

class ObserverLevel: public PartitionMember {
public:
  ObserverLevel(unsigned partition,
		EventCallback& callback,
		Arp* arp);
  virtual ~ObserverLevel();
  
  bool attach();
  void detach();
private:  
  // Implements PartitionMember
  Message& reply     (Message::Type);
  void     allocated (const Allocate&, unsigned);
  void     dissolved ();
  void     post      (const Transition&);
  void     post      (const InDatagram&);

private:
  EventCallback& _callback;         // object to notify
  EventStreams*  _streams;          // appliance streams
  Message        _reply;
};

}

#endif
