#include "PartitionMember.hh"

#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/collection/CollectionPorts.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"

#include <poll.h>

using namespace Pds;

//  Size the transition pool reasonably large enough for configure
static const unsigned MaxPayload = 0x1000000;  // 16MB
static const unsigned ConnectTimeOut = 250; // 1/4 second

PartitionMember::PartitionMember(unsigned char platform,
                                 Level::Type   level) :
  CollectionManager(level, platform, MaxPayload, ConnectTimeOut),
  _isallocated     (false),
  _pool            (MaxPayload,16),
  _index           ((unsigned)-1),
  _contains        (_xtcType)
{
}

PartitionMember::~PartitionMember()
{
}

const Ins& PartitionMember::occurrences() const { return _occurrences; }

void PartitionMember::message(const Node& hdr, const Message& msg)
{
  if (hdr.level() == Level::Control) {
    switch (msg.type()) {
    case Message::Ping:
    case Message::Join:
    case Message::Alias:
      if (header().level()!=Level::Control) {
        arpadd(hdr);
        Ins dst = msg.reply_to();
        Message& rply = reply(msg.type());
        ucast(rply, dst);
      }
      break;
    case Message::Transition:
      {
        bool lpost = false;
        bool lkill = false;

        const Transition& tr = reinterpret_cast<const Transition&>(msg);
        if (tr.phase() == Transition::Execute &&
            tr.id   () == TransitionId::Map) {
          if (_isallocated) break;
          const Allocation& alloc =
            reinterpret_cast<const Allocate&>(tr).allocation();
          unsigned    nnodes = alloc.nnodes();
          unsigned    index  = 0;
          for (unsigned n=0; n<nnodes; n++) {
            const Node* node = alloc.node(n);
            if (node->level() == header().level()) {
              if (header() == *node) {
                _isallocated = true;
                _allocator = hdr;
                _occurrences = msg.reply_to();
                allocated( alloc, _index=index );
                lpost = true;
              }
              index++;
            }
          }
        }
        else if (tr.phase() == Transition::Execute &&
                 tr.id   () == TransitionId::Unmap) {
          const Kill& kill = reinterpret_cast<const Kill&>(tr);
          if (_isallocated && kill.allocator() == _allocator) {
            lkill = true;
            lpost = true;
          }
        }
        else
          lpost = (_isallocated && hdr == _allocator);

        if (lpost) {
          //    if (tr.id() != TransitionId::L1Accept) {
          arpadd(hdr);
          if (tr.phase() == Transition::Execute) {
            Transition* ntr = new(&_pool) Transition(tr);
            post(*ntr);
          }
          //  Initiate a datagram for all segment levels.
          else if (header().level()==Level::Segment) {
            CDatagram* ndg = new(&_pool) CDatagram(Datagram(tr,_contains,header().procInfo()));
            if (tr.size() > sizeof(Transition) && _index==0) {
              const Xtc& tc = *reinterpret_cast<const Xtc*>(&tr+1);
              ndg->insert(tc,tc.payload());
            }
            post(*ndg);
          }
          //    }
        }
        if (lkill) {
          _isallocated = false;
          dissolved();
        }
        break;
      }
    default:
      break;
    }
  }
  if (msg.type()==Message::Occurrence && _isallocated) {
    Occurrence* occ = new(&_pool) Occurrence(reinterpret_cast<const Occurrence&>(msg));
    post(*occ);
  }
}


void    PartitionMember::allocated(const Allocation& allocate,
                                   unsigned          index)
{
}

void    PartitionMember::dissolved()
{
}

void    PartitionMember::confirm(int group)
{
  Ins dst(group, StreamPorts::sink().portId());
  NetServer srv(0, dst, 0, sizeof(Message));

  Message msg(Message::Ping);
  msg.reply_to(dst);

  pollfd pfd;
  pfd.fd = srv.fd();
  pfd.events = POLLIN;

  char buff[sizeof(Message)];

  while(1) {
    // request source-level to reply to Ins
    printf("Sending confirm request for %x.%d to %x.%d\n",
           dst.address(), dst.portId(),
           CollectionPorts::platform().address(),
           CollectionPorts::platform().portId());
    _send(msg, CollectionPorts::platform());

    int r = ::poll(&pfd,1,100);
    // wait for srv to receive reply
    if (r==0) {
      continue;
    }
    if (r<0) {
      perror("Confirm fail\n");
      continue;
    }

    if (srv.fetch(buff,0)==sizeof(Message)) {
      printf("Confirmed\n");
      return;
    }
  }
}
