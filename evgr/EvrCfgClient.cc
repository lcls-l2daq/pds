#include "pds/evgr/EvrCfgClient.hh"

#include "pds/config/EvrConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <new>

using namespace Pds;

static const unsigned EvrOutputs = 32;
static const unsigned EvrCodes   = 20;
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
    
    EventCodeType   etb[EvrCodes  ];
    PulseType       ptb[EvrOutputs];
    OutputMapType   omb[EvrOutputs];

    unsigned modid = reinterpret_cast<const DetInfo&>(_c.src()).devId();
    char*       pdst       = reinterpret_cast<char*>(dst);
    const char* psrc       = _buffer;
    const char* pend       = _buffer+len;
    
    while( psrc < pend ) {
      
      EventCodeType* et = etb;
      PulseType*     pt = ptb;
      OutputMapType* om = omb;
    
      unsigned npulses = 0;
      const EvrConfigType& tc = *reinterpret_cast<const EvrConfigType*>(psrc);      
      if (et + tc.neventcodes() >= etb + EvrCodes)
      {
        printf("EvrCfgClient::fetch(): Too many event codes. buffer size = %d \n", EvrCodes);
        return -1;
      }
      EventCodeType* peBase = et;
      ndarray<const EventCodeType,1> eventcodes = tc.eventcodes();
      for (unsigned i=0; i<tc.neventcodes(); ++i)
      {
        const EventCodeType& e = eventcodes[i];
        *new (et++) EventCodeType(e.code(), e.isReadout(), e.isCommand(), e.isLatch(), e.reportDelay(), e.reportWidth(), 
                                  0, 0, 0, e.desc(), e.readoutGroup());
      }
                
      ndarray<const PulseType,1> pulses = tc.pulses();
      for(unsigned j=0; j<tc.npulses(); j++) {
        const PulseType& p = pulses[j];
        bool lUsed=false;
        ndarray<const OutputMapType,1> omaps = tc.output_maps();
        for(unsigned k=0; k<tc.noutputs(); k++) {
          const OutputMapType& o = omaps[k];
          if ( o.source()==OutputMapType::Pulse &&
               o.source_id()==j )
            if (o.module() == modid) {              
              if (om >= omb + EvrOutputs)
              {
                printf("EvrCfgClient::fetch(): Too many output mappings. Max buffer size = %d.\n", EvrOutputs);
                return -1;
              }              
              *new (om++) OutputMapType
                (o.source(), npulses,
                 o.conn  (), o.conn_id(), modid);
              lUsed=true;
            }
        }
        if (lUsed)
        {
          if (pt >= ptb + EvrOutputs)
          {
            printf("EvrCfgClient::fetch(): Too many pulses. Max buffer size = %d.\n", EvrOutputs);
            return -1;
          }                        
          *new (pt++) PulseType(npulses++,
                                p.polarity(),
                                p.prescale(),
                                p.delay(),
                                p.width());
                        
          EventCodeType* pe = peBase;
          ndarray<const EventCodeType,1> eventcodes = tc.eventcodes();
          for (unsigned i=0; i<tc.neventcodes(); ++i, ++pe)
          {
            const EventCodeType& e = eventcodes[i];
            if (e.maskTrigger() & (1<<p.pulseId()))
              Pds::EvrConfig::setMask(*pe, 0x1, 1<<(npulses-1));
            //printf("EvrCfgClient::fetch() set event [%d] %d (org %d) masktrigger pulse %d (org %d) current mask 0x%x (org 0x%x)\n",
            //  i, pe->code(), e.code(), npulses-1, p.pulseId(), pe->maskTrigger(), e.maskTrigger() );
          }                                               
        }
      }

      if (pdst + 
          (et-etb)*sizeof(EventCodeType) +
          (pt-ptb)*sizeof(PulseType)   +
          (om-omb)*sizeof(OutputMapType) > 
          reinterpret_cast<char*>(dst) + maxSize )
      {
        printf("EvrCfgClient::fetch exceeding maxSize %d bytes\n",maxSize);
        return -1;
      }

      EvrConfigType& ntc = *new(pdst) EvrConfigType(et-etb, 
                                                    pt-ptb,
                                                    om-omb,
                                                    etb, ptb, omb, tc.seq_config());

      psrc += Pds::EvrConfig::size(tc);
      pdst += Pds::EvrConfig::size(ntc);
    }
    return pdst - reinterpret_cast<char*>(dst);
  }
  else
    return _c.fetch(tr, id, dst, maxSize);
}
