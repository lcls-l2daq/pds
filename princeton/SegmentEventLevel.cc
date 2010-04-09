#include "SegmentEventLevel.hh"

#include <string>
#include <sstream>

#include "pds/management/EventStreams.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/OutletWire.hh"
#include "pds/utility/WiredStreams.hh"
#include "pds/utility/InletWireServer.hh"

namespace Pds
{

SegmentEventLevel::  SegmentEventLevel(
  unsigned          platform,
  SegWireSettings&  settings,
  EventCallback&    callback,
  Arp*              arp) 
  :
  SegmentLevel  (platform, settings, callback, arp),      
  _pEventServer (NULL)
{
}

SegmentEventLevel::~SegmentEventLevel()
{
}

static std::string addressToStr( unsigned int uAddr )
{
    unsigned int uNetworkAddr = htonl(uAddr);
    const unsigned char* pcAddr = (const unsigned char*) &uNetworkAddr;
    std::stringstream sstream;
    sstream << 
      (int) pcAddr[0] << "." <<
      (int) pcAddr[1] << "." <<
      (int) pcAddr[2] << "." <<
      (int) pcAddr[3];
      
     return sstream.str();
}

void SegmentEventLevel::allocated(const Allocation & alloc, unsigned index)
{
  unsigned partition = alloc.partitionid();
  printf( "SegmentEventLevel::allocated(): partition id = %d  src inedx = %d\n", partition, index ); // !! for debug only

  InletWire & inlet = *_streams->wire(StreamParams::FrameWork);

  // Look for EVR segment node, and record its index
  unsigned int  nnodes            = alloc.nnodes();
  const int     iEvrNodeIndex     = 0; // Control_gui will always set evr as the first node
  
  for (unsigned n = 0; n < nnodes; n++)
  {
    const Node & node = *alloc.node(n);
    if (node.level() == Level::Segment)
    {
      printf( "Found Evr IP = %s\n", addressToStr(node.ip()).c_str() );
      break;
    }
  }  
  
  unsigned vectorid = 0;
  _pEventServer = 0;
  for (unsigned n = 0; n < nnodes; n++)
  {
    const Node & node = *alloc.node(n);
    if (node.level() == Level::Event)
    {
      Ins ins = StreamPorts::event(partition,
                 Level::Event,
                 vectorid,
                 iEvrNodeIndex);
                 
      if (vectorid == 0)
      {
        Ins srvIns(ins.portId());
        _pEventServer =
          new NetDgServer(srvIns,
              node.procInfo(),
              EventStreams::netbufdepth *
              EventStreams::MaxSize);
        inlet.add_input(_pEventServer);
      }

      Ins mcastIns(ins.address());
      _pEventServer->server().join(mcastIns, Ins(header().ip()));
      
      printf( "SegmentEventLevel::allocated(): dst id %d  mcastIns addr %x port %d\n", vectorid, mcastIns.address(), ins.portId() ); // !! for debug only
      
      vectorid++;
    }
  }

  /*
   * Known Issue:
   *   If the "event" level also runs in the same machine, it will register for other multicast addresses for
   *   the same NIC, resulting in the NetDgServer getting extra multicasts.
   *
   *   Hence, it is required to run the princeton program and the event program on separate machines.
   */
  //Ins bcastIns = StreamPorts::bcast(partition, Level::Event, iEvrNodeIndex);
  //_pEventServer->server().join(bcastIns, Ins(header().ip()));
  
  //printf( "SegmentEventLevel::allocated(): bcastIns addr %x port %d\n", bcastIns.address(), bcastIns.portId() ); // !! for debug only

  OutletWire *owire =
    _streams->stream(StreamParams::FrameWork)->outlet()->wire();
  owire->bind(OutletWire::Bcast, 
    StreamPorts::bcast(partition,Level::Event, index));
}

void SegmentEventLevel::dissolved()
{
  static_cast <InletWireServer*>(_streams->wire())->remove_input(_pEventServer);
  static_cast <InletWireServer*>(_streams->wire())->remove_outputs();
}

}       // namespace Pds
