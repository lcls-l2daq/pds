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
    virtual Message& reply     (Message::Type);
    virtual void     allocated (const Allocation&, unsigned index);
    virtual void     dissolved ();
    virtual void     post      (const Transition&);
    virtual void     post      (const Occurrence&);
    virtual void     post      (const InDatagram&);
    
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
