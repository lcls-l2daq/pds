/*
 * Configurator.cc
 *
 *  Created on: Apr 6, 2011
 *      Author: jackp
 */

#include "pds/pgp/Configurator.hh"
#include "pds/pgp/Destination.hh"
#include "pgpcard/PgpCardMod.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Pds {
  namespace Pgp {

    Configurator::Configurator(int f, unsigned d, bool g3) : _fd(f), _debug(d), _G3(g3) {
      _pgp = new Pds::Pgp::Pgp(_fd, true, g3);
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
      return _pgp->checkPciNegotiatedBandwidth();
    }

    void Configurator::loadRunTimeConfigAdditions(char* name) {
      if (name && strlen(name)) {
        FILE* f;
        Destination _d;
        unsigned maxCount = 1024;
        char path[240];
        char* home = getenv("HOME");
        sprintf(path,"%s/%s",home, name);
        f = fopen (path, "r");
        if (!f) {
          char s[200];
          sprintf(s, "Could not open %s ", path);
          perror(s);
        } else {
          unsigned myi = 0;
          unsigned dest, addr, data;
          while (fscanf(f, "%x %x %x", &dest, &addr, &data) && !feof(f) && myi++ < maxCount) {
            _d.dest(dest);
            printf("\nRun time config: dest %s, addr 0x%x, data 0x%x ", _d.name(), addr, data);
            if(_pgp->writeRegister(&_d, addr, data)) {
              printf("\nConfigurator::loadRunTimeConfigAdditions failed on dest %u address 0x%x\n", dest, addr);
            }
          }
          if (!feof(f)) {
            perror("Error reading");
          }
//          printf("\nSleeping 200 microseconds\n");
//          microSpin(200);
        }
      }
    }


    void Configurator::dumpPgpCard() {
    	_pgp->printStatus();
    }

    unsigned ConfigSynch::_getOne() {
       Pds::Pgp::RegisterSlaveImportFrame* rsif;
       unsigned             count = 0;
       while ((count < 6) && (rsif = _cfgrt->pgp()->read(_size)) != 0) {
         if (rsif->waiting() == Pds::Pgp::PgpRSBits::Waiting) {
           return Success;
         }
         count += 1;;
       }
       if (_printFlag) {
         printf("CspadConfigSynch::_getOne _pgp->read failed\n");
         if (count) printf(" after skipping %u\n", count);
       }
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
       _printFlag = false | (_cfgrt->_debug&1);
       for (unsigned cnt=_depth; cnt<_length; cnt++) { _getOne(); }
       return true;
     }
  }
}
