#ifndef Pds_TM6740Manager_hh
#define Pds_TM6740Manager_hh

#include "pds/camera/CameraManager.hh"

namespace PdsLeutron {
  class TM6740Camera;
};

namespace Pds {

  class Src;
  class GenericPool;

  class TM6740Manager : public CameraManager {
  public:
    TM6740Manager(const Src& src);
    ~TM6740Manager();

  private:
    void _configure(const void* tc);

    PdsLeutron::PicPortCL& camera();

  private:
    PdsLeutron::TM6740Camera* _camera;
  };
};

#endif
