#include "PartitionMember.hh"

#include "pds/utility/Transition.hh"
#include "pds/xtc/CDatagram.hh"

using namespace Pds;

static const unsigned MaxPayload = sizeof(Allocate);
static const unsigned ConnectTimeOut = 250; // 1/4 second

PartitionMember::PartitionMember(unsigned char platform,
				   Level::Type   level,
				   Arp*          arp) :
  CollectionManager(level, platform, MaxPayload, ConnectTimeOut, arp),
  _isallocated     (false),
  _pool            (MaxPayload,4),
  _index           (-1UL)
{
}

PartitionMember::~PartitionMember() 
{
}

void PartitionMember::message(const Node& hdr, const Message& msg) 
{
  if (hdr.level() == Level::Control) {
    switch (msg.type()) {
    case Message::Ping:
    case Message::Join:
      if (header().level()!=Level::Control) {
	arpadd(hdr);
        Ins dst = msg.reply_to();
        ucast(reply(msg.type()), dst);
      }
      break;
    case Message::Transition:
      {
	bool lpost = false;

        const Transition& tr = reinterpret_cast<const Transition&>(msg);
	if (tr.phase() == Transition::Execute) {
	  switch (tr.id()) {
	  case TransitionId::Map:
	    {
	      if (_isallocated) break;
	      const Allocate& alloc = reinterpret_cast<const Allocate&>(tr);
	      unsigned    nnodes = alloc.nnodes();
	      unsigned    index  = 0;
	      _rivals.flush();
	      for (unsigned n=0; n<nnodes; n++) {
		const Node* node = alloc.node(n);
		printf("node %x %d %d\n", node->ip(), node->uid(), node->pid());
		if (header() == *node) {
		  printf("node now in partition %s\n", alloc.partition());
		  _isallocated = true;
		  _allocator = hdr;
		  allocated( alloc, _index=index );
		}
		if (node->level() == header().level()) {
		  _rivals.insert(index, msg.reply_to());
		  index++;
		}
	      }
	    }
	    lpost = true;
	    break;
	  case TransitionId::Unmap:
	    {
	      const Kill& kill = reinterpret_cast<const Kill&>(tr);
	      if (_isallocated && kill.allocator() == _allocator) {
		printf("node removed from partition\n");
		_isallocated = false;
		dissolved();
		lpost = true;
	      }
	    }
	    break;
	  default:
	    lpost = (_isallocated && hdr == _allocator);
	    break;
	  }
	}
	else
	  lpost = (_isallocated && hdr == _allocator);

	if (lpost) {

	  if (tr.id() != TransitionId::L1Accept) {
	    arpadd(hdr);
	    OutletWireIns* dst;
	    if (tr.phase() == Transition::Execute) {
	      printf("PartitionMember::message inserting transition\n");
	      Transition* ntr = new(&_pool) Transition(tr);
	      post(*ntr);
	    }
	    //  Initiate a datagram for all segment levels and any others
	    //  which are not targetted by the timestamp vectoring.
	    else if (header().level()==Level::Segment || 
		     ((dst=_rivals.lookup(tr.sequence())) &&
		      dst->id() != _index)) {
	      printf("PartitionMember::message inserting datagram\n");
	      CDatagram* ndg = 
		new(&_pool) CDatagram(Datagram(tr, 
					       TypeId(TypeNum::Any),
					       Src(header())));
	      post(*ndg);
	    }
	  }
	}
      }
      break;
    default:
      break;
    }
  }
}


void    PartitionMember::allocated(const Allocate& allocate,
				   unsigned        index) 
{
}

void    PartitionMember::dissolved()
{
}

