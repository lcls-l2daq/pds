/*
 * ImpServer.cc
 *
 *  Created on: April 12, 2013
 *      Author: jackp
 */

#include "pds/imp/ImpServer.hh"
#include "pds/imp/ImpConfigurator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/config/ImpConfigType.hh"
#include "pds/config/ImpDataType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/imp/ImpDestination.hh"
#include "pds/imp/Processor.hh"
#include "pds/imp/ImpStatusRegisters.hh"
#include "pds/pgp/Pgp.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pgpcard/PgpCardMod.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

using namespace Pds;
//using namespace Pds::Imp;

ImpServer* ImpServer::_instance = 0;

class Task;
class TaskObject;

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec);
  if (diff) diff *= 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

ImpServer::ImpServer( const Pds::Src& client, unsigned configMask )
   : _xtc( _ImpDataType, client ),
     _cnfgrtr(0),
     _payloadSize(0),
     _configMask(configMask),
     _configureResult(0),
     _debug(0),
     _offset(0),
     _unconfiguredErrors(0),
     _compensateNoCountReset(1),
     _ignoreCount(0),
     _occPool(new GenericPool(sizeof(UserMessage),4)),
     _configured(false),
     _firstFetch(true),
     _getNewComp(false),
     _ignoreFetch(true) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  _task = new Pds::Task(Pds::TaskObject("IMPprocessor"));
  _dummy = (unsigned*)malloc(DummySize);
  strcpy(_runTimeConfigName, "");
  instance(this);
  printf("ImpServer::ImpServer() payload(%u)\n", _payloadSize);
}

unsigned ImpServer::configure(ImpConfigType* config) {
  if (_cnfgrtr == 0) {
    _cnfgrtr = new Imp::ImpConfigurator::ImpConfigurator(fd(), _debug);
  }
  _pgp = _cnfgrtr->pgp();
  // First calculate payload size in uint32s, 8 for the header 2 for each sample and 1 at the end
  _payloadSize = Pds::ImpData::Uint32sPerHeader + Pds::ImpConfig::get(*config,Imp::ConfigV1::NumberOfSamples) * Pds::ImpData::Uint32sPerSample + 1;
  // then convert to bytes
  _payloadSize *= sizeof(uint32_t);
  printf("ImpServer::configure, payload size %u\n", _payloadSize);
  unsigned c = flushInputQueue(fd());
  if (c) printf("ImpServer::configure flushed %u event%s before configuration\n", c, c>1 ? "s" : "");

  if ((_configureResult = _cnfgrtr->configure(config, _configMask))) {
    printf("ImpServer::configure failed 0x%x\n", _configureResult);
  } else {
    _cnfgrtr->resetSequenceCount();
    _xtc.extent = _payloadSize + sizeof(Xtc);
    printf("CspadServer::configure _payloadSize(%u) _xtc.extent(%u)\n",
        _payloadSize, _xtc.extent);
  }
  _firstFetch = true;
  _count = 0;
  _configured = _configureResult == 0;
  c = this->flushInputQueue(fd());
  if (c) printf("ImpServer::configure flushed %u event%s after confguration\n", c, c>1 ? "s" : "");

  return _configureResult;
}

void Pds::ImpServer::die() {
  printf("ImpServer::die has been called !!!!!!!\n");
}

void Pds::ImpServer::dumpFrontEnd() {
  disable();
  if (_cnfgrtr) _cnfgrtr->dumpFrontEnd();
  else printf("ImpServer::dumpFrontEnd found nil configurator\n");
}

void ImpServer::process() {
//  _ioIndex = ++_ioIndex % Pds::Imp::numberOfFrames;
//  Pds::Routine* r = new Pds::Imp::Processor(4, _ioIndex);
//  _task->call(r);
}

void ImpServer::allocated() {
//  _cnfgrtr->resetFrontEnd(Imp::MasterReset);
}

