#ifndef Pds_BldSequenceSrv_hh
#define Pds_BldSequenceSrv_hh

namespace Pds {

  class Sequence;
  class Env;

  class BldSequenceSrv {
  public:
    virtual ~BldSequenceSrv() {}
    virtual const Sequence& sequence() const = 0;
    virtual const Env&      env() const = 0;
  };

}

#endif
