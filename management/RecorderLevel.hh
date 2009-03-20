#ifndef PDS_RecorderLevel_HH
#define PDS_RecorderLevel_HH

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
  void     allocated (const Allocation&, unsigned);
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
