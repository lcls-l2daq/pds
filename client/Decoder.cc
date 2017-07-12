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

#include "pdsdata/xtc/XtcIterator.hh"

using namespace Pds;

/*
** ++
**
**
** --
*/

Decoder::Decoder(Level::Type level)
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
    //  printf("Transition: id %d\n", in->id());
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
    //  printf("\nMarker:\n");
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
    //  printf("\nOccurrence:\n");
  return _handleDg(in);
  }

/*
** ++
**
**
** --
*/

InDatagram* Decoder::_handleDg(InDatagram* in){
  Browser browser(in->datagram(), _depth);
  if (in->datagram().xtc.contains.id() == TypeId::Id_Xtc)
    browser.iterate();
  return in;
}
