#include "pds/evgr/EvrCfgClient.hh"

#include "pds/config/EvrConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <new>
#include <string.h>

using namespace Pds;

static const unsigned EvrOutputs = 32;
static const unsigned EvrCodes   = 64;
static const unsigned BufferSize = 0x200000;

EvrCfgClient::EvrCfgClient( const Src&  src,
                            char*       eclist) :
  CfgClientNfs(src),
  _buffer(new char[BufferSize])
{
  if (eclist) {
    char *n;
    char *p = strtok_r(eclist,",",&n);
    while(p) {
      printf("\tparsing %s\n",p);
      char* endp;
      char* q = strchr(p,'-');
      if (q) {
	unsigned v0 = strtoul(p,&endp,0);
	if (endp!=q) {
	  printf("EvrCfgClient::error parsing default eventcode range %s\n",p);
	  exit(1);
	}
	unsigned v1 = strtoul(q+1,&endp,0);
	if (*endp) {
	  printf("EvrCfgClient::error parsing default eventcode range %s\n",p);
	  exit(1);
	}
	while(v0 <= v1)
	  _default_codes.push_back(v0++);
      }
      else {
	unsigned v0 = strtoul(p,&endp,0);
	if (*endp) {
	  printf("EvrCfgClient::error parsing default eventcode %s\n",p);
	  exit(1);
	}
	_default_codes.push_back(v0);
      }
      p = strtok_r(0,",",&n);
    }
  }
}

EvrCfgClient::~EvrCfgClient()
{
  delete[] _buffer;
}

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
  std::list<unsigned> default_codes(_default_codes);

  if (id.value() == _evrConfigType.value()) {
    int len = CfgClientNfs::fetch(tr, id, _buffer, BufferSize);
    if (len <= 0) return len;
    
    EventCodeType   etb[EvrCodes  ];
    PulseType       ptb[EvrOutputs];
    OutputMapType   omb[EvrOutputs];

    unsigned modid = reinterpret_cast<const DetInfo&>(src()).devId();
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
        default_codes.remove(e.code());
      }
      //
      //  Add default codes that haven't yet been specified
      //
      if (default_codes.size() + et-etb > int(EvrCodes)) {
        printf("EvrCfgClient::fetch(): Too many eventcodes. Max buffer size = %d.\n", EvrCodes);
        return -1;
      }
      else
        for (std::list<unsigned>::iterator it=default_codes.begin();
             it!=default_codes.end(); it++)
          *new (et++) EventCodeType(*it, false, false, false, 0, 1, 0, 0, 0, 0, 0);
                
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
    return CfgClientNfs::fetch(tr, id, dst, maxSize);
}
