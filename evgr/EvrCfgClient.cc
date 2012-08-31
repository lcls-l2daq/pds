#include "pds/evgr/EvrCfgClient.hh"

#include "pds/config/EvrConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <new>

using namespace Pds;

static const unsigned EvrOutputs = 32;
static const unsigned BufferSize = 0x200000;

EvrCfgClient::EvrCfgClient( CfgClientNfs& c ) :
  _c     (c),
  _buffer(new char[BufferSize])
{
}

EvrCfgClient::~EvrCfgClient()
{
  delete[] _buffer;
}

const Src& EvrCfgClient::src() const { return _c.src(); }

void EvrCfgClient::initialize(const Allocation& a) { _c.initialize(a); }

//
//  Extract the EVR configuration relevant for this module
//  Includes:
//    All EventCode definitions
//    PulseType definitions which drive outputs on this module
//    Outputs belonging to this module
//    SequencerConfig (not necessary, but small)
//
int EvrCfgClient::fetch(const Transition& tr, 
                        const TypeId&     id, 
                        void*             dst,
                        unsigned          maxSize)
{
  if (id.value() == _evrConfigType.value()) {
    int len = _c.fetch(tr, id, _buffer, BufferSize);
    if (len <= 0) return len;

    EvrConfigType::PulseType     ptb[EvrOutputs];
    EvrConfigType::OutputMapType omb[EvrOutputs];

    unsigned modid = reinterpret_cast<const DetInfo&>(_c.src()).devId();
    char*       pdst       = reinterpret_cast<char*>(dst);
    const char* psrc       = _buffer;
    const char* pend       = _buffer+len;
    while( psrc < pend ) {
      EvrConfigType::PulseType*     pt = ptb;
      EvrConfigType::OutputMapType* om = omb;
      unsigned npulses = 0;
      const EvrConfigType& tc = *reinterpret_cast<const EvrConfigType*>(psrc);
      for(unsigned j=0; j<tc.npulses(); j++) {
        const EvrConfigType::PulseType& p = tc.pulse(j);
        bool lUsed=false;
        for(unsigned k=0; k<tc.noutputs(); k++) {
          const EvrConfigType::OutputMapType& o = tc.output_map(k);
          if ( o.source()==EvrConfigType::OutputMapType::Pulse &&
               o.source_id()==j )
            if (o.module() == modid) {
              *new (om++) EvrConfigType::OutputMapType
                (o.source(), npulses,
                 o.conn  (), o.conn_id(), modid);
              lUsed=true;
            }
        }
        if (lUsed)
          *new (pt++) EvrConfigType::PulseType(npulses++,
                                               p.polarity(),
                                               p.prescale(),
                                               p.delay(),
                                               p.width());
      }

      if (pdst + tc.size() > 
          reinterpret_cast<char*>(dst) + maxSize 
          + (pt-ptb)*sizeof(EvrConfigType::PulseType)
          + (om-omb)*sizeof(EvrConfigType::OutputMapType)) {
        printf("EvrCfgClient::fetch exceeding maxSize %d bytes\n",maxSize);
        return -1;
      }

      EvrConfigType& ntc = *new(pdst) EvrConfigType(tc.neventcodes(),
                                                    &tc.eventcode(0),
                                                    pt-ptb, ptb,
                                                    om-omb, omb,
                                                    tc.enableReadGroup(),
                                                    tc.seq_config());
      psrc +=  tc.size();
      pdst += ntc.size();
    }
    return pdst - reinterpret_cast<char*>(dst);
  }
  else
    return _c.fetch(tr, id, dst, maxSize);
}
