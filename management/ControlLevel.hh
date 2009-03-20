#ifndef PDS_CONTROLLEVEL_HH
#define PDS_CONTROLLEVEL_HH

#include "PartitionMember.hh"
#include "Query.hh"

namespace Pds {

  class ControlCallback;
  class ControlStreams;
  class Partition;
  class Arp;

  class ControlLevel: public PartitionMember {
  public:
    ControlLevel(unsigned platform,
		 ControlCallback& callback,
		 Arp* arp);
    virtual ~ControlLevel();

    unsigned partitionid() const;

    bool attach();
    void detach();

    virtual void message(const Node&, const Message&);

    /***  void reboot(); ***/
  private:
    /***  void check_complete(const Node& hdr, bool isjoining); ***/
    
  private:
    // Implements PartitionMember
    Message& reply     (Message::Type);
    void     allocated (const Allocation&, unsigned index);
    void     dissolved ();
    void     post      (const Transition&);
    void     post      (const InDatagram&);
    
  private:
    int                 _reason;
    ControlCallback&    _callback;
    ControlStreams*     _streams;
    Message             _reply;
    unsigned            _partitionid;
    Allocation          _allocation;
  };
  
}
#endif
