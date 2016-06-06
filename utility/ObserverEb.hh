#ifndef Pds_ObserverEb_hh
#define Pds_ObserverEb_hh

#include <vector>
#include "EbSGroup.hh"

namespace Pds
{

class ObserverEb : public EbSGroup
{
public:
  ObserverEb(const Src& id,
             const TypeId& ctns,
             Level::Type level,
             Inlet& inlet,
             OutletWire& outlet,
             int stream,
             int ipaddress,
             unsigned eventsize,
             unsigned eventpooldepth,
             VmonEb* vmoneb=0);
  ~ObserverEb();
public:
  Server*      accept(Server*);
protected:
  EbEventBase* _event(EbServer*);
  EbEventBase* _seek (EbServer*);
private:
  std::vector<EbEventBase*> _last;
};

} // namespace Pds
#endif
