/*
 * Cspad2x2Server.cc
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#include "pds/cspad2x2/Cspad2x2Server.hh"
#include "pds/cspad2x2/Cspad2x2Configurator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/config/CsPad2x2ConfigType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/cspad2x2/Cspad2x2Destination.hh"
#include "pds/cspad2x2/Processor.hh"
#include "pgpcard/PgpCardMod.h"
#include "pdsdata/xtc/DetInfo.hh"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <new>

using namespace Pds;

Cspad2x2Server* Cspad2x2Server::_instance = 0;

class Task;
class TaskObject;

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

Cspad2x2Server::Cspad2x2Server( const Pds::Src& client, Pds::TypeId& myDataType, unsigned configMask )
   : _xtc( myDataType, client ),
     _cnfgrtr(0),
     _configMask(configMask),
     _configureResult(0),
     _debug(0),
     _offset(0),
     _occPool(new GenericPool(sizeof(UserMessage),4)),
     _configured(false),
     _firstFetch(true),
     _ignoreFetch(true) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  _task = new Pds::Task(Pds::TaskObject("CSPADprocessor"));
  _dummy = (unsigned*)malloc(DummySize);
  strcpy(_runTimeConfigName, "");
  instance(this);
}

unsigned Cspad2x2Server::configure(CsPad2x2ConfigType* config) {
  if (config == 0) {
    printf("Cspad2x2Server::configure was passed a nil config!\n");
    _configureResult = 0xdead;
  } else {
    if (_cnfgrtr == 0) {
      _cnfgrtr = new Pds::CsPad2x2::Cspad2x2Configurator::Cspad2x2Configurator(config, fd(), _debug);
      _cnfgrtr->runTimeConfigName(_runTimeConfigName);
    } else {
      printf("Cspad2x2Configurator already instantiated\n");
    }
    unsigned c = flushInputQueue(fd());
    if (c) printf("Cspad2x2Server::configure flushed %u event%s before configuration\n", c, c>1 ? "s" : "");
    if ((_configureResult = _cnfgrtr->configure(config, _configMask))) {
      printf("Cspad2x2Server::configure failed 0x%x\n", _configureResult);
      _cnfgrtr->dumpPgpCard();
    } else {
      _payloadSize = config->payloadSize();
      _xtc.extent = _payloadSize + sizeof(Xtc);
      printf("Cspad2x2Server::configure _payloadSize(%u) _xtc.extent(%u)\n", _payloadSize, _xtc.extent);
    }
    _firstFetch = true;
    _count = 0;
    _configured = _configureResult == 0;
    c = this->flushInputQueue(fd());
    if (c) printf("Cspad2x2Server::configure flushed %u event%s after configuration\n", c, c>1 ? "s" : "");
  }
  return _configureResult;
}

void Pds::Cspad2x2Server::die() {
  printf("Cspad2x2Server::die has been called !!!!!!!\n");
}

void Pds::Cspad2x2Server::dumpFrontEnd() {
  if (_cnfgrtr != 0) _cnfgrtr->dumpFrontEnd();
  else printf("Cspad2x2Server::dumpFrontEnd() found nil configurator\n");
}

void Cspad2x2Server::process() {
//  _ioIndex = ++_ioIndex % Pds::Xamps::numberOfFrames;
//  Pds::Routine* r = new Pds::Xamps::Processor(4, _ioIndex);
//  _task->call(r);
}

unsigned Pds::Cspad2x2Server::enable() {
  _d.dest(Pds::CsPad2x2::Cspad2x2Destination::CR);
  unsigned ret = _configureResult;
  if (_configureResult != 0xdead) {
    ret = _pgp->writeRegister(
        &_d,
        CsPad2x2::Cspad2x2Configurator::RunModeAddr,
        _cnfgrtr->configuration().activeRunMode());
    ::usleep(10000);
    _firstFetch = true;
    flushInputQueue(fd());
    if (_debug & 0x20) printf("Cspad2x2Server::enable %s\n", ret ? "FAILED!" : "SUCCEEDED");
  } else {
    printf("Cspad2x2Server::enable found nil config!\n");
  }
  _ignoreFetch = ret ? true : false;
  return ret;
}

void Pds::Cspad2x2Server::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigName, name);
  printf("Pds::Cspad2x2Server::runTimeConfigName(%s)\n", name);
}

unsigned Pds::Cspad2x2Server::disable(bool flush) {
  _ignoreFetch = true;
  unsigned ret = _configureResult;
  _d.dest(Pds::CsPad2x2::Cspad2x2Destination::CR);
  if (_configureResult != 0xdead) {
    if (_pgp && _cnfgrtr) {
      ret = _pgp->writeRegister(
          &_d,
          CsPad2x2::Cspad2x2Configurator::RunModeAddr,
          _cnfgrtr->configuration().inactiveRunMode());
      ::usleep(10000);
    } else {
      printf("Cspad2x2Server::disable found nil _pgp %p pointer or _cnfgrtr %p\n", _pgp, _cnfgrtr);
      ret = 0xffffffff;
    }
    if (flush && _pgp) flushInputQueue(fd());
    if (_debug & 0x20) printf("Cspad2x2Server::disable %s\n", ret ? "FAILED!" : "SUCCEEDED");
  } else {
    printf("Cspad2x2Server::disable found nil config!\n");
  }
  return ret;
}

unsigned Pds::Cspad2x2Server::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("Cspad2x2Server::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int Pds::Cspad2x2Server::fetch( char* payload, int flags ) {
   int ret = 0;
   PgpCardRx       pgpCardRx;
   unsigned        xtcSize = 0;
   enum {Ignore=-1};

   if (_ignoreFetch) {
     unsigned c = this->flushInputQueue(fd(), false);
     if (_debug & 1) printf("Cspad2x2Server::fetch() ignored and flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_configured == false)  {
     printf("Cspad2x2Server::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
     unsigned c = this->flushInputQueue(fd());
     printf("\tWe flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_debug & 1) printf("Cspad2x2Server::fetch called ");

   _xtc.damage = 0;

   memcpy( payload, &_xtc, sizeof(Xtc) );
   xtcSize = sizeof(Xtc);
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

   pgpCardRx.model   = sizeof(&pgpCardRx);
   pgpCardRx.maxSize = _payloadSize / sizeof(__u32);
   pgpCardRx.data    = (__u32*)(payload + xtcSize);

   if ((ret = read(fd(), &pgpCardRx, sizeof(PgpCardRx))) < 0) {
     if (errno == ERESTART) {
       disable(false);
       char message[400];
       sprintf(message, "Pgpcard problem! Restart the DAQ system\nIf this does not work, Power cycle %s\n",
           getenv("HOSTNAME"));
       UserMessage* umsg = new (_occPool) UserMessage;
       umsg->append(message);
       umsg->append(DetInfo::name(static_cast<const DetInfo&>(_xtc.src)));
       _mgr->appliance().post(umsg);
       _ignoreFetch = true;
       printf("Cspad2x2Server::fetch exiting because of ERESTART\n");
       exit(-ERESTART);
     }
     perror ("Cspad2x2Server::fetch pgpCard read error");
     ret =  Ignore;
   } else ret *= sizeof(__u32);  // correct for pgpcard read not returning # of bytes
   Pds::Pgp::DataImportFrame* data = (Pds::Pgp::DataImportFrame*)(payload + xtcSize);

   if ((ret > 0) && (ret < (int)_payloadSize)) {
     printf("Cspad2x2Server::fetch() returning Ignore, ret was %d, looking for %u, frame(%u)",
         ret, _payloadSize, data->frameNumber() - 1);
     if (_debug & 4 || ret < 0) printf("\n\topcode(0x%x) acqcount(0x%x) fiducials(0x%x) _count(%u) lane(%u) vc(%u)",
         data->second.opCode, data->acqCount(), data->fiducials(), _count, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
     ret = Ignore;
//     printf("\n\t ");
//     unsigned* u= (unsigned*)data;
//     for (unsigned j=0; j<(sizeof(Pds::Pgp::DataImportFrame)/sizeof(unsigned)); j++) {
//       printf(" 0x%x", u[j]);
//     }
     printf("\n");
   }

   unsigned damageMask = 0;
   if (pgpCardRx.eofe)      damageMask |= 1;
   if (pgpCardRx.fifoErr)   damageMask |= 2;
   if (pgpCardRx.lengthErr) damageMask |= 4;
   if (damageMask) {
     printf("CsPadServer::fetch %s damageMask 0x%x, quad(%u)", ret>0 ? "setting user damage" : "ignoring wrong length", damageMask, data->elementId());
     if (pgpCardRx.lengthErr) printf(" LENGTH_ERROR rxSize(%u)", (unsigned)pgpCardRx.rxSize);
     if (pgpCardRx.fifoErr) printf(" FIFO_ERROR");
     if (pgpCardRx.eofe) printf(" EOFE_ERROR");
     printf(" frame %u\n", data->frameNumber() - 1);
     if (ret > 0) {
       damageMask |= 0xe0;
       _xtc.damage.increase(Pds::Damage::UserDefined);
       _xtc.damage.userBits(damageMask);
     }
   } else {
     unsigned oldCount = _count;
     _count = data->frameNumber() - 1;  // cspad2x2 starts counting at 1, not zero
     if (_debug & 4 || ret < 0) printf("\n\tquad(%u) opcode(0x%x) acqcount(0x%x) fiducials(0x%x) _oldCount(%u) _count(%u)lane(%u) vc(%u)\n",
         data->elementId(), data->second.opCode, data->acqCount(), data->fiducials(), oldCount, _count, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
     if ((_count < oldCount) || (_count - oldCount > 2)) {
       printf("CsPadServer::fetch ignoring unreasonable frame number, 0x%x came after 0x%x\n", _count, oldCount);
       ret = Ignore;
     }
   }
   if (ret > 0) {
     ret += xtcSize;
   }
   if (_debug & 1) printf(" returned %d\n", ret);
   return ret;
}

bool Cspad2x2Server::more() const {
  bool ret = false;
  if (_debug & 2) printf("Cspad2x2Server::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned Cspad2x2Server::offset() const {
  unsigned ret = 0;
  if (_debug & 2) printf("Cspad2x2Server::offset(%u)\n", ret);
  return (ret);
}

unsigned Cspad2x2Server::count() const {
  if (_debug & 2) printf( "Cspad2x2Server::count(%u)\n", _count);
  return _count + _offset;
}

unsigned Cspad2x2Server::flushInputQueue(int f, bool printFlag) {
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
      if (!count) {
        if (printFlag) printf("\n\tflushed lanes ");
      }
      count += 1;
      ::read(f, &pgpCardRx, sizeof(PgpCardRx));
      if (printFlag) printf("-%u-", pgpCardRx.pgpLane);
    }
  } while ((ret > 0) && (count < 100));
  if (count) {
    printf("\n");
    if (count == 100 && printFlag) {
      printf("\tCspad2x2Server::flushInputQueue: pgpCardRx lane(%u) vc(%u) rxSize(%u) eofe(%s) lengthErr(%s)\n",
          pgpCardRx.pgpLane, pgpCardRx.pgpVc, pgpCardRx.rxSize, pgpCardRx.eofe ? "true" : "false",
          pgpCardRx.lengthErr ? "true" : "false");
      printf("\t\t"); for (ret=0; ret<8; ret++) printf("%u ", _dummy[ret]);  printf("/n");
    }
  }
  return count;
}

void Cspad2x2Server::setCspad2x2( int f ) {
  if (unsigned c = this->flushInputQueue(f)) {
    printf("Cspad2x2Server::setCspad2x2 read %u time%s after opening pgpcard driver\n", c, c==1 ? "" : "s");
  }
  fd( f );
  _pgp = new Pds::Pgp::Pgp::Pgp(f);
  Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
}

void Cspad2x2Server::printHisto(bool c) {
  printf("Cspad2x2Server event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      if (c) _histo[i] = 0;
    }
  }
}
