/*
 * XampsServer.cc
 *
 *  Created on: Nov 15, 2010
 *      Author: jackp
 */

#include "pds/xamps/XampsServer.hh"
#include "pds/xamps/XampsConfigurator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/config/XampsConfigType.hh"
#include "pds/config/XampsDataType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/xamps/XampsConfigurator.hh"
#include "pds/xamps/XampsDestination.hh"
#include "pds/xamps/Processor.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "PgpCardMod.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

using namespace Pds;
//using namespace Pds::Xamps;

XampsServer* XampsServer::_instance = 0;

class Task;
class TaskObject;
class Pds::Xamps::XampsDestination;
class DetInfo;

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec);
  if (diff) diff *= 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

XampsServer::XampsServer( const Pds::Src& client, unsigned configMask )
   : _xtc( _XampsDataType, client ),
     _cnfgrtr(0),
     _elements(ElementsPerSegmentLevel),
     _payloadSize(sizeof(XampsDataType)),
     _configMask(configMask),
     _configureResult(0),
     _debug(0),
     _offset(0),
     _unconfiguredErrors(0),
     _configured(false),
     _firstFetch(true),
     _iHaveLaneZero(false) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  _task = new Pds::Task(Pds::TaskObject("XAMPSprocessor"));
  instance(this);
  printf("XampsServer::XampsServer() payload(%u)\n", _payloadSize);
}

//void XampsServer::laneTest() {
//  uint32_t deviceId = static_cast<DetInfo&>(_xtc.src).devId();
//  uint32_t lanes[2] = { 0xff, 0xff };
//  unsigned ret = 0;
//  _d.dest(Pds::Xamps::XampsDestination::Internal);
//  if (_pgp->readRegister(&_d, Xamps::XampsConfigurator::LaneIdAddr, 0x11, lanes)) ret |= 1;
//  _d.dest(Pds::Xamps::XampsDestination::InternalLane1);
//  if (_pgp->readRegister(&_d, Xamps::XampsConfigurator::LaneIdAddr, 0x12, lanes+1)) ret |= 2;
//  if ((lanes[0] & 0xfffffff0) != Xamps::XampsConfigurator::LaneIdStuffing) ret |= 4;
//  else lanes[0] &= 3;
//  if ((lanes[1] & 0xfffffff0) != Xamps::XampsConfigurator::LaneIdStuffing) ret |= 8;
//  else lanes[1] &= 3;  // clear out the stuffing
//  if (!ret) {
//    if (deviceId & 1 == 0 ) {
//      if ((lanes[0] == 0) && (lanes[1] == 1)) {
//        _iHaveLaneZero = true;
//      } else {
//        printf("\nBAD LANES for Master Device ID %u, (%u,%u)\n", deviceId, lanes[0], lanes[1]);
//        ret = 0x10;
//      }
//    } else {
//      if ((lanes[0] == 2) && (lanes[1] == 3)) {
//        _iHaveLaneZero = false;
//      } else {
//        printf("\nBAD LANES for Slave Device ID %u, (%u,%u)\n", deviceId, lanes[0], lanes[1]);
//        ret = 0x20;
//      }
//    }
//  }
//  _iHaveLaneZero = true;                 // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  if (ret) {
//    printf("\nLANE TEST FAILED laned0=%x, lane1=%x, ret=0x%x\n",
//        (unsigned)lanes[0], (unsigned)lanes[1], ret);
////    usleep(100000);
////    _exit(16);
//  }
//}

unsigned XampsServer::configure(XampsConfigType* config) {
  if (_cnfgrtr == 0) {
    _cnfgrtr = new Pds::Xamps::XampsConfigurator::XampsConfigurator(config, fd(), _debug);
  } else {
    printf("XampsConfigurator already instantiated\n");
  }
  unsigned c = flushInputQueue(fd());
  if (c) printf("XampsServer::configure flushed %u event%s before configuration\n", c, c>1 ? "s" : "");
  //  if (_iHaveLaneZero) {
  if ((_configureResult = _cnfgrtr->configure(_configMask))) {
    printf("XampsServer::configure failed 0x%x\n", _configureResult);
  } else {
    _xtc.extent = (_payloadSize * _elements) + sizeof(Xtc);
    printf("CspadServer::configure _elements(%u) _payloadSize(%u) _xtc.extent(%u)\n",
        _elements, _payloadSize, _xtc.extent);
  }
  _firstFetch = true;
  _count = _elementsThisCount = 0;
  _configured = _configureResult == 0;
  c = this->flushInputQueue(fd());
  if (c) printf("XampsServer::configure flushed %u event%s after confguration\n", c, c>1 ? "s" : "");
  //  }
  return _configureResult;
}

void Pds::XampsServer::die() {
  if (_pgp != 0) {
    printf("XampsServer::die has been called !!!!!!!\n");
   }
}

void Pds::XampsServer::dumpFrontEnd() {
  disable();
  _cnfgrtr->dumpFrontEnd();
}

