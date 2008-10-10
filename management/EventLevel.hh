#ifndef PDS_EVENTLEVEL_HH
#define PDS_EVENTLEVEL_HH

#include "PartitionMember.hh"

namespace Pds {

class EventCallback;
class EventStreams;
class Arp;
class EbIStream;

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
  void     allocated (const Allocate&, unsigned);
  void     post      (const Transition&);
  void     post      (const InDatagram&);

private:
  EventCallback& _callback;         // object to notify
  EventStreams*  _streams;          // appliance streams
  EbIStream*     _inlet;
  Message        _reply;
};

}

#endif
