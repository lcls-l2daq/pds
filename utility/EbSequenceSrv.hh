#ifndef Pds_EbSequenceSrv_hh
#define Pds_EbSequenceSrv_hh

namespace Pds {

  class Sequence;
  class Env;

  class EbSequenceSrv {
  public:
    virtual ~EbSequenceSrv() {}
    virtual const Sequence& sequence() const = 0;
    virtual const Env&      env() const = 0;
  };

}

#endif
