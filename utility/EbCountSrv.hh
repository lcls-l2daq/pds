#ifndef Pds_EbCountSrv_hh
#define Pds_EbCountSrv_hh

namespace Pds {

  class EbCountSrv {
  public:
    virtual ~EbCountSrv() {}
    virtual unsigned count() const = 0;
  };

}

#endif
