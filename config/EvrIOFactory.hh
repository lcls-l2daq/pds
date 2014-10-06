#ifndef Pds_EvrIOFactory_hh
#define Pds_EvrIOFactory_hh

#include "pds/config/EvrIOConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <map>
#include <vector>

namespace Pds {
  class AliasFactory;
  class EvrIOFactory {
  public:
    EvrIOFactory();
    ~EvrIOFactory();
  public:
    void insert(unsigned mod_id, unsigned chan, const DetInfo& info);
    EvrIOConfigType* config(const AliasFactory& aliases);
  private:
    typedef std::map< unsigned,std::vector<DetInfo> > MapType;
    MapType _map;
  };
};

#endif
