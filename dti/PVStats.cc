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

      _pv.push_back( new PVWriter((pvbase+"USLINKS").c_str()) );
      _pv.push_back( new PVWriter((pvbase+"BPLINK" ).c_str()) );
      _pv.push_back( new PVWriter((pvbase+"DSLINKS").c_str()) );

      for (unsigned i = 0; i < Module::NUsLinks; ++i)
      {
        std::ostringstream u;
        u << pvbase << "US:" << i << ":";
        std::string base = u.str();
        _pv.push_back( new PVWriter((base+"RXERRS").c_str()) );
        _pv.push_back( new PVWriter((base+"RXFULL").c_str()) );
        _pv.push_back( new PVWriter((base+"IBRECV").c_str()) );
        _pv.push_back( new PVWriter((base+"IBEVT" ).c_str()) );
        _pv.push_back( new PVWriter((base+"OBRCVD").c_str()) );
        _pv.push_back( new PVWriter((base+"OBSENT").c_str()) );
      }

      for (unsigned i = 0; i < Module::NDsLinks; ++i)
      {
        std::ostringstream d;
        d << pvbase << "DS:" << i << ":";
        std::string base = d.str();
        _pv.push_back( new PVWriter((base+"RXERRS").c_str()) );
        _pv.push_back( new PVWriter((base+"RXFULL").c_str()) );
        _pv.push_back( new PVWriter((base+"OBSENT").c_str()) );
      }

      _pv.push_back( new PVWriter((pvbase+"QPLLLOCK").c_str()) );

      for (unsigned i = 0; i < 4; ++i)
      {
        std::ostringstream m;
        m << pvbase << "MONCLK:" << i << ":";
        std::string base = m.str();
        _pv.push_back( new PVWriter((base+"RATE").c_str()) );
        _pv.push_back( new PVWriter((base+"SLOW").c_str()) );
        _pv.push_back( new PVWriter((base+"FAST").c_str()) );
        _pv.push_back( new PVWriter((base+"LOCK").c_str()) );
      }

      // @todo: _pv.push_back( new PVWriter((pvbase+"USLINKOBL0").c_str()) );
      // @todo: _pv.push_back( new PVWriter((pvbase+"USLINKOBL1A").c_str()) );
      // @todo: _pv.push_back( new PVWriter((pvbase+"USLINKOBL1R").c_str()) );

      printf("PVs allocated\n");
    }

    void PVStats::update(const Stats& ns, const Stats& os, double dt)
    {
#define PVPUTU(i,v) { *reinterpret_cast<unsigned*>(_pv[i]->data()) = unsigned(v); _pv[i]->put(); }
#define PVPUTD(i,v) { *reinterpret_cast<double  *>(_pv[i]->data()) = double  (v); _pv[i]->put(); }

      unsigned p = 0;

      PVPUTU(p++, ns.usLinkUp);
      PVPUTU(p++, ns.bpLinkUp);
      PVPUTU(p++, ns.dsLinkUp);

      for (unsigned i = 0; i < Module::NUsLinks; ++i)
      {
        PVPUTD(p++, double(ns.us[i].rxErrs - os.us[i].rxErrs) / dt);
        PVPUTD(p++, double(ns.us[i].rxFull - os.us[i].rxFull) / dt);
        PVPUTD(p++, double(ns.us[i].ibRecv - os.us[i].ibRecv) / dt);
        PVPUTD(p++, double(ns.us[i].ibEvt  - os.us[i].ibEvt ) / dt);
        PVPUTD(p++, double(ns.us[i].obRecv - os.us[i].obRecv) / dt);
        PVPUTD(p++, double(ns.us[i].obSent - os.us[i].obSent) / dt);
      }

      for (unsigned i = 0; i < Module::NDsLinks; ++i)
      {
        PVPUTD(p++, double(ns.ds[i].rxErrs - os.ds[i].rxErrs) / dt);
        PVPUTD(p++, double(ns.ds[i].rxFull - os.ds[i].rxFull) / dt);
        PVPUTD(p++, double(ns.ds[i].obSent - os.ds[i].obSent) / dt);
      }

      PVPUTU(p++, ns.qpllLock);

      for (unsigned i = 0; i < 4; ++i)
      {
        PVPUTD(p++, (ns.monClk[i].rate)*1.e-6);
        PVPUTU(p++,  ns.monClk[i].slow);
        PVPUTU(p++,  ns.monClk[i].fast);
        PVPUTU(p++,  ns.monClk[i].lock);
      }

      // @todo: PVPUTD(p++, double(ns.usLinkObL0  - os.usLinkObL0)  / dt);
      // @todo: PVPUTD(p++, double(ns.usLinkObL1A - os.usLinkObL1A) / dt);
      // @todo: PVPUTD(p++, double(ns.usLinkObL1R - os.usLinkObL1R) / dt);

#undef PVPUTU
#undef PVPUTD

      ca_flush_io();
    }
  };
};
