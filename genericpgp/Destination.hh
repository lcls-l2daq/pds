/*
 * Destination.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef GenericPgp_DESTINATION_HH_
#define GenericPgp_DESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace GenericPgp {

    class Destination : public Pds::Pgp::Destination {
      public:
        enum FEdest {Data, Registers, NumberOf};

        Destination() {}
        ~Destination() {}

      public:
        unsigned            lane() { return  0; }
        unsigned            vc() {return _dest & 1; }
        const char*         name();
    };
  }
}

#endif
