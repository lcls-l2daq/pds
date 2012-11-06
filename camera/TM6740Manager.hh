#ifndef Pds_TM6740Manager_hh
#define Pds_TM6740Manager_hh

#include "pds/camera/FexCameraManager.hh"

namespace Pds {

  class TM6740Manager : public FexCameraManager {
  public:
    TM6740Manager(const Src& src);
    ~TM6740Manager();

  private:
    void _configure(const void* tc);
  };
};

#endif
