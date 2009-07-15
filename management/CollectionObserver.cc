#include "CollectionObserver.hh"

#include "pds/utility/Transition.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"

#include <string.h>

using namespace Pds;

static const unsigned MaxPayload = sizeof(Allocate);
static const unsigned ConnectTimeOut = 250; // 1/4 second

CollectionObserver::CollectionObserver(unsigned char platform,
				       const char*   partition,
				       unsigned      node) :
  CollectionManager(Level::Observer, platform, MaxPayload, ConnectTimeOut, NULL),
  _partition       (partition),
  _node            (node),
  _pool            (MaxPayload,4),
  _isallocated     (false)
{
}

CollectionObserver::~CollectionObserver()
{
}

void CollectionObserver::message(const Node& hdr, const Message& msg)
{
  if (hdr.level() == Level::Control) {
    if (msg.type()==Message::Transition) {
      const Transition& tr = reinterpret_cast<const Transition&>(msg);
      if (tr.phase() == Transition::Execute) {
	if (tr.id() == TransitionId::Map) {
	  const Allocation& alloc = 
	    reinterpret_cast<const Allocate&>(tr).allocation();
	  if (strcmp(alloc.partition(),_partition))
	    return;
	  if (_isallocated) {
	    printf("CollectionObserver found allocate without dissolve for partition %s\n"
		   "Dissolving...\n", _partition);
	    dissolved();
	  }
	  _isallocated = true;
	  _allocator   = hdr;
	  allocated( alloc, _node );
	}
	else if (!(_isallocated && hdr==_allocator))
	  return;
	
	Transition* ntr = new(&_pool) Transition(tr);
	post(*ntr);

	if (tr.id() == TransitionId::Unmap) {
	  _isallocated = false;
	  dissolved();
	}
      }
    }
  }
}
