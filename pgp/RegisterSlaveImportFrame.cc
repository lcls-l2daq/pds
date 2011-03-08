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

//    RegisterSlaveExportFrame::FEdest RegisterSlaveImportFrame::dest() {
//      unsigned ret = 0;
//      if (bits.lane) ret |= RegisterSlaveExportFrame::laneMask;
//      if (bits.vc & RegisterSlaveExportFrame::laneMask) ret |= 1;
//      if (bits._vc==0) {
//        ret = RegisterSlaveExportFrame::concentratorMask;
//        if (bits._lane) {
//          printf("ERROR ERROR bad dest lane(%u), vc(%u)\n", bits._lane, bits._vc);
//        }
//      }
//      return (RegisterSlaveExportFrame::FEdest) ret;
//    }

    unsigned RegisterSlaveImportFrame::timeout(LastBits* l) {
      return l->timeout;
    }

    unsigned RegisterSlaveImportFrame::failed(LastBits* l) {
      return l->failed;
    }

    void RegisterSlaveImportFrame::print(unsigned size) {
      char ocn[][20] = {"read", "write", "set", "clear"};
//      char dn[][20]  = {"Q0", "Q1", "Q2", "Q3", "Cncntr"};
      printf("Register Slave Import Frame:\n\t");
      printf("timeout(%s), failed(%s)\n\t",
          lbits.timeout ? "true" : "false", lbits.failed ? "true" : "false");
      printf("lane(%u), vc(%u), opcode(%s), addr(0x%x), waiting(%s), tid(0x%x)\n\t",
          bits._lane, bits._vc, &ocn[bits.oc][0],
          bits._addr, bits._waiting ? "waiting" : "not waiting", bits._tid);
      uint32_t* u = &_data;
      printf("\n\tdata: ");
      for (unsigned i=0; i<(size-2);) {  //
        printf("0x%04x ", u[i++]);
        if (!(i&7)) printf("\n\t      ");
      }
      printf(" size(%u)\n", size);
//      u = (uint32_t*)this;
//      printf("\t"); for (unsigned i=0;i<size;i++) printf("0x%04x ", u[i]); printf("\n");
    }
  }
}
