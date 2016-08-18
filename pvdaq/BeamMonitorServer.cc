#include "pds/pvdaq/BeamMonitorServer.hh"
#include "pds/pvdaq/PvServer.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/utility/Occurrence.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/generic1d.ddl.h"
#include <stdio.h>
#include <sstream>

//
//  To do:
//    Check format identifier PV for expected value (throw occurrence)
//    Read SW version PV and write to log file
//    Validate raw PV .NORD file; write damage to event
//    Monitor config PVs; if they change while running, throw occurrence
//    Check config status PV for successful configuration
//    Hide EPICS_CA* env's within executable
//    Check connection status on all PVs
//    Check out-of-sync PV
//    Remove EVR argument from command-line; configure which eventcode to expect data on (PV?)
//    Name each channel (read from PV) (alias, sub-alias?) [hmmm]
//

namespace Pds {
  namespace PvDaq {
    class ConfigMonitor : public Pds_Epics::PVMonitorCb {
    public:
      ConfigMonitor(BeamMonitorServer& o) : _o(o) {}
      ~ConfigMonitor() {}
    public:
      void updated() { _o.config_updated(); }
    private:
      BeamMonitorServer& _o;
    };
  };
};

static const unsigned NPRINT=20;
static const unsigned _API_VERSION=0;

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
  _pvbase     (pvbase),
  _exp_nord   (0),
  _config_pvs (NCHANNELS),
  _offset_pvs (NCHANNELS),
  _enabled    (false),
  _configured (false),
  _chan_mask  (0),
  _ConfigBuff (0),
  _configMonitor(new ConfigMonitor(*this)),
  _nprint     (NPRINT),
  _rdp        (0),
  _wrp        (0),
  _pool       (8)
{
  _xtc = Xtc(_data_type, info);

  char wvbase[32];
  sprintf(wvbase,"WV8:%s",pvbase);

  //
  //  Create PvServers for fetching configuration data
  //
  char pvname[64];
  sprintf(pvname,"%s:ChanEnable",wvbase);
  _chan_enable = new PvServer(pvname);

  for(unsigned i=0; i<NCHANNELS; i++) {
    sprintf(pvname,"%s:NumberOfSamples%d_RBV",wvbase, i);
    printf("Creating EpicsCA(%s)\n",pvname);
    _config_pvs[i] = new PvServer(pvname);

    sprintf(pvname,"%s:Delay%d_RBV",wvbase, i);
    printf("Creating EpicsCA(%s)\n",pvname);
    _offset_pvs[i] = new PvServer(pvname);
  }

  sprintf(pvname,"%s:CFG_DONE",wvbase);
  _cfg_done = new Pds_Epics::EpicsCA(pvname,_configMonitor);
  sprintf(pvname,"%s:CFG_ERRS",wvbase);
  _cfg_errs = new PvServer(pvname);
  sprintf(pvname,"%s:SYNC",wvbase);
  _sync     = new PvServer(pvname);

  //
  //  Create EpicsCA for monitoring event data
  //
  sprintf(pvname,"%s:RAW",wvbase);
  _raw = new Pds_Epics::EpicsCA(pvname,this);
  sprintf(pvname,"%s:RAW.NORD",wvbase);
  _raw_nord = new PvServer(pvname);

  //
  //  Create PVs for validating setup
  //
  sprintf(pvname,"IOC:%s:FORMAT_VERSION",pvbase);
  _api_version = new PvServer(pvname);

  sprintf(pvname,"IOC:%s:VERSION",pvbase);
  _ioc_version = new PvServer(pvname);

  for(unsigned i=0; i<_pool.size(); i++) {
    _pool[i] = new char[0x400000];
    reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                       TimeStamp(0,0,0,0));
  }
}

BeamMonitorServer::~BeamMonitorServer() 
{
  delete _cfg_done;
  delete _cfg_errs;
  delete _sync;
  delete _raw;
  delete _raw_nord;
  delete _chan_enable;
  for(unsigned i=0; i<_config_pvs.size(); i++)
    delete _config_pvs[i];
  for(unsigned i=0; i<_offset_pvs.size(); i++)
    delete _offset_pvs[i];
  delete _api_version;
  delete _ioc_version;
}

//
//  Called by "post" function to write event data into the XTC
//
int  BeamMonitorServer::fill(char* payload, const void* p)
{
  Dgram* dg = reinterpret_cast<Dgram*>(_pool[_rdp]);
  if (_last > dg->seq.clock())
    return -1;

  if (++_rdp == _pool.size()) _rdp=0;

  _last = dg->seq.clock();

  _seq = dg->seq;
  _xtc = dg->xtc;

  memcpy(payload, &dg->xtc, dg->xtc.extent);
  return dg->xtc.extent;
}

