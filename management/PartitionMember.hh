#ifndef Pds_PartitionMember_hh
#define Pds_PartitionMember_hh

#include "pds/collection/CollectionManager.hh"

#include "pdsdata/xtc/Level.hh"
#include "pds/collection/Message.hh"
#include "pds/collection/Node.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/OutletWireInsList.hh"

namespace Pds {

  class Allocation;
  class Arp;
  class InDatagram;
  class Transition;

  class PartitionMember : public CollectionManager {
  public:
    PartitionMember(unsigned char platform,
		    Level::Type   level,
		    Arp*          arp);
    virtual ~PartitionMember();

    virtual bool     attach   () = 0;
    virtual void     detach   () = 0;
  public:
    virtual void     message  (const Node&    hdr,
			       const Message& msg);
  private:
    virtual Message& reply     (Message::Type) = 0;
    virtual void     allocated (const Allocation&,
			        unsigned);
    virtual void     dissolved ();
    virtual void     post      (const Transition&) = 0;
    virtual void     post      (const InDatagram&) = 0;
  protected:
    bool        _isallocated;
    Node        _allocator;
    GenericPool _pool;
    unsigned    _index;
    OutletWireInsList _rivals;        // list of nodes at this level
  };

}

#endif
