/*
 * XampsExternalRegisters.hh
 *
 *  Created on: May 10, 2011
 *      Author: jackp
 */

#ifndef XAMPSEXTERNALREGISTERS_HH_
#define XAMPSEXTERNALREGISTERS_HH_
#include "pds/pgp/Pgp.hh"

namespace Pds {

  namespace Xamps {

    class XtrnalRegister {
      public:
        XtrnalRegister() {};
        ~XtrnalRegister() {};
        uint32_t addr;
        uint32_t mask;
    };

    enum {NumberOfExternalRegisters=19};

    class XampsExternalRegisters {
      public:
        XampsExternalRegisters(Pds::Pgp::Pgp* p) : pgp(p) {};
        ~XampsExternalRegisters() {};
        int      read();
        void     print();
        unsigned readVersion(uint32_t*);
      public:
        Pds::Pgp::Pgp*    pgp;
        uint32_t          values[NumberOfExternalRegisters];
    };

  }

}

#endif /* XAMPSEXTERNALREGISTERS_HH_ */
