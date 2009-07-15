#ifndef Pds_Opal1kManager_hh
#define Pds_Opal1kManager_hh

#include "pds/camera/CameraManager.hh"

namespace PdsLeutron {
  class Opal1kCamera;
};

namespace Pds {

  class Src;
  class GenericPool;
  class Opal1kConfig;

  class Opal1kManager : public CameraManager {
  public:
    Opal1kManager(const Src& src);
    ~Opal1kManager();

  private:
    Pds::Damage _handle    ();
    void        _register  ();
    void        _unregister();

  private:
    void _configure(const void*);

    PdsLeutron::PicPortCL& camera();

  private:
    PdsLeutron::Opal1kCamera*  _camera;
    //  buffer management
    bool            _outOfOrder;
    GenericPool*    _occPool;
  };
};

#endif
