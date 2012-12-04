#ifndef Pds_OrcaManager_hh
#define Pds_OrcaManager_hh

#include "pds/camera/FexCameraManager.hh"

namespace Pds {

  class OrcaManager : public FexCameraManager {
  public:
    OrcaManager(const Src& src);
    ~OrcaManager();

  private:
    void _configure(const void*);
  };
};

#endif
