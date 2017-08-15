#include "pds/xpm/PVCtrls.hh"
#include "pds/xpm/Module.hh"

#include <sstream>

#include <stdio.h>

using Pds_Epics::EpicsCA;
using Pds_Epics::PVMonitorCb;

namespace Pds {
  namespace Xpm {

#define Q(a,b)      a ## b
#define PV(name)    Q(name, PV)

#define TOU(value)  *reinterpret_cast<unsigned*>(value)
#define PRT(value)  printf("%60.60s: %32.32s: 0x%02x\n", __PRETTY_FUNCTION__, _channel.epicsName(), value)

#define PVG(i)      PRT(TOU(data()));    _ctrl.module().i;
#define PVP(i)      PRT( ( TOU(data()) = _ctrl.module().i ) );  put();

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

    CPV(ClearLinks,     { PVG(clearLinks());                          },
                        {                                             })

    CPV(LinkDebug,      { PVG(setLinkDebug(TOU(data())));             },
                        { PVP(getLinkDebug());                        })
    CPV(Inhibit,        { PVG(setInhibit  (TOU(data())));             },
                        { PVP(getInhibit  ());                        })
    CPV(TagStream,      { PVG(setTagStream(TOU(data())));             },
                        { PVP(getTagStream());                        })

    CPV(LinkTxDelay,    { PVG(linkTxDelay  (_idx, TOU(data())));      },
                        { PVP(linkTxDelay  (_idx));                   })
    CPV(LinkPartition,  { PVG(linkPartition(_idx, TOU(data())));      },
                        { PVP(linkPartition(_idx));                   })
    CPV(LinkTrgSrc,     { PVG(linkTrgSrc   (_idx, TOU(data())));      },
                        { PVP(linkTrgSrc   (_idx));                   })
    CPV(LinkLoopback,   { PVG(linkLoopback (_idx, TOU(data()) != 0)); },
                        { PVP(linkLoopback (_idx));                   })
    CPV(TxLinkReset,    { PVG(txLinkReset  (_idx));                   },
                        {                                             })
    CPV(RxLinkReset,    { PVG(rxLinkReset  (_idx));                   },
                        {                                             })
    CPV(LinkEnable,     { PVG(linkEnable(_idx, TOU(data()) != 0));    },
                        { PVP(linkEnable(_idx));                      })

    CPV(PLL_BW_Select,  { PVG(pllBwSel  (_idx, TOU(data())));         },
                        { PVP(pllBwSel  (_idx));                      })
    CPV(PLL_FreqTable,  { PVG(pllFrqTbl (_idx, TOU(data())));         },
                        { PVP(pllFrqTbl (_idx));                      })
    CPV(PLL_FreqSelect, { PVG(pllFrqSel (_idx, TOU(data())));         },
                        { PVP(pllFrqSel (_idx));                      })
    CPV(PLL_Rate,       { PVG(pllRateSel(_idx, TOU(data())));         },
                        { PVP(pllRateSel(_idx));                      })
    CPV(PLL_PhaseInc,   { PVG(pllPhsInc (_idx));                      },
                        {                                             })
    CPV(PLL_PhaseDec,   { PVG(pllPhsDec (_idx));                      },
                        {                                             })
    CPV(PLL_Bypass,     { PVG(pllBypass (_idx, TOU(data())));         },
                        { PVP(pllBypass (_idx));                      })
    CPV(PLL_Reset,      { PVG(pllReset  (_idx));                      },
                        {                                             })
    CPV(PLL_Skew,       { PVG(pllSkew   (_idx, TOU(data())));         },
                        {                                             })