void Pds::ImpServer::enable() {
  Pds::Imp::ImpDestination d;
  d.dest(Pds::Imp::ImpDestination::CommandVC);
  if (usleep(10000)<0) perror("ImpServer::enable ulseep failed\n");
  _ignoreFetch = false;
  _firstFetch = true;
  printf("ImpServer::enable() found _pgp %p\n", _pgp);
  _pgp->writeRegister(&d, Pds::Imp::enableTriggersAddr, Pds::Imp::enable);
  _pgp->writeRegister(&d, Pds::Imp::unGateTriggersAddr, Pds::Imp::enable);
  flushInputQueue(fd());
  if (_debug & 0x20) printf("ImpServer::enable\n");
  _ignoreFetch = false;
}

void Pds::ImpServer::disable(bool flush) {
  _ignoreFetch = true;
  Pds::Imp::ImpDestination d;
  d.dest(Pds::Imp::ImpDestination::CommandVC);
  if (_pgp) {
    _pgp->writeRegister(&d, Pds::Imp::enableTriggersAddr, Pds::Imp::disable);
    _pgp->writeRegister(&d, Pds::Imp::unGateTriggersAddr, Pds::Imp::disable);
  }
  else printf("ImpServer::disable(%s) found nil _pgp %p\n", flush ? "true" : "false", _pgp);
  if (flush) flushInputQueue(fd());
  if (usleep(10000)<0) perror("ImpServer::disable ulseep 1 failed\n");
  if (_debug & 0x20) printf("ImpServer::disable\n");
}

