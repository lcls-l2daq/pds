/*
 * pnCCDServer.cc
 *
 *  Created on: May 30, 2013
 *      Author: jackp
 */

#include "pds/pnccd/pnCCDServer.hh"
#include "pds/pnccd/pnCCDTrigMonitor.hh"
#include "pds/pnccd/pnCCDConfigurator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/pnccd/Processor.hh"
#include "pds/pnccd/FrameV0.hh"
#include "pds/pgp/Pgp.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pgpcard/PgpCardMod.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

using namespace Pds;
//using namespace Pds::pnCCD;

pnCCDServer* pnCCDServer::_instance = 0;

class Task;
class TaskObject;
class DetInfo;

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec);
  if (diff) diff *= 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

pnCCDServer::pnCCDServer( const Pds::Src& client, unsigned configMask )
   : _pnCCDDataType(new Pds::TypeId(Pds::TypeId::Id_pnCCDframe, Pds::PNCCD::FrameV0::Version)),
     _xtc( *_pnCCDDataType, client ),
     _cnfgrtr(0),
     _payloadSize(0),
     _configMask(configMask),
     _configureResult(0),
     _debug(0),
     _offset(0),
     _quads(0),
     _quadsThisCount(0),
     _quadMask(0),
     _unconfiguredErrors(0),
     _compensateNoCountReset(1),
     _ignoreCount(0),
     _occPool(new GenericPool(sizeof(UserMessage),4)),
     _configured(false),
     _firstFetch(true),
     _getNewComp(false),
     _ignoreFetch(true),
     _selfTrigMonitor(0) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  _task = new Pds::Task(Pds::TaskObject("IMPprocessor"));
  _dummy = (unsigned*)malloc(DummySize);
  strcpy(_runTimeConfigName, "");
  instance(this);
  printf("pnCCDServer::pnCCDServer() payload(%u)\n", _payloadSize);
}

unsigned pnCCDServer::configure(pnCCDConfigType* config) {
  if (_cnfgrtr == 0) {
    printf("pnCCDServer::configure() making configurator, may be before fd is know!\n");
    _cnfgrtr = new pnCCD::pnCCDConfigurator(fd(), _debug);
    _cnfgrtr->fd(fd());
  }
  _payloadSize = config->payloadSizePerLink();
  _quads = config->numLinks();
  _xtc.extent = (_payloadSize * _quads) + sizeof(Xtc);
  _firstFetch = true;
  _count = 0;
  unsigned result = _cnfgrtr->configure(config);
  _configured = result == 0;
  unsigned c = this->flushInputQueue(fd());
  if (c) printf("pnCCDServer::configure flushed %u event%s after confguration\n", c, c>1 ? "s" : "");

  return 0;
}

void Pds::pnCCDServer::die() {
  printf("pnCCDServer::die has been called !!!!!!!\n");
}

void Pds::pnCCDServer::dumpFrontEnd() {
  if (_pgp) {
    disable();
    _cnfgrtr->dumpFrontEnd();
  }
}

void pnCCDServer::process() {
//  _ioIndex = ++_ioIndex % Pds::pnCCD::numberOfFrames;
//  Pds::Routine* r = new Pds::pnCCD::Processor(4, _ioIndex);
//  _task->call(r);
}

void pnCCDServer::allocated() {
//  _cnfgrtr->resetFrontEnd(pnCCD::MasterReset);
}

void Pds::pnCCDServer::enable() {
  if(_selfTrigMonitor) _selfTrigMonitor->disable(true);
  _ignoreFetch = false;
  if (_debug & 0x20) printf("pnCCDServer::enable\n");
}

void Pds::pnCCDServer::disable() {
  if(_selfTrigMonitor) _selfTrigMonitor->enable(false);
  _ignoreFetch = true;
  if (usleep(10000)<0) perror("pnCCDServer::disable ulseep 1 failed\n");
  if (_debug & 0x20) printf("pnCCDServer::disable\n");
}

