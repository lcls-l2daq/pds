/*
 * RegisterSlaveExportFrame.cc
 *
 *  Created on: Mar 29, 2010
 *      Author: jackp
 */

#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "stdio.h"
#include <time.h>
#include <linux/types.h>
//#include "pds/csPad/PgpCardMod.h"
#include "PgpCardMod.h"

namespace Pds {
  namespace Pgp {

    int       RegisterSlaveExportFrame::_fd  = 0;
    fd_set    RegisterSlaveExportFrame::_fds;

    void RegisterSlaveExportFrame::FileDescr(int i) {
      _fd = i;
      FD_ZERO(&_fds);
      FD_SET(_fd,&_fds);
    }

    RegisterSlaveExportFrame::RegisterSlaveExportFrame(
        opcode o,
        FEdest dt,
        unsigned a,
        unsigned transID,
        uint32_t da,
        waitState w)
    {
      bits.tid     = transID & ((1<<23)-1);
      bits.waiting = w & 1;
      bits.lane    = (dt & laneMask) ? 1 : 0;
      bits.mbz    = 0;
      bits.vc      = dt & concentratorMask ? 0 : (dt & 1) + 1;
      bits.oc      = o;
      bits.addr    = a & addrMask;
      _data         = da;
      NotSupposedToCare = 0;
    }

    RegisterSlaveExportFrame::FEdest RegisterSlaveExportFrame::dest() {
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

    unsigned RegisterSlaveExportFrame::post(__u32 size) {
      struct timeval  timeout;
      PgpCardTx       pgpCardTx;
      PgpCardRx       pgpCardRx;
      RegisterSlaveImportFrame inf;

      pgpCardRx.model   = sizeof(&pgpCardRx);
      pgpCardRx.maxSize = sizeof(inf);
      pgpCardRx.data    = (__u32*)&inf;

      pgpCardTx.model   = (sizeof(&pgpCardTx));
      pgpCardTx.cmd     = IOCTL_Normal_Write;
      pgpCardTx.pgpVc   = bits.vc;
      pgpCardTx.pgpLane = bits.lane;
      pgpCardTx.size    = size / sizeof(__u32);
      pgpCardTx.data    = (__u32 *)this;

      // Wait for write ready
      timeout.tv_sec=0;
      timeout.tv_usec=1000;
      if (select( _fd+1, NULL, &_fds, NULL, &timeout) > 0) {
        ::write(_fd, &pgpCardTx, sizeof(pgpCardTx));
        if ((!bits.waiting) && (bits.oc != Pds::Pgp::RegisterSlaveExportFrame::read)) {
          if (select( _fd+1, &_fds, NULL, NULL, &timeout) > 0) {
            ::read(_fd,&pgpCardRx,sizeof(PgpCardRx));
          } else {
            printf("RegisterSlaveExportFrame read select timed out\n");
            return Failure;
          }
        }
      } else {
        printf("RegisterSlaveExportFrame post select timed out\n");
        return Failure;
      }
      //    this->print();
      return Success;
    }

    void RegisterSlaveExportFrame::print() {
      char ocn[][20] = {"read", "write", "set", "clear"};
      char dn[][20]  = {"Q0", "Q1", "Q2", "Q3", "Cncntr"};
      printf("Register Slave Export Frame:\n\t");
      printf("lane(%u), vc(%u), dest(%s), opcode(%s), addr(0x%x), waiting(%s), tid(0x%x)\n\t",
          bits.lane, bits.vc, &dn[(int)dest()][0], &ocn[bits.oc][0],
          bits.addr, bits.waiting ? "waiting" : "not waiting", bits.tid);
      printf("data(0x%x)\n", (unsigned)_data);

    }
  }
}
