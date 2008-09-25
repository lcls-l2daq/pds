#ifndef PDS_INLETWIREINS_HH
#define PDS_INLETWIREINS_HH

#include "pds/service/Ins.hh"

namespace Pds {

class InletWireIns {
public:
  InletWireIns(unsigned id);
  InletWireIns(unsigned id, const Ins& rcvr);

  unsigned id() const;
  const Ins& rcvr() const;

private:
  unsigned _id;
  Ins _rcvr;
};
}
#endif

