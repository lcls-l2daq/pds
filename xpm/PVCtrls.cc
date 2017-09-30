#include "pds/xpm/PVCtrls.hh"
#include "pds/xpm/Module.hh"
#include "pds/service/Semaphore.hh"

#include <sstream>

#include <stdio.h>

//#define SET_PLL

using Pds_Epics::EpicsCA;
using Pds_Epics::PVMonitorCb;

namespace Pds {
  namespace Xpm {

#define Q(a,b)      a ## b
#define PV(name)    Q(name, PV)

#define TOU(value)  *reinterpret_cast<unsigned*>(value)
#define PRT(value)  printf("%60.60s: %32.32s: 0x%02x\n", __PRETTY_FUNCTION__, _channel.epicsName(), value)

#define PVG(i)      PRT(TOU(data()));    _ctrl.module().i;
#define GPVG(i)   {                                        \
      _ctrl.sem().take();                                  \
      PRT(TOU(data()));    _ctrl.module().i;               \
      _ctrl.sem().give(); }
#define PVP(i)      PRT( ( TOU(data()) = _ctrl.module().i ) );  put();
#define GPVP(i) {                                               \
    _ctrl.sem().take();                                         \
    TOU(data()) = _ctrl.module().i ;                            \
    _ctrl.sem().give();                                         \
    put();                                                      \
    PRT( ( _ctrl.module().i ) ); }

#define CPV(name, updatedBody, connectedBody)                           \
                                                                        \
    class PV(name) : public EpicsCA,                                    \
                     public PVMonitorCb                                 \
    {                                                                   \
    public:                                                             \
      PV(name)(PVCtrls& ctrl, const char* pvName, unsigned idx = 0) :   \
        EpicsCA(pvName, this),                                          \
        _ctrl(ctrl),                                                    \
        _idx(idx) {}                                                    \
      virtual ~PV(name)() {}                                            \
    public:                                                             \
      void updated();                                                   \
      void connected(bool);                                             \
    public:                                                             \
      void put() { if (this->EpicsCA::connected())  _channel.put(); }   \
    private:                                                            \
      PVCtrls& _ctrl;                                                   \
      unsigned _idx;                                                    \
    };                                                                  \
    void PV(name)::updated()                                            \
    {                                                                   \
      updatedBody                                                       \
    }                                                                   \
    void PV(name)::connected(bool c)                                    \
    {                                                                   \
      this->EpicsCA::connected(c);                                      \
      connectedBody                                                     \
    }

    CPV(LinkDebug,      { GPVG(setLinkDebug (TOU(data())));            },
                        { GPVP(getLinkDebug ());                       })

    CPV(LinkTxDelay,    { GPVG(linkTxDelay  (_idx, TOU(data())));      },
                        { GPVP(linkTxDelay  (_idx));                   })
    CPV(LinkPartition,  { GPVG(linkPartition(_idx, TOU(data())));      },
                        { GPVP(linkPartition(_idx));                   })
    CPV(LinkTrgSrc,     { GPVG(linkTrgSrc   (_idx, TOU(data())));      },
                        { GPVP(linkTrgSrc   (_idx));                   })
    CPV(LinkLoopback,   { GPVG(linkLoopback (_idx, TOU(data()) != 0)); },
                        { GPVP(linkLoopback (_idx));                   })
    CPV(TxLinkReset,    { if (TOU(data())!=0) 
                            GPVG(txLinkReset  (_idx));                 },
                        {                                              })
    CPV(RxLinkReset,    { if (TOU(data())!=0)
                            GPVG(rxLinkReset  (_idx));                 },
                        {                                              })
    CPV(LinkEnable,     { GPVG(linkEnable(_idx, TOU(data()) != 0));    },
                        { GPVP(linkEnable(_idx));                      })

    CPV(LinkRxReady,    { },
                        { GPVP(linkRxReady(_idx));                     })
    CPV(LinkTxReady,    { },
                        { GPVP(linkTxReady(_idx));                     })
    CPV(LinkIsXpm,      { },
                        { GPVP(linkIsXpm(_idx));                       })
    CPV(LinkRxErr,      { },
                        { GPVP(linkRxErr(_idx));                       })

    CPV(PLL_BW_Select,  { GPVG(pllBwSel  (_idx, TOU(data())));         },
                        { GPVP(pllBwSel  (_idx));                      })
    CPV(PLL_FreqTable,  { GPVG(pllFrqTbl (_idx, TOU(data())));         },
                        { GPVP(pllFrqTbl (_idx));                      })
    CPV(PLL_FreqSelect, { GPVG(pllFrqSel (_idx, TOU(data())));         },
                        { GPVP(pllFrqSel (_idx));                      })
    CPV(PLL_Rate,       { GPVG(pllRateSel(_idx, TOU(data())));         },
                        { GPVP(pllRateSel(_idx));                      })
    CPV(PLL_PhaseInc,   { GPVG(pllPhsInc (_idx));                      },
                        {                                              })
    CPV(PLL_PhaseDec,   { GPVG(pllPhsDec (_idx));                      },
                        {                                              })
    CPV(PLL_Bypass,     { GPVG(pllBypass (_idx, TOU(data())));         },
                        { GPVP(pllBypass (_idx));                      })
    CPV(PLL_Reset,      { GPVG(pllReset  (_idx));                      },
                        {                                              })
    CPV(PLL_Skew,       { GPVG(pllSkew   (_idx, TOU(data())));         },
                        {                                              })
    CPV(PLL_LOS,        {                                              },
                        { GPVP(pllStatus0(_idx));                      })
    CPV(PLL_LOL,        {                                              },
                        { GPVP(pllStatus1(_idx));                      })

    CPV(ModuleInit,     { GPVG(init     ());     }, { })
    CPV(DumpPll,        { GPVG(dumpPll  (_idx)); }, { })
    CPV(DumpTiming,     { PVG(dumpTiming(_idx)); }, { })

    PVCtrls::PVCtrls(Module& m, Semaphore& sem) : _pv(0), _m(m), _sem(sem) {}
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
      o << title << ":";
      std::string pvbase = o.str();

