#include "pds/ibeb/RdmaSegment.hh"
#include "pds/ibeb/RdmaWrPort.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/service/Semaphore.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/smldata.ddl.h"
#include "pdsdata/psddl/generic1d.ddl.h"

#include <new>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

//#define VERBOSE

static const unsigned short outlet_port = 11000;
static const unsigned max_dg_size = 0x4000;
static bool lverbose = false;

static void dump(const Pds::Datagram* dg)
{
  char buff[128];
  time_t t = dg->seq.clock().seconds();
  strftime(buff,128,"%H:%M:%S",localtime(&t));
  printf("[%p] %s.%09u %016lx %s extent 0x%x damage %x\n",
         dg,
         buff,
         dg->seq.clock().nanoseconds(),
         dg->seq.stamp().pulseID(),
         Pds::TransitionId::name(Pds::TransitionId::Value((dg->seq.stamp().fiducials()>>24)&0xff)),
         dg->xtc.extent, dg->xtc.damage.value());
}

using Pds::IbEb::RdmaComplete;
using Pds::IbEb::RdmaSegment;

namespace Pds {

  class DmaSim : public Routine {
    enum { ALIGN_BYTES=32 };
  public:
    DmaSim(unsigned sz,
           unsigned depth,
           unsigned ndst,
           unsigned ndstbuf) :
      _sz     (sz),
      _depth  (depth),
      _ndst   (ndst),
      _ndstbuf(ndstbuf),
      _bsz    ((sz+32+ALIGN_BYTES-1)&~(ALIGN_BYTES-1)),
      _pool   (new char[_bsz*_depth+ALIGN_BYTES]),
      _rd     (0),
      _wr     (0),
      _task   (new Task(TaskObject("dma")))
    { if (pipe(_fd))
        perror("pipe");
      _task->call(this); }
    ~DmaSim()
    { delete _pool; }
  public:  
    // poll interface
    int      fd    ()       const { return _fd[0]; }
  public:
    //  data interface
    //  number of 32b words
    unsigned words(char* p) const 
    { return reinterpret_cast<uint32_t*>(p)[0]-8; }
    //  event destination node
    unsigned dst  (char* p) const 
    { return reinterpret_cast<uint16_t*>(p)[2]; }
    //  event destination buffer
    unsigned tgt  (char* p) const 
    { return reinterpret_cast<uint16_t*>(p)[3]; }
    uint8_t* data (char* p) const
    { return &reinterpret_cast<uint8_t*>(p)[32]; }
    //  done with buffer
    void     ack   (char* p) 
    { _rd++; }
  public:
    void routine() {
      uint32_t* data = new uint32_t[_bsz>>2];
      data[0] = _bsz>>2;
      memset(&data[1],0,7*sizeof(uint32_t));

      uint16_t* d16 = reinterpret_cast<uint16_t*>(&data[8]);
      for(unsigned i=0; i<(_sz>>1); i++)
        d16[i] = (i&0xffff);

      char* ptr = (char*)(((uintptr_t)_pool+ALIGN_BYTES)&~(ALIGN_BYTES-1));

      if (lverbose)
        printf("DmaSim writing to pool at %p, bsz %x\n",ptr,_bsz);

      while(1) {
        if (_wr < _rd+_depth) {
          char* bptr = ptr + _bsz*(_wr % _depth);
          memcpy(bptr, data, _bsz);
          //  Tag the event for destination
          reinterpret_cast<uint16_t*>(bptr)[2] = (_wr%_ndst);
          reinterpret_cast<uint16_t*>(bptr)[3] = (_wr/_ndst)%_ndstbuf;
          ::write(_fd[1],&bptr,sizeof(bptr));
          _wr++;
          if (lverbose)
            printf("DmaSim wrote event %08x to fd %d\n",
                   reinterpret_cast<uint32_t*>(bptr)[1], _fd[1]);
        }
      }
    }
  private:
    unsigned             _sz;
    unsigned             _depth;
    unsigned             _ndst;
    unsigned             _ndstbuf;
    unsigned             _bsz;
    char*                _pool;
    volatile uint64_t    _rd;
    volatile uint64_t    _wr;
    int                  _fd[2];
    Task*                _task;
  };
};

using namespace Pds;

static RdmaSegment* rseg = 0;

void sigHandler(int signal) {
  if (rseg)
    delete rseg;

  ::exit(signal);
}

typedef Pds::SmlData::ProxyV1 SmlD;
typedef Pds::Generic1D::DataV0 GenD;

static const TypeId  smlType((TypeId::Type)SmlD::TypeId,SmlD::Version);
static const TypeId  genType((TypeId::Type)GenD::TypeId,GenD::Version);

