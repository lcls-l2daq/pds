#ifndef Xpm_PVCtrls_hh
#define Xpm_PVCtrls_hh

#include "pds/epicstools/EpicsCA.hh"

#include <string>
#include <vector>

namespace Pds {
  namespace Xpm {

    class Module;

    class PVCtrls
    {
    public:
      PVCtrls(Module&);
      ~PVCtrls();
    public:
      void allocate(const std::string& title);
      void update();
    public:
      Module& module();
    public:
      void l0Select  (unsigned v);
      void fixedRate (unsigned v);
      void acRate    (unsigned v);
      void acTimeslot(unsigned v);
      void seqIdx    (unsigned v);
      void seqBit    (unsigned v);

      void setL0Select();
    public:
      enum { FixedRate, ACRate, Sequence };
    private:
      std::vector<Pds_Epics::EpicsCA*> _pv;
      Module&  _m;
      unsigned _l0Select;
      unsigned _fixedRate;
      unsigned _acRate;
      unsigned _acTimeslot;
      unsigned _seqIdx;
      unsigned _seqBit;
    };
  };
};

#endif
