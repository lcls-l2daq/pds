/*
 * CspadLinkCounters.hh
 *
 *  Created on: Jan 19, 2011
 *      Author: jackp
 */

#ifndef CSPAD2x2LINKCOUNTERS_HH_
#define CSPAD2x2LINKCOUNTERS_HH_

#include <stdio.h>
#include <stdint.h>

namespace Pds {
  namespace CsPad2x2 {

    class Cspad2x2LinkCounters {
      public:
        Cspad2x2LinkCounters() : cellErrors(0), linkDown(0), linkErrors() {};
        ~Cspad2x2LinkCounters() {};

        void print();

      public:
        uint32_t cellErrors;
        uint32_t linkDown;
        uint32_t linkErrors;
    };
  }
}

#endif /* CSPAD2x2LINKCOUNTERS_HH_ */
