/*
 * CspadDestination.hh
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#ifndef CSPADDESTINATION_HH_
#define CSPADDESTINATION_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {namespace Pgp{ class Destination;}}

namespace Pds {

  namespace CsPad {

    class CspadDestination : public Pds::Pgp::Destination {
      public:
        CspadDestination() {}
        ~CspadDestination() {}

        enum FEdest {Q0=0, Q1=1, Q2=2, Q3=3, CR=4, NumberOf=5};
        enum {laneMask=2, concentratorMask=4};

      public:
        unsigned            lane() {return ((_dest & laneMask) ? 1 : 0);}
        unsigned            vc() {return (_dest & concentratorMask ? 0 : (_dest & 1) + 1); }
        char*               name();
//        CspadDestination::FEdest getDest(Pds::Pgp::RegisterSlaveImportFrame*);
    };
  }
}

#endif /* DESTINATION_HH_ */
