/*
 * CspadServer.cc
 *
 *  Created on: Nov 15, 2010
 *      Author: jackp
 */

#include "pds/csPad/CspadServer.hh"
#include "pds/csPad/CspadConfigurator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/csPad/CspadConfigurator.hh"
#include "pdsdata/cspad/ElementV1.hh"
#include "PgpCardMod.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

using namespace Pds;
//using namespace Pds::CsPad;

CspadServer* CspadServer::_instance = 0;

static Pds::TypeId _CsPadDataType(Pds::TypeId::Id_CspadElement,
                                    Pds::CsPad::ElementV1::Version);

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

CspadServer::CspadServer( const Pds::Src& client, unsigned configMask )
   : _xtc( _CsPadDataType, client ),
     _cnfgrtr(0),
     _quads(0),
     _configMask(configMask),
     _configureResult(0),
     _debug(0),
     _offset(0),
     _configured(false),
     _firstFetch(true) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  instance(this);
}

unsigned CspadServer::configure(CsPadConfigType* config) {
  if (_cnfgrtr == 0) {
    _cnfgrtr = new Pds::CsPad::CspadConfigurator::CspadConfigurator(config, fd(), _debug);
  } else {
    printf("CspadConfigurator already instantiated\n");
  }
  unsigned c = flushInputQueue(fd());
  if (c) printf("CspadServer::configure flushed %u event%s before configuration\n", c, c>1 ? "s" : "");
  if ((_configureResult = _cnfgrtr->configure(_configMask))) {
    printf("CspadServer::configure failed 0x%x\n", _configureResult);
  } else {
    _quads = 0;
    for (unsigned i=0; i<4; i++) if ((1<<i) & config->quadMask()) _quads += 1;
    _payloadSize = config->payloadSize();
    _xtc.extent = (_payloadSize * _quads) + sizeof(Xtc);
    printf("CspadServer::configure _quads(%u) _payloadSize(%u) _xtc.extent(%u)\n",
        _quads, _payloadSize, _xtc.extent);
  }
  _firstFetch = true;
  _count = _quadsThisCount = 0;
  _configured = _configureResult == 0;
  c = this->flushInputQueue(fd());
  if (c) printf("CspadServer::configure flushed %u event%s after confguration\n", c, c>1 ? "s" : "");
  return _configureResult;
}

void Pds::CspadServer::die() {
  if (_cnfgrtr != 0) {
    _cnfgrtr->writeRegister(
          Pds::Pgp::RegisterSlaveExportFrame::CR,
          CsPad::CspadConfigurator::RunModeAddr,
          _cnfgrtr->configuration().inactiveRunMode());
    printf("CspadServer::die has been called !!!!!!!\n");
   }
}

void Pds::CspadServer::dumpFrontEnd() {
  disable();
  _cnfgrtr->dumpFrontEnd();
}

void Pds::CspadServer::enable() {
  _cnfgrtr->writeRegister(
      Pds::Pgp::RegisterSlaveExportFrame::CR,
      CsPad::CspadConfigurator::RunModeAddr,
      _cnfgrtr->configuration().activeRunMode());
  ::usleep(10000);
  _firstFetch = true;
  flushInputQueue(fd());
  if (_debug & 0x20) printf("CspadServer::enable\n");
}

void Pds::CspadServer::disable() {
  _cnfgrtr->writeRegister(
      Pds::Pgp::RegisterSlaveExportFrame::CR,
      CsPad::CspadConfigurator::RunModeAddr,
      _cnfgrtr->configuration().inactiveRunMode());
  ::usleep(10000);
  flushInputQueue(fd());
  if (_debug & 0x20) printf("CspadServer::disable\n");
}

