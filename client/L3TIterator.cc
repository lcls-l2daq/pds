#include "pds/client/L3TIterator.hh"
#include "pds/xtc/SummaryDg.hh"

#include <stdio.h>

int L3TIterator::process(Xtc* xtc)
{
  if (xtc->contains.id()==TypeId::Id_Xtc) {
    iterate(xtc);
    return 1;
  }
  if (xtc->contains.value()==SummaryDg::Xtc::typeId().value()) {
    const SummaryDg::Xtc& s = *static_cast<SummaryDg::Xtc*>(xtc);
    _found = true;
    _pass  = (s.l3tresult()==SummaryDg::Pass);
  }
  return 0;
}
