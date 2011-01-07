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
    unsigned  RegisterSlaveExportFrame::count = 0;
    unsigned  RegisterSlaveExportFrame::errors = 0;

    void RegisterSlaveExportFrame::FileDescr(int i) {
      _fd = i;
      printf("RegisterSlaveExportFrame::FileDescr(%d)\n", _fd);
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

    // parameter is the size of the post in number of 32 bit words
    unsigned RegisterSlaveExportFrame::post(__u32 size) {
      struct timeval  timeout;
      PgpCardTx       pgpCardTx;
      int             ret;
      fd_set          fds;

      pgpCardTx.model   = (sizeof(&pgpCardTx));
      pgpCardTx.cmd     = IOCTL_Normal_Write;
      pgpCardTx.pgpVc   = bits.vc;
      pgpCardTx.pgpLane = bits.lane;
      pgpCardTx.size    = size;
      pgpCardTx.data    = (__u32 *)this;

      // Wait for write ready
      timeout.tv_sec=0;
      timeout.tv_usec=100000;
      FD_ZERO(&fds);
      FD_SET(_fd,&fds);
//      uint32_t* u = (uint32_t*)this;
//      printf("\n\t-->"); for (unsigned i=0;i<size;i++) printf("0x%x ", u[i]); printf("<--\n");
      if ((ret = select( _fd+1, NULL, &fds, NULL, &timeout)) > 0) {
        ::write(_fd, &pgpCardTx, sizeof(pgpCardTx));
      } else {
        if (ret < 0) {
          perror("RegisterSlaveExportFrame post select error: ");
        } else {
          printf("RegisterSlaveExportFrame post select timed out on %u\n", count);
        }
        if (errors++ < 11) print(count, size);
        return Failure;
      }
      count += 1;
      return Success;
    }

    void RegisterSlaveExportFrame::print(unsigned n, unsigned s) {
      char ocn[][20] = {"read", "write", "set", "clear"};
      char dn[][20]  = {"Q0", "Q1", "Q2", "Q3", "Cncntr"};
      printf("Register Slave Export Frame: %u %u\n\t", s, n);
      printf("lane(%u), vc(%u), dest(%s), opcode(%s), addr(0x%x), waiting(%s), tid(0x%x), ",
          bits.lane, bits.vc, &dn[(int)dest()][0], &ocn[bits.oc][0],
          bits.addr, bits.waiting ? "waiting" : "not waiting", bits.tid);
      printf("data(0x%x)\n", (unsigned)_data);
      uint32_t* u = (uint32_t*)this;
      printf("\t"); for (unsigned i=0;i<s;i++) printf("0x%x ", u[i]); printf("\n");
    }
  }
}
