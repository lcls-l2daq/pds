/*
 * RegisterSlaveImportFrame.hh
 *
 *  Created on: Mar 30, 2010
 *      Author: jackp
 */

#ifndef PGPREGISTERSLAVEIMPORTFRAME_HH_
#define PGPREGISTERSLAVEIMPORTFRAME_HH_

#include <stdint.h>
#include "pds/pgp/RegisterSlaveExportFrame.hh"

namespace Pds {
  namespace Pgp {

    class LastBits {
      public:
        unsigned mbz1:        14;      //31:18
        unsigned timeout:      1;      //17
        unsigned failed:       1;      //16
        unsigned mbz2:        16;      //15:0
    };

    class RegisterSlaveImportFrame {
      public:
        RegisterSlaveImportFrame() {};
        ~RegisterSlaveImportFrame() {};

      public:
        unsigned tid()                                {return bits.tid;}
        unsigned addr()                               {return bits.addr;}
        RegisterSlaveExportFrame::waitState waiting() {return (RegisterSlaveExportFrame::waitState)bits.waiting;}
        unsigned lane()                               {return bits.lane;}
        unsigned vc()                                 {return bits.vc;}
        RegisterSlaveExportFrame::opcode opcode()     {return (RegisterSlaveExportFrame::opcode)bits.oc;}
        uint32_t data()                               {return _data;}
        unsigned timeout()                            {return lbits.timeout;}
        unsigned failed()                             {return lbits.failed;}
        RegisterSlaveExportFrame::FEdest   dest();
        void print();
        enum VcTypes {dataVC=3};

      public:
        SEbits    bits;
        uint32_t  _data;
        LastBits  lbits;
    };

  }
}

#endif /* PGPREGISTERSLAVEIMPORTFRAME_HH_ */
