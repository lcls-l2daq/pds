/*
 * XampsDestination.hh
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#ifndef XAMPSDESTINATION_HH_
#define XAMPSDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace Xamps {

    class XampsDestination : public Pds::Pgp::Destination {
      public:
        enum FEdest {External, Internal, NumberOf};

        XampsDestination() {}
        ~XampsDestination() {}

      public:
        unsigned            lane() { return  0; }
        unsigned            vc() {return _dest ? 2 : 1; }
        char*               name();
    };
  }
}

#endif /* DESTINATION_HH_ */
