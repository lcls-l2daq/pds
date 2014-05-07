/*
 * Epix10kServer.cc
 *
 *  Created on: 2014.4.15
 *      Author: jackp
 */

#include "pds/epix10k/Epix10kServer.hh"
#include "pds/epix10k/Epix10kConfigurator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/config/EpixSamplerDataType.hh"
#include "pds/config/EpixDataType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/epix10k/Epix10kDestination.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pgpcard/PgpCardMod.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <omp.h>

using namespace Pds;
//using namespace Pds::Epix10k;

Epix10kServer* Epix10kServer::_instance = 0;

class Task;
class TaskObject;
class Pds::Epix10k::Epix10kDestination;
class DetInfo;

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec);
  if (diff) diff *= 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}



Epix10kServer::Epix10kServer( const Pds::Src& client, unsigned configMask )
   : _xtcTop(TypeId(TypeId::Id_Xtc,1), client),
     _xtcEpix( _epixDataType, client ),
     _xtcSamplr(_epixSamplerDataType, client),
     _xtcConfig(TypeId(TypeId::Id_EpixSamplerConfig,1), client),
     _samplerConfig(0),
     _cnfgrtr(0),
     _elements(ElementsPerSegmentLevel),
     _payloadSize(0),
     _configureResult(0),
     _debug(0),
     _offset(0),
     _unconfiguredErrors(0),
     _processorBuffer(0),
     _scopeBuffer(0),
     _configured(false),
     _firstFetch(true),
     _ignoreFetch(true),
     _resetOnEveryConfig(false),
     _scopeEnabled(false),
     _scopeHasArrived(false),
     _maintainLostRunTrigger(false) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  _task = new Pds::Task(Pds::TaskObject("EPIX10Kprocessor"));
  strcpy(_runTimeConfigName, "");
  instance(this);
  printf("Epix10kServer::Epix10kServer() payload(%u)\n", _payloadSize);
}

unsigned Epix10kServer::configure(Epix10kConfigType* config, bool forceConfig) {
  unsigned firstConfig = _resetOnEveryConfig || forceConfig;
  if (_cnfgrtr == 0) {
    firstConfig = 1;
    _cnfgrtr = new Pds::Epix10k::Epix10kConfigurator::Epix10kConfigurator(fd(), _debug);
    _cnfgrtr->runTimeConfigName(_runTimeConfigName);
    _cnfgrtr->maintainLostRunTrigger(_maintainLostRunTrigger);
    printf("Epix10kServer::configure making new configurator %p, firstConfig %u\n", _cnfgrtr, firstConfig);
  }
  _xtcTop.extent = sizeof(Xtc);
  _xtcSamplr.extent = 0;
  unsigned c = flushInputQueue(fd());
  if (c) printf("Epix10kServer::configure flushed %u event%s before configuration\n", c, c>1 ? "s" : "");

  if ((_configureResult = _cnfgrtr->configure(config, firstConfig))) {
    printf("Epix10kServer::configure failed 0x%x, first %u\n", _configureResult, firstConfig);
  } else {
    _payloadSize = EpixDataType::_sizeof(*config);
    if (_processorBuffer) free(_processorBuffer);
    _processorBuffer = (char*) calloc(1, _payloadSize);
    printf("Epix10kServer::configure allocated processorBuffer size %u, firstConfig %u\n", _payloadSize, firstConfig);
    if (!_processorBuffer) {
      printf("Epix10kServer::configure FAILED to allocated processor buffer!!!\n");
      return 0xdeadbeef;
    }
    _xtcEpix.extent = (_payloadSize * _elements) + sizeof(Xtc);
    _xtcTop.extent += _xtcEpix.extent;
    if ((_scopeEnabled = config->scopeEnable())) {
      _samplerConfig = new EpixSamplerConfigType(
          1,  // version
          config->runTrigDelay(),
          config->daqTrigDelay(),
          config->dacSetting(),
          config->adcClkHalfT(),
          config->adcPipelineDelay(),
          config->digitalCardId0(),
          config->digitalCardId1(),
          config->analogCardId0(),
          config->analogCardId1(),
          2,  // number of Channels
          config->scopeTraceLength(),
          config->baseClockFrequency(),
          0  // testPatterEnable
      );
      _xtcSamplr.extent = EpixSamplerDataType::_sizeof(*_samplerConfig) + sizeof(Xtc);
      _xtcTop.extent += _xtcSamplr.extent;
      _xtcConfig.extent = sizeof( EpixSamplerConfigType) + sizeof(Xtc);
      if (!_scopeBuffer) _scopeBuffer = (unsigned*)calloc(_xtcSamplr.sizeofPayload(), 1);
    } else {
      if (_samplerConfig) {
        delete(_samplerConfig);
        _samplerConfig = 0;
      }
      _xtcSamplr.extent = 0;
      if (_scopeBuffer) {
        free(_scopeBuffer);
        _scopeBuffer = 0;
      }
    }
    printf("Epix10kServer::configure _elements(%u) _payloadSize(%u) _xtcTop.extent(%u) _xtcEpix.extent(%u) firstConfig(%u)\n",
        _elements, _payloadSize, _xtcTop.extent, _xtcEpix.extent, firstConfig);

    _firstFetch = true;
    _count = _elementsThisCount = 0;
  }
  _configured = _configureResult == 0;
  c = this->flushInputQueue(fd());
  clearHisto();
  if (c) printf("Epix10kServer::configure flushed %u event%s after confguration\n", c, c>1 ? "s" : "");

  return _configureResult;
}

