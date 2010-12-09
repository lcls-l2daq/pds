/*
 * RegisterSlaveImportFrame.cc
 *
 *  Created on: Mar 30, 2010
 *      Author: jackp
 */

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "stdio.h"

namespace Pds {
  namespace Pgp {

    RegisterSlaveExportFrame::FEdest RegisterSlaveImportFrame::dest() {
      unsigned ret = 0;
      if (bits.lane) ret |= RegisterSlaveExportFrame::laneMask;
      if (bits.vc & RegisterSlaveExportFrame::laneMask) ret |= 1;
      if (bits.vc==0) {
        ret = RegisterSlaveExportFrame::concentratorMask;
        if (bits.lane) {
          printf("ERROR ERROR bad dest lane(%u), vc(%u)\n", bits.lane, bits.vc);
        }
      }
      return (RegisterSlaveExportFrame::FEdest) ret;
    }

    void RegisterSlaveImportFrame::print() {
      char ocn[][20] = {"read", "write", "set", "clear"};
      char dn[][20]  = {"Q0", "Q1", "Q2", "Q3", "Cncntr"};
      printf("Register Slave Import Frame:\n\t");
      printf("timeout(%s), failed(%s)\n\t",
          lbits.timeout ? "true" : "false", lbits.failed ? "true" : "false");
      printf("lane(%u), vc(%u), dest(%s), opcode(%s), addr(0x%x), waiting(%s), tid(0x%x)\n\t",
          bits.lane, bits.vc, &dn[(int)dest()][0], &ocn[bits.oc][0],
          bits.addr, bits.waiting ? "waiting" : "not waiting", bits.tid);
      printf("data(0x%x)\n", (unsigned)_data);
    }
  }
}
