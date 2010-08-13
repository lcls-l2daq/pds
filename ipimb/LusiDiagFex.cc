#include "pds/ipimb/LusiDiagFex.hh"

#include "pds/xtc/CDatagram.hh"

#include "pds/config/IpmFexConfigType.hh"
#include "pds/config/DiodeFexConfigType.hh"
#include "pds/config/CfgClientNfs.hh"

#include "pdsdata/lusi/IpmFexConfigV1.hh"
#include "pdsdata/lusi/IpmFexV1.hh"
#include "pdsdata/lusi/DiodeFexV1.hh"
#include "pdsdata/ipimb/DataV1.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <new>

using namespace Pds;

const int OutSize = 0x400;
const int OutEntries = 16;

static const unsigned ipmMask = ( 1 << Pds::DetInfo::XppSb1Ipm) |
			        ( 1 << Pds::DetInfo::XppSb2Ipm) |
				( 1 << Pds::DetInfo::XppSb3Ipm);

static const unsigned pimMask = ( 1 << Pds::DetInfo::XppSb1Pim) |
				( 1 << Pds::DetInfo::XppMonPim) |
				( 1 << Pds::DetInfo::XppSb3Pim) |
				( 1 << Pds::DetInfo::XppSb4Pim);

#define IS_IPM(det) ((1<<det) & ipmMask)
#define IS_PIM(det) ((1<<det) & pimMask)

static inline unsigned ipmIndex(DetInfo::Detector det) { return det; }
static inline unsigned ipmIndexBound() { return DetInfo::NumDetector; }

static inline unsigned pimIndex(DetInfo::Detector det) { return det; }
static inline unsigned pimIndexBound() { return DetInfo::NumDetector; }

typedef Pds::Lusi::IpmFexV1 IpmFexType;
static  Pds::TypeId _ipmFexType(Pds::TypeId::Id_IpmFex, IpmFexType::Version);

typedef Pds::Lusi::DiodeFexV1 DiodeFexType;
static  Pds::TypeId _diodeFexType(Pds::TypeId::Id_DiodeFex, DiodeFexType::Version);

typedef Pds::Ipimb::DataV1 IpimbDataType;

LusiDiagFex::LusiDiagFex() : 
  _pool(OutSize,OutEntries), 
  _ipm_config(new IpmFexConfigType[ipmIndexBound()]),
  _pim_config(new DiodeFexConfigType[pimIndexBound()]),
  _odg(0) {}

LusiDiagFex::~LusiDiagFex() 
{
  delete[] _pim_config;
  delete[] _ipm_config;
}

InDatagram* LusiDiagFex::process(InDatagram* in)
{
  const Datagram& dg = in->datagram();
  _odg = new (&_pool)CDatagram(dg);
  iterate(const_cast<Xtc*>(&dg.xtc));
  return _odg;
}

int LusiDiagFex::process(Xtc* xtc)
{
  DetInfo::Detector det = static_cast<const DetInfo&>(xtc->src).detector();
  if (IS_IPM(det)) {
    // keep a copy of the raw data
    _odg->insert(*xtc, xtc->payload());

    Xtc& tc = *new (&_odg->xtc) Xtc(_ipmFexType, xtc->src);
    tc.extent += sizeof(IpmFexType);

    const IpmFexConfigType& cfg  = _ipm_config[det];
    const IpimbDataType& data = *reinterpret_cast<const IpimbDataType*>(xtc->payload());
    IpmFexType& fex = *new (_odg->xtc.alloc(sizeof(IpmFexType))) IpmFexType;
    int s; // need to fetch the cap selection from the IPIMB configuration for each channel
    s=_cap[0]; fex.channel[0] = (cfg.diode[0].base[s] - data.channel0Volts())*cfg.diode[0].scale[s];
    s=_cap[1]; fex.channel[1] = (cfg.diode[1].base[s] - data.channel1Volts())*cfg.diode[1].scale[s];
    s=_cap[2]; fex.channel[2] = (cfg.diode[2].base[s] - data.channel2Volts())*cfg.diode[2].scale[s];
    s=_cap[3]; fex.channel[3] = (cfg.diode[3].base[s] - data.channel3Volts())*cfg.diode[3].scale[s];
    fex.sum = fex.channel[0] + fex.channel[1] + fex.channel[2] + fex.channel[3];
    fex.xpos = cfg.yscale*(fex.channel[1] - fex.channel[3])/(fex.channel[1] + fex.channel[3]);
    fex.ypos = cfg.xscale*(fex.channel[0] - fex.channel[2])/(fex.channel[0] + fex.channel[2]);
  }
  else if (IS_PIM(det)) {
    // keep a copy of the raw data
    _odg->insert(*xtc, xtc->payload());

    Xtc& tc = *new (&_odg->xtc) Xtc(_diodeFexType, xtc->src);
    tc.extent += sizeof(DiodeFexType);

    const DiodeFexConfigType& cfg  = _pim_config[det];
    const IpimbDataType& data = *reinterpret_cast<const IpimbDataType*>(xtc->payload());
    DiodeFexType& fex = *new (_odg->xtc.alloc(sizeof(DiodeFexType))) DiodeFexType;
    int s=_cap[0]; // need to fetch the cap selection from the IPIMB configuration for each channel
    fex.value = (cfg.base[s] - data.channel0Volts())*cfg.scale[s];
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
  DetInfo::Detector det = static_cast<const DetInfo&>(cfg.src()).detector();
  if (IS_IPM(det)) {
    if ( cfg.fetch(tr, _ipmFexConfigType, 
		   &_ipm_config[ipmIndex(det)], sizeof(IpmFexConfigType)) <= 0 )
      return false;
    _cap[0] = (ipimb_config.chargeAmpRange()>>0)&0x3;
    _cap[1] = (ipimb_config.chargeAmpRange()>>2)&0x3;
    _cap[2] = (ipimb_config.chargeAmpRange()>>4)&0x3;
    _cap[3] = (ipimb_config.chargeAmpRange()>>6)&0x3;
  }
  else if (IS_PIM(det)) {
    if ( cfg.fetch(tr, _diodeFexConfigType, 
		   &_pim_config[pimIndex(det)], sizeof(DiodeFexConfigType)) <= 0 )
      return false;
    _cap[0] = (ipimb_config.chargeAmpRange()>>0)&0x3;
  }
  return true;
}

void LusiDiagFex::recordConfigure(InDatagram* dg, const Src& src)
{
  DetInfo::Detector det = static_cast<const DetInfo&>(src).detector();
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