unsigned Pds::ImpServer::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("ImpServer::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int Pds::ImpServer::fetch( char* payload, int flags ) {
   int ret = 0;
   PgpCardRx       pgpCardRx;
   unsigned        offset = 0;
   enum {Ignore=-1};

   if (_ignoreFetch) {
     unsigned c = this->flushInputQueue(fd(), false);
     printf("ImpServer::fetch() ignored and flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_configured == false)  {
     if (++_unconfiguredErrors<20) printf("ImpServer::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
     unsigned c = this->flushInputQueue(fd());
     if (_unconfiguredErrors<20 && c) printf("\tWe flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_debug & 1) printf("ImpServer::fetch called ");

   _xtc.damage = 0;

   memcpy( payload, &_xtc, sizeof(Xtc) );
   offset = sizeof(Xtc);
   if (_firstFetch) {
     _firstFetch = false;
//     _getNewComp = true;
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

   pgpCardRx.model   = sizeof(&pgpCardRx);
   pgpCardRx.maxSize = _payloadSize / sizeof(__u32);
   pgpCardRx.data    = (__u32*)(payload + offset);

   if ((ret = read(fd(), &pgpCardRx, sizeof(PgpCardRx))) < 0) {
     if (errno == ERESTART) {
       disable(false);
       char message[400];
       sprintf(message, "Pgpcard problem! Restart the DAQ system\nIf this does not work, Power cycle %s\n",getenv("HOSTNAME"));
       UserMessage* umsg = new (_occPool) UserMessage;
       umsg->append(message);
       umsg->append(DetInfo::name(static_cast<const DetInfo&>(_xtc.src)));
       _mgr->appliance().post(umsg);
       _ignoreFetch = true;
       printf("ImpServer::fetch exiting because of ERESTART\n");
       exit(-ERESTART);
     }
     perror ("ImpServer::fetch pgpCard read error");
     ret =  Ignore;
   } else ret *= sizeof(__u32);
   ImpDataType* data = (ImpDataType*)(payload + offset);

   if ((ret > 0) && (ret < (int)_payloadSize)) {
     printf("ImpServer::fetch() returning Ignore, ret was %d, looking for %u\n", ret, _payloadSize);
//     if (_debug & 4 || ret < 0) printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) raw_count(%u) lane(%u) vc(%u)\n",
//         data->elementId(), data->_frameType, data->acqCount(), data->frameNumber(), pgpCardRx.pgpLane, pgpCardRx.pgpVc);
     uint32_t* u = (uint32_t*)data;
     printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
     ret = Ignore;
   }

   if (ret > (int) _payloadSize) printf("ImpServer::fetch pgp read returned too much _payloadSize(%u) ret(%d)\n", _payloadSize, ret);

   unsigned damageMask = 0;
   if (pgpCardRx.eofe)      damageMask |= 1;
   if (pgpCardRx.fifoErr)   damageMask |= 2;
   if (pgpCardRx.lengthErr) damageMask |= 4;
   if (damageMask) {
     damageMask |= 0xe0;
     _xtc.damage.increase(Pds::Damage::UserDefined);
     _xtc.damage.userBits(damageMask);
     printf("ImpServer::fetch setting user damage 0x%x", damageMask);
     if (pgpCardRx.lengthErr) printf(", rxSize(%u), maxSize(%u) ret(%d) offset(%u) (bytes)",
         (unsigned int)(long unsigned int)(pgpCardRx.rxSize*sizeof(uint32_t)), (unsigned)_payloadSize, ret, offset);
     printf("\n");
   } else {
     unsigned oldCount = _count;
//     if (_getNewComp) {
//       _compensateNoCountReset = data->frameNumber();
//       _getNewComp = false;
//     }
     _count = data->frameNumber() - _compensateNoCountReset;  // compensate for no trigger reset !!!!!!!!!!!!!!!!
     if (_debug & 4 || ret < 0) {
//       printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) _oldCount(%u) _count(%u) lane(%u) vc(%u)\n",
//           data->elementId(), data->_frameType, data->acqCount(),  oldCount, _count, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
       uint16_t* u = (uint16_t*)data;
       printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("%x ", u[i]); printf("\n");
     }
     if (_count == oldCount) {
       if (_count) printf("ImpServer::fetch found duplicate frame count 0x%x, comp 0x%x\n", _count, _compensateNoCountReset);
       memcpy( payload, &_xtc, sizeof(Xtc) );
       uint16_t* u = (uint16_t*)data;
       printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("%x ", u[i]); printf("\n");
     }
   }
   if (ret > 0) {
     ret += offset;
   }
   if (_debug & 5) printf(" returned %d\n", ret);
   return ret;
}

bool ImpServer::more() const {
  bool ret = 0;
  if (_debug & 2) printf("ImpServer::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned ImpServer::offset() const {
  unsigned ret =  0;
  if (_debug & 2) printf("ImpServer::offset(%u)\n", ret);
  return (ret);
}

unsigned ImpServer::count() const {
  if (_debug & 2) printf( "ImpServer::count(%u)\n", _count + _offset);
  return _count + _offset;
}

unsigned ImpServer::flushInputQueue(int f, bool printFlag) {
  fd_set          fds;
  struct timeval  timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = 2500;
  int ret;
  unsigned count = 0;
  PgpCardRx       pgpCardRx;
  pgpCardRx.model   = sizeof(&pgpCardRx);
  pgpCardRx.maxSize = DummySize;
  pgpCardRx.data    = _dummy;
  do {
    FD_ZERO(&fds);
    FD_SET(f,&fds);
    ret = select( f+1, &fds, NULL, NULL, &timeout);
    if (ret>0) {
      count += 1;
      ::read(f, &pgpCardRx, sizeof(PgpCardRx));
    }
  } while ((ret > 0) && (count < 100));
  if (count) {
    if (count == 100 && (count < 100)) {
      printf("\tImpServer::flushInputQueue: pgpCardRx lane(%u) vc(%u) rxSize(%u) eofe(%s) lengthErr(%s)\n",
          pgpCardRx.pgpLane, pgpCardRx.pgpVc, pgpCardRx.rxSize, pgpCardRx.eofe ? "true" : "false",
                                                                pgpCardRx.lengthErr ? "true" : "false");
      printf("\t\t"); for (ret=0; ret<8; ret++) printf("%u ", _dummy[ret]);  printf("/n");
    }
  }
  return count;
}


void ImpServer::setImp( int f ) {
  fd( f );
  if (unsigned c = this->flushInputQueue(f)) {
    printf("ImpServer::setImp read %u time%s after opening pgpcard driver\n", c, c==1 ? "" : "s");
  }
//  _pgp = new Pds::Pgp::Pgp::Pgp(f);
  Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
  if (_cnfgrtr == 0) {
    _cnfgrtr = new Pds::Imp::ImpConfigurator::ImpConfigurator(fd(), _debug);
  }
}

void ImpServer::printHisto(bool c) {
  printf("ImpServer ignored count %u times\n", _ignoreCount);
  printf("ImpServer event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      if (c) _histo[i] = 0;
    }
  }
}
