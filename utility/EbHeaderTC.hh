#ifndef PDS_EB_HEADER_TC_HH
#define PDS_EB_HEADER_TC_HH 

#include "pds/xtc/xtc.hh"
#include "pds/collection/Level.hh"

/*
** ++
**
**   The following class provides a convienent way to wrap up a "header"
**   Tagged container (TC), whose contents and identity vary as a function
**   of the level in the dataflow hiararchy at which the Event Builder 
**   resides.This TC is used to establish the identity of the datagrams
**   constructed by the event builder, which of course must vary as a 
**   function of level.
**
** --
*/
namespace Pds {
class EbHeaderTC : public TC
  {
  public:
    EbHeaderTC(Level::Type level) :
      TC(TypeNumPrimary::Id_InXtcContainer) {}
  };
}
#endif