namespace Pds {
  class HeaderSim : public Routine {
  public:
    HeaderSim(unsigned maxEventSize, DetInfo& info, DmaSim& sim, RdmaSegment& outlet, Semaphore& sem) :
      _pid(0),
      _maxEventSize(maxEventSize),
      _sim(sim),
      _info(info),
      _outlet(outlet),
      _sem(sem),
      _task   (new Task(TaskObject("header")))
    { _task->call(this); }
    ~HeaderSim() {}
  public:
    void routine() {

      while(1) {

        char* buffer = (char*)_outlet.alloc(_maxEventSize);
        if (buffer==0) {
          continue;
        }

        char* p;
        if (::read(_sim.fd(),&p,sizeof(p))<0)
          perror("read");
        else {
          // Fake datagram header
          _sem.take();
          struct timespec ts;
          clock_gettime(CLOCK_REALTIME,&ts);
          Dgram dg;
          dg.seq = Sequence(Sequence::Event,TransitionId::L1Accept,
                            ClockTime(ts),TimeStamp(_pid++));

          // Make proxy to data (small data)
          Datagram* pdg = new (buffer)
          Datagram(dg,smlType,_info);
          {SmlD* t = new (pdg->xtc.next())
            SmlD((char*)pdg->xtc.next()-(char*)0+sizeof(Datagram),
                   genType,
                   _sim.words(p)*sizeof(uint32_t)+sizeof(GenD));
            pdg->xtc.alloc(t->_sizeof()); }
          Datagram* tdg = new ((char*)pdg->xtc.next())
          Datagram(genType,_info);
          new (tdg->xtc.alloc(_sim.words(p)*sizeof(uint32_t)+sizeof(GenD)))
          GenD(_sim.words(p)*sizeof(uint32_t),_sim.data(p));

          _outlet.queue(_sim.dst(p), _sim.tgt(p),
                        pdg, (char*)tdg->xtc.next()-buffer);
          _sim.ack(p);
          _sem.give();
        }
      }
    }
  private:
    uint64_t      _pid;
    unsigned      _maxEventSize;
    DmaSim&       _sim;
    DetInfo&      _info;
    RdmaSegment&  _outlet;
    Semaphore&    _sem;
    Task*         _task;
  };
};

int main(int argc, char* argv[])
{
  unsigned   bigSize   = 4096;
  unsigned   evtDepth  = 32;
  unsigned   id        = 0;
  unsigned   numEbs    = 1;

  int c;
  while ( (c=getopt( argc, argv, "i:s:n:e:hv")) != EOF ) {
    switch(c) {
    case 'i': id         = strtoul(optarg,NULL,0); break;
    case 's': bigSize    = strtoul(optarg,NULL,0); break;
    case 'n': evtDepth   = strtoul(optarg,NULL,0); break;
    case 'e': numEbs     = strtoul(optarg,NULL,0); break;
    case 'v': lverbose   = true; break;
    default:
      break;
    }
  }

  RdmaSegment::verbose(lverbose);
  Pds::IbEb::RdmaWrPort::verbose(lverbose);

  ::signal( SIGINT, sigHandler );

  //  Create RingPool for output datagram (small data and big data)
  //  Setup RDMA end point for this memory block
  //  Exchange with Event level once?  or measure how long queue pair setup takes
  //  Fill in with dummy data
  //  Loop:
  //    (1) recover RingPool deallocs from RDMA_RD complete
  //    (2a) Write new dg header
  //    (2) For current event, if small data target (event level) is free, push
  //  

  unsigned maxEventSize = bigSize+sizeof(SmlD)+2*sizeof(Xtc)+sizeof(Datagram);
  
  DetInfo info    (0,DetInfo::XppGon,0,DetInfo::Wave8,id);

  RdmaSegment outlet(maxEventSize*evtDepth,
                     maxEventSize,
                     id,
                     numEbs,
                     evtDepth);

  rseg = &outlet;

  //
  //  Simulate DMA in a separate thread
  //
  Semaphore sem(Semaphore::FULL);
  DmaSim sim(bigSize,evtDepth,numEbs,outlet.nBuffers());
  HeaderSim header(maxEventSize, info, sim, outlet, sem);

  timespec ts_last;
  clock_gettime(CLOCK_REALTIME,&ts_last);

  //
  //  Poll for:
  //    1)  New event
  //          Put in ringpool
  //          If push target is free
  //            Launch RDMA_WR
  //          Else
  //            Queue
  //    2)  RDMA_RD completions
  //          Free memory in ringpool
  //          Free push target
  //          If target was queued
  //            Launch RDMA_WR
  //
  const int MAX_WC=32;
  ibv_wc wc[MAX_WC];
  while(1) {
    int nc = ibv_poll_cq(outlet.cq(), MAX_WC, wc);
    if (nc<0) continue;
    for(int i=0; i<nc; i++) {
      if (wc[i].status!=IBV_WC_SUCCESS) {
        printf("wc error %x  opcode %x  wr_id %llx\n",
               wc[i].status,wc[i].opcode,(unsigned long long)wc[i].wr_id);
        abort();
        continue;
      }
      outlet.complete(wc[i], sem);
    }
  }

  return 1;
}
