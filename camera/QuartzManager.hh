#ifndef Pds_QuartzManager_hh
#define Pds_QuartzManager_hh

#include "pds/camera/FexCameraManager.hh"

namespace Pds {

  class QuartzManager : public FexCameraManager {
  public:
    QuartzManager(const Src& src);
    ~QuartzManager();

  private:
    void _configure(const void*);
  };
};

#endif
