#ifndef PDS_EVENTLEVEL_HH
#define PDS_EVENTLEVEL_HH

#include "pds/collection/CollectionManager.hh"

namespace Pds {

class EventCallback;
class EventStreams;
class Arp;

class EventLevel: public CollectionManager {
public:
  EventLevel(unsigned partition,
	     EventCallback& callback,
	     Arp* arp);
  virtual ~EventLevel();
  
  void attach();
  
private:
  // Implements CollectionManager
  virtual void message  (const Node& hdr, const Message& msg);
  virtual void connected(const Node& hdr, const Message& msg);
  virtual void timedout();
  virtual void disconnected();

private:
  Node           _dissolver;
  EventCallback& _callback;
  EventStreams*  _streams;
  int            _mcast;
  Ins            _segmentList[32];
  unsigned       _nextSegment;
  unsigned       _src;
};

}

#endif
