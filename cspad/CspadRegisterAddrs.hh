/*
 * RegisterRange.hh
 *
 *  Created on: Apr 8, 2010
 *      Author: jackp
 */

#ifndef CSPADREGISTERADDRS_HH_
#define CSPADREGISTERADDRS_HH_

namespace Pds {
  namespace CsPad {

    class CspadRegisterAddrs {
      public:
        CspadRegisterAddrs() : addr(0), readFlag(1) {};
        ~CspadRegisterAddrs() {};

        unsigned addr;
        unsigned readFlag;
    };
  }
}

#endif /* CSPADREGISTERADDRS_HH_ */
