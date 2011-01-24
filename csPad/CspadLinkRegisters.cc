/*
 * CspadLinkRegisters.cc
 *
 *  Created on: Jan 21, 2011
 *      Author: jackp
 */

#include "pds/csPad/CspadLinkRegisters.hh"
#include "pds/csPad/CspadLinkCounters.hh"

namespace Pds {
  namespace CsPad {
    void CspadLinkRegisters::print() {
      printf("LocLink(%u), RemLink(%u), RxCount(%u), ",
          locLink(), remLink(), (unsigned)rxCount);
      l.print();
    }
  }
}
