/*
 * CspadLinkCounters.hh
 *
 *  Created on: Jan 19, 2011
 *      Author: jackp
 */

#ifndef CSPADLINKCOUNTERS_HH_
#define CSPADLINKCOUNTERS_HH_

#include <stdio.h>
#include <stdint.h>

namespace Pds {
  namespace CsPad {

    class CspadLinkCounters {
      public:
        CspadLinkCounters() : cellErrors(0), linkDown(0), linkErrors() {};
        ~CspadLinkCounters() {};

        void print();

      public:
        uint32_t cellErrors;
        uint32_t linkDown;
        uint32_t linkErrors;
    };
  }
}

#endif /* CSPADLINKCOUNTERS_HH_ */
