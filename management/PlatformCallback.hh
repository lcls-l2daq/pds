#ifndef Pds_PlatformCallback_hh
#define Pds_PlatformCallback_hh

namespace Pds {

  class PlatformCallback {
  public:
    virtual ~PlatformCallback() {}
    virtual void available(const Node&) = 0;
  };

};

#endif