    CPV(L0Select,            { _ctrl.l0Select(TOU(data()));
                               _ctrl.setL0Select();           },
                             {                                })
    CPV(L0Select_FixedRate,  { _ctrl.fixedRate(TOU(data()));
                               _ctrl.setL0Select();           },
                             {                                })
    CPV(L0Select_ACRate,     { _ctrl.acRate(TOU(data()));
                               _ctrl.setL0Select();           },
                             {                                })
    CPV(L0Select_ACTimeslot, { _ctrl.acTimeslot(TOU(data()));
                               _ctrl.setL0Select();           },
                             {                                })
    CPV(L0Select_Sequence,   { _ctrl.seqIdx(TOU(data()));
                               _ctrl.setL0Select();           },
                             {                                })
    CPV(L0Select_SeqBit,     { _ctrl.seqBit(TOU(data()));
                               _ctrl.setL0Select();           },
                             {                                })

    CPV(SetL0Enabled,   { PVG(setL0Enabled (TOU(data()) != 0));   },
                        { PVP(getL0Enabled ());                   })

    CPV(L1TrgClear,     { PVG(setL1TrgClr  (_idx, TOU(data())));  },
                        { PVP(getL1TrgClr  (_idx));               })
    CPV(L1TrgEnable,    { PVG(setL1TrgEnb  (_idx, TOU(data())));  },
                        { PVP(getL1TrgEnb  (_idx));               })
    CPV(L1TrgSource,    { PVG(setL1TrgSrc  (_idx, TOU(data())));  },
                        { PVP(getL1TrgSrc  (_idx));               })
    CPV(L1TrgWord,      { PVG(setL1TrgWord (_idx, TOU(data())));  },
                        { PVP(getL1TrgWord (_idx));               })
    CPV(L1TrgWrite,     { PVG(setL1TrgWrite(_idx, TOU(data())));  },
                        { PVP(getL1TrgWrite(_idx));               })

    CPV(AnaTagReset,    { _ctrl.module().setPartition(_idx);
                          PVG(_analysisRst = TOU(data()));        },
                        { _ctrl.module().setPartition(_idx);
                          PVP(_analysisRst);                      })
    CPV(AnaTag,         { _ctrl.module().setPartition(_idx);
                          PVG(_analysisTag = TOU(data()));        },
                        { _ctrl.module().setPartition(_idx);
                          PVP(_analysisTag);                      })
    CPV(AnaTagPush,     { _ctrl.module().setPartition(_idx);
                          PVG(_analysisPush = TOU(data()));       },
                        { _ctrl.module().setPartition(_idx);
                          PVP(_analysisPush);                     })

    CPV(PipelineDepth,  { _ctrl.module().setPartition(_idx);
                          PVG(_pipelineDepth = TOU(data()));      },
                        { _ctrl.module().setPartition(_idx);
                          PVP(_pipelineDepth);                    })

    CPV(MsgHeader,      { PVG(messageHdr(_idx, TOU(data())));     },
                        { PVP(messageHdr(_idx));                  })
    CPV(MsgInsert,      { PVG(messageIns(_idx, TOU(data())));     },
                        { PVP(messageIns(_idx));                  })
    CPV(MsgPayload,     { _ctrl.module().setPartition(_idx);
                          PVG(_messagePayload = TOU(data()));     },
                        { _ctrl.module().setPartition(_idx);
                          PVP(_messagePayload);                   })
    CPV(InhInterval,    { PVG(inhibitInt(_idx, TOU(data())));     },
                        { PVP(inhibitInt(_idx));                  })
    CPV(InhLimit,       { PVG(inhibitLim(_idx, TOU(data())));     },
                        { PVP(inhibitLim(_idx));                  })
    CPV(InhEnable,      { PVG(inhibitEnb(_idx, TOU(data())));     },
                        { PVP(inhibitEnb(_idx));                  })

