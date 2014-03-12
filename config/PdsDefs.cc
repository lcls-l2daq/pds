#include "pds/config/PdsDefs.hh"

#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/QuartzConfigType.hh"
#include "pds/config/FccdConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/EvsConfigType.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/config/EvrIOConfigType.hh"
#include "pds/config/ControlConfigType.hh"
#include "pds/config/AcqConfigType.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/config/PrincetonConfigType.hh"
#include "pds/config/IpimbConfigType.hh"
#include "pds/config/IpmFexConfigType.hh"
#include "pds/config/DiodeFexConfigType.hh"
#include "pds/config/PimImageConfigType.hh"
#include "pds/config/EncoderConfigType.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/config/ImpConfigType.hh"
#include "pds/config/Gsc16aiConfigType.hh"
#include "pds/config/TimepixConfigType.hh"
#include "pds/config/CsPad2x2ConfigType.hh"
#include "pds/config/OceanOpticsConfigType.hh"
#include "pds/config/FliConfigType.hh"
#include "pds/config/AndorConfigType.hh"
#include "pds/config/PimaxConfigType.hh"
#include "pds/config/UsdUsbConfigType.hh"
#include "pds/config/OrcaConfigType.hh"
#include "pds/config/RayonixConfigType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/config/EpixConfigType.hh"
//#include "pds/config/ProjectionConfigType.hh"
//#include "pds/config/SeqConfigType.hh"
#include <cassert>

#ifdef BUILD_EXTRA
#include "pds/config/XampsConfigType.hh"
#include "pds/config/FexampConfigType.hh"
#include "pds/config/PhasicsConfigType.hh"
#endif

#include <stdio.h>
#include <sstream>
using std::istringstream;
using std::ostringstream;

using namespace Pds_ConfigDb;

//
//  Define database-only types
//
//static Pds::TypeId       _eventCodeSetType(Pds::TypeId::Any, 1);
//static const std::string _eventCodeSetName("EventCodes");

bool operator==(const Pds::TypeId& a, const Pds::TypeId& b)
{ return a.value()==b.value(); }

PdsDefs::ConfigType PdsDefs::configType(const Pds::TypeId& id)
{
  unsigned i=0;
  do {
    if (id.value() == typeId(ConfigType(i))->value() )
      break;
  } while(++i < NumberOf);
  return ConfigType(i);
}

const Pds::TypeId* PdsDefs::typeId(ConfigType id)
{ 
  Pds::TypeId* type(0);
  switch(id) {
  case Encoder      : type = &_encoderConfigType;   break;
  case Evr          : type = &_evrConfigType;       break;
  case EvrIO        : type = &_evrIOConfigType;     break;
  case Evs          : type = &_evsConfigType;       break;
    //  case Sequencer    : type = &_seqConfigType;       break;
  case AcqADC       : type = &_acqConfigType;       break;
  case AcqTDC       : type = &_acqTdcConfigType;    break;
  case Opal1k       : type = &_opal1kConfigType;    break;
  case Quartz       : type = &_quartzConfigType;    break;
  case Fccd         : type = &_fccdConfigType;      break;
  case TM6740       : type = &_tm6740ConfigType;    break;
  case FrameFex     : type = &_frameFexConfigType;  break;
  case pnCCD        : type = &_pnCCDConfigType;     break;
  case Princeton    : type = &_princetonConfigType; break;
  case Ipimb        : type = &_ipimbConfigType;     break;
  case IpmDiode     : type = &_ipmFexConfigType;    break;
  case PimDiode     : type = &_diodeFexConfigType;  break;
  case PimImage     : type = &_pimImageConfigType;  break;
  case RunControl   : type = &_controlConfigType;   break;
  case Cspad        : type = &_CsPadConfigType;     break;
#ifdef BUILD_EXTRA
  case Xamps        : type = &_XampsConfigType;     break;
  case Fexamp       : type = &_FexampConfigType;    break;
  case Phasics      : type = &_PhasicsConfigType;   break;
#endif
  case Imp          : type = &_ImpConfigType;       break;
  case Gsc16ai      : type = &_gsc16aiConfigType;   break;
  case Timepix      : type = &_timepixConfigType;   break;
  case Cspad2x2     : type = &_CsPad2x2ConfigType;  break;
  case OceanOptics  : type = &_oceanOpticsConfigType; break;
  case Fli          : type = &_fliConfigType; break;
  case Andor        : type = &_andorConfigType; break;
  case Pimax        : type = &_pimaxConfigType; break;
  case UsdUsb       : type = &_usdusbConfigType; break;
  case Orca         : type = &_orcaConfigType; break;
  case Rayonix      : type = &_rayonixConfigType;   break;
  case EpixSampler  : type = &_epixSamplerConfigType;   break;
  case Epix         : type = &_epixConfigType;   break;
    //  case Projection   : type = &_projectionConfigType; break;
  default: 
    printf("PdsDefs::typeId id %d not found\n",unsigned(id));
    break;
  }
  return type;
}

