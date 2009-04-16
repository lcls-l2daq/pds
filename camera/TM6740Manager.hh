#ifndef Pds_TM6740Manager_hh
#define Pds_TM6740Manager_hh

#include "pds/camera/CameraManager.hh"
#include "pds/config/TM6740ConfigType.hh"

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
    void _configure(char*);
    void _configure(InDatagram*);

    PdsLeutron::PicPortCL& camera();
    const TypeId& camConfigType();

  private:
    PdsLeutron::TM6740Camera* _camera;
    const TM6740ConfigType*   _configdata;
    Xtc                       _configtc;
  };
};

#endif