void XampsServer::process() {
//  _ioIndex = ++_ioIndex % Pds::Xamps::numberOfFrames;
//  Pds::Routine* r = new Pds::Xamps::Processor(4, _ioIndex);
//  _task->call(r);
}

void Pds::XampsServer::enable() {
  _d.dest(Pds::Xamps::XampsDestination::External);
  _pgp->writeRegister(
      &_d,
      Xamps::XampsConfigurator::RunModeAddr,
      _cnfgrtr->testModeState() | Xamps::XampsConfigurator::RunModeValue);
  ::usleep(10000);
  _firstFetch = true;
  flushInputQueue(fd());
  if (_debug & 0x20) printf("XampsServer::enable\n");
}

void Pds::XampsServer::disable() {
  _pgp->writeRegister(
      &_d,
      Xamps::XampsConfigurator::RunModeAddr,
      _cnfgrtr->testModeState());
  ::usleep(10000);
  flushInputQueue(fd());
  if (_debug & 0x20) printf("XampsServer::disable\n");
}

unsigned Pds::XampsServer::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("XampsServer::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int Pds::XampsServer::fetch( char* payload, int flags ) {
   int ret = 0;
   PgpCardRx       pgpCardRx;
   unsigned        offset = 0;
   enum {Ignore=-1};

   if (_configured == false)  {
     if (++_unconfiguredErrors<20) printf("XampsServer::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
     unsigned c = this->flushInputQueue(fd());
     if (_unconfiguredErrors<20 && c) printf("\tWe flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_debug & 1) printf("XampsServer::fetch called ");

   _xtc.damage = 0;

   _elementsThisCount %= _elements;

   if (!_elementsThisCount) {
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
     perror ("XampsServer::fetch pgpCard read error");
     ret =  Ignore;
   } else ret *= sizeof(__u32);
   Pds::Pgp::DataImportFrame* data = (Pds::Pgp::DataImportFrame*)(payload + offset);

   if ((ret > 0) && (ret < (int)_payloadSize)) {
     printf("XampsServer::fetch() returning Ignore, ret was %d, looking for %u\n", ret, _payloadSize);
     if (_debug & 4 || ret < 0) printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) raw_count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
         data->elementId(), data->_frameType, data->acqCount(), data->frameNumber(), _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
     uint32_t* u = (uint32_t*)data;
     printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
     ret = Ignore;
   }

   if (ret > (int) _payloadSize) printf("XampsServer::fetch pgp read returned too much _payloadSize(%u) ret(%d)\n", _payloadSize, ret);

   unsigned damageMask = 0;
   if (pgpCardRx.eofe)      damageMask |= 1;
   if (pgpCardRx.fifoErr)   damageMask |= 2;
   if (pgpCardRx.lengthErr) damageMask |= 4;
   if (damageMask) {
     damageMask |= 0xe0;
     _xtc.damage.increase(Pds::Damage::UserDefined);
     _xtc.damage.userBits(damageMask);
     printf("XampsServer::fetch setting user damage 0x%x", damageMask);
     if (pgpCardRx.lengthErr) printf(", rxSize(%u), maxSize(%u) ret(%d) offset(%u) (bytes)",
         (unsigned)pgpCardRx.rxSize*sizeof(uint32_t), _payloadSize, ret, offset);
     printf("\n");
   } else {
     unsigned oldCount = _count;
     _count = data->frameNumber()/* - 1*/;  // cspad starts counting at 1, not zero, I wonder what xamps does
     if (_debug & 4 || ret < 0) {
       printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) _oldCount(%u) _count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
           data->elementId(), data->_frameType, data->acqCount(),  oldCount, _count, _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
       uint32_t* u = (uint32_t*)data;
       printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
     }
     if ((_count != oldCount) && (_elementsThisCount)) {
       _elementsThisCount = 0;
       memcpy( payload, &_xtc, sizeof(Xtc) );
       ret = sizeof(Xtc);
     }
   }
   if (ret > 0) {
     _elementsThisCount += 1;
     ret += offset;
   }
   if (_debug & 5) printf(" returned %d\n", ret);
   return ret;
}

bool XampsServer::more() const {
  bool ret = _elements > 1;
  if (_debug & 2) printf("XampsServer::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned XampsServer::offset() const {
  unsigned ret = _elementsThisCount == 1 ? 0 : sizeof(Xtc) + _payloadSize * (_elementsThisCount-1);
  if (_debug & 2) printf("XampsServer::offset(%u)\n", ret);
  return (ret);
}

unsigned XampsServer::count() const {
  if (_debug & 2) printf( "XampsServer::count(%u)\n", _count + _offset);
  return _count + _offset;
}

unsigned XampsServer::flushInputQueue(int f) {
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

void XampsServer::setXamps( int f ) {
  if (unsigned c = this->flushInputQueue(f)) {
    printf("XampsServer::setXamps read %u time%s after opening pgpcard driver\n", c, c==1 ? "" : "s");
  }
  fd( f );
  _pgp = new Pds::Pgp::Pgp::Pgp(f);
  Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
}

void XampsServer::printHisto(bool c) {
  printf("XampsServer event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      if (c) _histo[i] = 0;
    }
  }
}
