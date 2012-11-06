#ifndef Pds_Opal1kManager_hh
#define Pds_Opal1kManager_hh

#include "pds/camera/FexCameraManager.hh"

namespace Pds {

  class Opal1kManager : public FexCameraManager {
  public:
    Opal1kManager(const Src& src);
    ~Opal1kManager();

  private:
    void _configure(const void*);
  };
};

#endif