void Pds::Epix10kServer::die() {
    printf("Epix10kServer::die has been called !!!!!!!\n");
}

void Pds::Epix10kServer::dumpFrontEnd() {
  disable();
  _cnfgrtr->dumpFrontEnd();
}

static unsigned* procHisto = (unsigned*) calloc(1000, sizeof(unsigned));

void Epix10kServer::process(char* d) {
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

void Epix10kServer::allocated() {
//  _cnfgrtr->resetFrontEnd();
}

void Pds::Epix10kServer::enable() {
  if (usleep(10000)<0) perror("Epix10kServer::enable ulseep failed\n");
  usleep(10000);
  _cnfgrtr->enableExternalTrigger(true);
  flushInputQueue(fd());
  _firstFetch = true;
  _ignoreFetch = false;
  if (_debug & 0x20) printf("Epix10kServer::enable\n");
}

void Pds::Epix10kServer::disable() {
  _ignoreFetch = true;
  _cnfgrtr->enableExternalTrigger(false);
  flushInputQueue(fd());
  if (usleep(10000)<0) perror("Epix10kServer::disable ulseep 1 failed\n");
  if (_debug & 0x20) printf("Epix10kServer::disable\n");
}

void Pds::Epix10kServer::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigName, name);
  printf("Pds::Epix10kServer::runTimeConfigName(%s)\n", name);
}

