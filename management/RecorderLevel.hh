#ifndef PDS_EVENTLEVEL_HH
#define PDS_EVENTLEVEL_HH

#include "pds/collection/CollectionManager.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/OutletWireInsList.hh"

namespace Pds {

class EventCallback;
class EventStreams;
class Arp;

class RecorderLevel: public CollectionManager {
public:
  RecorderLevel(unsigned partition,
		unsigned index,
		EventCallback& callback,
		Arp* arp);
  virtual ~RecorderLevel();
  
  void attach();
  
private:
  // Implements CollectionManager
  virtual void message  (const Node& hdr, const Message& msg);

private:
  Node           _dissolver;        // source of resign control message
  int            _index;            // partition-wide vectoring index for receiving event data
  EventCallback& _callback;         // object to notify
  EventStreams*  _streams;          // appliance streams
  GenericPool    _pool;
  OutletWireInsList _rivals;        // list of nodes at this level
};

}

#endif
