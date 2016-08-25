#ifndef Pds_BldSequenceSrv_hh
#define Pds_BldSequenceSrv_hh

namespace Pds {

  class Sequence;
  class Env;

  class BldSequenceSrv {
  public:
    virtual ~BldSequenceSrv() {}
    virtual unsigned fiducials() const = 0;
  };

}

#endif
