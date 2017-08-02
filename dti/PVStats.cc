#include "pds/dti/PVStats.hh"
#include "pds/dti/Module.hh"

#include "pds/epicstools/PVWriter.hh"
using Pds_Epics::PVWriter;

#include <sstream>
#include <string>
#include <vector>

#include <stdio.h>

using Pds_Epics::PVWriter;

namespace Pds {
  namespace Dti {

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
      o << title << ":DTI:";
      std::string pvbase = o.str();

      _pv.push_back( new PVWriter((pvbase+"UsLinks").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"BpLink" ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"DsLinks").c_str()) );

      _pv.push_back( new PVWriter((pvbase+"USRxErrs").c_str(), Module::NUsLinks) );
      _pv.push_back( new PVWriter((pvbase+"UsRxFull").c_str(), Module::NUsLinks) );
      _pv.push_back( new PVWriter((pvbase+"UsIbRecv").c_str(), Module::NUsLinks) );
      _pv.push_back( new PVWriter((pvbase+"UsIbEvt" ).c_str(), Module::NUsLinks) );
      _pv.push_back( new PVWriter((pvbase+"UsObRecv").c_str(), Module::NUsLinks) );
      _pv.push_back( new PVWriter((pvbase+"UsObSent").c_str(), Module::NUsLinks) );

      _pv.push_back( new PVWriter((pvbase+"BpObSent").c_str()) );

      _pv.push_back( new PVWriter((pvbase+"DsRxErrs").c_str(), Module::NDsLinks) );
      _pv.push_back( new PVWriter((pvbase+"DsRxFull").c_str(), Module::NDsLinks) );
      _pv.push_back( new PVWriter((pvbase+"DsObSent").c_str(), Module::NDsLinks) );

      _pv.push_back( new PVWriter((pvbase+"QpllLock").c_str()) );

      _pv.push_back( new PVWriter((pvbase+"MonClkRate").c_str(), 4) );
      _pv.push_back( new PVWriter((pvbase+"MonClkSlow").c_str(), 4) );
      _pv.push_back( new PVWriter((pvbase+"MonClkFast").c_str(), 4) );
      _pv.push_back( new PVWriter((pvbase+"MonClkLock").c_str(), 4) );

      _pv.push_back( new PVWriter((pvbase+"UsLinkObL0").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"UsLinkObL1A").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"UsLinkObL1R").c_str()) );

      printf("PVs allocated\n");
    }

    void PVStats::update(const Stats& ns, const Stats& os, double dt)
    {
#define PVPUTU(i,v)    { *reinterpret_cast<unsigned*>(_pv[i]->data())    = unsigned(v); _pv[i]->put(); }
#define PVPUTD(i,v)    { *reinterpret_cast<double  *>(_pv[i]->data())    = double  (v); _pv[i]->put(); }
#define PVPUTAU(p,m,v) { for (unsigned i = 0; i < m; ++i)                                \
                           reinterpret_cast<unsigned*>(_pv[p]->data())[i] = unsigned(v); \
                         _pv[p]->put();                                                  \
                       }
#define PVPUTAD(p,m,v) { for (unsigned i = 0; i < m; ++i)                                \
                           reinterpret_cast<double  *>(_pv[p]->data())[i] = double  (v); \
                         _pv[p]->put();                                                  \
                       }

      PVPUTU ( 0, ns.usLinkUp);
      PVPUTU ( 1, ns.bpLinkUp);
      PVPUTU ( 2, ns.dsLinkUp);

      PVPUTAD( 3, Module::NUsLinks, double(ns.us[i].rxErrs - os.us[i].rxErrs) / dt);
      PVPUTAD( 4, Module::NUsLinks, double(ns.us[i].rxFull - os.us[i].rxFull) / dt);
      PVPUTAD( 5, Module::NUsLinks, double(ns.us[i].ibRecv - os.us[i].ibRecv) / dt);
      PVPUTAD( 6, Module::NUsLinks, double(ns.us[i].ibEvt  - os.us[i].ibEvt ) / dt);
      PVPUTAD( 7, Module::NUsLinks, double(ns.us[i].obRecv - os.us[i].obRecv) / dt);
      PVPUTAD( 8, Module::NUsLinks, double(ns.us[i].obSent - os.us[i].obSent) / dt);

      PVPUTD ( 9, double(ns.bpObSent - os.bpObSent) / dt);

      PVPUTAD(10, Module::NDsLinks, double(ns.ds[i].rxErrs - os.ds[i].rxErrs) / dt);
      PVPUTAD(11, Module::NDsLinks, double(ns.ds[i].rxFull - os.ds[i].rxFull) / dt);
      PVPUTAD(12, Module::NDsLinks, double(ns.ds[i].obSent - os.ds[i].obSent) / dt);

      PVPUTU (13, ns.qpllLock);

      PVPUTAD(14, 4, (ns.monClk[i].rate)*1.e-6);
      PVPUTAU(15, 4,  ns.monClk[i].slow);
      PVPUTAU(16, 4,  ns.monClk[i].fast);
      PVPUTAU(17, 4,  ns.monClk[i].lock);

      PVPUTD (18, double(ns.usLinkObL0  - os.usLinkObL0)  / dt);
      PVPUTD (19, double(ns.usLinkObL1A - os.usLinkObL1A) / dt);
      PVPUTD (20, double(ns.usLinkObL1R - os.usLinkObL1R) / dt);

#undef PVPUTU
#undef PVPUTD
#undef PVPUTAU
#undef PVPUTAD

      ca_flush_io();
    }
  };
};
