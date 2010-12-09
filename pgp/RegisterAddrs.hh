/*
 * RegisterRange.hh
 *
 *  Created on: Apr 8, 2010
 *      Author: jackp
 */

#ifndef REGISTERADDRS_HH_
#define REGISTERADDRS_HH_

namespace RcePgp {

  class RegisterAddrs {
    public:
      RegisterAddrs() : addr(0), readFlag(1) {};
      ~RegisterAddrs() {};

      unsigned addr;
      unsigned readFlag;
  };

}

#endif /* REGISTERADDRS_HH_ */
