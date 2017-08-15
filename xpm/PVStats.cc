#include "pds/xpm/PVStats.hh"
#include "pds/xpm/Module.hh"

#include "pds/epicstools/PVWriter.hh"
using Pds_Epics::PVWriter;

#include <sstream>
#include <string>
#include <vector>

#include <stdio.h>

using Pds_Epics::PVWriter;

namespace Pds {
  namespace Xpm {

    PVStats::PVStats() : _pv(0) {}
    PVStats::~PVStats() {}

    void PVStats::allocate(const std::string& title) {
      if (ca_current_context() == NULL) {
        printf("Initializing context\n");
        SEVCHK ( ca_context_create(ca_enable_preemptive_callback ),
                 "Calling ca_context_create" );
      }

      for(unsigned i=0; i<_pv.size(); i++)
        delete _pv[i];
      _pv.resize(0);

      std::ostringstream o;
      o << title << ":XPM:";
      std::string pvbase = o.str();

      _pv.push_back( new PVWriter((pvbase+"L0InpRate").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"L0AccRate").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"L1Rate").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"NumL0Inp" ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"NumL0Acc" ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"NumL1" ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"DeadFrac").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"DeadTime").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"DeadFLnk").c_str(),32) );

      _pv.push_back( new PVWriter((pvbase+"RxClks").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"TxClks").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"RxRsts").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"CrcErrs").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"RxDecErrs").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"RxDspErrs").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"BypassRsts").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"BypassDones").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"RxLinkUp").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"FIDs").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"SOFs").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"EOFs").c_str()) );

      printf("PVs allocated\n");
    }

    void PVStats::begin(const L0Stats& s)
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

    void PVStats::update(const L0Stats& ns, const L0Stats& os, double dt)
    {
      unsigned numl0     = ns.numl0    - os.numl0;
      PVPUT(0, double(numl0)/dt);
      unsigned numl0Acc  = ns.numl0Acc - os.numl0Acc;
      PVPUT(1, double(numl0Acc)/dt);
      PVPUT(3, ns.numl0    - _begin.numl0);
      PVPUT(4, ns.numl0Acc - _begin.numl0Acc);
      PVPUT(6, numl0 ? double(ns.numl0Inh - os.numl0Inh) / double(numl0) : 0);
      unsigned l0Enabled = ns.l0Enabled - os.l0Enabled;
      if (l0Enabled) {
        PVPUT (7,     double(ns.l0Inhibited - os.l0Inhibited) / double(l0Enabled));
        PVPUTA(8, 32, double(ns.linkInh[i]  - os.linkInh[i])  / double(l0Enabled));
      }
      ca_flush_io();
    }
    void PVStats::update(const CoreCounts& nc, const CoreCounts& oc, double dt)
    {
      PVPUT( 9, double(nc.rxClkCount       - oc.rxClkCount      ) / dt);
      PVPUT(10, double(nc.txClkCount       - oc.txClkCount      ) / dt);
      PVPUT(11, double(nc.rxRstCount       - oc.rxRstCount      ) / dt);
      PVPUT(12, double(nc.crcErrCount      - oc.crcErrCount     ) / dt);
      PVPUT(13, double(nc.rxDecErrCount    - oc.rxDecErrCount   ) / dt);
      PVPUT(14, double(nc.rxDspErrCount    - oc.rxDspErrCount   ) / dt);
      PVPUT(15, double(nc.bypassResetCount - oc.bypassResetCount) / dt);
      PVPUT(16, double(nc.bypassDoneCount  - oc.bypassDoneCount ) / dt);
      PVPUT(17, double(nc.rxLinkUp                              )     );
      PVPUT(18, double(nc.fidCount         - oc.fidCount        ) / dt);
      PVPUT(19, double(nc.sofCount         - oc.sofCount        ) / dt);
      PVPUT(20, double(nc.eofCount         - oc.eofCount        ) / dt);
      //ca_flush_io();  // Revisit: Let the L0Stats update do the flush...
    }
  };
};