unsigned Pds::Epix10kServer::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("Epix10kServer::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int Pds::Epix10kServer::fetch( char* payload, int flags ) {
   int ret = 0;
   PgpCardRx       pgpCardRx;
   unsigned        offset = 0;
   enum {Ignore=-1};

   if (_configured == false)  {
     if (++_unconfiguredErrors<20) printf("Epix10kServer::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
     unsigned c = this->flushInputQueue(fd());
     if (_unconfiguredErrors<20 && c) printf("\tWe flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_debug & 1) printf("Epix10kServer::fetch called ");

   _xtcTop.damage = 0;
   _xtcEpix.damage = 0;

   _elementsThisCount = 0;

   if (!_elementsThisCount) {
   }

   pgpCardRx.model   = sizeof(&pgpCardRx);
   pgpCardRx.maxSize = _payloadSize / sizeof(__u32);
   pgpCardRx.data   = (__u32*)_processorBuffer;

   if ((ret = read(fd(), &pgpCardRx, sizeof(PgpCardRx))) < 0) {
     if (errno == ERESTART) {
       disable();
       _ignoreFetch = true;
       printf("Epix10kServer::fetch exiting because of ERESTART\n");
       exit(-ERESTART);
     }
     perror ("Epix10kServer::fetch pgpCard read error");
     ret =  Ignore;
   } else ret *= sizeof(__u32);

   if (_ignoreFetch) return Ignore;

   unsigned damageMask = 0;
   if (pgpCardRx.eofe)      damageMask |= 1;
   if (pgpCardRx.fifoErr)   damageMask |= 2;
   if (pgpCardRx.lengthErr) damageMask |= 4;

   if (pgpCardRx.pgpVc == Epix10k::Epix10kDestination::Oscilloscope) {
     if (damageMask) {
       _xtcSamplr.damage.increase(Pds::Damage::UserDefined);
       _xtcSamplr.damage.userBits(damageMask | 0xe0);
     } else {
       _xtcSamplr.damage = 0;
     }
     if (_scopeBuffer)  {
       memcpy(_scopeBuffer, _processorBuffer, _xtcSamplr.sizeofPayload());
       _scopeHasArrived = true;
     }
     return Ignore;
   }

   if (pgpCardRx.pgpVc == Epix10k::Epix10kDestination::Data) {

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
           printf("Epix10kServer::fetch exceptional period %llu, not %u\n", diff, peak);
         }
       }
       memcpy(&_lastTime, &_thisTime, sizeof(timespec));
     }

     Pds::Pgp::DataImportFrame* data = (Pds::Pgp::DataImportFrame*)(_processorBuffer);

     if ((ret > 0) && (ret < (int)_payloadSize)) {
       printf("Epix10kServer::fetch() returning Ignore, ret was %d, looking for %u\n", ret, _payloadSize);
       if ((_debug & 4) || ret < 0) printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) raw_count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
           data->elementId(), data->_frameType, data->acqCount(), data->frameNumber(), _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
       uint32_t* u = (uint32_t*)data;
       printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
       ret = Ignore;
     }

     if (ret > (int) _payloadSize) printf("Epix10kServer::fetch pgp read returned too much _payloadSize(%u) ret(%d)\n", _payloadSize, ret);

     if (damageMask) {
       damageMask |= 0xe0;
       _xtcEpix.damage.increase(Pds::Damage::UserDefined);
       _xtcEpix.damage.userBits(damageMask);
       printf("Epix10kServer::fetch setting user damage 0x%x", damageMask);
       if (pgpCardRx.lengthErr) printf(", rxSize(%zu), payloadSize(%u) ret(%d) offset(%u) (bytes)",
           (unsigned)pgpCardRx.rxSize*sizeof(uint32_t), _payloadSize, ret, offset);
       printf("\n");
     } else {
       unsigned oldCount = _count;
       _count = data->frameNumber() - 1;  // epix10k starts counting at 1, not zero
       if ((_debug & 4) || ret < 0) {
         printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) _oldCount(%u) _count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
             data->elementId(), data->_frameType, data->acqCount(),  oldCount, _count, _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
         uint32_t* u = (uint32_t*)data;
         printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
       }
     }
     offset = sizeof(Xtc);
     if (_scopeHasArrived) {
       memcpy(payload + offset, &_xtcSamplr, sizeof(Xtc));
       offset += sizeof(Xtc);
       memcpy(payload + offset, _scopeBuffer, _xtcSamplr.sizeofPayload());
       offset += _xtcSamplr.sizeofPayload();
       _scopeHasArrived = false;
     }
     memcpy(payload + offset, &_xtcEpix, sizeof(Xtc));
     offset += sizeof(Xtc);
     process(payload + offset);
     _xtcTop.extent = offset + _payloadSize;
     memcpy(payload, &_xtcTop, sizeof(Xtc));
   } else {  // wrong vc for us
     return Ignore;
   }
   if (ret > 0) {
     ret += offset;
   }
   if (_debug & 5) printf(" returned %d\n", ret);
   return ret;
}

bool Epix10kServer::more() const {
  bool ret = false;
  if (_debug & 2) printf("Epix10kServer::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned Epix10kServer::offset() const {
  unsigned ret = _elementsThisCount == 1 ? 0 : sizeof(Xtc) + _payloadSize * (_elementsThisCount-1);
  if (_debug & 2) printf("Epix10kServer::offset(%u)\n", ret);
  return (ret);
}

unsigned Epix10kServer::count() const {
  if (_debug & 2) printf( "Epix10kServer::count(%u)\n", _count + _offset);
  return _count + _offset;
}

unsigned Epix10kServer::flushInputQueue(int f) {
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

void Epix10kServer::setEpix10k( int f ) {
  fd( f );
  Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
  if (unsigned c = this->flushInputQueue(f)) {
    printf("Epix10kServer::setEpix10k read %u time%s after opening pgpcard driver\n", c, c==1 ? "" : "s");
  }
//  if (_cnfgrtr == 0) {
//    _cnfgrtr = new Pds::Epix10k::Epix10kConfigurator::Epix10kConfigurator(fd(), _debug);
//  }
}

void Epix10kServer::clearHisto() {
  for (unsigned i=0; i<sizeOfHisto; i++) {
    _histo[i] = 0;
  }
}

void Epix10kServer::printHisto(bool c) {
  unsigned histoSum = 0;
  printf("Epix10kServer event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      histoSum += _histo[i];
      if (c) _histo[i] = 0;
    }
  }
  printf("\tHisto Sum was %u\n", histoSum);
  histoSum = 0;
  printf("Epix10kServer unshuffle event periods\n");
  for (unsigned i=0; i<1000; i++) {
    if (procHisto[i]) {
      printf("\t%3u 0.1ms   %8u\n", i, procHisto[i]);
      histoSum += procHisto[i];
      if (c) procHisto[i] = 0;
    }
  }
  printf("\tProchisto (events) Sum was %u\n", histoSum);
}
