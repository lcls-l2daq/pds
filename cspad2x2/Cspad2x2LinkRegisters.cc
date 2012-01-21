/*
 * Cspad2x2LinkRegisters.cc
 *
 *  Created on: Jan 9 2012
 *      Author: jackp
 */

#include "pds/cspad2x2/Cspad2x2LinkRegisters.hh"
#include "pds/cspad2x2/Cspad2x2LinkCounters.hh"

namespace Pds {
  namespace CsPad2x2 {
    void Cspad2x2LinkRegisters::print() {
      printf("LocLink(%u), RemLink(%u), RxCount(%u), ",
          locLink(), remLink(), (unsigned)rxCount);
      l.print();
    }
  }
}
