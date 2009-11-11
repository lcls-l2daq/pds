#ifndef PDS_EVENTLEVEL_HH
#define PDS_EVENTLEVEL_HH

#include "PartitionMember.hh"

namespace Pds {

class EventCallback;
class WiredStreams;
class Arp;

class EventLevel: public PartitionMember {
public:
  EventLevel(unsigned       platform,
	     EventCallback& callback,
	     Arp*           arp);
  virtual ~EventLevel();
  
  bool attach();
  void detach();
private:
  // Implements PartitionMember
  Message& reply     (Message::Type);
  void     allocated (const Allocation&, unsigned);
  void     dissolved ();
  void     post      (const Transition&);
  void     post      (const Occurrence&);
  void     post      (const InDatagram&);

private:
  EventCallback& _callback;         // object to notify
  WiredStreams*  _streams;          // appliance streams
  Message        _reply;
};

}

#endif
