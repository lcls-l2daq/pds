/*
 * Configurator.cc
 *
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "pds/genericpgp/Configurator.hh"
#include "pds/genericpgp/Destination.hh"
#include "pds/config/GenericPgpConfigType.hh"

using namespace Pds::GenericPgp;

static unsigned tid=0;
static unsigned VersionAddr=0;  // used to flush input

Configurator::Configurator(int f, unsigned d) :
  Pds::Pgp::Configurator(f, d),
  _testModeState(0), _config(0), _rhisto(0) {
}

Configurator::~Configurator() {}

void Configurator::start() {
  if (!_pgp)
    printf("Configurator::start found nil _pgp so not set\n");
  else
    _execute_sequence(_config->sequence_length()[1],
                      &_config->sequence()[_config->sequence_length()[0]],
                      const_cast<uint32_t*>(_config->payload().data()));
}

void Configurator::stop() {
  if (!_pgp)
    printf("Configurator::stop found nil _pgp so not set\n");
  else
    _execute_sequence(_config->sequence_length()[2],
                      &_config->sequence()[_config->sequence_length()[0]+
					   _config->sequence_length()[1]],
                      const_cast<uint32_t*>(_config->payload().data()));
}

void Configurator::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigFileName, name);
  printf("Configurator::runTimeConfigName(%s)\n", name);
}

/*
**  Not sure what to do here
*/
bool Configurator::_flush(unsigned index) {
  enum {numberOfTries=50};
  unsigned version = 0;
  unsigned failCount = 0;
  bool ret = false;
  printf("\n\t--flush-%u-", index);
  _d.dest(Destination::Registers);
  while ((failCount++<numberOfTries)
      && (Failure == _pgp->readRegister(&_d, VersionAddr, tid++, &version))) {
    printf("%s(%u)-", _d.name(), failCount);
  }
  if (failCount<numberOfTries) {
    printf("%s version(0x%x)\n\t", _d.name(), version);
  }
  else {
    ret = true;
    printf("_flush FAILED!!\n\t");
  }
  return ret;
}

unsigned Configurator::configure( GenericPgpConfigType* c, unsigned first) {
  _config = c;

  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);

  unsigned ret = _execute_sequence(_config->sequence_length()[0], 
                                   &_config->sequence()[0],
                                   const_cast<uint32_t*>(_config->payload().data()));

  ret <<= 1;

  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  printf("- 0x%x - \n\tdone \n", ret);
  printf(" it took %lld.%lld milliseconds with first %u\n", diff/1000000LL, diff%1000000LL, first);
  if (ret) dumpFrontEnd();

  return ret;
}

unsigned Configurator::_execute_sequence(unsigned               length, 
                                         const GenericPgp::CRegister* seq, 
                                         uint32_t*              payload)
{
  unsigned ret = 0;

  _d.dest(Destination::Registers);
  for(unsigned i=0; i<length; i++) {
    const CRegister& reg = seq[i];
    switch (CRegister::Action(reg.action())) {
    case CRegister::RegisterWrite:
      if (_pgp->writeRegister(&_d, reg.address(), payload[reg.offset()])) {
        printf("Configurator failed writing address 0x%x at sequence step %d\n", 
               reg.address(),i);
        ret |= Failure;
      }
      break;
    case CRegister::RegisterWriteA:
      if (_pgp->writeRegister(&_d, reg.address(), payload[reg.offset()], false, Pgp::PgpRSBits::Waiting)) {
        printf("Configurator failed writing address 0x%x at sequence step %d\n", 
               reg.address(),i);
        ret |= Failure;
      }
      else {
	for(unsigned i=0; i<6; i++) {
	  Pds::Pgp::RegisterSlaveImportFrame* rsif = _pgp->read(4);
	  if (!rsif)
	    break;
	  if (rsif->waiting() == Pds::Pgp::PgpRSBits::Waiting)
	    break;
	}
      }
      break;
    case CRegister::RegisterRead:
      if (_pgp->readRegister(&_d, reg.address(), tid++, &payload[reg.offset()], reg.datasize())) {
        printf("Configurator failed readback address 0x%x to payload %u[%u] at sequence step %d\n", 
               reg.address(),reg.offset(),reg.datasize(),i);
        ret |= Failure;
      }
      break;
    case CRegister::RegisterVerify:
      { uint32_t* tmp = new uint32_t[reg.datasize()];
        if (_pgp->readRegister(&_d, reg.address(), tid++, tmp, reg.datasize())) {
          printf("Configurator failed readback address 0x%x at sequence step %d\n", 
                 reg.address(),i);
          ret |= Failure;
        }
        else {
          for(unsigned j=0; j<reg.datasize(); j++)
            if ((tmp[j]&reg.mask()) != (payload[reg.offset()+j]&reg.mask())) {
              printf("Configurator verify failed address 0x%x [word %d %x!=%x] at sequence step %d\n",
                     reg.address(),j,tmp[j]&reg.mask(),payload[reg.offset()+j]&reg.mask(),i);
              ret |= Failure;
              break;
            }
        }
        delete[] tmp;
      }
      break;
    case CRegister::Spin:
      microSpin(payload[reg.offset()]);
      break;
    case CRegister::Usleep:
      if (usleep(payload[reg.offset()]))
        printf("Configurator failed to sleep %d usec\n",payload[reg.offset()]);
      break;
    case CRegister::Flush:
      if (_flush()) {
	printf("Configurator::configure determined that we lost contact with the front end, exiting!\n");
	return 1;
      }

    default:
      break;
    }
  }
  return ret;
}

void Configurator::dumpFrontEnd() {
#if 0
  timespec      start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  int ret = Success;
  if (_debug & 0x100) {
    uint32_t count = Configurator::sequenceCount();
    uint32_t acount = Configurator::acquisitionCount();
    printf("\tSequenceCount(%u), Acquisition Count(%u)\n", count, acount);
  }
  if (_debug & 0x400) {
    printf("Checking Configuration, no news is good news ...\n");
    if (Failure == checkWrittenConfig(false)) {
      printf("Configurator::checkWrittenConfig() FAILED !!!\n");
    }
//    enableRunTrigger(false);
//    if (Failure == checkWrittenASIC(false)) {
//      printf("Configurator::checkWrittenASICConfig() FAILED !!!\n");
//    }
//    enableRunTrigger(true);
  }
  clock_gettime(CLOCK_REALTIME, &end);
  uint64_t diff = timeDiff(&end, &start) + 50000LL;
  if (_debug & 0x700) {
    printf("Configurator::dumpFrontEnd took %lld.%lld milliseconds", diff/1000000LL, diff%1000000LL);
    printf(" - %s\n", ret == Success ? "Success" : "Failed!");
  }
  return;
#endif
}