unsigned Pds::pnCCDServer::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("pnCCDServer::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int Pds::pnCCDServer::fetch( char* payload, int flags ) {
  int ret = 0;
  PgpCardRx       pgpCardRx;
  unsigned        xtcOffset = 0;
  bool            grabOffset = false;
  enum {Ignore=-1};

  if (_ignoreFetch) {
    unsigned c = this->flushInputQueue(fd(), false);
    if (_debug & 1) printf("pnCCDServer::fetch() ignored and flushed %u input buffer%s\n", c, c>1 ? "s" : "");
    return Ignore;
  }

  if (_configured == false)  {
    printf("pnCCDServer::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
    unsigned c = this->flushInputQueue(fd(), false);
    printf("\tWe flushed %u input buffer%s\n", c, c>1 ? "s" : "");
    return Ignore;
  }

  if (_debug & 1) printf("pnCCDServer::fetch called ");

  _xtc.damage = 0;

  _quadsThisCount %= _quads;

  if (!_quadsThisCount) {
    _quadMask = 0;
    memcpy( payload, &_xtc, sizeof(Xtc) );
    xtcOffset = sizeof(Xtc);
    if (_firstFetch) {
      _firstFetch = false;
      grabOffset = true;
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
  pgpCardRx.maxSize = 1 + _payloadSize / sizeof(__u32);
  pgpCardRx.data    = (__u32*)(payload + xtcOffset);
  Pds::PNCCD::FrameV0* data = (Pds::PNCCD::FrameV0*)(payload + xtcOffset);

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
      printf("CspadServer::fetch exiting because of ERESTART\n");
      exit(-ERESTART);
    }
    perror ("pnCCDServer::fetch pgpCard read error");
    ret =  Ignore;
  } else ret *= sizeof(__u32);

  if (grabOffset) {
    _offset = data->frameNumber();
  }

  data->elementId(pgpCardRx.pgpLane);

  if ((ret > 0) && (ret < (int)_payloadSize)) {
    printf("pnCCDServer::fetch() returning Ignore, ret was %d, looking for %u, frame(%u) quad(%u) quadmask(%x) ",
        ret, _payloadSize, data->frameNumber(), data->elementId(), _quadMask);
    if (_debug & 4 || ret < 0) printf("\n\tframeNumber(%u) _quadsThisCount(%u) lane(%u) vc(%u)",
        data->frameNumber(), _quadsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
    ret = Ignore;
    //     printf("\n\t ");
    //     unsigned* u= (unsigned*)data;
    //     for (unsigned j=0; j<(sizeof(Pds::PNCCD::FrameV0)/sizeof(unsigned)); j++) {
    //       printf(" 0x%x", u[j]);
    //     }
    printf("\n");
  }

  unsigned damageMask = 0;
  if (pgpCardRx.eofe)      damageMask |= 1;
  if (pgpCardRx.fifoErr)   damageMask |= 2;
  if (pgpCardRx.lengthErr) damageMask |= 4;
  if (damageMask) {
    printf("pnCCDServer::fetch %s damageMask 0x%x, quad(%u) quadmask(%x)", ret>0 ? "setting user damage" : "ignoring wrong length", damageMask, data->elementId(), _quadMask);
    if (pgpCardRx.lengthErr) printf(" LENGTH_ERROR rxSize(%u)", (unsigned)pgpCardRx.rxSize);
    if (pgpCardRx.fifoErr) printf(" FIFO_ERROR");
    if (pgpCardRx.eofe) printf(" EOFE_ERROR");
    printf(" frame %u\n", data->frameNumber() - _offset);
    if (ret > 0) {
      damageMask |= 0xe0;
      _xtc.damage.increase(Pds::Damage::UserDefined);
      _xtc.damage.userBits(damageMask);
    }
  } else {
    unsigned oldCount = _count;
    _count = data->frameNumber() - _offset;
    if (_debug & 4 || ret < 0) printf("\n\tquad(%u)  _oldCount(%u) _count(%u) _quadsThisCount(%u) lane(%u) vc(%u)\n",
        data->elementId(), oldCount, _count, _quadsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
    if ((_count != oldCount) && (_quadsThisCount)) {
      if ((_count < oldCount) || (_count - oldCount > 10)) {
        printf("pnCCDServer::fetch ignoring unreasonable frame number, %u followed %u, quadMask 0x%x, quad %u\n", _count, oldCount, _quadMask, data->elementId());
        ret = Ignore;
      } else {
        int missing = _quads;
        for(unsigned k=0; k<4; k++) { if (_quadMask & 1<<k) missing -= 1; }
        printf("pnCCDServer::fetch detected missing %d quad%s in frame(%u) has %u quads,  quadMask %x, because quad %u,  frame %u arrived\n",
            missing, missing > 1 ? "s" : "", oldCount, _quadsThisCount, _quadMask, data->elementId(), _count);
        _quadsThisCount = 0;
        _quadMask = 1 << data->elementId();
        memcpy( payload, &_xtc, sizeof(Xtc) );
        ret = sizeof(Xtc);
      }
    }
  }
  if (ret > 0) {
    ret += xtcOffset;
    if (_quadMask & (1 << data->elementId())) {
      printf("pnCCDServer::fetch duplicate quad 0x%x with quadmask 0x%x, count %u, lane %u\n", data->elementId(), _quadMask, _count, pgpCardRx.pgpLane);
      damageMask |= 0xd0 | 1 << data->elementId();
      _xtc.damage.increase(Pds::Damage::UserDefined);
      _xtc.damage.userBits(damageMask);
    } else {
      _quadsThisCount += 1;
    }
    _quadMask |= 1 << data->elementId();
  }
  if (_debug & 1) printf(" returned %d\n", ret);
  return ret;
}

bool pnCCDServer::more() const {
  bool ret = _quads > 1;
  if (_debug & 2) printf("pnCCDServer::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned pnCCDServer::offset() const {
  unsigned ret = _quadsThisCount == 1 ? 0 : sizeof(Xtc) + _payloadSize * (_quadsThisCount-1);
  if (_debug & 2) printf("pnCCDServer::offset(%u)\n", ret);
  return (ret);
}

unsigned pnCCDServer::count() const {
  if (_debug & 2) printf( "pnCCDServer::count(%u)\n", _count);
  return _count;
}

unsigned pnCCDServer::flushInputQueue(int f, bool printFlag) {
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
  if (count && printFlag) {
    printf("\n");
    if (count == 100) {
      printf("\tpnCCDServer::flushInputQueue: pgpCardRx lane(%u) vc(%u) rxSize(%u) eofe(%s) lengthErr(%s)\n",
          pgpCardRx.pgpLane, pgpCardRx.pgpVc, pgpCardRx.rxSize, pgpCardRx.eofe ? "true" : "false",
                                                                pgpCardRx.lengthErr ? "true" : "false");
      printf("\t\t"); for (ret=0; ret<8; ret++) printf("%u ", _dummy[ret]);  printf("/n");
    }
  }
  return count;
}


void pnCCDServer::setpnCCD( int f ) {
  fd( f );
  if (unsigned c = this->flushInputQueue(f)) {
    printf("pnCCDServer::setpnCCD read %u time%s after opening pgpcard driver\n", c, c==1 ? "" : "s");
  }
  Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
  if (_cnfgrtr == 0) {
    _cnfgrtr = new Pds::pnCCD::pnCCDConfigurator(fd(), _debug);
  }
}

void pnCCDServer::printHisto(bool c) {
  printf("pnCCDServer ignored count %u times\n", _ignoreCount);
  printf("pnCCDServer event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      if (c) _histo[i] = 0;
    }
  }
}

void pnCCDServer::flagTriggerError() {
  printf("CspadServer::flagSyncError pnCCD has entered self-triggered mode\n");
  Pds::Occurrence* occ = new (_occPool) Pds::Occurrence(Pds::OccurrenceId::ClearReadout);
  _mgr->appliance().post(occ);
  UserMessage* umsg = new (_occPool) UserMessage("pnCCD has entered self triggered mode during a run");
  _mgr->appliance().post(umsg);
}
