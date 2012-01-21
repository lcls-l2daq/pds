/*
 * Cspad2x2LinkCounters.hh
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#include "pds/cspad2x2/Cspad2x2LinkCounters.hh"

namespace Pds {
  namespace CsPad2x2 {

    void Cspad2x2LinkCounters::print() {
      printf("CellErrors(%u), LinkDown(%u), LinkErrors(%u)",
          (unsigned)cellErrors, (unsigned)linkDown, (unsigned)linkErrors);
    };
  }
}
