/*
 * RegisterRange.hh
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#ifndef CSPAD2x2REGISTERADDRS_HH_
#define CSPAD2x2REGISTERADDRS_HH_

namespace Pds {
  namespace CsPad2x2 {

    class Cspad2x2RegisterAddrs {
      public:
        Cspad2x2RegisterAddrs() : addr(0), readFlag(1) {};
        ~Cspad2x2RegisterAddrs() {};

        unsigned addr;
        unsigned readFlag;
    };
  }
}

#endif /* CSPADREGISTERADDRS_HH_ */
