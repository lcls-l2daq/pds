#ifndef PDS_EBKGROUP_HH
#define PDS_EBKGROUP_HH

#include <vector>
#include "EbK.hh"

namespace Pds
{

class EbKGroup : public EbK
{
public:
  EbKGroup(const Src& id,
    const TypeId& ctns,
    Level::Type level,
    Inlet& inlet,
    OutletWire& outlet,
    int stream,
    int ipaddress,
    unsigned eventsize,
    unsigned eventpooldepth,
    int slowEb,
    VmonEb* vmoneb=0);
  ~EbKGroup();

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
