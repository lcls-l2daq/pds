#ifndef COLLECTION_PORTS_HH
#define COLLECTION_PORTS_HH

#include "pds/service/Ins.hh"

namespace Pds {
  namespace CollectionPorts {
    Ins platform  ();
    Ins collection(unsigned char platform);
    Ins tagserver (unsigned char platform);
  }
}

#endif
