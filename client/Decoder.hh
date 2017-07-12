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

#ifndef PDS_DECODER
#define PDS_DECODER

#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/Level.hh"

namespace Pds {

class Decoder : public Appliance
  {
  public:
    Decoder(Level::Type level);
   ~Decoder() {}
  private:
    Transition* transitions(Transition* in);
    InDatagram* events     (InDatagram* in);
    InDatagram* occurrences(InDatagram* in);
    InDatagram* markers    (InDatagram* in);
    InDatagram* _handleDg(InDatagram* in);
  private:
    int _depth;
  };
}
#endif
