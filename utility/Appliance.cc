/*
** ++
**  Package:
**      odfUtility
**
**  Abstract:
**	Definition of member functions declared by odfAppliance. Note:
**      this include only non-inline functions (see odfApplinace.inc).
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 20 1,1997
**
**  Revision History:
**
** --
*/

#include "Appliance.hh" // Get declarations...

using namespace Pds;

/*
** --
**
**
** ++
*/

Appliance::Appliance() :
  _datagrams(0),
  _time(0)
  {
  _handlers[Sequence::Occurrence] = &Appliance::occurrences;
  _handlers[Sequence::Event     ] = &Appliance::events;
  _handlers[Sequence::Marker    ] = &Appliance::markers;
  }


/*
**  As it stands, transitions pass through the appliance stream
**  unmodified, unable to indicate failure, for example.
*/

void Appliance::post(Transition* input)
  {
    Appliance* n = (Appliance*)next();
    Transition* output = n->transitions(input);
    if      (output == 0) delete input;
    else if (output != (Transition*) DontDelete) {
      if (input != output) delete input;
      n->post(output);
    }
  }

/*
** --
**
**
** ++
*/

InDatagram* Appliance::markers(InDatagram* in){
  return in;
}

/*
** --
**
**
** ++
*/

void Appliance::post(InDatagram* input)
  {
  _datagrams++;
  ((Appliance*)next())->event(input);
  }

/*
** --
**
**
** ++
*/
void Appliance::event(InDatagram* input)
  {
  InDatagram* output = vector(input);
  if      (!output)     delete input;
  else if (output != (InDatagram*) DontDelete)
    {
    if(input != output) delete input;
    post(output);
    }
  }
