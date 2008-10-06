#ifndef PDSCOLLECTIONSOURCE_HH
#define PDSCOLLECTIONSOURCE_HH

#include "CollectionServerManager.hh"

namespace Pds {
class CollectionSource: public CollectionServerManager {
public:
  CollectionSource(unsigned maxpayload, Arp* arp=0);
  virtual ~CollectionSource();

public:
  bool connect(int interface);

private:
  // Implements Server from ServerManager
  virtual int commit(char* datagram, char* load, int size, const Ins& src);

  // Implements ServerManager
  virtual int processTmo();
};

}
#endif
