#ifndef PDS_EVENTCALLBACK_HH
#define PDS_EVENTCALLBACK_HH

namespace Pds {

class Node;
class SetOfStreams;

class EventCallback {
public:
  enum Reason {PlatformUnavailable, Reasons};

  virtual ~EventCallback() {}

  virtual void attached (SetOfStreams& streams) = 0;
  virtual void failed   (Reason reason) = 0;
  virtual void dissolved(const Node& who) = 0;
};

}
#endif
