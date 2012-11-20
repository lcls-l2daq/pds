#include "SourceLevel.hh"
#include "Query.hh"

#include "pds/utility/StreamPorts.hh"
#include "pds/collection/CollectionPorts.hh"
#include "pds/collection/CollectionSource.hh"
#include "pds/collection/Message.hh"
#include "pds/collection/Node.hh"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

static const unsigned MaxPayload = sizeof(Pds::PartitionAllocation);
static Pds::Node       UnassignedNode(Pds::Level::Source,-1);
static Pds::Allocation UnassignedAlloc("Unassigned","Unassigned",-1);

namespace Pds {

  class Controller {
  public:
    Controller() :
      _node (UnassignedNode),
      _alloc(UnassignedAlloc),
      _time (::time(NULL))
    {}
    Controller(unsigned partitionid) :
      _node (UnassignedNode),
      _alloc(UnassignedAlloc.partition(),
	     UnassignedAlloc.dbpath(),
	     partitionid),
      _time (::time(NULL))
    {}
    Controller(const Node& node,
	       unsigned partitionid) :
      _node (node),
      _alloc(UnassignedAlloc.partition(),
	     UnassignedAlloc.dbpath(),
	     partitionid),
      _time (::time(NULL))
    {
      _alloc.add(node);
    }
    Controller(const Node& node,
	       const Allocation& alloc) :
      _node (node),
      _alloc(alloc),
      _time (::time(NULL))
    {}
  public:
    const Node& node() const { return _node; }
    const Allocation& allocation() const { return _alloc; }
    const time_t& time() const { return _time; }
  private:
    Node       _node;
    Allocation _alloc;
    time_t     _time;
  };

  class SourcePing : public Message {
  public:
    SourcePing(unsigned pleasereply) :
      Message(Message::Ping, sizeof(*this)),
      _payload(pleasereply)
    {}
  public:
    unsigned reply() const {return _payload;}
  private:
    unsigned _payload;
  };

};

using namespace Pds;


SourceLevel::SourceLevel() : 
  CollectionSource(MaxPayload, NULL),
  _control(new Controller[MaxPartitions()])
{
  for(unsigned k=0; k<SourceLevel::MaxPartitions(); k++)
    _control[k] = Controller(k);
}

SourceLevel::~SourceLevel() 
{
  delete[] _control;
}

unsigned SourceLevel::MaxPartitions()
{
  return StreamPorts::MaxPartitions;
}

bool SourceLevel::connect(int i)
{
  if (CollectionSource::connect(i)) {
    SourcePing sp(1);
    mcast(sp);
    return true;
  }
  return false;
}

void SourceLevel::_verify_partition(const Node& hdr, const Allocation& alloc)
{
  _control[alloc.partitionid()] = Controller(hdr,alloc);
}

void SourceLevel::_assign_partition(const Node& hdr, const Ins& dst)
{
#if 1
  partition = hdr.platform();
#else
  unsigned partition = MaxPartitions();
  for(unsigned p=0; p<MaxPartitions(); p++) {
    if (_control[p].node()==hdr) { partition = p; break; }
    if (_control[p].node()==UnassignedNode || 
	_control[p].allocation().nnodes()==0) { partition = p; }
  }
#endif
  if (partition == MaxPartitions()) 
    printf("*** warning: no partitions available for control %x/%d\n",
	   hdr.ip(), hdr.pid());
  else {
    _control[partition] = Controller(hdr,partition);
    PartitionGroup reply(partition);
    ucast(reply, dst);
  }
}

void SourceLevel::_resign_partition(const Node& hdr)
{
  for(unsigned k=0; k<MaxPartitions(); k++)
    if (_control[k].node()==hdr)
      _control[k] = Controller(k);
}

// void SourceLevel::_resign_partition(unsigned partitionid)
// {
//   _control[partitionid] = Controller(partitionid);
// }

void SourceLevel::_show_partition(unsigned partition, const Ins& dst)
{
  PartitionAllocation pa(_control[partition].allocation());
  ucast(pa, dst);
}

void SourceLevel::message(const Node& hdr, const Message& msg)
{
  if (hdr.level()==Level::Source) {
    const SourcePing& sp = reinterpret_cast<const SourcePing&>(msg);
    if (!(hdr == header())) {
      if (sp.reply()) {
	SourcePing reply(0);
	Ins dst = msg.reply_to();
	ucast(reply, dst);
      }
      in_addr ip; ip.s_addr = htonl(hdr.ip());
      printf("*** warning: another source level running on %s pid %d\n",
	     inet_ntoa(ip), hdr.pid());
      exit(1);
    }
  }
  else {
    if (msg.type() == Message::Ping) {
      Message reply(msg.type());
      Ins dst = msg.reply_to();
      ucast(reply, dst);
    }
    else if (hdr.level() == Level::Control) {
      if (msg.type () == Message::Join)
	_assign_partition(hdr, msg.reply_to());
      else if (msg.type () == Message::Resign)
	_resign_partition(hdr);
      else if (msg.type() == Message::Query) {
	const Query& query = reinterpret_cast<const Query&>(msg);
	if (query.type() == Query::Partition) {
	  _verify_partition(hdr, reinterpret_cast<const PartitionAllocation&>
			    (query).allocation());
	}
      }
      printf("Request %s from {%s, platform %d, group %d, pid %d, uid %d, ip %08x}\n",
             msg.type_name(), 
             Level::name(hdr.level()), 
             hdr.platform(), hdr.group(), 
             hdr.pid(), hdr.uid(), hdr.ip());
      dump();
    }
    else if (msg.type() == Message::Query) {
      const Query& query = reinterpret_cast<const Query&>(msg);
      if (query.type() == Query::Partition) {
	_show_partition(reinterpret_cast<const PartitionAllocation&>
			(query).allocation().partitionid(),
			msg.reply_to());
      }
    }
  }
}

void SourceLevel::dump() const
{
  printf("       When           | Platform | Partition  |       Node             \n"
	 "     Allocated        |          | id/name    | level/ pid /     ip    \n"
	 "----------------------+----------+------------+------------------------\n");
  for(unsigned k=0; k<MaxPartitions(); k++) {
    const Node&       n = _control[k].node();
    const Allocation& a = _control[k].allocation();
    if (!(n==UnassignedNode)) {
      char* ts = ctime(&_control[k].time());
      printf("%*s%s     %03d      %02d/%7s", 
	     int(21-strlen(ts))," ",ts, n.platform(), a.partitionid(), a.partition());
      if (!a.nnodes())
	printf("\n");
      else
	for(unsigned m=0; m<a.nnodes(); m++) {
	  const Node& p=*a.node(m);
	  unsigned ip = p.ip();
	  printf("%s       %1d/%05d/%d.%d.%d.%d\n", 
		 m==0 ? "":"                                             ",
		 p.level(), p.pid(),
		 (ip>>24), (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);
	}
    }
  }
}
