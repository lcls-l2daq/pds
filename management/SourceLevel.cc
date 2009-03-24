#include "SourceLevel.hh"
#include "Query.hh"

#include "pds/collection/CollectionPorts.hh"
#include "pds/collection/CollectionSource.hh"
#include "pds/collection/Message.hh"
#include "pds/collection/Node.hh"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

static const unsigned MaxPayload = sizeof(Pds::PartitionAllocation);
static Pds::Node Unassigned(Pds::Level::Source,-1);

namespace Pds {

  class Controller {
  public:
    Controller() :
      _node(Unassigned)
    {}
    Controller(const Node& node,
	       const Allocation& alloc) :
      _node (node),
      _alloc(alloc)
    {}
  public:
    const Node& node() const { return _node; }
    const Allocation& allocation() const { return _alloc; }
  private:
    Node       _node;
    Allocation _alloc;
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
  _control(new Controller[MaxPartitions])
{
}

SourceLevel::~SourceLevel() 
{
  delete[] _control;
}

bool SourceLevel::connect(int i)
{
  bool v = CollectionSource::connect(i);
  SourcePing sp(1);
  mcast(sp);
  return v;
}

void SourceLevel::_verify_partition(const Node& hdr, const Allocation& alloc)
{
  _control[alloc.partitionid()] = Controller(hdr,alloc);
}

void SourceLevel::_assign_partition(const Node& hdr, const Ins& dst)
{
  unsigned partition = MaxPartitions;
  for(unsigned p=0; p<MaxPartitions; p++) {
    if (_control[p].node()==hdr) { partition = p; break; }
    if (_control[p].node()==Unassigned) { partition = p; }
  }
  if (partition == MaxPartitions) 
    printf("*** warning: no partitions available for control %x/%d\n",
	   hdr.ip(), hdr.pid());
  else {
    _control[partition] = Controller(hdr,Allocation());
    PartitionGroup reply(partition);
    ucast(reply, dst);
  }
}

void SourceLevel::_resign_partition(const Node& hdr)
{
  for(unsigned k=0; k<MaxPartitions; k++)
    if (_control[k].node()==hdr)
      _control[k] = Controller(Unassigned,Allocation());
}

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
  printf("            Control   | Platform | Partition  |   Node\n"
	 "        ip     / pid  |          | id/name    | level/ pid /     ip\n"
	 "----------------------+----------+------------+-------------\n");
  for(unsigned k=0; k<MaxPartitions; k++) {
    const Node&       n = _control[k].node();
    const Allocation& a = _control[k].allocation();
    if (!(n==Unassigned)) {
      unsigned ip = n.ip();
      char ipbuff[32];
      sprintf(ipbuff,"%d.%d.%d.%d/%05d",
	     (ip>>24), (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff, n.pid());
      printf("%*s%s     %03d      %02d/%7s", 
	     21-strlen(ipbuff)," ",ipbuff, n.platform(), a.partitionid(), a.partition());
      if (!a.nnodes())
	printf("\n");
      else
	for(unsigned m=0; m<a.nnodes(); m++) {
	  const Node& p=*a.node(m);
	  ip = p.ip();
	  printf("%s       %1d/%05d/%d.%d.%d.%d\n", 
		 m==0 ? "":"                                             ",
		 p.level(), p.pid(),
		 (ip>>24), (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);
	}
    }
  }
}
