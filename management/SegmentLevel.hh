#ifndef ODFSEGMENTLEVEL_HH
#define ODFSEGMENTLEVEL_HH

#include "pds/collection/CollectionManager.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {

class SegWireSettings;
class SegmentOptions;
class SegStreams;
class Arp;
class Node;
class EventCallback;
class EbIStream;

class SegmentLevel: public CollectionManager {
public:
  SegmentLevel(unsigned partition,
	       int      index,
	       SegWireSettings& settings,
	       EventCallback& callback,
	       Arp* arp);

  virtual ~SegmentLevel();

  void attach();

public:
  // Implements CollectionManager
  virtual void message(const Node& hdr, const Message& msg);

private:
  SegWireSettings&      _settings;
  Node           _dissolver;
  int            _index;
  EventCallback& _callback;
  SegStreams*    _streams;
  GenericPool    _pool;
  EbIStream*     _inlet;
};

}
#endif
