#ifndef PDS_EB_DUMMY_TC_HH
#define PDS_EB_DUMMY_TC_HH

#include "pds/xtc/xtc.hh"
#include "pds/collection/Level.hh"
/*
** ++
**
**   The following class provides a convienent way to wrap up a "dummy"
**   Tagged container (TC), whose contents and identity vary as a function
**   of the level in the dataflow hiararchy at which the Event Builder 
**   resides.This TC is used whenever a TC is needed to "dummy up" a
**   contribution into a constructed event to account for a contribution
**   which does not arrrive.
**
** --
*/

namespace Pds {
class EbDummyTC : public TC
  {
  public:
    EbDummyTC(Level::Type level) :
      TC(TypeIdQualified(level == Level::Control ?
			       TypeNumPrimary::Id_FragmentContainer :
			       (level == Level::Event ?
				TypeNumPrimary::Id_ModuleContainer  :
				TypeNumPrimary::Id_XTC)),
	    TypeIdQualified(level == Level::Control ?
			       TypeNumPrimary::Id_EventContainer :
			       (level == Level::Event ?
				TypeNumPrimary::Id_FragmentContainer :
				TypeNumPrimary::Id_ModuleContainer))) {}
  };
}
#endif
