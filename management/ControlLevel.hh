#ifndef PDS_CONTROLLEVEL_HH
#define PDS_CONTROLLEVEL_HH

#include "pds/collection/CollectionManager.hh"

namespace Pds {

  class ControlCallback;
  class ControlStreams;
  class Partition;
  class Arp;
  class StreamPortAssignment;

  class ControlLevel: public CollectionManager {
  public:
    ControlLevel(unsigned partition,
		 ControlCallback& callback,
		 Arp* arp);
    virtual ~ControlLevel();
    
    void attach();
    
    void add_bld(int id);
    
    /***  void reboot(); ***/
  private:
    /***  void check_complete(const Node& hdr, bool isjoining); ***/
    
  private:
    // Implements CollectionManager
    virtual void message     (const Node& hdr, const Message& msg);
    virtual void connected   (const Node& hdr, const Message& msg);
    virtual void timedout    ();
    virtual void disconnected();
    
  private:
    int _reason;
    Node _dissolver;
    ControlCallback& _callback;
    ControlStreams* _streams;
    char* _buffer;
    StreamPortAssignment* _bldServers;
  };
  
}
#endif