    CPV(ModuleInit,     { PVG(init());        }, { })
    CPV(DumpPll,        { PVG(dumpPll(_idx)); }, { })

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
      o << title << ":XPM:";
      std::string pvbase = o.str();

#define NPV(name)  _pv.push_back( new PV(name)(*this, (pvbase+#name).c_str()) )
#define NPVN(name, n)                                                        \
      for (unsigned i = 0; i < n; ++i) {                                     \
        std::ostringstream str;  str << i;  std::string idx = str.str();     \
        _pv.push_back( new PV(name)(*this, (pvbase+#name+idx).c_str(), i) ); \
      }

      NPV ( ModuleInit                              );
      NPVN( DumpPll,            Module::NAmcs       );

      NPV ( ClearLinks                              );
      NPV ( LinkDebug                               );
      NPV ( Inhibit                                 );
      NPV ( TagStream                               );
      NPVN( LinkTxDelay,        Module::NAmcs * Module::NDSLinks);
      NPVN( LinkPartition,      Module::NAmcs * Module::NDSLinks);
      NPVN( LinkTrgSrc,         Module::NAmcs * Module::NDSLinks);
      NPVN( LinkLoopback,       Module::NAmcs * Module::NDSLinks);
      NPVN( TxLinkReset,        Module::NAmcs * Module::NDSLinks);
      NPVN( RxLinkReset,        Module::NAmcs * Module::NDSLinks);
      NPVN( LinkEnable,         Module::NAmcs * Module::NDSLinks);
      NPVN( PLL_BW_Select,      Module::NAmcs       );
      NPVN( PLL_FreqTable,      Module::NAmcs       );
      NPVN( PLL_FreqSelect,     Module::NAmcs       );
      NPVN( PLL_Rate,           Module::NAmcs       );
      NPVN( PLL_PhaseInc,       Module::NAmcs       );
      NPVN( PLL_PhaseDec,       Module::NAmcs       );
      NPVN( PLL_Bypass,         Module::NAmcs       );
      NPVN( PLL_Reset,          Module::NAmcs       );
      NPVN( PLL_Skew,           Module::NAmcs       );
      NPV ( L0Select                                );
      NPV ( L0Select_FixedRate                      );
      NPV ( L0Select_ACRate                         );
      NPV ( L0Select_ACTimeslot                     );
      NPV ( L0Select_Sequence                       );
      NPV ( L0Select_SeqBit                         );
      NPV ( SetL0Enabled                            );
      NPVN( L1TrgClear,         Module::NPartitions );
      NPVN( L1TrgEnable,        Module::NPartitions );
      NPVN( L1TrgSource,        Module::NPartitions );
      NPVN( L1TrgWord,          Module::NPartitions );
      NPVN( L1TrgWrite,         Module::NPartitions );
      NPVN( AnaTagReset,        Module::NPartitions );
      NPVN( AnaTag,             Module::NPartitions );
      NPVN( AnaTagPush,         Module::NPartitions );
      NPVN( PipelineDepth,      Module::NPartitions );
      NPVN( MsgHeader,          Module::NPartitions );
      NPVN( MsgInsert,          Module::NPartitions );
      NPVN( MsgPayload,         Module::NPartitions );
      NPVN( InhInterval,        Module::NPartitions );
      NPVN( InhLimit,           Module::NPartitions );
      NPVN( InhEnable,          Module::NPartitions );

      // Wait for monitors to be established
      ca_pend_io(0);
    }

    Module& PVCtrls::module() { return _m; }

    void PVCtrls::l0Select  (unsigned v) { _l0Select   = v; }
    void PVCtrls::fixedRate (unsigned v) { _fixedRate  = v; }
    void PVCtrls::acRate    (unsigned v) { _acRate     = v; }
    void PVCtrls::acTimeslot(unsigned v) { _acTimeslot = v; }
    void PVCtrls::seqIdx    (unsigned v) { _seqIdx     = v; }
    void PVCtrls::seqBit    (unsigned v) { _seqBit     = v; }

    void PVCtrls::setL0Select()
    {
      switch(_l0Select)
      {
        case FixedRate:
          _m.setL0Select_FixedRate(_fixedRate);
          break;
        case ACRate:
          _m.setL0Select_ACRate(_acRate, _acTimeslot);
          break;
        case Sequence:
          _m.setL0Select_Sequence(_seqIdx, _seqBit);
          break;
        default:
          printf("%s: Invalid L0Select value: %d\n", __func__, _l0Select);
          break;
      }
    }
  };
};
