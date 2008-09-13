/*
** ++
**  Package:
**	odfUtility
**
**  Abstract:
**      this class maintains a database (as a bit-list) of the clients
**      currently being serviced by a event builder. Bit offsets within
**      the list correspond to the identity of a particular client. If a
**      bit is set, the client has been entered into the database. If a
**      bit is clear, the client has either been removed or is not known
**      to the database. Note: clients are specified as input arguments to 
**      the class's functions as MASKS not bit offsets (i.e., they are of 
**      the form: "1 << ID"). 
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

#ifndef PDS_EBCLIENTS
#define PDS_EBCLIENTS

#include "pds/service/EbBitMask.hh"

namespace Pds {
class EbClients
  {
  public:
    EbClients(const EbClients&); 
    EbClients(EbBitMask clients);
    EbClients(void);
    EbBitMask insert(EbBitMask id);
    EbBitMask remove(EbBitMask id);
    EbBitMask present(EbBitMask id) const;
    EbBitMask remaining()          const;
    const EbClients& operator=(const EbClients&); 
  private:
    EbBitMask _clients;  // bit-list of clients...
  };
}
/*
** ++
**
**    Constructor; simply copies the input argument into the internal list...
**
** --
*/

inline Pds::EbClients::EbClients(void){}

inline Pds::EbClients::EbClients(EbBitMask clients) :
  _clients(clients)
  {
  }

/*
** ++
**
**    Copy constructor (simply copies the internal list)...
**
** --
*/

inline Pds::EbClients::EbClients(const Pds::EbClients& in) :
  _clients(in._clients)
  {
  }

/*
** ++
**
**    Assignment operator (simply copies the internal list)...
**
** --
*/

inline const Pds::EbClients& Pds::EbClients::operator=(const Pds::EbClients& in) 
  {
  _clients = in._clients;
  return *this;
  }

/*
** ++
**
**   This function inserts the specified client into the list. The function 
**   returns the previous state of the client within the database. That is, 
**   a TRUE (non-zero) value if the client had already been inserted into 
**   the database and a FALSE (zero) value, if it had not.
**
** --
*/

inline EbBitMask Pds::EbClients::insert(EbBitMask id) 
  {
  EbBitMask clients = _clients;
  EbBitMask wasSet  = clients & id;

  if(wasSet.isZero()) _clients = clients | id;

  return wasSet;
  }

/*
** ++
**
**   The function returns the state of the specified client within the 
**   database. That is, a TRUE (non-zero) value if the client is inserted 
**   into the database and a FALSE (zero) value, if it is not.
**
** --
*/

inline EbBitMask Pds::EbClients::present(EbBitMask id) const
  {
  return _clients & id;
  }

/*
** ++
**
**   This function removes the specified client from the database. The
**   state of database is returned AFTER the removal. A zero value
**   specifies that the database is now empty.
**
** --
*/

inline EbBitMask Pds::EbClients::remove(EbBitMask id) 
  {
  EbBitMask clients = _clients;
  clients &= ~id;
  _clients = clients;
  return clients;
  }

/*
** ++
**
**   This function returns the current state of the database. If the
**   function returns zero (0), the database is empty. If the return
**   value is non-zero, it represents the clients still registered
**   with the database.
**
** --
*/

inline EbBitMask Pds::EbClients::remaining() const
  {
  return _clients;
  }

#endif
