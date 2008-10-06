#ifndef PDSCOLLECTIONMANAGER_HH
#define PDSCOLLECTIONMANAGER_HH

#include "CollectionServerManager.hh"

#include "pds/service/Semaphore.hh"

namespace Pds {
class CollectionManager: public CollectionServerManager {
public:
  CollectionManager(Level::Type level,
                    unsigned platform,
                    unsigned maxpayload,
                    unsigned timeout,
                    Arp* arp=0);
  virtual ~CollectionManager();

public:
  bool connect(int interface=0);

private:
  // Implements Server from ServerManager
  virtual int commit(char* datagram, char* load, int size, const Ins& src);

  // Implements ServerManager
  virtual int processTmo();

private:
  Semaphore _sem;
};

}
#endif
