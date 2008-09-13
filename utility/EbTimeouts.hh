#ifndef PDS_TIMEOUTS
#define PDS_TIMEOUTS

#include "pds/collection/Level.hh"
#include "pds/xtc/Sequence.hh"
#include "StreamParams.hh"
#include "EventId.hh"

namespace Pds {
class EbTimeouts {
public:
  EbTimeouts(const EbTimeouts& ebtimeouts);
  EbTimeouts(unsigned crates, 
	     StreamParams::StreamType stream, 
	     Level::Type level);
  
  // Basic expiration time of the event builder at the current
  // level. The total timeout for a given sequence is
  // duration()*timeouts(sequence). The total time a given sequence
  // can take to reach the current level is
  // duration()*times(sequence). Unit is ms.
  unsigned duration() const;

  // Number of times a given sequence is allowed to expire before
  // timing out at the current level.
  int timeouts(const Sequence* sequence) const;

  // Number of basic durations a given sequence can take to be posted
  // from the event builder at the current level.
  int times(const Sequence* sequence) const;

  // For debugging
  void dump() const; 

private:
  short _tmos[Sequence::NumberOfServices*Sequence::NumberOfTypes];
  short _time[Sequence::NumberOfServices*Sequence::NumberOfTypes];
  unsigned _duration;
};
}
#endif
