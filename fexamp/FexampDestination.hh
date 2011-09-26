/*
 * FexampDestination.hh
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#ifndef FEXAMPDESTINATION_HH_
#define FEXAMPDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace Fexamp {

    class FexampDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {InvalidDataVC, VC1, VC2, NumberOf};

        FexampDestination() {}
        ~FexampDestination() {}

      public:
        unsigned            lane() { return  0; }
        unsigned            vc() {return _dest; }
        char*               name();
    };
  }
}

#endif /* DESTINATION_HH_ */
