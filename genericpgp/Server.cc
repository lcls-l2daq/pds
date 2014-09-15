/*
 * Server.cc
 *
 */

#include "pds/genericpgp/Server.hh"
#include "pds/genericpgp/Configurator.hh"
#include "pds/genericpgp/Destination.hh"
#include "pds/config/GenericPgpConfigType.hh"
#include "pds/config/EpixDataType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/config/EpixSamplerDataType.hh"
#include "pds/pgp/DataImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/SysClk.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pgpcard/PgpCardMod.h"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <omp.h>
#include <new>

using namespace Pds;

GenericPgp::Server* GenericPgp::Server::_instance = 0;


GenericPgp::Server::Server( const Pds::Src& client, unsigned configMask )
  : _xtc( _xtcType, client ),
    _cnfgrtr(0),
    _elements(ElementsPerSegmentLevel),
    _payloadSize(0),
    _configureResult(0),
    _debug(0),
    _offset(0),
    _unconfiguredErrors(0),
    _configured(false),
    _ignoreFetch(true),
    _resetOnEveryConfig(false),
    _monitor(sizeOfHisto) 
{
  _task = new Pds::Task(Pds::TaskObject("EPIXprocessor"));
  strcpy(_runTimeConfigName, "");
  instance(this);
  printf("Server::Server() payload(%u)\n", _payloadSize);
}

unsigned GenericPgp::Server::configure(GenericPgpConfigType* config, bool forceConfig) {
  unsigned firstConfig = _resetOnEveryConfig || forceConfig;
  if (_cnfgrtr == 0) {
    firstConfig = 1;
    _cnfgrtr = new GenericPgp::Configurator(fd(), _debug);
    _cnfgrtr->runTimeConfigName(_runTimeConfigName);
    printf("Server::configure making new configurator %p, firstConfig %u\n", _cnfgrtr, firstConfig);
  }

  unsigned c = flushInputQueue(fd());
  if (c) printf("Server::configure flushed %u event%s before configuration\n", c, c>1 ? "s" : "");

  if ((_configureResult = _cnfgrtr->configure(config, firstConfig))) {
    printf("Server::configure failed 0x%x, first %u\n", _configureResult, firstConfig);
  } else {

    for(unsigned i=0; i<_payload_buffer.size(); i++)
      delete[] _payload_buffer[i];

    _payload_xtc   .resize(config->number_of_streams());
    _payload_buffer.resize(config->number_of_streams());

    { unsigned sz=EpixDataType::_sizeof(*config);
      _payload_xtc   [0] = Xtc(_epixDataType, client());
      _payload_xtc   [0].extent += sz*_elements;
      _payload_buffer[0] = new char[sz];
      _payloadSize = sz; }

    for(unsigned i=1; i<config->number_of_streams(); i++) {
      unsigned sz=0;
      Pds::TypeId payload_id(Pds::TypeId::Any,0);
      if (config->stream()[i].config_type()==_epixSamplerConfigType.value()) {
	const EpixSamplerConfigType& c =
	  *reinterpret_cast<const EpixSamplerConfigType*>(config->payload().data()+
							  config->stream()[i].config_offset());
	payload_id = _epixSamplerDataType;
	sz = EpixSamplerDataType::_sizeof(c);
      }
      else {
	printf("Unknown configuration type for stream[%d] 0x%x\n",
	       i, config->stream()[i].config_type());
	return 0xdeadbeef;
      }
      _payload_xtc   [i] = Xtc(payload_id, client());
      _payload_xtc   [i].extent += sz;
      _payload_buffer[i] = new char[sz];
    }
  }

  _count = _elementsThisCount = 0;
  _configured = _configureResult == 0;
  c = this->flushInputQueue(fd());
  _monitor.clear();
  if (c) printf("Server::configure flushed %u event%s after confguration\n", c, c>1 ? "s" : "");

  return _configureResult;
}

void GenericPgp::Server::die() {
    printf("Server::die has been called !!!!!!!\n");
}

void GenericPgp::Server::dumpFrontEnd() {
  if (_cnfgrtr) _cnfgrtr->dumpFrontEnd();
  else printf("Server::dumpFrontEnd() found nil configurator\n");
}

