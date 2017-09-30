#include "pds/xpm/PVPStats.hh"
#include "pds/xpm/Module.hh"

#include "pds/epicstools/PVWriter.hh"
using Pds_Epics::PVWriter;

#include <sstream>
#include <string>
#include <vector>

#include <stdio.h>

namespace Pds {
  namespace Xpm {

    PVPStats::PVPStats() : _pv(0) {}
    PVPStats::~PVPStats() {}

    void PVPStats::allocate(const std::string& title) {
      if (ca_current_context() == NULL) {
        printf("Initializing context\n");
        SEVCHK ( ca_context_create(ca_enable_preemptive_callback ),
                 "Calling ca_context_create" );
      }

      for(unsigned i=0; i<_pv.size(); i++)
        delete _pv[i];
      _pv.resize(0);

      std::string pvbase = title + ":";

      _pv.push_back( new PVWriter((pvbase+"L0InpRate").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"L0AccRate").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"L1Rate"   ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"NumL0Inp" ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"NumL0Acc" ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"NumL1"    ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"DeadFrac" ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"DeadTime" ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"DeadFLnk" ).c_str(),32) );
      _pv.push_back( new PVWriter((pvbase+"RunTime"  ).c_str()) );

      printf("Partition PVs allocated\n");
    }

    void PVPStats::begin(const L0Stats& s)
    {
      _begin=s;
      printf("Begin NumL0 %lu  Acc %lu\n",
             s.numl0, s.numl0Acc);
    }

#define PVPUT(i,v)    { *reinterpret_cast<double*>(_pv[i]->data()) = double(v); _pv[i]->put(); }
#define PVPUTA(p,m,v) { for (unsigned i = 0; i < m; ++i)                                \
                          reinterpret_cast<double  *>(_pv[p]->data())[i] = double  (v); \
                        _pv[p]->put();                                                  \
                      }

    void PVPStats::update(const L0Stats& ns, const L0Stats& os)
    {
      PVPUT(9, double(ns.l0Enabled)*14.e-6/13.);
      unsigned l0Enabled = ns.l0Enabled - os.l0Enabled;
      double dt = double(l0Enabled)*14.e-6/13.;
      unsigned numl0     = ns.numl0    - os.numl0;
      PVPUT(0, double(numl0)/dt);
      unsigned numl0Acc  = ns.numl0Acc - os.numl0Acc;
      PVPUT(1, double(numl0Acc)/dt);
      PVPUT(3, ns.numl0    - _begin.numl0);
      PVPUT(4, ns.numl0Acc - _begin.numl0Acc);
      PVPUT(6, numl0 ? double(ns.numl0Inh - os.numl0Inh) / double(numl0) : 0);
      if (l0Enabled) {
        PVPUT (7,     double(ns.l0Inhibited - os.l0Inhibited) / double(l0Enabled));
        PVPUTA(8, 32, double(ns.linkInh[i]  - os.linkInh[i])  / double(numl0));
      }
      ca_flush_io();
    }
  };
};
