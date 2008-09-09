/*
** ++
**  Package:
**	Service
**
**  Abstract:
**      non-inline functions for class "ServerClusterScan"
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

#include "ServerScan.hh"
#include "SelectManager.hh"

using namespace Pds;

/*
** ++
**
**
** --
*/

template<class T> 
void ServerScan<T>::iterate(EbBitMask servers)
  {   
  T**               server    = _list->table(); 
  EbBitMask      remaining = servers & _list->managed();
  EbBitMask      id(EbBitMask::ONE);

  do
    {
    if((id & remaining).isZero()) continue;
    process(*server);
    remaining &= ~id;
    }
  while(server++, (id <<= 1).isNotZero(), remaining.isNotZero());
  }

