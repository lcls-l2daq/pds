/*
 * RegisterSlaveExportFrame.cc
 *
 *  Created on: Mar 29, 2010
 *      Author: jackp
 */

#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/PgpRSBits.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Destination.hh"
#include "pds/pgp/Pgp.hh"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "stdio.h"
#include <time.h>
#include <linux/types.h>
//#include "pgpcard/PgpCardMod.h"

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
        PgpRSBits::opcode o,
        Destination* dest,
        unsigned a,
        unsigned transID,
        uint32_t da,
        PgpRSBits::waitState w)
    {
      bits._tid     = transID & ((1<<23)-1);
      bits._waiting = w;
//      printf("RegisterSlaveExportFrame::RegisterSlaveExportFrame() lane %u offset %u\n", dest->lane(), Pgp::portOffset());
      bits._lane    = (dest->lane() & 3) + Pgp::portOffset();
      bits.mbz    = 0;
      bits._vc      = dest->vc() & 3;
      bits.oc      = o;
      bits._addr    = a & addrMask;
      _data         = da;  // NB, for read request size of block requested minus one is placed in data field
      NotSupposedToCare = 0;
    }

    // parameter is the size of the post in number of 32 bit words
    unsigned RegisterSlaveExportFrame::post(__u32 size) {
      struct timeval  timeout;
      PgpCardTx       pgpCardTx;
      int             ret;
      fd_set          fds;

      pgpCardTx.model   = (sizeof(&pgpCardTx));
      pgpCardTx.cmd     = IOCTL_Normal_Write;
      pgpCardTx.pgpVc   = bits._vc;
      pgpCardTx.pgpLane = bits._lane;
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
//      char dn[][20]  = {"Q0", "Q1", "Q2", "Q3", "Cncntr"};
      printf("Register Slave Export Frame: %u %u\n\t", s, n);
      printf("lane(%u), vc(%u), opcode(%s), addr(0x%x), waiting(%s), tid(0x%x), ",
          bits._lane, bits._vc, &ocn[bits.oc][0],
          bits._addr, bits._waiting ? "waiting" : "not waiting", bits._tid);
      printf("data(0x%x)\n", (unsigned)_data);
      uint32_t* u = (uint32_t*)this;
      printf("\t"); for (unsigned i=0;i<s;i++) printf("0x%x ", u[i]); printf("\n");
    }
  }
}
