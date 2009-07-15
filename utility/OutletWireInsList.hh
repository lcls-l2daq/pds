/*
** ++
**  Package:
**	odfUtility
**
**  Abstract:
**     This class implements a database of Internet Addresses (as Inter-Net
**     Specifiers (INS)) and their associated DataFlow identifiers ("IDs").
**     Methods are provided to insert and remove items from this database.
**     In actuality, two parallel databases are maintained, one a doubly-
**     linked list and a second, an array. The list is sorted by ID number
**     low to high), as is the array. Entries within the array can be
**     accessed (see "lookup") by their index number. If this number varies
**     randomly for each call to "lookup", the database may also be 
**     accessed randomly. This feature is exploited by the "odfOutletWire" 
**     class, to access the set of destinations at the event level.
**      
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 1,1998
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_OUTLETWIREINSLIST
#define PDS_OUTLETWIREINSLIST

#include "OutletWireIns.hh"
#include "EbTmoMask.hh"
#include "pdsdata/xtc/Sequence.hh"

namespace Pds {
class OutletWireInsList
  {
  public:
    OutletWireInsList();
   ~OutletWireInsList();
    int            insert(unsigned id, const Ins& ins); 
    int            remove(unsigned id); 
    void           flush ();
    // Note: we don't check if the database is empty, hence protection
    // against this condition must be done by the caller.
    OutletWireIns* lookup(const Sequence& seq) {return lookup(seq.stamp().fiducials()>>4);}
    OutletWireIns* lookup(unsigned index) {return _ins[index % _entries];}
    bool           isempty() const {return !_entries;}
  private:
    OutletWireIns* _locate(unsigned id); 
    void           _resort(); 
  private:
    OutletWireIns  _list; // Current address list (as a doubly-linked list)
    unsigned       _entries; // Number of entries in following array
    OutletWireIns* _ins[EbTmoMask::BitMaskBits]; // Current address list
    OutletWireIns  _bcast;
  };
}
#endif
