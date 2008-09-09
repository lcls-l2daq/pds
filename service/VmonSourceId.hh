// ---------------------------------------------------------------------------
// Description:
//   Pack/unpack source id information for vmon
//
// Author(s):
//   APerazzo@lbl.gov
//
// History:
//   17-Jun-2000 Created
// ---------------------------------------------------------------------------


#ifndef PDS_VMONSOURCEID_HH
#define PDS_VMONSOURCEID_HH

#include <stdio.h>

namespace Pds {

class VmonSourceId {
public:
  enum Level {Control, Source, Segment, Fragment, Event, Mon, 
	      Invalid, NumberOfLevels=Invalid};

  VmonSourceId(const VmonSourceId& sourceid)
    : _id(sourceid._id) 
  {}

  VmonSourceId(Level level, unsigned id)
    : _id(id)
  {
    _id &= ~(0xff<<24);
    _id |= (level & 0xff) << 24;
  }

  /*
  VmonSourceId(Level level, 
		  unsigned main = 0, unsigned secnd = 0, unsigned det = 0) 
    : _id(0)
  {
    _id |= (level & 0xff) << 24;
    _id |= (main  & 0xff) << 16;
    _id |= (secnd & 0xff) <<  8;
    _id |= (det   & 0xff) <<  0;
  }
  */

  Level       level() const {return Level((_id >> 24) & 0xff);}
  unsigned    main () const {return       (_id >> 16) & 0xff;}
  unsigned    secnd() const {return       (_id >>  8) & 0xff;}
  unsigned    det()   const {return       (_id >>  0) & 0xff;}

  void print() const {
    printf("(sourceid %d:%02d:%02d)\n", level(), main(), secnd());
  }

  void name(char* s) const {
    sprintf(s, "%d:%02d:%02d", level(), main(), secnd());
  }

private:
  unsigned _id;
};

}
#endif
