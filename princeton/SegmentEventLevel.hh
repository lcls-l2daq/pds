#ifndef PDS_SEGMENT_EVENT_LEVEL_HH
#define PDS_SEGMENT_EVENT_LEVEL_HH

/*
 * Note: This file is no longer required for PrincetonServer.
 *       Originally this was used to get EVR data for the princeton server,
 *       but later it was changed to use Occurance. Now this file is
 *       deprecated, and only remain as a reference for segment level to
 *       get the EVR data
 */

#include "pds/management/SegmentLevel.hh"
#include "pds/utility/NetDgServer.hh"

namespace Pds
{

class SegmentEventLevel: public SegmentLevel
{
public:
  SegmentEventLevel(
    unsigned          platform,
    SegWireSettings&  settings,
    EventCallback&    callback,
    Arp*              arp,
    int               slowEb);

  virtual ~SegmentEventLevel();

private:
  // Implements PartitionMember
  virtual void    allocated (const Allocation&, unsigned);
  virtual void    dissolved ();

  NetDgServer*    _pEventServer;  // Server for listening to the event level datagram
};

} // namespace Pds
#endif

