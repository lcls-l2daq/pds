#include "pds/pvdaq/BeamMonitorServer.hh"
#include "pds/pvdaq/PvServer.hh"
#include "pds/xtc/InDatagram.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/beammonitor.ddl.h"

using namespace Pds::PvDaq;

//
//  Things that belong in pds/config/BeamMonitorType.hh
//
typedef Pds::BeamMonitor::ConfigV0 BMConfig;

static Pds::TypeId _config_type( Pds::TypeId::Type(BMConfig::TypeId),
                                 BMConfig::Version );
static unsigned    _config_size = sizeof(BMConfig);

typedef Pds::BeamMonitor::DataV0 BMData;

static Pds::TypeId _data_type( Pds::TypeId::Type(BMData::TypeId),
                               BMData::Version );
//static unsigned    _data_size = sizeof(BMData);

//#define DBUG

BeamMonitorServer::BeamMonitorServer(const char*         pvbase,
                                     const Pds::DetInfo& info) :
  _config_pvs (NCHANNELS),
  _enabled    (false),
  _chan_mask  (0)
{
  _xtc = Xtc(_data_type, info);

  //
  //  Create PvServers for fetching configuration data
  //
  char pvname[64];
  sprintf(pvname,"%s:ChanEnable",pvbase);
  _chan_enable = new PvServer(pvname);

  for(unsigned i=0; i<16; i++) {
    sprintf(pvname,"%s:NumberOfSamples%d_RBV",pvbase,i);
    printf("Creating EpicsCA(%s)\n",pvname);
    _config_pvs[i] = new PvServer(pvname);
  }

  //
  //  Create EpicsCA for monitoring event data
  //
  sprintf(pvname,"%s:RAW",pvbase);
  _raw = new Pds_Epics::EpicsCA(pvname,this);
}

BeamMonitorServer::~BeamMonitorServer() 
{
  delete _raw;
  delete _chan_enable;
  for(unsigned i=0; i<_config_pvs.size(); i++)
    delete _config_pvs[i];
}

//
//  Called by "post" function to write event data into the XTC
//
int BeamMonitorServer::fill(char* copyTo, const void* data)
{
  //
  //  Set the xtc header
  //
  char* p = copyTo;
  Xtc& xtc = *new (p) Xtc(_xtc);
  p += sizeof(Xtc);

  //
  //  Parse the raw data and fill the copyTo payload
  //
  unsigned fiducial = _raw->nsec()&0x1ffff;
  _seq = Pds::Sequence(Pds::Sequence::Event,
                       Pds::TransitionId::L1Accept,
                       Pds::ClockTime(0,0),
                       Pds::TimeStamp(0,fiducial,0));

  uint32_t* p32 = reinterpret_cast<uint32_t*>(p);

  dbr_double_t* inp = (dbr_double_t*)data;

#ifdef DBUG
  for(unsigned i=0; i<24; i++)
    printf("%x ",unsigned(inp[i]));
  printf("\n");
#endif

  inp += 14;  // skip the header

  // 16-bit samples
  if (_len_125) {
    for(unsigned i=0; i<8; i++) {
      if ((_chan_mask&&(1<<i))==0) continue;
#ifdef DBUG
      printf("fill chan[%d] id %x\n",i,unsigned(*inp));
#endif
      inp++;
      unsigned j=0;
      while(j<_rawlen[i]) {
        *p32++ = *inp++;
        j     += 2;
      }
      while(j<_len_125) {
        *p32++ = 0;
        j     += 2;
      }
    }
  }

  // 32-bit samples
  if (_len_5) {
    for(unsigned i=8; i<16; i++) {
      if ((_chan_mask&&(1<<i))==0) continue;
#ifdef DBUG
      printf("fill chan[%d] id %x\n",i,unsigned(*inp));
#endif
      inp++;
      unsigned j=0;
      while(j<_rawlen[i]) {
        *p32++ = *inp++;
        j++;
      }
      while(j<_len_5) {
        *p32++ = 0;
        j++;
      }
    }
  }
  
  int len = reinterpret_cast<char*>(p32)-copyTo;
  _xtc.extent = xtc.extent = len;
#ifdef DBUG
  printf("fill extent %d (0x%x)\n",len,len);
#endif
  return len;
}

#include "pds/service/GenericPool.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"
static Pds::GenericPool pool(0x100000,4);

//
//  Callback when new event data is available
//
void BeamMonitorServer::updated()
{
#ifdef DBUG
  if (!_enabled) {
    InDatagram* dg = new (&pool) CDatagram(Datagram(Transition(Pds::TransitionId::Configure,
                                                               Pds::Env(0)),
                                                    _xtcType,
                                                    ProcInfo(Pds::Level::Segment,0,0)));
    fire(dg);
    delete dg;
    _enabled = true;
  }
  else {
    char* p = new char[0x100000];
    fill(p,_raw->data());
    delete[] p;
  }
  return;
#endif
  if (_enabled) {
    post(_raw->data());
  }
}

Pds::Transition* BeamMonitorServer::fire(Pds::Transition* tr)
{
  switch (tr->id()) {
  case Pds::TransitionId::Enable:
    _enabled=true; break;
  case Pds::TransitionId::Disable:
    _enabled=false; break;
  case Pds::TransitionId::L1Accept:
  default:
    break;
  }
  return tr;
}

Pds::InDatagram* BeamMonitorServer::fire(Pds::InDatagram* dg)
{
  if (dg->seq.service()==Pds::TransitionId::Configure) {
    //
    //  Fetch the configuration data for inserting into the XTC
    //
    _chan_enable  ->fetch((char*)&_chan_mask,4);
    for(unsigned i=0; i<NCHANNELS; i++) {
      _config_pvs[i]->fetch((char*)&_rawlen[i],4);
      printf("_rawlen[%d]=%d\n",i,_rawlen[i]);
    }

    _len_125=0;
    _len_5  =0;
    for(unsigned i=0; i<8; i++)
      if (_rawlen[i]>_len_125) _len_125=_rawlen[i];
    for(unsigned i=8; i<16; i++)
      if (_rawlen[i]>_len_5  ) _len_5  =_rawlen[i];

    if ((_chan_mask&0x00ff)==0) _len_125=0;
    if ((_chan_mask&0xff00)==0) _len_5  =0;

    _xtc.extent = sizeof(Pds::Xtc) + 8*_len_125*sizeof(uint16_t) + 8*_len_5*sizeof(uint32_t);
    
    Pds::Xtc* xtc = new ((char*)dg->xtc.next())
      Pds::Xtc( _config_type, _xtc.src );
    new (xtc->alloc(_config_size)) BMConfig(_len_125, _len_5);
    dg->xtc.alloc(xtc->extent);

    printf("BeamMonitorServer::fire len_125,5 = (%u,%u),  chan_mask (0x%x) \n", _len_125, _len_5, _chan_mask);
  }
  return dg;
}
