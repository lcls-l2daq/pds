/*
 * Pgp.hh
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#ifndef PGP_HH_
#define PGP_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/Destination.hh"

namespace Pds {
  namespace Pgp {

    class Pgp {
      public:
        Pgp(int);
        virtual ~Pgp();

      public:
        enum {BufferWords=2048};
        enum {Success=0, Failure=1, SelectSleepTimeUSec=10000};
        Pds::Pgp::RegisterSlaveImportFrame* read(
                          unsigned size = (sizeof(Pds::Pgp::RegisterSlaveImportFrame)/sizeof(uint32_t)));
        unsigned       writeRegister(
                          Destination*,
                          unsigned,
                          uint32_t,
                          bool pf = false,
                          Pds::Pgp::PgpRSBits::waitState = Pds::Pgp::PgpRSBits::notWaiting);
        unsigned       writeRegisterBlock(
                          Destination*,
                          unsigned,
                          uint32_t*,
                          unsigned size = sizeof(RegisterSlaveExportFrame)/sizeof(uint32_t),
                          Pds::Pgp::PgpRSBits::waitState = Pds::Pgp::PgpRSBits::notWaiting,
                          bool pf=false);
       unsigned      readRegister(
                          Destination*,
                          unsigned,
                          unsigned,
                          uint32_t*,
                          unsigned size=(sizeof(Pds::Pgp::RegisterSlaveImportFrame)/sizeof(uint32_t)),
                          bool pf=false);

      private:
        int        _fd;
        unsigned   _readBuffer[BufferWords];
    };
  }
}

#endif /* PGP_HH_ */