unsigned Pds::CspadServer::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("CspadServer::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int Pds::CspadServer::fetch( char* payload, int flags ) {
   int ret = 0;
   PgpCardRx       pgpCardRx;
   unsigned        offset = 0;
   enum {Ignore=-1};

   if (_configured == false)  {
     printf("CspadServer::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
     unsigned c = this->flushInputQueue(fd());
     printf("\tWe flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_debug & 1) printf("CspadServer::fetch called ");

   _xtc.damage = 0;

   _quadsThisCount %= _quads;

   if (!_quadsThisCount) {
     memcpy( payload, &_xtc, sizeof(Xtc) );
     offset = sizeof(Xtc);
     if (_firstFetch) {
       _firstFetch = false;
       clock_gettime(CLOCK_REALTIME, &_lastTime);
     } else {
       clock_gettime(CLOCK_REALTIME, &_thisTime);
       long long unsigned diff = timeDiff(&_thisTime, &_lastTime);
       diff += 500000;
       diff /= 1000000;
       if (diff > sizeOfHisto-1) diff = sizeOfHisto-1;
       _histo[diff] += 1;
       memcpy(&_lastTime, &_thisTime, sizeof(timespec));
     }
   }

   pgpCardRx.model   = sizeof(&pgpCardRx);
   pgpCardRx.maxSize = _payloadSize / sizeof(__u32);
   pgpCardRx.data    = (__u32*)(payload + offset);

   if ((ret = read(fd(), &pgpCardRx, sizeof(PgpCardRx))) < 0) {
     perror ("CspadServer::fetch pgpCard read error");
     ret =  Ignore;
   } else ret *= sizeof(__u32);

   if ((ret > 0) && (ret < (int)_payloadSize)) {
     printf("CspadServer::fetch() returning Ignore, ret was %d ", ret);
     ret = Ignore;
   }

   unsigned damageMask = 0;
   if (pgpCardRx.eofe)      damageMask |= 1;
   if (pgpCardRx.fifoErr)   damageMask |= 2;
   if (pgpCardRx.lengthErr) damageMask |= 4;
   if (damageMask) {
     damageMask |= 0xe0;
     _xtc.damage.increase(Pds::Damage::UserDefined);
     _xtc.damage.userBits(damageMask);
     printf("CsPadServer::fetch setting user damage 0x%x", damageMask);
     if (pgpCardRx.lengthErr) printf(", rxSize(%u)", (unsigned)pgpCardRx.rxSize);
     printf("\n");
   } else {
     Pds::Pgp::DataImportFrame* data = (Pds::Pgp::DataImportFrame*)(payload + offset);
     unsigned oldCount = _count;
     _count = data->frameNumber() - 1;  // cspad starts counting at 1, not zero
     if (_debug & 4 || ret < 0) printf("\n\tportNumber(%u) fiducials(0x%x) _oldCount(%u) _count(%u) _quadsThisCount(%u) lane(%u) vc(%u)\n",
         data->portNumber(), data->fiducials(), oldCount, _count, _quadsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
     if ((_count != oldCount) && (_quadsThisCount)) {
       _quadsThisCount = 0;
       memcpy( payload, &_xtc, sizeof(Xtc) );
       ret = sizeof(Xtc);
     }
   }
   if (ret > 0) {
     _quadsThisCount += 1;
     ret += offset;
   }
   if (_debug & 1) printf(" returned %d\n", ret);
   return ret;
}

bool CspadServer::more() const {
  bool ret = _quads > 1;
  if (_debug & 2) printf("CspadServer::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned CspadServer::offset() const {
  unsigned ret = _quadsThisCount == 1 ? 0 : sizeof(Xtc) + _payloadSize * (_quadsThisCount-1);
  if (_debug & 2) printf("CspadServer::offset(%u)\n", ret);
  return (ret);
}

unsigned CspadServer::count() const {
  if (_debug & 2) printf( "CspadServer::count(%u)\n", _count);
  return _count + _offset;
}

unsigned CspadServer::flushInputQueue(int f) {
  fd_set          fds;
  struct timeval  timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = 2500;
  int ret;
  unsigned dummy[5];
  unsigned count = 0;
  PgpCardRx       pgpCardRx;
  pgpCardRx.model   = sizeof(&pgpCardRx);
  pgpCardRx.maxSize = 5;
  pgpCardRx.data    = dummy;
  do {
    FD_ZERO(&fds);
    FD_SET(f,&fds);
    ret = select( f+1, &fds, NULL, NULL, &timeout);
    if (ret>0) {
      count += 1;
      ::read(f, &pgpCardRx, sizeof(PgpCardRx));
    }
  } while (ret > 0);
  return count;
}

void CspadServer::setCspad( int f ) {
  if (unsigned c = this->flushInputQueue(f)) {
    printf("CspadServer::setCspad read %u time%s after opening pgpcard driver\n", c, c==1 ? "" : "s");
  }
  fd( f );
  Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
}

void CspadServer::printHisto(bool c) {
  printf("CspadServer event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      if (c) _histo[i] = 0;
    }
  }
}
