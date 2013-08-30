#include "pds/ipimb/LusiDiagFex.hh"

#include "pds/xtc/CDatagram.hh"

#include "pds/config/IpmFexConfigType.hh"
#include "pds/config/DiodeFexConfigType.hh"
#include "pds/config/CfgClientNfs.hh"

#include "pdsdata/psddl/lusi.ddl.h"
#include "pdsdata/psddl/ipimb.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <new>

namespace Pds {
  class IpimbCapSetting {
  public:
    int ndiodes;
    int cap[4];
  };
};

using namespace Pds;

static const int OutSize = 0x800;
static const int OutEntries = 32;
static const int max_configs=32;

#define IS_IPM(det) (_cap_config[det].ndiodes >1)
#define IS_PIM(det) (_cap_config[det].ndiodes==1)
#define SET_IPM(det) (_cap_config[det].ndiodes=4)
#define SET_PIM(det) (_cap_config[det].ndiodes=1)

static inline unsigned ipmIndex(unsigned det) { return det; }
static inline unsigned pimIndex(unsigned det) { return det; }

typedef Pds::Lusi::IpmFexV1 IpmFexType;
static  Pds::TypeId _ipmFexType(Pds::TypeId::Id_IpmFex, IpmFexType::Version);

typedef Pds::Lusi::DiodeFexV1 DiodeFexType;
static  Pds::TypeId _diodeFexType(Pds::TypeId::Id_DiodeFex, DiodeFexType::Version);

typedef Pds::Ipimb::DataV2 IpimbDataType;

LusiDiagFex::LusiDiagFex() : 
  _pool(OutSize,OutEntries), 
  _ipm_config(new IpmFexConfigType  [max_configs]),
  _pim_config(new DiodeFexConfigType[max_configs]),
  _cap_config(new IpimbCapSetting   [max_configs]),
  _odg(0) {}

LusiDiagFex::~LusiDiagFex() 
{
  delete[] _pim_config;
  delete[] _ipm_config;
  delete[] _cap_config;
}

void LusiDiagFex::reset() { _map.clear(); }

InDatagram* LusiDiagFex::process(InDatagram* in)
{
  const Datagram& dg = in->datagram();
  _odg = new (&_pool)CDatagram(dg);
  iterate(const_cast<Xtc*>(&dg.xtc));
  return _odg;
}

int LusiDiagFex::process(Xtc* xtc)
{
  if (static_cast<const DetInfo&>(xtc->src).device()!=DetInfo::Ipimb) {
    _odg->insert(*xtc, xtc->payload());
	return 1; }

  unsigned det=0;
  while(_map[det] != xtc->src.phy()) det++;
  
  if (IS_IPM(det)) {
    // keep a copy of the raw data
    _odg->insert(*xtc, xtc->payload());

    Xtc& tc = *new (&_odg->xtc) Xtc(_ipmFexType, xtc->src);
    tc.extent += sizeof(IpmFexType);

    const IpmFexConfigType& cfg  = _ipm_config[det];
    const IpimbDataType& data = *reinterpret_cast<const IpimbDataType*>(xtc->payload());
    float fex_channel[4];
    int s; // need to fetch the cap selection from the IPIMB configuration for each channel
    s=_cap_config[det].cap[0]; fex_channel[0] = (cfg.diode()[0].base()[s] - data.channel0Volts())*cfg.diode()[0].scale()[s];
    s=_cap_config[det].cap[1]; fex_channel[1] = (cfg.diode()[1].base()[s] - data.channel1Volts())*cfg.diode()[1].scale()[s];
    s=_cap_config[det].cap[2]; fex_channel[2] = (cfg.diode()[2].base()[s] - data.channel2Volts())*cfg.diode()[2].scale()[s];
    s=_cap_config[det].cap[3]; fex_channel[3] = (cfg.diode()[3].base()[s] - data.channel3Volts())*cfg.diode()[3].scale()[s];
    float fex_sum = fex_channel[0] + fex_channel[1] + fex_channel[2] + fex_channel[3];
    float fex_xpos = cfg.xscale()*(fex_channel[1] - fex_channel[3])/(fex_channel[1] + fex_channel[3]);
    float fex_ypos = cfg.yscale()*(fex_channel[0] - fex_channel[2])/(fex_channel[0] + fex_channel[2]);
    *new (_odg->xtc.alloc(sizeof(IpmFexType))) IpmFexType(fex_channel, fex_sum, fex_xpos, fex_ypos);
  }
  else if (IS_PIM(det)) {
    // keep a copy of the raw data
    _odg->insert(*xtc, xtc->payload());

    Xtc& tc = *new (&_odg->xtc) Xtc(_diodeFexType, xtc->src);
    tc.extent += sizeof(DiodeFexType);

    const DiodeFexConfigType& cfg  = _pim_config[det];
    const IpimbDataType& data = *reinterpret_cast<const IpimbDataType*>(xtc->payload());
    int s=_cap_config[det].cap[0]; // need to fetch the cap selection from the IPIMB configuration for each channel
    float fex_value = (cfg.base()[s] - data.channel0Volts())*cfg.scale()[s];
    *new (_odg->xtc.alloc(sizeof(DiodeFexType))) DiodeFexType(fex_value);
  }
  else {
    // keep a copy of the raw data
    _odg->insert(*xtc, xtc->payload());
  }
  return 1;
}

bool LusiDiagFex::configure(CfgClientNfs& cfg,
			    Transition& tr,
			    const IpimbConfigType& ipimb_config)
{
  unsigned det = _map.size();
  _map.push_back(cfg.src().phy());

  if ( cfg.fetch(tr, _ipmFexConfigType, 
		 &_ipm_config[ipmIndex(det)], sizeof(IpmFexConfigType)) > 0 ) {
    SET_IPM(det);
    _cap_config[det].cap[0] = (ipimb_config.chargeAmpRange()>> 0)&0xf;
    _cap_config[det].cap[1] = (ipimb_config.chargeAmpRange()>> 4)&0xf;
    _cap_config[det].cap[2] = (ipimb_config.chargeAmpRange()>> 8)&0xf;
    _cap_config[det].cap[3] = (ipimb_config.chargeAmpRange()>>12)&0xf;
  }
  else if ( cfg.fetch(tr, _diodeFexConfigType, 
		      &_pim_config[pimIndex(det)], sizeof(DiodeFexConfigType)) > 0 ) {
    SET_PIM(det);
    _cap_config[det].cap[0] = (ipimb_config.chargeAmpRange()>>0)&0xf;
  }
  return true;
}

void LusiDiagFex::recordConfigure(InDatagram* dg, const Src& src)
{
  unsigned det=0;
  while(_map[det] != src.phy()) det++;

  if (IS_IPM(det)) {
    Xtc tc = Xtc(_ipmFexConfigType, src);
    tc.extent += sizeof(IpmFexConfigType);
    dg->insert(tc, &_ipm_config[det]);
  }
  else if (IS_PIM(det)) {
    Xtc tc = Xtc(_diodeFexConfigType, src);
    tc.extent += sizeof(DiodeFexConfigType);
    dg->insert(tc, &_pim_config[det]);
  }
}
