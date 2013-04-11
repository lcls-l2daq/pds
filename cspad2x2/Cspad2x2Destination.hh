/*
 * Cspad2x2Destination.hh
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#ifndef CSPAD2X2DESTINATION_HH_
#define CSPAD2X2DESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;class RegisterSlaveImportFrame;}}

namespace Pds {

  namespace CsPad2x2 {

    class Cspad2x2Destination : public Pds::Pgp::Destination {
      public:
        Cspad2x2Destination() {}
        ~Cspad2x2Destination() {}

        enum FEdest {Q0=0, CR=1, NumberOf=2};
        enum {concentratorMask=1};

      public:
        unsigned            lane() {return 0;}
        unsigned            vc() {return (_dest  ? 0 : 1); }
        const char*         name();
//        Cspad2x2Destination::FEdest getDest(Pds::Pgp::RegisterSlaveImportFrame*);
    };
  }
}

#endif /* DESTINATION_HH_ */
