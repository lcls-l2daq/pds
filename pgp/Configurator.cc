/*
 * Configurator.cc
 *
 *  Created on: Apr 6, 2011
 *      Author: jackp
 */

#include "pds/pgp/Configurator.hh"
//#include "PgpCardMod.h"
#include <time.h>
#include <stdio.h>

namespace Pds {
  namespace Pgp {

    Configurator::Configurator(int f, unsigned d) : _fd(f), _debug(d){
      _pgp = new Pds::Pgp::Pgp(_fd);
    }

    Configurator::~Configurator() {}

    void Configurator::print() {}

    long long int Configurator::timeDiff(timespec* end, timespec* start) {
      long long int diff;
      diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
      diff += end->tv_nsec;
      diff -= start->tv_nsec;
      return diff;
    }

    void Configurator::microSpin(unsigned m) {
      long long unsigned gap = 0;
      timespec start, now;
      clock_gettime(CLOCK_REALTIME, &start);
      while (gap < m) {
        clock_gettime(CLOCK_REALTIME, &now);
        gap = timeDiff(&now, &start) / 1000LL;
      }
    }

    unsigned Configurator::checkPciNegotiatedBandwidth() {
      PgpCardStatus status;
      _pgp->readStatus(&status);
      return (status.PciLStatus >> 4)  & 0x3f;
    }

    void Configurator::dumpPgpCard() {
      PgpCardStatus status;
      _pgp->readStatus(&status);
      printf("PGP Card Status:\n");
      printf("\tVersion(0x%x)\n", status.Version);
      printf("\tNegotiated Link Width(0x%x)\n", (status.PciLStatus>>4)&0x3f);
      printf("\tPgpLocLinkReady(%u %u %u %u)\n",
          status.Pgp3LocLinkReady,
          status.Pgp2LocLinkReady,
          status.Pgp1LocLinkReady,
          status.Pgp0LocLinkReady);
      printf("\tPgpRemLinkReady(%u %u %u %u)\n",
          status.Pgp3RemLinkReady,
          status.Pgp2RemLinkReady,
          status.Pgp1RemLinkReady,
          status.Pgp0RemLinkReady);
      printf("\tTxWrite(0x%x)\n", status.TxWrite);
      printf("\tTxRead (0x%x)\n", status.TxRead);
      printf("\tRxWrite(0x%x)\n", status.RxWrite);
      printf("\tRxRead (0x%x)\n", status.RxRead);
//      printf("\t(0x%x)\n", status.);
//      printf("\t(0x%x)\n", status.);
//      printf("\t(0x%x)\n", status.);
//      printf("\t(0x%x)\n", status.);
//      printf("\t(0x%x)\n", status.);
//      printf("\t(0x%x)\n", status.);
//      printf("\t(0x%x)\n", status.);
//      printf("\t(0x%x)\n", status.);
    }

    unsigned ConfigSynch::_getOne() {
       Pds::Pgp::RegisterSlaveImportFrame* rsif;
       unsigned             count = 0;
       while ((rsif = _cfgrt->pgp()->read(_size)) != 0) {
         if (rsif->waiting() == Pds::Pgp::PgpRSBits::Waiting) {
           return Success;
         }
         count += 1;;
       }
       printf("CspadConfigSynch::_getOne _pgp->read failed after skipping %u\n", count);
       return Failure;
     }

     bool ConfigSynch::take() {
       if (_depth == 0) {
         if (_getOne() == Failure) {
           return false;
         }
       } else {
         _depth -= 1;
       }
       return true;
     }

     bool ConfigSynch::clear() {
       int cnt = _length;
       while (cnt) {
         //      printf("/nClearing %d ", _length-cnt);
         if (_getOne() == Failure) {
           printf("CspadConfigSynch::clear receive clearing failed, missing %d\n", cnt);
           return false;
         }
         cnt--;
       }
       return true;
     }
  }
}
