/*
** ++
**  Package:
**	odfServices
**
**  Abstract:
**     Non-line functions used by "odfOutletWireInsList.hh"
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

#include "OutletWireInsList.hh"

using namespace Pds;

/*
** ++
**
**    Constructor simply initializes the listhead to point to itself and
**    sets the current size of the lookup table to zero.
**   
** --
*/

OutletWireInsList::OutletWireInsList() : 
  _list(),
  _entries(0)  
  {
  }

/*
** ++
**
**    The destructor simply deletes all the items currently on the list.
**   
** --
*/

OutletWireInsList::~OutletWireInsList() 
  {
  OutletWireIns* dst;

  while((dst = _list.remove()) != _list.empty()) delete dst;
  }

/*
** ++
**
**   This function will search for the entry in the list which matches the 
**   ID specified as an argument. If a match is found, a pointer to the
**   entry is returned. Note, that since the list is sorted by its ID number, 
**   if a match cannot be found the function will return a pointer to the
**   entry just closest to the specified ID. This will allow the "insert"
**   function to install a new item at the appropriate location. 
**   
** --
*/

OutletWireIns* OutletWireInsList::_locate(unsigned id) 
  {
  OutletWireIns* dst = _list.forward();

  while(dst != _list.empty())
    {
    if(id <= dst->id()) break;
    dst = dst->forward();
    }

  return dst;
  }

/*
** ++
**
**    The class actually maintains two lists.  A doubly-linked list which is 
**    sorted by ID and a lookup-table, which is sorted by its ID (low to high).
**    This function is called whenever the doubly-linked list is modified 
**    (entries either inserted or removed). It will re-order the lookup table, 
**    such that its entries are consistent with their IDs. Note: that this 
**    function assumes it is only called if there is at least one entry, thus 
**    the use of the do-while loop.
**   
** --
*/

void OutletWireInsList::_resort() 
  {
  OutletWireIns* dst   = _list.forward();
  OutletWireIns** next = &_ins[0];

  do *next++ = dst; while(dst = dst->forward(), dst != _list.empty());
  }

/*
** ++
**
**    Insert the entry as specified by its ID. The function searches its 
**    list for a match. If a match is found, the InterNet Specification 
**    (INS) of the entry is simply replaced with thst passed as an
**    argument. If the entry cannot be matched a new entry 
**    is allocated and inserted. The lookup table is than resorted.
**    The function returns a positive value if an entry was replaced and
**    a zero (0) value if the entry was created.
**   
** --
*/

int OutletWireInsList::insert(unsigned id, const Ins& ins) 
  {
  OutletWireIns* dst = _locate(id);
  if(dst->id() != id)
    {
    dst->insert(new OutletWireIns(id, ins));
    _entries++;
    _resort();
    return 0;
    }

  dst->replace(ins);
  return 1;
  }

/*
** ++
**
**    Remove the entry as specified by its ID. The function searches its list 
**    for a match. If a match is found, the entry is removed and deleted from 
**    the list. If the removal leaves the list non-empty its lookup table is 
**    resorted. The function returns a positive value if an entry could be 
**    found and deleted and a zero (0) if not. 
**   
** --
*/

int OutletWireInsList::remove(unsigned id) 
  {
  OutletWireIns* dst = _locate(id);

  if(dst->id() == id)
    {
    dst->disconnect();
    delete dst;
    if(_entries--) _resort();
    return 1;
    }
  return 0;
  }


void OutletWireInsList::flush()
{
  OutletWireIns* dst;
  while((dst = _list.remove()) != _list.empty()) delete dst;
  _entries = 0;
}
