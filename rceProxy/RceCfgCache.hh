#ifndef Pds_RceCfgCache_hh
#define Pds_RceCfgCache_hh

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/Xtc.hh"

namespace Pds {

  class CfgClientNfs;
  class Allocation;
  class Transition;
  class InDatagram;

  class RceCfgCache 
  {
  public:
    RceCfgCache(CfgClientNfs& cfg,
		const DetInfo::Device dev);
    ~RceCfgCache();
  public:
    const Src& src       () const;
    const Xtc& tc        () const;
    bool       scan      () const;
    Damage&    damage    ();
    void       initialize(const Allocation& alloc);
    int        fetch     (const Transition& tr);
    const void* current() const;
    void next();
    void* allocate();
    void record(InDatagram* dg) const;
  private:
    CfgClientNfs& _cfg;
    Xtc           _cfgtc;
    unsigned      _bsize;
    char*         _buffer;
    char*         _cur_config;
    char*         _end_config;
    bool          _scan;
  };
};

#endif
