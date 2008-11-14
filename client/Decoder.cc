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

#include "Decoder.hh"
#include "Browser.hh"
#include <stdio.h>

#include "pds/xtc/ZcpDatagramIterator.hh"

using namespace Pds;

/*
** ++
**
**
** --
*/

Decoder::Decoder(Level::Type level) :
  _pool(sizeof(ZcpDatagramIterator),16)
{
  if      (level == Level::Control)  _depth = 1;
  else if (level == Level::Event)    _depth = 0;
  else _depth = -1;
}

/*
** ++
**
**
** --
*/

Transition* Decoder::transitions(Transition* in) 
  {
  printf("Transition: id %d\n", in->id());
  return in;
  }

/*
** ++
**
**
** --
*/

InDatagram* Decoder::events(InDatagram* in) 
  {
  return _handleDg(in);
  }

/*
** ++
**
**
** --
*/

InDatagram* Decoder::markers(InDatagram* in) 
  {
  printf("\nMarker:\n");
  return _handleDg(in);
  }

/*
** ++
**
**
** --
*/

InDatagram* Decoder::occurrences(InDatagram* in) 
  {
  printf("\nOccurrence:\n");
  return _handleDg(in);
  }

/*
** ++
**
**
** --
*/

InDatagram* Decoder::_handleDg(InDatagram* in){
  InDatagramIterator* iter = in->iterator(&_pool);
  int advance=0;
  Browser browser(in->datagram(), iter, _depth, advance);
  if (in->datagram().xtc.contains.id() == TypeId::Id_Xtc)
    if (browser.iterate() < 0)
      printf("..Terminated.\n");
  delete iter;
  return in;
}
