/*
** ++
**  Package:
**	odfUtility
**
**  Abstract:
**     This class defines any given entry in database of entries as
**     defined by the class "odfOutletWireInsList.hh". An entry binds an
**     ID and an Inter-Net-Specifier (or address) together. Member
**     functions are defined to create or replace entries.
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

#ifndef PDS_OUTLETWIREINS
#define PDS_OUTLETWIREINS

#include "pds/service/LinkedList.hh"
#include "pds/service/Ins.hh"

namespace Pds {
class OutletWireIns : public LinkedList<OutletWireIns>
  {
  public:
    OutletWireIns();
    OutletWireIns(unsigned id, const Ins&);
   ~OutletWireIns() {}
    OutletWireIns& operator=(const OutletWireIns&);
    Ins&  ins();
    unsigned id()  const;
    void     replace(const Ins&);
    void     replace(unsigned id, const Ins&);
  private:
    unsigned _id;  // DataFlow identifier for specified address
    Ins   _ins; // Internet address
  };
}
/*
** ++
**
**   Default constructor, assigns the value minus one (-1) to the
**   identifier and a NULL InterNet address. This ID value is not
**   considered to be a valid identifier, and thus this type of
**   entry can be considered as the "blackhole" entry.
**   
** --
*/

inline Pds::OutletWireIns::OutletWireIns() : 
  Pds::LinkedList<Pds::OutletWireIns>(),
  _id((unsigned) -1),
  _ins()
  {
  }

/*
** ++
**
**   This constructor accepts as arguments an identifier and address
**   to bind as a single database entry.
**   
** --
*/

inline Pds::OutletWireIns::OutletWireIns(unsigned id, const Pds::Ins& ins) : 
  Pds::LinkedList<Pds::OutletWireIns>(),
  _id(id),
  _ins(ins)
  {
  }

/*
** ++
**
**    Returns the ID of the database entry.
**   
** --
*/

inline unsigned Pds::OutletWireIns::id()  const 
  {
  return _id;
  }

/*
** ++
**
**    Returns the InterNet Address of the database entry
**   
** --
*/

inline Pds::Ins& Pds::OutletWireIns::ins()
  {
  return _ins;
  }

/*
** ++
**
**    Replaces the Internet Address of the specified entry, with the
**    value specified by the input argument.
**   
** --
*/

inline void Pds::OutletWireIns::replace(const Ins& ins)  
  {
  _ins = ins;
  }


/*
** ++
**
**    Assignment operator. Simply replaces the entire entry with
**    the entry specified by its input argument.
**   
** --
*/

inline 
Pds::OutletWireIns& Pds::OutletWireIns::operator=(const Pds::OutletWireIns& in)  
  {
  _id  = in._id;
  _ins = in._ins;
  return *this;
  }

/*
** ++
**
**    Replaces both the ID and address of the entry with the values
**    specified by the input arguments.
**   
** --
*/

inline void Pds::OutletWireIns::replace(unsigned id, const Pds::Ins& ins)  
  {
  _id  = id;
  _ins = ins;
  }

#endif