static unsigned* procHisto = (unsigned*) calloc(1000, sizeof(unsigned));

void GenericPgp::Server::process(char* d) {
  Pds::Epix::ElementV1* e = (Pds::Epix::ElementV1*) _payload_buffer[0];
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
  long long unsigned diff = SysClk::diff(end,start);
  diff += 50000;
  diff /= 100000;
  if (diff > 1000-1) diff = 1000-1;
  procHisto[diff] += 1;
}

void GenericPgp::Server::allocated() {
//  _cnfgrtr->resetFrontEnd();
}

void GenericPgp::Server::enable() {
  if (usleep(10000)<0) perror("Server::enable usleep failed\n");
  usleep(10000);
  _cnfgrtr->start();
  flushInputQueue(fd());
  _ignoreFetch = false;
  _payload_mask = 0;
  _monitor.start();
  if (_debug & 0x20) printf("Server::enable\n");
}

void GenericPgp::Server::disable() {
  _ignoreFetch = true;
  if (_cnfgrtr) {
    _cnfgrtr->stop();
    flushInputQueue(fd());
    if (usleep(10000)<0) perror("Server::disable ulseep 1 failed\n");
    if (_debug & 0x20) printf("Server::disable\n");
  } else {
    printf("Server::disable() found nil configurator, so did not disable\n");
  }

}

void GenericPgp::Server::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigName, name);
  printf("Server::runTimeConfigName(%s)\n", name);
}

unsigned GenericPgp::Server::unconfigure(void) {
  unsigned c = flushInputQueue(fd());
  if (c) printf("Server::unconfigure flushed %u event%s\n", c, c>1 ? "s" : "");
  return 0;
}

int GenericPgp::Server::fetch( char* payload, int flags ) {
   int ret = 0;
   PgpCardRx       pgpCardRx;
   enum {Ignore=-1};

   if (_configured == false)  {
     if (++_unconfiguredErrors<20) printf("Server::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
     unsigned c = this->flushInputQueue(fd());
     if (_unconfiguredErrors<20 && c) printf("\tWe flushed %u input buffer%s\n", c, c>1 ? "s" : "");
     return Ignore;
   }

   if (_debug & 1) printf("Server::fetch called ");

   _xtc.damage = 0;

   pgpCardRx.model   = sizeof(&pgpCardRx);
   pgpCardRx.maxSize = _payloadSize / sizeof(__u32);
   pgpCardRx.data    = (__u32*)_payload_buffer[0];

   if ((ret = read(fd(), &pgpCardRx, sizeof(PgpCardRx))) < 0) {
     if (errno == ERESTART) {
       disable();
       char message[400];
       sprintf(message, "Pgpcard problem! Restart the DAQ system\nIf this does not work, Power cycle %s\n",
           getenv("HOSTNAME"));
       UserMessage* umsg = new (_occPool) UserMessage;
       umsg->append(message);
       umsg->append(DetInfo::name(static_cast<const DetInfo&>(_xtc.src)));
       _app->post(umsg);
       _ignoreFetch = true;
       printf("Server::fetch exiting because of ERESTART\n");
       exit(-ERESTART);
     }
     perror ("Server::fetch pgpCard read error");
     ret =  Ignore;
   } else ret *= sizeof(__u32);

   if (_ignoreFetch) return Ignore;

   unsigned damageMask = 0;
   if (pgpCardRx.eofe)      damageMask |= 1;
   if (pgpCardRx.fifoErr)   damageMask |= 2;
   if (pgpCardRx.lengthErr) damageMask |= 4;

   const GenericPgpConfigType& c = _cnfgrtr->configuration();
   for(unsigned i=0; i<c.number_of_streams(); i++) {
     if (pgpCardRx.pgpVc == c.stream()[i].pgp_channel()) {

       _payload_mask |= 1<<i;

       _payload_xtc[i].damage = 0;
       if (damageMask) {
	 _payload_xtc[i].damage.increase(Pds::Damage::UserDefined);
	 _payload_xtc[i].damage.userBits(damageMask | 0xe0);
       }

       if (i==0) {
	 Pds::Pgp::DataImportFrame* data = reinterpret_cast<Pds::Pgp::DataImportFrame*>(pgpCardRx.data);
	 if ((ret > 0) && (ret < (int)_payloadSize)) {
	   printf("Server::fetch() returning Ignore, ret was %d, looking for %u\n", ret, _payloadSize);
	   if ((_debug & 4) || ret < 0) printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) raw_count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
					       data->elementId(), data->_frameType, data->acqCount(), data->frameNumber(), _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
	   uint32_t* u = (uint32_t*)data;
	   printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
	   ret = Ignore;
	 }
	 if (ret > (int) _payloadSize) printf("Server::fetch pgp read returned too much _payloadSize(%u) ret(%d)\n", _payloadSize, ret);

	 if (!damageMask) {
	   unsigned oldCount = _count;
	   _count = data->frameNumber() - 1;  // starts counting at 1, not zero
	   if ((_debug & 5) || ret < 0) {
	     printf("\telementId(%u) frameType(0x%x) acqcount(0x%x) _oldCount(%u) _count(%u) _elementsThisCount(%u) lane(%u) vc(%u)\n",
		    data->elementId(), data->_frameType, data->acqCount(),  oldCount, _count, _elementsThisCount, pgpCardRx.pgpLane, pgpCardRx.pgpVc);
	     uint32_t* u = (uint32_t*)data;
	     printf("\tDataHeader: "); for (int i=0; i<16; i++) printf("0x%x ", u[i]); printf("\n");
	   }
	 }
	 
	 _monitor.event(_count);

	 //  Assume this is the last contribution to be fetched
	 Xtc& xtc = *new(payload) Xtc(_xtc.contains, _xtc.src);
	 for(unsigned j=0; j<c.number_of_streams(); j++) {
	   if (_payload_mask & (1<<j)) {
	     Xtc& pxtc = *new(&xtc) Xtc(_payload_xtc[j]);
	     pxtc.extent = _payload_xtc[j].extent;
	     if (j==0)
	       process((char*)xtc.alloc(pxtc.sizeofPayload()));
	     else
	       memcpy(xtc.alloc(pxtc.sizeofPayload()),
		      _payload_buffer[j], pxtc.sizeofPayload());
	     _payload_mask ^= (1<<j);
	   }
	 }
	 _xtc = xtc;
	 return xtc.extent;
       }
       else {
	 memcpy(_payload_buffer[i],_payload_buffer[0], 
		_payload_xtc[i].sizeofPayload());
       }
     }
   }

   return Ignore;
}

