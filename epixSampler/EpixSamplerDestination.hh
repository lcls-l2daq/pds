/*
 * EpixSamplerDestination.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef EPIXSAMPLERDESTINATION_HH_
#define EPIXSAMPLERDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace EpixSampler {

    class EpixSamplerDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {Data, Registers, NumberOf};

        EpixSamplerDestination() {}
        ~EpixSamplerDestination() {}

      public:
        unsigned            lane() { return  0; }
        unsigned            vc() {return _dest & 1; }
        const char*         name();
    };
  }
}

#endif /* DESTINATION_HH_ */
