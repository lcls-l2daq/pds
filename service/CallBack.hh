#ifndef PDS_CALLBACK
#define PDS_CALLBACK

namespace Pds {
class CallBack
  {
  public:
    virtual ~CallBack() {};
    virtual void fire() = 0;
  };
}

#endif
