/*
** ++
**  Package:
**	Service
**
**  Abstract:
**      This class is an abstract class, which allows the construction
**      of a class to iterate over the set of servers managed by an
**      object of class "ServerManager". The user of this class
**      is asked to provide an implementation of the "process" function.
**      When the "iterate" function is called, it will call back this
**      function for each managed server, passing as an argument a
**      pointer to a managed server. The caller of the "iterate" function
**      can specify a list of server's to mask against the managed list.
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

#ifndef PDS_SERVERSCAN
#define PDS_SERVERSCAN

#include "EbBitMask.hh"

namespace Pds {

template<class T> class SelectManager;

template<class T> class ServerScan
  {
  public:
    ServerScan(SelectManager<T>* list) : _list(list) {}
  public:
    virtual ~ServerScan() {}
  public:
    virtual void process(T*) = 0;
  public:
    void iterate(EbBitMask servers = EbBitMask(EbBitMask::FULL));
  private:
    SelectManager<T>* _list;     // List of servers to iterate over
  };

}
#endif
