#ifndef Pds_CollectionObserver_hh
#define Pds_CollectionObserver_hh

#include "pds/collection/CollectionManager.hh"

#include "pds/collection/Node.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {

  class Allocation;
  class Transition;
  class InDatagram;

  class CollectionObserver : public CollectionManager {
  public:
    CollectionObserver(unsigned char platform,
		       const char*   partition,
		       unsigned      node);
    ~CollectionObserver();
  public:
    virtual void allocated(const Allocation&,
			   unsigned index) = 0;
    virtual void dissolved() = 0;
  public:
    virtual void post(const Transition&) = 0;
    virtual void post(const InDatagram&) = 0;
  private:
    void message(const Node& hdr, const Message& msg);
  private:
    const char* _partition;
    unsigned    _node;
    GenericPool _pool;
    bool        _isallocated;
    Node        _allocator;
  };

};

#endif