const Pds::TypeId* PdsDefs::typeId(const UTypeName& name)
{
#define test(type) { if (name==Pds::TypeId::name(type.id())) return &type; }
  test(_evrConfigType);
  test(_evrIOConfigType);
  test(_evsConfigType);
  //  test(_seqConfigType);
  test(_acqConfigType);
  test(_acqTdcConfigType);
  test(_ipimbConfigType);
  test(_ipmFexConfigType);
  test(_diodeFexConfigType);
  test(_pimImageConfigType);
  test(_encoderConfigType);
  test(_opal1kConfigType);
  test(_quartzConfigType);
  test(_fccdConfigType);
  test(_tm6740ConfigType);
  test(_pnCCDConfigType);
  test(_frameFexConfigType);
  test(_controlConfigType);
  test(_princetonConfigType);    
  test(_CsPadConfigType);
#ifdef BUILD_EXTRA
  test(_XampsConfigType);
  test(_FexampConfigType);
  test(_PhasicsConfigType);
#endif
  test(_ImpConfigType);
  test(_gsc16aiConfigType);
  test(_timepixConfigType);
  test(_CsPad2x2ConfigType);
  test(_oceanOpticsConfigType);    
  test(_fliConfigType);    
  test(_andorConfigType);    
  test(_pimaxConfigType);    
  test(_usdusbConfigType);    
  test(_orcaConfigType);   
  test(_rayonixConfigType); 
  test(_epixSamplerConfigType);
  test(_epixConfigType);
  //  test(_projectionConfigType);    
#undef test
  //  database-only types
  //  if (name==_eventCodeSetName) return &_eventCodeSetType;

  //  printf("PdsDefs::typeId id %s not found\n",name.data());
  return 0;
}

const Pds::TypeId* PdsDefs::typeId(const QTypeName& name)
{
#define test(type) { if (name==PdsDefs::qtypeName(type)) return &type; }
  test(_evrConfigType);
  test(_evrIOConfigType);
  test(_evsConfigType);
  //  test(_seqConfigType);
  test(_acqConfigType);
  test(_acqTdcConfigType);
  test(_ipimbConfigType);
  test(_ipmFexConfigType);
  test(_diodeFexConfigType);
  test(_pimImageConfigType);
  test(_encoderConfigType);
  test(_opal1kConfigType);
  test(_quartzConfigType);
  test(_fccdConfigType);
  test(_tm6740ConfigType);
  test(_pnCCDConfigType);
  test(_frameFexConfigType);
  test(_controlConfigType);
  test(_princetonConfigType);
  test(_CsPadConfigType);
#ifdef BUILD_EXTRA
  test(_XampsConfigType);
  test(_FexampConfigType);
  test(_PhasicsConfigType);
#endif
  test(_ImpConfigType);
  test(_gsc16aiConfigType);
  test(_timepixConfigType);
  test(_CsPad2x2ConfigType);
  test(_oceanOpticsConfigType);    
  test(_fliConfigType);    
  test(_andorConfigType);  
  test(_pimaxConfigType);      
  test(_usdusbConfigType); 
  test(_epixSamplerConfigType);   
  test(_epixConfigType);
  //  test(_projectionConfigType);    
#undef test
  //  database-only types
  //  if (name==_eventCodeSetName) return &_eventCodeSetType;

  printf("PdsDefs::typeId no match for %s\n",name.c_str());

  return 0;
}

UTypeName PdsDefs::utypeName(ConfigType type)
{
  return PdsDefs::utypeName(*PdsDefs::typeId(type));
}

UTypeName PdsDefs::utypeName(const Pds::TypeId& type)
{
  //  database-only types
  //  if (type==_eventCodeSetType) return UTypeName(_eventCodeSetName);

  ostringstream o;
  o << Pds::TypeId::name(type.id());  
  return UTypeName(o.str());
}

QTypeName PdsDefs::qtypeName(const Pds::TypeId& type)
{
  //  database-only types
  //  if (type==_eventCodeSetType) return QTypeName(_eventCodeSetName);

  ostringstream o;
  o << Pds::TypeId::name(type.id()) << "_v" << type.version();
  return QTypeName(o.str());
}

QTypeName PdsDefs::qtypeName(const UTypeName& utype)
{
  const Pds::TypeId* type_id = PdsDefs::typeId(utype);
  assert(type_id != 0);
  return PdsDefs::qtypeName(*type_id);
}