void BeamMonitorServer::updated()
{
  if (!_enabled)
    return;

  Dgram* dg = new (_pool[_wrp]) Dgram;

  //
  //  Parse the raw data and fill the copyTo payload
  //
  unsigned fiducial = _raw->nsec()&0x1ffff;

  dg->seq = Pds::Sequence(Pds::Sequence::Event,
                          Pds::TransitionId::L1Accept,
                          Pds::ClockTime(_raw->sec(),_raw->nsec()),
                          Pds::TimeStamp(0,fiducial,0));

  dg->env = _wrp;

  if (++_wrp==_pool.size()) _wrp=0;

  //
  //  Set the xtc header
  //
  dg->xtc = Xtc(_xtc.contains, _xtc.src);

  int len = sizeof(Xtc);

  do {
    // validate raw data
    unsigned nord = unsigned(*reinterpret_cast<double*>(_raw_nord->data()));
    if (nord != _exp_nord) {
      if (_nprint) {
        printf("Incorrect .NORD; read %d, expected %d\n",
               nord, _exp_nord);
        --_nprint;
      }
      dg->xtc.damage.increase(Pds::Damage::UserDefined);
      dg->xtc.damage.userBits(1);
      break;
    }

    // validate raw data
    unsigned sync = *reinterpret_cast<uint32_t*>(_sync->data());
    if (sync != 1) {
      if (_nprint) {
        printf("Sync PV == 0\n");
        --_nprint;
      }
      dg->xtc.damage.increase(Pds::Damage::UserDefined);
      dg->xtc.damage.userBits(2);
      break;
    }

    bool lconnected=true;
    for(unsigned n=0; n<_config_pvs.size(); n++)
      lconnected &= static_cast<Pds_Epics::EpicsCA*>(_config_pvs[n])->connected();
    if (!lconnected) {
      if (_nprint) {
        printf("Some config PVs not connected\n");
        --_nprint;
      }
      dg->xtc.damage.increase(Pds::Damage::UserDefined);
      dg->xtc.damage.userBits(4);
      break;
    }

    unsigned size_calc = 0;
    for(unsigned n=0; n<NCHANNELS; n++){
      if (_SampleType[n] == G1DCfg::UINT16){
        size_calc = size_calc + _Length[n]*2;
      }
      if (_SampleType[n] == G1DCfg::UINT32){
        size_calc = size_calc + _Length[n]*4;
      }
    }
    G1DData* dataptr = new (dg->xtc.alloc(size_calc)) G1DData;
    *reinterpret_cast<uint32_t*>(dataptr)= size_calc;
    uint32_t* p32 = reinterpret_cast<uint32_t*>(dataptr)+1;
    dbr_double_t* inp = (dbr_double_t*)_raw->data();
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
        *p32++ = unsigned(*inp++);
      }
    }
    for(unsigned i=NCHANNELS/2; i<NCHANNELS; i++) {
      if ((_chan_mask&&(1<<i))==0) continue;
      inp ++; //skip channel header
      for(unsigned j=0; j<_Length[i]; j++) {
        *p32++ = unsigned(*inp++);
      }
    }
  
    len = reinterpret_cast<char*>(p32)-reinterpret_cast<char*>(&dg->xtc);
  } while(0);

  dg->xtc.extent = len;

#ifdef DBUG
  printf("fill extent %d (0x%x)\n",len,len);
#endif
  post(dg);
}

#include "pds/service/GenericPool.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"
static Pds::GenericPool pool(0x100000,4);

//
//  Callback when new event data is available
//

Pds::Transition* BeamMonitorServer::fire(Pds::Transition* tr)
{
  switch (tr->id()) {
  case Pds::TransitionId::Configure:
    _nprint=NPRINT;
    _configured=true; break;
  case Pds::TransitionId::Unconfigure:
    _configured=false; break;
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
    _last = ClockTime(0,0);
    _rdp  = 0;
    _wrp  = 0;
    for(unsigned i=0; i<_pool.size(); i++)
      reinterpret_cast<Dgram*>(_pool[i])->seq = Sequence(ClockTime(0,0),
                                                         TimeStamp(0,0,0,0));

    std::string msg;
#if 0
    if (*reinterpret_cast<uint32_t*>(_cfg_errs->data())!=0) {
      msg += "IOC detected configuration errors.";
    }
#endif
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

    _exp_nord = 15;  // event header and trailer word
    for(unsigned i=0; i<NCHANNELS/2; i++) {
      if (_chan_mask & (1<<i)) {
        _exp_nord += 1+_Length[i]/2;
      }
    }
    for(unsigned i=NCHANNELS/2; i<NCHANNELS; i++) {
      if (_chan_mask & (1<<i)) {
        _exp_nord += 1+_Length[i];
      }
    }
    printf("Exp nord %u\n",_exp_nord);

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

    //  Print the IOC software version
    printf("IOC software version is %s\n",reinterpret_cast<char*>(_ioc_version->data()));

    //  Validate the IOC API version
    { unsigned vsn = *reinterpret_cast<uint32_t*>(_api_version->data());
      if (vsn != _API_VERSION) {
        std::ostringstream o;
        o << "Incorrect IOC API version (" << vsn << "!=" << _API_VERSION << ").";
        msg += o.str().c_str();
        printf("%s\n",o.str().c_str());
      }
    }

    if (!msg.empty()) {
        std::ostringstream o;
        o << "Wave8[" << _pvbase << "] config error.";
        Pds::UserMessage* umsg = new (_occPool) Pds::UserMessage(o.str().c_str());
        umsg->append(msg.c_str());
        sendOcc(umsg);
        dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
  }

  return dg;
}

void BeamMonitorServer::config_updated()
{
  if (*reinterpret_cast<uint32_t*>(_cfg_done->data())==1 && _configured) {
    Pds::UserMessage* umsg = new (_occPool)Pds::UserMessage;
    umsg->append("Detected configuration change.  Forcing reconfiguration.");
    sendOcc(umsg);
    Occurrence* occ = new(_occPool) Occurrence(OccurrenceId::ClearReadout);
    sendOcc(occ);
  }
}
