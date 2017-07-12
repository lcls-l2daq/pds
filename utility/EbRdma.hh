#ifndef Pds_EbRdma_hh
#define Pds_EbRdma_hh

#include <vector>
#include "EbS.hh"

namespace Pds
{

  class EbRdma : public EbBase
  {
  public:
    EbRdma(const Src&    id,
           const TypeId& ctns,
           Level::Type   level,
           Inlet&        inlet,
           OutletWire&   outlet,
           int           stream,
           int           ipaddress,
           char*         pool,
           unsigned      eventsize,
           unsigned      eventpooldepth,
           VmonEb*       vmoneb=0);
    ~EbRdma();

protected:
  virtual EbBase::IsComplete _is_complete( EbEventBase*, const EbBitMask& );

private:
  void                       _insert     ( EbEventBase* );

public:
  void setClientMask(std::vector<EbBitMask>& lGroupClientMask) {_lGroupClientMask = lGroupClientMask;}

private:
  std::vector<EbBitMask> _lGroupClientMask;
};

} // namespace Pds
#endif
