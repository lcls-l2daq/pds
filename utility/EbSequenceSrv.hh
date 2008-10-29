#ifndef Pds_EbSequenceSrv_hh
#define Pds_EbSequenceSrv_hh

namespace Pds {

  class Sequence;

  class EbSequenceSrv {
  public:
    virtual ~EbSequenceSrv() {}
    virtual const Sequence& sequence() const = 0;
  };

}

#endif
