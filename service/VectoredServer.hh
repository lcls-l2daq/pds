/*
** ++
**  Package:
**	Services
**
**  Abstract:
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

#ifndef PDS_VECTOREDSERVER
#define PDS_VECTOREDSERVER

#include "Server.hh"

namespace Pds {

class VectoredServer : public Server
  {
  public:
    VectoredServer(unsigned id,
		      const Ins&, 
                      int sizeofDatagram, 
                      int maxPayload,
                      int maxDatagrams = 40);
  };

}
/*
** ++
**
**
** --
*/

inline Pds::VectoredServer::VectoredServer(unsigned id,
					    const Ins& ins, 
                                            int sizeofDatagram, 
                                            int maxPayload,
                                            int maxDatagrams) : 
  Pds::Server(id,
	    Pds::Port::VectoredServer, 
            ins, 
            sizeofDatagram, 
            maxPayload,
            maxDatagrams)
  {
  }

#endif