#define NPV(name)  _pv.push_back( new PV(name)(*this, (pvbase+#name).c_str()) )
#define NPVN(name, n)                                                        \
      for (unsigned i = 0; i < n; ++i) {                                     \
        std::ostringstream str;  str << i;  std::string idx = str.str();     \
        _pv.push_back( new PV(name)(*this, (pvbase+#name+idx).c_str(), i) ); \
      }

      NPV ( ModuleInit                              );
      NPVN( DumpPll,            Module::NAmcs       );
      NPVN( DumpTiming,         2                   );

      NPV ( LinkDebug                               );
      NPVN( LinkTxDelay,        32);
      NPVN( LinkPartition,      32);
      NPVN( LinkTrgSrc,         32);
      NPVN( LinkLoopback,       32);
      NPVN( TxLinkReset,        32);
      NPVN( RxLinkReset,        32);
      NPVN( LinkEnable,         32);
#if 0
      NPVN( LinkRxReady,        32);
      NPVN( LinkTxReady,        32);
      NPVN( LinkIsXpm,          32);
      NPVN( LinkRxErr,          32);
#endif
      NPVN( PLL_BW_Select,      Module::NAmcs       );
      NPVN( PLL_FreqTable,      Module::NAmcs       );
      NPVN( PLL_FreqSelect,     Module::NAmcs       );
      NPVN( PLL_Rate,           Module::NAmcs       );
      NPVN( PLL_PhaseInc,       Module::NAmcs       );
      NPVN( PLL_PhaseDec,       Module::NAmcs       );
      NPVN( PLL_Bypass,         Module::NAmcs       );
#ifdef SET_PLL
      //  This makes the timing disappear
      NPVN( PLL_Reset,          Module::NAmcs       );
#endif
      NPVN( PLL_Skew,           Module::NAmcs       );
      NPVN( PLL_LOS,            Module::NAmcs       );
      NPVN( PLL_LOL,            Module::NAmcs       );

      // Wait for monitors to be established
      ca_pend_io(0);
    }

    void PVCtrls::dump() const
    {
      _sem.take();
      unsigned enable=0;
      for(unsigned i=0; i<16; i++)
        if (_m.linkEnable(i))
          enable |= (1<<i);
      _sem.give();
    }

    Module& PVCtrls::module() { return _m; }
    Semaphore& PVCtrls::sem() { return _sem; }

  };
};
