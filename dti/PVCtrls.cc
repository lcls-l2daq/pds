#include "pds/dti/PVCtrls.hh"
#include "pds/dti/Module.hh"

#include "pds/epicstools/PVMonitor.hh"
using Pds_Epics::PVMonitor;

#include <sstream>
#include <string>
#include <vector>

#include <stdio.h>

using Pds_Epics::PVMonitor;

namespace Pds {
  namespace Dti {

    class UsLinkEnablePV : public Pds_Epics::EpicsCA,
                           public Pds_Epics::PVMonitorCb
    {
    public:
      UsLinkEnablePV(Module& m, const char* pvName, unsigned nelem) :
        Pds_Epics::EpicsCA(pvName, this),
        _m(m),
        _nelem(nelem) {}
      virtual ~UsLinkEnablePV() {}
    public:
      void updated()
      {
        unsigned v = *reinterpret_cast<unsigned*>(data());

        //printf("%s: %s: 0x%02x\n", __func__, _channel.epicsName(), v);

        for (unsigned i = 0; i < _nelem; ++i)
        {
          _m._usLinkConfig[i].enable(v & 1);
          v >>= 1;
        }
      }
    private:
      Module&  _m;
      unsigned _nelem;
    };

    class ClearCountersPV : public Pds_Epics::EpicsCA,
                            public Pds_Epics::PVMonitorCb
    {
    public:
      ClearCountersPV(Module& m, const char* pvName) :
        Pds_Epics::EpicsCA(pvName, this),
        _m(m) {}
      virtual ~ClearCountersPV() {}
    public:
      void updated()
      {
        //printf("%s: %s: %d\n", __func__, _channel.epicsName(), *reinterpret_cast<unsigned*>(data()));

        _m.clearCounters();
      }
    private:
      Module& _m;
    };

    PVCtrls::PVCtrls(Module& m) : _pv(0), _m(m) {}
    PVCtrls::~PVCtrls() {}

    void PVCtrls::allocate(const std::string& title)
    {
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

      _pv.push_back( new UsLinkEnablePV (_m, (pvbase+"UsLinkEnable" ).c_str(), Module::NUsLinks) );
      _pv.push_back( new ClearCountersPV(_m, (pvbase+"ClearCounters").c_str()) );

      // Wait for monitors to be established
      ca_pend_io(0);
    }
  };
};
