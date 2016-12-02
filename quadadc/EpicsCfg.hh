#ifndef Pds_QuadAdc_EpicsCfg_hh
#define Pds_QuadAdc_EpicsCfg_hh

#include "pds/epicstools/PvServer.hh"

//
//  Class to retrieve QuadAdc configuration from EPICS PVs
//
namespace Pds {
  namespace QuadAdc {
    class EpicsCfg {
    public:
      EpicsCfg(const char* base);
      ~EpicsCfg();
    public:
      unsigned channelMask ();
      unsigned ratePrescale();
      unsigned numSamples  ();
      unsigned delay       ();
      unsigned evtCode     ();
    private:
      Pds_Epics::PvServer _channelMask;
      Pds_Epics::PvServer _sampleRate;
      Pds_Epics::PvServer _numSamples;
      Pds_Epics::PvServer _delay;
      Pds_Epics::PvServer _evtCode;
    };
  };
};

#endif
