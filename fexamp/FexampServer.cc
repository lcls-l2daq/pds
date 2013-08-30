/*
 * FexampServer.cc
 *
 *  Created on: Nov 15, 2010
 *      Author: jackp
 */

#include "pds/fexamp/FexampServer.hh"
#include "pds/fexamp/FexampConfigurator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/config/FexampConfigType.hh"
#include "pds/config/FexampDataType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/fexamp/FexampDestination.hh"
#include "pds/fexamp/Processor.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pgpcard/PgpCardMod.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

using namespace Pds;
//using namespace Pds::Fexamp;

FexampServer* FexampServer::_instance = 0;

class Task;
class TaskObject;
class Pds::Fexamp::FexampDestination;
class DetInfo;

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec);
  if (diff) diff *= 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

FexampServer::FexampServer( const Pds::Src& client, unsigned configMask )
   : _xtc( _FexampDataType, client ),
     _cnfgrtr(0),
     _elements(ElementsPerSegmentLevel),
     _payloadSize(0),
     _configMask(configMask),
     _configureResult(0),
     _debug(0),
     _offset(0),
     _unconfiguredErrors(0),
     _configured(false),
     _firstFetch(true) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  _task = new Pds::Task(Pds::TaskObject("FEXAMPprocessor"));
  instance(this);
  printf("FexampServer::FexampServer() payload(%u)\n", _payloadSize);
}

unsigned FexampServer::configure(FexampConfigType* config) {
  if (_cnfgrtr == 0) {
    _cnfgrtr = new Pds::Fexamp::FexampConfigurator::FexampConfigurator(fd(), _debug);
  }
  // first calculate payload size in uint32s
  _payloadSize = FexampDataType::Uint32sPerSingleSampleArray;
  _payloadSize *=  (config->get(FexampConfigType::PerMclkCount)+1)>>4;
  _payloadSize +=  FexampDataType::Uint32sInLastWords;
  // then convert to bytes and add the size of the element header
  _payloadSize *= sizeof(uint32_t);
  _payloadSize +=  sizeof(FexampDataType);
  unsigned c = flushInputQueue(fd());
  if (c) printf("FexampServer::configure flushed %u event%s before configuration\n", c, c>1 ? "s" : "");

  if ((_configureResult = _cnfgrtr->configure(config, _configMask))) {
    printf("FexampServer::configure failed 0x%x\n", _configureResult);
  } else {
    _xtc.extent = (_payloadSize * _elements) + sizeof(Xtc);
    printf("CspadServer::configure _elements(%u) _payloadSize(%u) _xtc.extent(%u)\n",
        _elements, _payloadSize, _xtc.extent);
  }
  _firstFetch = true;
  _count = _elementsThisCount = 0;
  _configured = _configureResult == 0;
  c = this->flushInputQueue(fd());
  if (c) printf("FexampServer::configure flushed %u event%s after confguration\n", c, c>1 ? "s" : "");

  return _configureResult;
}

void Pds::FexampServer::die() {
  if (_pgp != 0) {
    printf("FexampServer::die has been called !!!!!!!\n");
   }
}

void Pds::FexampServer::dumpFrontEnd() {
  disable();
  _cnfgrtr->dumpFrontEnd();
}

void FexampServer::process() {
//  _ioIndex = ++_ioIndex % Pds::Fexamp::numberOfFrames;
//  Pds::Routine* r = new Pds::Fexamp::Processor(4, _ioIndex);
//  _task->call(r);
}

void FexampServer::allocated() {
  _cnfgrtr->resetFrontEnd(Fexamp::MasterReset);
}

void Pds::FexampServer::enable() {
  if (usleep(10000)<0) perror("FexampServer::enable ulseep failed\n");
  usleep(10000);
  _cnfgrtr->enableExternalTrigger(true);
  _firstFetch = true;
  flushInputQueue(fd());
  if (_debug & 0x20) printf("FexampServer::enable\n");
}

void Pds::FexampServer::disable() {
  _cnfgrtr->enableExternalTrigger(false);
  flushInputQueue(fd());
  if (usleep(10000)<0) perror("FexampServer::disable ulseep 1 failed\n");
  if (_debug & 0x20) printf("FexampServer::disable\n");
}

