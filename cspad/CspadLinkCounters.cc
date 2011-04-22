/*
 * XampsLinkCounters.hh
 *
 *  Created on: Jan 19, 2011
 *      Author: jackp
 */

#include "pds/cspad/CspadLinkCounters.hh"

namespace Pds {
  namespace CsPad {

    void CspadLinkCounters::print() {
      printf("CellErrors(%u), LinkDown(%u), LinkErrors(%u)",
          (unsigned)cellErrors, (unsigned)linkDown, (unsigned)linkErrors);
    };
  }
}
