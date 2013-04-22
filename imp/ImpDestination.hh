/*
 * ImpDestination.hh
 *
 *  Created on: April 12, 2013
 *      Author: jackp
 */

#ifndef IMPDESTINATION_HH_
#define IMPDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace Imp {

    class ImpDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {InvalidDataVC, InvalidVC1, InvalidVC2, CommandVC, NumberOf};

        ImpDestination() {}
        ~ImpDestination() {}

      public:
        unsigned            lane() { return  0; }
        unsigned            vc() {return _dest; }
        const char*         name();
    };
  }
}

#endif /* DESTINATION_HH_ */
