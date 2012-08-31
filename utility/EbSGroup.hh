#ifndef PDS_EBSGROUP_HH
#define PDS_EBSGROUP_HH

#include <vector>
#include "EbS.hh"

namespace Pds 
{

class EbSGroup : public EbS
{
public:
  EbSGroup(const Src& id,
    const TypeId& ctns,
    Level::Type level,
    Inlet& inlet,
    OutletWire& outlet,
    int stream,
    int ipaddress,
    unsigned eventsize,
    unsigned eventpooldepth,
    VmonEb* vmoneb=0);
  ~EbSGroup();

protected:
  virtual EbBase::IsComplete _is_complete( EbEventBase*, const EbBitMask& );

public:
  void setClientMask(std::vector<EbBitMask>& lGroupClientMask) {_lGroupClientMask = lGroupClientMask;}
  
private:  
  std::vector<EbBitMask> _lGroupClientMask;
};

} // namespace Pds 
#endif