bool GenericPgp::Server::more() const {
  bool ret = _elements > 1;
  if (_debug & 2) printf("Server::more(%s)\n", ret ? "true" : "false");
  return ret;
}

unsigned GenericPgp::Server::offset() const {
  unsigned ret = _elementsThisCount == 1 ? 0 : sizeof(Xtc) + _payloadSize * (_elementsThisCount-1);
  if (_debug & 2) printf("Server::offset(%u)\n", ret);
  return (ret);
}

unsigned GenericPgp::Server::count() const {
  if (_debug & 2) printf( "Server::count(%u)\n", _count + _offset);
  return _count + _offset;
}

unsigned GenericPgp::Server::flushInputQueue(int f) {
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

void GenericPgp::Server::setFd( int f ) {
  fd( f );
  Pds::Pgp::RegisterSlaveExportFrame::FileDescr(f);
  if (unsigned c = this->flushInputQueue(f)) {
    printf("Server::setEpix read %u time%s after opening pgpcard driver\n", c, c==1 ? "" : "s");
  }
//  if (_cnfgrtr == 0) {
//    _cnfgrtr = new Pds::Epix::EpixConfigurator::EpixConfigurator(fd(), _debug);
//  }
}

void GenericPgp::Server::printHisto(bool c) {
  _monitor.print();
  unsigned histoSum = 0;
  printf("Server unshuffle event periods\n");
  for (unsigned i=0; i<1000; i++) {
    if (procHisto[i]) {
      printf("\t%3u 0.1ms   %8u\n", i, procHisto[i]);
      histoSum += procHisto[i];
      if (c) procHisto[i] = 0;
    }
  }
  printf("\tProchisto (events) Sum was %u\n", histoSum);
}
