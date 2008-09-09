#ifndef COLLECTION_PORTS_HH
#define COLLECTION_PORTS_HH

#include "pds/service/Ins.hh"

namespace PdsCollectionPorts {
  Pds::Ins platform();
  Pds::Ins collection(unsigned char partition);
}

#endif
