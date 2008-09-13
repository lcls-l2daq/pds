#ifndef ODFSEGMENTLEVEL_HH
#define ODFSEGMENTLEVEL_HH

#include "pds/collection/CollectionManager.hh"

namespace Pds {

class SegmentOptions;
class SegWireSettings;
class SegStreams;
class Arp;
class Node;
class EventCallback;

class SegmentLevel: public CollectionManager {
public:
  SegmentLevel(unsigned partition,
	       const SegmentOptions& options,
	       SegWireSettings& settings,
	       EventCallback& callback,
	       Arp* arp);

  virtual ~SegmentLevel();

  void attach();

private:
  // Implements CollectionManager
  virtual void message     (const Node& hdr, const Message& msg);
  virtual void connected   (const Node& hdr, const Message& msg);
  virtual void timedout    ();
  virtual void disconnected();

private:
  const SegmentOptions& _options;
  SegWireSettings&      _settings;
  Node           _dissolver;
  EventCallback& _callback;
  SegStreams*    _streams;
  char*          _transition;
  unsigned       _dst;
};

}
#endif
