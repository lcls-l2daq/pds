/*
 * Epix10kDestination.hh
 *
 *  Created on: 2014.4.15
 *      Author: jackp
 */

#ifndef EPIX10KDESTINATION_HH_
#define EPIX10KDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace Epix10k {

    class Epix10kDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {Data, Registers, Oscilloscope, NumberOf};

        Epix10kDestination() {}
        ~Epix10kDestination() {}

      public:
        unsigned            lane() { return  0; }
        unsigned            vc() {return _dest < NumberOf ? _dest & 3 : 0; }
        const char*         name();
    };
  }
}

#endif /* EPIX10KDESTINATION_HH_ */
