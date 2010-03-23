#ifndef PDS_SEGMENT_EVENT_LEVEL_HH
#define PDS_SEGMENT_EVENT_LEVEL_HH

#include "pds/management/SegmentLevel.hh"
#include "pds/utility/NetDgServer.hh"

namespace Pds 
{

class SegmentEventLevel: public SegmentLevel 
{
public:
  SegmentEventLevel(
    const char*       strEvrIp,
    unsigned          platform,
    SegWireSettings&  settings,
    EventCallback&    callback,
    Arp*              arp);

  virtual ~SegmentEventLevel();

private:
  // Implements PartitionMember  
  virtual void    allocated (const Allocation&, unsigned);
  virtual void    dissolved ();

  int             _iEvrIp;
  NetDgServer*    _pEventServer;  // Server for listening to the event level datagram
};

} // namespace Pds 
#endif
 
