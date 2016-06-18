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

    Configurator::Configurator(int f, unsigned d) : _fd(f), _debug(d) {
      _pgp = new Pds::Pgp::Pgp(_fd, true);
      _G3 = _pgp->G3Flag();
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

    unsigned Configurator::getCurrentFiducial(bool pr) {
      unsigned ret;
      timespec start, end;
      if (pr) {
        clock_gettime(CLOCK_REALTIME, &start);
      }
      ret = _pgp->getCurrentFiducial();
      if (pr) {
        clock_gettime(CLOCK_REALTIME, &end);
      printf("Configurator took %llu nsec to get fiducial\n", timeDiff(&end, &start));
      }
      return ret;
    }

    bool Configurator::evrEnabled() {
    	return _pgp->evrEnabled();
    }

    int   Configurator::evrEnable(bool e) {
      int ret = 0;
      unsigned z = 0;
      unsigned count = 0;
      if (e) {
        while ((evrEnabled()==false) && (count++ < 3) && (ret==0)) {
          printf("Configurator attempting to enable evr %u\n", count);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Set_PLL_RST, z);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Clr_PLL_RST, z);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Set_Reset, z);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Clr_Reset, z);
          ret |= _pgp->IoctlCommand( IOCTL_Evr_Enable, z);
          usleep(4000);
        }
      } else {
        ret = _pgp->IoctlCommand( IOCTL_Evr_Disable, z);
      }
      if (ret || (count >= 3) || (evrEnabled()==false)) {
        printf("Configurator failed to %sable evr!\n", e ? "en" : "dis");
      }
    	return ret;
    }

    int   Configurator::fiducialTarget(unsigned t) {
    	int ret = 0;
    	unsigned arg = (_pgp->portOffset()<<28) | (t & 0x1ffff);
    	ret |= _pgp->IoctlCommand( IOCTL_Evr_Fiducial, arg);
    	if (ret) {
    	  printf("Configurator failed to set %u to fiducial 0x%x\n", _pgp->portOffset(), t);
    	}
    	return ret;
    }

    int   Configurator::waitForFiducialMode(bool b) {
      int ret = 0;
      unsigned one = 1;
      ret |= _pgp->IoctlCommand(
          b ? IOCTL_Evr_LaneModeFiducial : IOCTL_Evr_LaneModeNoFiducial,
          one << _pgp->portOffset());
      if (ret) {
        printf("Configurator failed to set fiducial mode for %u to %s\n",
            _pgp->portOffset(), b ? "true" : "false");
      }
      return ret;
    }

    int   Configurator::evrRunCode(unsigned u) {
      int ret = 0;
      unsigned arg = (_pgp->portOffset() << 28) | (u & 0xff);
      ret |= _pgp->IoctlCommand( IOCTL_Evr_RunCode, arg);
      if (ret) {
        printf("Configurator failed to set evr run code %u for %u\n",
            u, _pgp->portOffset());
      }
      return ret;
    }

    int   Configurator::evrRunDelay(unsigned u) {
      int ret = 0;
      unsigned arg = (_pgp->portOffset() << 28) | (u & 0xfffffff);
      ret |= _pgp->IoctlCommand( IOCTL_Evr_RunDelay, arg);
      if (ret) {
        printf("Configurator failed to set evr run delay %u for %u\n",
            u, _pgp->portOffset());
      }
      return ret;
    }

    int   Configurator::evrDaqCode(unsigned u) {
      int ret = 0;
      unsigned arg = (_pgp->portOffset() << 28) | (u & 0xff);
      ret |= _pgp->IoctlCommand( IOCTL_Evr_AcceptCode, arg);
      if (ret) {
        printf("Configurator failed to set evr daq code %u for %u\n",
            u, _pgp->portOffset());
      }
      return ret;
    }

    int   Configurator::evrDaqDelay(unsigned u) {
      int ret = 0;
      unsigned arg = (_pgp->portOffset() << 28) | (u & 0xfffffff);
      ret |= _pgp->IoctlCommand( IOCTL_Evr_AcceptDelay, arg);
      if (ret) {
        printf("Configurator failed to set evr daq delay %u for %u\n",
            u, _pgp->portOffset());
      }
      return ret;
    }

    int   Configurator::evrLaneEnable(bool e) {
      int ret = 0;
      if (_pgp) {
        unsigned mask = 1 << _pgp->portOffset();
        if (e) {
          ret |= _pgp->IoctlCommand( IOCTL_Evr_LaneEnable, mask);
        } else {
          ret |= _pgp->IoctlCommand( IOCTL_Evr_LaneDisable, mask);
        }
        printf("Configurator  pgpEVR %sable lane completed\n", e ? "en" : "dis");
      } else {
    	  printf("Configurator failed to %sable lane because no pgp object\n", e ? "en" : "dis");
      }
//      if (ret) {
//        printf("Configurator failed to %sable lane mask %u\n", e ? "en" : "dis", mask);
//      } else {
//        printf("Configurator did %sable lane mask %u\n",  e ? "en" : "dis", mask);
//      }
      return ret;
    }


    int Configurator::evrEnableHdrChk(unsigned vc, bool e) {
      int ret;
      unsigned arg = (_pgp->portOffset()<<28) | (vc<<24) | (e ? 1 : 0);
      ret = _pgp->IoctlCommand( IOCTL_Evr_En_Hdr_Check, arg);
      printf("Configurator::evrEnableHdrChk offset %d, arg 0x%x, %s\n",
          _pgp->portOffset(), arg, e ? "true" : "false");
      return ret;
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
         printf("ConfigSynch::_getOne _pgp->read failed\n");
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
