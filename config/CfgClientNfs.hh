#ifndef Pds_CfgClientNfs_hh
#define Pds_CfgClientNfs_hh

#include "pdsdata/xtc/Src.hh"

#include <string>
#include <map>

namespace Pds_ConfigDb { class XtcClient; };

namespace Pds {

  class Allocation;
  class Transition;
  class TypeId;

  class CfgClientNfs {
  public:
    CfgClientNfs( const Src& src );
    virtual ~CfgClientNfs();

    const Src& src() const;

    void initialize(const Allocation&);

    virtual int fetch(const Transition& tr, 
                      const TypeId&     id, 
                      void*             dst,
                      unsigned          maxSize=0x100000);
    
  private:
    Src _src;
    Pds_ConfigDb::XtcClient* _db;
    std::string              _path;
    unsigned                 _key;
  public:
    class CacheEntry {
    public:
      CacheEntry() : size(0), p(0) {}
      int   size;
      char* p;
    };
  private:
    std::map<unsigned,CacheEntry> _cache;
  };

};

#endif

    
