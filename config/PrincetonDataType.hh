#ifndef Pds_PrincetonDataType_hh
#define Pds_PrincetonDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/princeton/FrameV1.hh"
#include "pdsdata/princeton/InfoV1.hh"

typedef Pds::Princeton::FrameV1 PrincetonDataType;
typedef Pds::Princeton::InfoV1  PrincetonInfoType;

static Pds::TypeId _princetonDataType(Pds::TypeId::Id_PrincetonFrame,
          PrincetonDataType::Version);

static Pds::TypeId _princetonInfoType(Pds::TypeId::Id_PrincetonInfo,
          PrincetonInfoType::Version);
          
#endif
