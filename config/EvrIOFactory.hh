#ifndef Pds_EvrIOFactory_hh
#define Pds_EvrIOFactory_hh

#include "pds/config/EvrIOConfigType.hh"
#include "pds/config/PartitionConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <map>
#include <list>

namespace Pds {
  class AliasFactory;
  class EvrIOFactory {
  public:
    EvrIOFactory();
    ~EvrIOFactory();
  public:
    void insert(unsigned mod_id, unsigned chan, const DetInfo& info);
    EvrIOConfigType* config(const AliasFactory& aliases,
			    const PartitionConfigType* =0) const;
  private:
    typedef std::map< unsigned,std::list<DetInfo> > MapType;
    MapType _map;
  };
};

#endif
