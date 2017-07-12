#ifndef PDS_MARKER_ID_HH
#define PDS_MARKER_ID_HH

//
// Enumeration of the types of Marker that are available
// (see /utility/Marker.hh). 
//
// $Id: MarkerId.hh 5 2008-09-13 00:15:17Z weaver $
// $Name$
//

namespace Pds {

class MarkerId
{
public:
  enum Value
  {
    Unknown = 0,    // Reserved value - never valid		*not logged*
    
    TimeTick = 1,	// Used for updating scalers, etc.	*not logged*
    OepRequest = 2,	// Action requests for OEP		*not logged*
    DhpRequest = 3,	// Action requests for DHP		*not logged*
    OprMarker = 4,	// Markers with OPR-oriented meaning	*logged*
    L1AFanOut = 5,      // Fan-out of L1Accept to all OEP nodes *logged*
    ExternalGate = 6,   // FCPM front panel inputs              *logged*
    
    // Reserved marker type numbers (never logged):
    Reserved7 = 7,
    Reserved8 = 8,
    Reserved9 = 9,
    Reserved10 = 10,
    Reserved11 = 11,
    Reserved12 = 12,
    Reserved13 = 13,
    Reserved14 = 14,
    Reserved15 = 15,
    Reserved16 = 16,
    Reserved17 = 17,
    Reserved18 = 18,
    Reserved19 = 19,
    Reserved20 = 20,
    Reserved21 = 21,
    Reserved22 = 22
  };
  enum { numberof = 23 };
  
  // Bit mask of recognized marker types (could be used to generate error
  // messages if stray markers are detected):
  enum { Recognized = 0x0000007E };
  
  // Bit mask of markers to be logged to persistent store (filters out
  // markers used only for time synchronization purposes):
  enum { Loggable = 0x00000070 };

  enum ExternalGateSource {
    Gate3 = 0,
    Gate2 = 1
  };
};

}

#endif