unsigned Pds::FexampServer::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("FexampServer::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int Pds::FexampServer::fetch( char* payload, int flags ) {
   int ret = 0;
   PgpCardRx       pgpCardRx;
   unsigned        offset = 0;
   enum {Ignore=-1};

   if (_configured == false)  {
     if (++_unconfiguredErrors<20) printf("FexampServer::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
     unsigned c = this->flushInputQueue(fd());
     if (_unconfiguredErrors<20 && c) printf("\tWe flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_debug & 1) printf("FexampServer::fetch called ");

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
     perror ("FexampServer::fetch pgpCard read error");
     ret =  Ignore;
   } else ret *= sizeof(__u32);
   Pds::Pgp::DataImportFrame* data = (Pds::Pgp::DataImportFrame*)(payload + offset);

   if ((ret > 0) && (ret < (int)_payloadSize)) {
     printf("FexampServer::fetch() returning Ignore, ret was %d, looking for %u\n", ret, _payloadSize);
     if (_debug & 4 || ret < 0) printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) raw_count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
         data->elementId(), data->_frameType, data->acqCount(), data->frameNumber(), _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
     uint32_t* u = (uint32_t*)data;
     printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
     ret = Ignore;
   }

   if (ret > (int) _payloadSize) printf("FexampServer::fetch pgp read returned too much _payloadSize(%u) ret(%d)\n", _payloadSize, ret);

   unsigned damageMask = 0;
   if (pgpCardRx.eofe)      damageMask |= 1;
   if (pgpCardRx.fifoErr)   damageMask |= 2;
   if (pgpCardRx.lengthErr) damageMask |= 4;
   if (damageMask) {
     damageMask |= 0xe0;
     _xtc.damage.increase(Pds::Damage::UserDefined);
     _xtc.damage.userBits(damageMask);
     printf("FexampServer::fetch setting user damage 0x%x", damageMask);
     if (pgpCardRx.lengthErr) printf(", rxSize(%u), maxSize(%u) ret(%d) offset(%u) (bytes)",
         (unsigned)pgpCardRx.rxSize*sizeof(uint32_t), _payloadSize, ret, offset);
     printf("\n");
   } else {
     unsigned oldCount = _count;
     _count = data->frameNumber() - 1;  // cspad starts counting at 1, not zero, I wonder what fexamp does
     if (_debug & 4 || ret < 0) {
       printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) _oldCount(%u) _count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
           data->elementId(), data->_frameType, data->acqCount(),  oldCount, _count, _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
       uint32_t* u = (uint32_t*)data;
       printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
     }
     if ((_count != oldCount) && (_elementsThisCount)) {
       printf("FexampServer::fetch found new frame %d when frame %d had only %u elements\n", _count, oldCount, _elementsThisCount);
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

bool FexampServer::more() const {
  bool ret = _elements > 1;
  if (_debug & 2) printf("FexampServer::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned FexampServer::offset() const {
  unsigned ret = _elementsThisCount == 1 ? 0 : sizeof(Xtc) + _payloadSize * (_elementsThisCount-1);
  if (_debug & 2) printf("FexampServer::offset(%u)\n", ret);
  return (ret);
}

unsigned FexampServer::count() const {
  if (_debug & 2) printf( "FexampServer::count(%u)\n", _count + _offset);
  return _count + _offset;
}

unsigned FexampServer::flushInputQueue(int f) {
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

void FexampServer::setFexamp( int f ) {
  if (unsigned c = this->flushInputQueue(f)) {
    printf("FexampServer::setFexamp read %u time%s after opening pgpcard driver\n", c, c==1 ? "" : "s");
  }
  fd( f );
//  _pgp = new Pds::Pgp::Pgp::Pgp(f);
  Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
  if (_cnfgrtr == 0) {
    _cnfgrtr = new Pds::Fexamp::FexampConfigurator::FexampConfigurator(fd(), _debug);
  }
}

void FexampServer::printHisto(bool c) {
  printf("FexampServer event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      if (c) _histo[i] = 0;
    }
  }
}
