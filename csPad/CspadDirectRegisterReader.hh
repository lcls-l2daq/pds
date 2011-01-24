/*
 * CspadDirectRegisterReader.hh
 *
 *  Created on: Jan 20, 2011
 *      Author: jackp
 */

#ifndef CSPADDIRECTREGISTERREADER_HH_
#define CSPADDIRECTREGISTERREADER_HH_
#include "pds/pgp/RegisterSlaveImportFrame.hh"


namespace Pds {
  namespace CsPad {
    class CspadDirectRegisterReader {
      public:
        CspadDirectRegisterReader(int f) : _fd(f) {};
        ~CspadDirectRegisterReader() {};
      public:
        int _readPgpCard(Pds::Pgp::RegisterSlaveImportFrame*);
        int readRegister(
            Pds::Pgp::RegisterSlaveExportFrame::FEdest dest,
            unsigned addr,
            unsigned tid,
            uint32_t* retp);
      private:
        int _fd;

    };
  }
}

#endif /* CSPADDIRECTREGISTERREADER_HH_ */
