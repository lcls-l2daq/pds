/*
 * EpixServer.cc
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#include "pds/epix/EpixServer.hh"
#include "pds/epix/EpixConfigurator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixDataType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/epix/EpixDestination.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pgpcard/PgpCardMod.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <omp.h>

using namespace Pds;
//using namespace Pds::Epix;

EpixServer* EpixServer::_instance = 0;

class Task;
class TaskObject;
class Pds::Epix::EpixDestination;
class DetInfo;

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec);
  if (diff) diff *= 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}



EpixServer::EpixServer( const Pds::Src& client, unsigned configMask )
   : _xtc( _epixDataType, client ),
     _cnfgrtr(0),
     _elements(ElementsPerSegmentLevel),
     _payloadSize(0),
     _configureResult(0),
     _debug(0),
     _offset(0),
     _unconfiguredErrors(0),
     _processorBuffer(0),
     _configured(false),
     _firstFetch(true),
     _ignoreFetch(true),
     _resetOnEveryConfig(false) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  _task = new Pds::Task(Pds::TaskObject("EPIXprocessor"));
  instance(this);
  printf("EpixServer::EpixServer() payload(%u)\n", _payloadSize);
}

unsigned EpixServer::configure(EpixConfigType* config, bool forceConfig) {
  unsigned firstConfig = _resetOnEveryConfig || forceConfig;
  if (_cnfgrtr == 0) {
    firstConfig = 1;
    _cnfgrtr = new Pds::Epix::EpixConfigurator::EpixConfigurator(fd(), _debug);
    printf("EpixServer::configure making new configurator %p, firstConfig %u\n", _cnfgrtr, firstConfig);
  }
  _payloadSize = EpixDataType::_sizeof(*config);
  if (_processorBuffer) free(_processorBuffer);
  _processorBuffer = (char*) calloc(1, _payloadSize);
  printf("EpixServer::configure allocated processorBuffer size %u, firstConfig %u\n", _payloadSize, firstConfig);
  if (!_processorBuffer) {
    printf("EpixServer::configure FAILED to allocated processor buffer!!!\n");
    return 0xdeadbeef;
  }
  unsigned c = flushInputQueue(fd());
  if (c) printf("EpixServer::configure flushed %u event%s before configuration\n", c, c>1 ? "s" : "");

  if ((_configureResult = _cnfgrtr->configure(config, firstConfig))) {
    printf("EpixServer::configure failed 0x%x, first %u\n", _configureResult, firstConfig);
  } else {
    _xtc.extent = (_payloadSize * _elements) + sizeof(Xtc);
    printf("EpixServer::configure _elements(%u) _payloadSize(%u) _xtc.extent(%u) firstConfig(%u)\n",
        _elements, _payloadSize, _xtc.extent, firstConfig);
  }
  _firstFetch = true;
  _count = _elementsThisCount = 0;
  _configured = _configureResult == 0;
  c = this->flushInputQueue(fd());
  clearHisto();
  if (c) printf("EpixServer::configure flushed %u event%s after confguration\n", c, c>1 ? "s" : "");

  return _configureResult;
}

void Pds::EpixServer::die() {
    printf("EpixServer::die has been called !!!!!!!\n");
}

void Pds::EpixServer::dumpFrontEnd() {
  disable();
  _cnfgrtr->dumpFrontEnd();
}

static unsigned* procHisto = (unsigned*) calloc(1000, sizeof(unsigned));

void EpixServer::process(char* d) {
  Pds::Epix::ElementV1* e = (Pds::Epix::ElementV1*) _processorBuffer;
  timespec end;
  timespec start;
  clock_gettime(CLOCK_REALTIME, &start);
  Pds::Epix::ElementV1* o = (Pds::Epix::ElementV1*) d;

  //  header
  memcpy(o, e, sizeof(Pds::Epix::ElementV1));

  //  frame data
  unsigned nrows   = _cnfgrtr->configuration().numberOfRows()/2;
  unsigned colsize = _cnfgrtr->configuration().numberOfColumns()*sizeof(uint16_t);
  ndarray<const uint16_t,2> iframe = e->frame(_cnfgrtr->configuration());
  ndarray<const uint16_t,2> oframe = o->frame(_cnfgrtr->configuration());
  for(unsigned i=0; i<nrows; i++) {
    memcpy(const_cast<uint16_t*>(&oframe[nrows+i+0][0]), &iframe[2*i+0][0], colsize);
    memcpy(const_cast<uint16_t*>(&oframe[nrows-i-1][0]), &iframe[2*i+1][0], colsize);
  }

  unsigned tsz = reinterpret_cast<const uint8_t*>(e) + e->_sizeof(_cnfgrtr->configuration()) - reinterpret_cast<const uint8_t*>(iframe.end());
  memcpy(const_cast<uint16_t*>(oframe.end()), iframe.end(), tsz);

  clock_gettime(CLOCK_REALTIME, &end);
  long long unsigned diff = timeDiff(&end, &start);
  diff += 50000;
  diff /= 100000;
  if (diff > 1000-1) diff = 1000-1;
  procHisto[diff] += 1;
}

void EpixServer::allocated() {
//  _cnfgrtr->resetFrontEnd();
}

void Pds::EpixServer::enable() {
  if (usleep(10000)<0) perror("EpixServer::enable ulseep failed\n");
  usleep(10000);
  _cnfgrtr->enableExternalTrigger(true);
  flushInputQueue(fd());
  _firstFetch = true;
  _ignoreFetch = false;
  if (_debug & 0x20) printf("EpixServer::enable\n");
}

void Pds::EpixServer::disable() {
  _ignoreFetch = true;
  _cnfgrtr->enableExternalTrigger(false);
  flushInputQueue(fd());
  if (usleep(10000)<0) perror("EpixServer::disable ulseep 1 failed\n");
  if (_debug & 0x20) printf("EpixServer::disable\n");
}

unsigned Pds::EpixServer::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("EpixServer::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int Pds::EpixServer::fetch( char* payload, int flags ) {
   int ret = 0;
   PgpCardRx       pgpCardRx;
   unsigned        offset = 0;
   enum {Ignore=-1};

   if (_configured == false)  {
     if (++_unconfiguredErrors<20) printf("EpixServer::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
     unsigned c = this->flushInputQueue(fd());
     if (_unconfiguredErrors<20 && c) printf("\tWe flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_debug & 1) printf("EpixServer::fetch called ");

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
       unsigned peak = 0;
       unsigned max = 0;
       unsigned count = 0;
       diff += 500000;
       diff /= 1000000;
       if (diff > sizeOfHisto-1) diff = sizeOfHisto-1;
       _histo[diff] += 1;
       for (unsigned i=0; i<sizeOfHisto; i++) {
         if (_histo[i]) {
           if (_histo[i] > max) {
             max = _histo[i];
             peak = i;
           }
           count = 0;
         }
         if (i > count && count > 200) break;
         count += 1;
       }
       if (max > 100) {
         if ((diff >= (peak<<1)) || (diff <= (peak>>1))) {
           printf("EpixServer::fetch exceptional period %llu, not %u\n", diff, peak);
         }
       }
       memcpy(&_lastTime, &_thisTime, sizeof(timespec));
     }
   }

   pgpCardRx.model   = sizeof(&pgpCardRx);
   pgpCardRx.maxSize = _payloadSize / sizeof(__u32);
//   pgpCardRx.data    = (__u32*)(payload + offset);
   pgpCardRx.data   = (__u32*)_processorBuffer;

   if ((ret = read(fd(), &pgpCardRx, sizeof(PgpCardRx))) < 0) {
     if (errno == ERESTART) {
       disable();
       char message[400];
       sprintf(message, "Pgpcard problem! Restart the DAQ system\nIf this does not work, Power cycle %s\n",
           getenv("HOSTNAME"));
       UserMessage* umsg = new (_occPool) UserMessage;
       umsg->append(message);
       umsg->append(DetInfo::name(static_cast<const DetInfo&>(_xtc.src)));
       _mgr->appliance().post(umsg);
       _ignoreFetch = true;
       printf("EpixServer::fetch exiting because of ERESTART\n");
       exit(-ERESTART);
     }
     perror ("EpixServer::fetch pgpCard read error");
     ret =  Ignore;
   } else ret *= sizeof(__u32);

   if (_ignoreFetch) return Ignore;

   Pds::Pgp::DataImportFrame* data = (Pds::Pgp::DataImportFrame*)(_processorBuffer);

   if ((ret > 0) && (ret < (int)_payloadSize)) {
     printf("EpixServer::fetch() returning Ignore, ret was %d, looking for %u\n", ret, _payloadSize);
     if ((_debug & 4) || ret < 0) printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) raw_count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
         data->elementId(), data->_frameType, data->acqCount(), data->frameNumber(), _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
     uint32_t* u = (uint32_t*)data;
     printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
     ret = Ignore;
   }

   if (ret > (int) _payloadSize) printf("EpixServer::fetch pgp read returned too much _payloadSize(%u) ret(%d)\n", _payloadSize, ret);

   unsigned damageMask = 0;
   if (pgpCardRx.eofe)      damageMask |= 1;
   if (pgpCardRx.fifoErr)   damageMask |= 2;
   if (pgpCardRx.lengthErr) damageMask |= 4;
   if (damageMask) {
     damageMask |= 0xe0;
     _xtc.damage.increase(Pds::Damage::UserDefined);
     _xtc.damage.userBits(damageMask);
     printf("EpixServer::fetch setting user damage 0x%x", damageMask);
     if (pgpCardRx.lengthErr) printf(", rxSize(%zu), maxSize(%u) ret(%d) offset(%u) (bytes)",
         (unsigned)pgpCardRx.rxSize*sizeof(uint32_t), _payloadSize, ret, offset);
     printf("\n");
   } else {
     unsigned oldCount = _count;
     _count = data->frameNumber() - 1;  // epix starts counting at 1, not zero
     if ((_debug & 4) || ret < 0) {
       printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) _oldCount(%u) _count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
           data->elementId(), data->_frameType, data->acqCount(),  oldCount, _count, _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
       uint32_t* u = (uint32_t*)data;
       printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
     }
   }
   process(payload + offset);
   if (ret > 0) {
     _elementsThisCount += 1;
     ret += offset;
   }
   if (_debug & 5) printf(" returned %d\n", ret);
   return ret;
}

bool EpixServer::more() const {
  bool ret = _elements > 1;
  if (_debug & 2) printf("EpixServer::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned EpixServer::offset() const {
  unsigned ret = _elementsThisCount == 1 ? 0 : sizeof(Xtc) + _payloadSize * (_elementsThisCount-1);
  if (_debug & 2) printf("EpixServer::offset(%u)\n", ret);
  return (ret);
}

unsigned EpixServer::count() const {
  if (_debug & 2) printf( "EpixServer::count(%u)\n", _count + _offset);
  return _count + _offset;
}

unsigned EpixServer::flushInputQueue(int f) {
  fd_set          fds;
  struct timeval  timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = 2500;
  int ret;
  unsigned dummy[2048];
  unsigned count = 0;
  PgpCardRx       pgpCardRx;
  pgpCardRx.model   = sizeof(&pgpCardRx);
  pgpCardRx.maxSize = 2048;
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

void EpixServer::setEpix( int f ) {
  fd( f );
  Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
  if (unsigned c = this->flushInputQueue(f)) {
    printf("EpixServer::setEpix read %u time%s after opening pgpcard driver\n", c, c==1 ? "" : "s");
  }
//  if (_cnfgrtr == 0) {
//    _cnfgrtr = new Pds::Epix::EpixConfigurator::EpixConfigurator(fd(), _debug);
//  }
}

void EpixServer::clearHisto() {
  for (unsigned i=0; i<sizeOfHisto; i++) {
    _histo[i] = 0;
  }
}

void EpixServer::printHisto(bool c) {
  unsigned histoSum = 0;
  printf("EpixServer event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      histoSum += _histo[i];
      if (c) _histo[i] = 0;
    }
  }
  printf("\tHisto Sum was %u\n", histoSum);
  histoSum = 0;
  printf("EpixServer unshuffle event periods\n");
  for (unsigned i=0; i<1000; i++) {
    if (procHisto[i]) {
      printf("\t%3u 0.1ms   %8u\n", i, procHisto[i]);
      histoSum += procHisto[i];
      if (c) procHisto[i] = 0;
    }
  }
  printf("\tProchisto (events) Sum was %u\n", histoSum);
}
