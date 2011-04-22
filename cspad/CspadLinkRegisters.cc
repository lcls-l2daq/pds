/*
 * CspadLinkRegisters.cc
 *
 *  Created on: Jan 21, 2011
 *      Author: jackp
 */

#include "pds/cspad/CspadLinkRegisters.hh"
#include "pds/cspad/CspadLinkCounters.hh"

namespace Pds {
  namespace CsPad {
    void CspadLinkRegisters::print() {
      printf("LocLink(%u), RemLink(%u), RxCount(%u), ",
          locLink(), remLink(), (unsigned)rxCount);
      l.print();
    }
  }
}
