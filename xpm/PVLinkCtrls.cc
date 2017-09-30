#include "pds/xpm/PVLinkCtrls.hh"
#include "pds/xpm/Module.hh"

#include <sstream>

#include <stdio.h>

using Pds_Epics::EpicsCA;
using Pds_Epics::PVMonitorCb;

namespace Pds {
  namespace Xpm {

    PVLinkCtrls::PVLinkCtrls(Module& m) : _pv(0), _m(m) {}
    PVLinkCtrls::~PVLinkCtrls() {}

    void PVLinkCtrls::allocate(const std::string& title)
    {
    }

    Module& PVLinkCtrls::module() { return _m; }
  };
};
