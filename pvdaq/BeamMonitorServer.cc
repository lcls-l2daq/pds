#include "pds/pvdaq/BeamMonitorServer.hh"
#include "pds/pvdaq/PvServer.hh"
#include "pds/xtc/InDatagram.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/generic1d.ddl.h"

using namespace Pds::PvDaq;
typedef Pds::Generic1D::ConfigV0 G1DCfg;
//
//  Things that belong in pds/config/BeamMonitorType.hh
//
static const int NCHANNELS = BeamMonitorServer::NCHANNELS;
static const uint32_t _SampleType[NCHANNELS]= {G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16, G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32};
static const double _Period[NCHANNELS]= {8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7};
typedef Pds::Generic1D::ConfigV0 G1DConfig;


static Pds::TypeId _config_type( Pds::TypeId::Type(G1DConfig::TypeId),
                                 G1DConfig::Version );

typedef Pds::Generic1D::DataV0 G1DData;

static Pds::TypeId _data_type( Pds::TypeId::Type(G1DData::TypeId),
                               G1DData::Version );
//static unsigned    _data_size = sizeof(BMData);

//#define DBUG

BeamMonitorServer::BeamMonitorServer(const char*         pvbase,
                                     const Pds::DetInfo& info) :
  _config_pvs (NCHANNELS),
  _offset_pvs (NCHANNELS),
  _enabled    (false),
  _chan_mask  (0),
  _ConfigBuff (0)
{
  _xtc = Xtc(_data_type, info);



  //
  //  Create PvServers for fetching configuration data
  //
  char pvname[64];
  sprintf(pvname,"%s:ChanEnable",pvbase);
  _chan_enable = new PvServer(pvname);

  for(unsigned i=0; i<NCHANNELS; i++) {
    sprintf(pvname,"%s:NumberOfSamples%d_RBV",pvbase, i);
    printf("Creating EpicsCA(%s)\n",pvname);
    _config_pvs[i] = new PvServer(pvname);

    sprintf(pvname,"%s:Delay%d_RBV",pvbase, i);
    printf("Creating EpicsCA(%s)\n",pvname);
    _offset_pvs[i] = new PvServer(pvname);
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
  for(unsigned i=0; i<_offset_pvs.size(); i++)
    delete _offset_pvs[i];
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
  unsigned size_calc = 0;
  for(unsigned n=0; n<NCHANNELS; n++){
    if (_SampleType[n] == G1DCfg::UINT16){
      size_calc = size_calc + _Length[n]*2;
    }
    if (_SampleType[n] == G1DCfg::UINT32){
      size_calc = size_calc + _Length[n]*4;
    }
  }
  G1DData* dataptr = new (xtc.alloc(size_calc)) G1DData;
  *reinterpret_cast<uint32_t*>(dataptr)= size_calc;
  uint32_t* p32 = reinterpret_cast<uint32_t*>(dataptr)+1;
  dbr_double_t* inp = (dbr_double_t*)data;
#ifdef DBUG					
  for(unsigned i=0; i<24; i++) 
  printf("%x ",unsigned(inp[i]));
  printf("\n");
#endif
  inp +=14; //skip event header
  for(unsigned i=0; i<NCHANNELS/2; i++) {
    if ((_chan_mask&&(1<<i))==0) continue;
    inp ++; //skip channel header
    for(unsigned j=0; j<_Length[i]/2; j++) {
      *p32++ = *inp++;
    }
  }
  for(unsigned i=NCHANNELS/2; i<NCHANNELS; i++) {
    if ((_chan_mask&&(1<<i))==0) continue;
    inp ++; //skip channel header
    for(unsigned j=0; j<_Length[i]; j++) {
      *p32++ = *inp++;
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
    printf("Channel Mask %x\n", _chan_mask);
    for(unsigned i=0; i<NCHANNELS; i++) {
      _config_pvs[i]->fetch((char*)&_Length[i],4);
      _offset_pvs[i]->fetch((char*)&_Offset[i],4);
      printf("_Length[%d]=%d\n",i,_Length[i]);
      printf("_Offset[%d]=%d\n",i,_Offset[i]);
    }

    //_xtc.extent = sizeof(Pds::Xtc) + 8*_len_125*sizeof(uint16_t) + 8*_len_5*sizeof(uint32_t);

    int config_size = G1DConfig(NCHANNELS, 0, 0, 0, 0)._sizeof();
    Pds::Xtc* xtc = new ((char*)dg->xtc.next())
      Pds::Xtc( _config_type, _xtc.src );
    new (xtc->alloc(config_size)) G1DConfig(NCHANNELS, _Length, _SampleType, _Offset, _Period);
    dg->xtc.alloc(xtc->extent);
    if (_ConfigBuff) {
      delete _ConfigBuff;
    }
    _ConfigBuff = new char[config_size];
    new (_ConfigBuff) G1DConfig(NCHANNELS, _Length, _SampleType, _Offset, _Period);
  }
  return dg;
}
