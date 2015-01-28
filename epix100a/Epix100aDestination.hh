/*
 * Epix100aDestination.hh
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#ifndef EPIX100ADESTINATION_HH_
#define EPIX100ADESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace Epix100a {

    class Epix100aDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {Data, Registers, Oscilloscope, NumberOf};

        Epix100aDestination() {}
        ~Epix100aDestination() {}

      public:
        unsigned            lane() { return  0; }
        unsigned            vc() {return _dest < NumberOf ? _dest & 3 : 0; }
        const char*         name();
    };
  }
}

#endif /* EPIX100ADESTINATION_HH_ */
