#ifndef Pds_IocHostCallback_hh
#define Pds_IocHostCallback_hh

namespace Pds {
  class IocNode;
  class IocHostCallback {
  public:
    virtual ~IocHostCallback() {}
    virtual void connected(const IocNode&) = 0;
  };

};

#endif
