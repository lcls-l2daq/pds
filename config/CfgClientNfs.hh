#ifndef Pds_CfgClientNfs_hh
#define Pds_CfgClientNfs_hh

#include "pdsdata/xtc/Src.hh"

namespace Pds {

  class Allocate;
  class Transition;
  class TypeId;

  class CfgClientNfs {
  public:
    CfgClientNfs( const Src& src );
    ~CfgClientNfs() {}

    const Src& src() const;

    void initialize(const Allocate&);

    int fetch(const Transition& tr, 
	      const TypeId&     id, 
	      char*             dst);

  private:
    enum { PathSize=128 };
    Src      _src;
    char     _path[PathSize];
  };

};

#endif

    
