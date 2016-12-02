#include "pds/quadadc/EpicsCfg.hh"

#include <sstream>

namespace Pds {
  namespace QuadAdc {
    class PvCb : public Pds_Epics::PVMonitorCb {
    public:
      PvCb() {}
      ~PvCb() {}
    public:
      void updated() {}
    };
  };
};

using namespace Pds::QuadAdc;

static const char* pvname(const char* base,
                          const char* name) {
  std::ostringstream o;
  o << base << ":" << name;
  return o.str().c_str();
}

static PvCb _cb;

EpicsCfg::EpicsCfg(const char* base) :
  _channelMask(pvname(base,"CHAN_MASK")),
  _sampleRate (pvname(base,"SAMP_RATE")),
  _numSamples (pvname(base,"NSAMPLES" )),
  _delay      (pvname(base,"DELAY_TKS")),
  _evtCode    (pvname(base,"EVT_CODE" ))
{
}

EpicsCfg::~EpicsCfg()
{
}
 
unsigned EpicsCfg::channelMask ()
{
  unsigned v;
  _channelMask.fetch(reinterpret_cast<char*>(&v),sizeof(v));
  return v;
}
unsigned EpicsCfg::sampleRate()
{
  unsigned v;
  _sampleRate.fetch(reinterpret_cast<char*>(&v),sizeof(v));
  return v;
}

unsigned EpicsCfg::numSamples  ()
{
  unsigned v;
  _numSamples.fetch(reinterpret_cast<char*>(&v),sizeof(v));
  return v;
}

unsigned EpicsCfg::delay_ns    ()
{
  unsigned v;
  _delay.fetch(reinterpret_cast<char*>(&v),sizeof(v));
  return v;
}

unsigned EpicsCfg::evtCode     ()
{
  unsigned v;
  _evtCode.fetch(reinterpret_cast<char*>(&v),sizeof(v));
  return v;
}
