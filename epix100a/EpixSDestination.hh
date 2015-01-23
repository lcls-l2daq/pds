/*
 * EpixSDestination.hh
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#ifndef EPIXSDESTINATION_HH_
#define EPIXSDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace EpixS {

    class EpixSDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {Data, Registers, Oscilloscope, NumberOf};

        EpixSDestination() {}
        ~EpixSDestination() {}

      public:
        unsigned            lane() { return  0; }
        unsigned            vc() {return _dest < NumberOf ? _dest & 3 : 0; }
        const char*         name();
    };
  }
}

#endif /* EPIXSDESTINATION_HH_ */
