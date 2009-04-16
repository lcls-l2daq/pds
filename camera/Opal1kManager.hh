#ifndef Pds_Opal1kManager_hh
#define Pds_Opal1kManager_hh

#include "pds/camera/CameraManager.hh"
#include "pds/config/Opal1kConfigType.hh"

namespace PdsLeutron {
  class Opal1kCamera;
};

namespace Pds {

  class Src;
  class GenericPool;

  class Opal1kManager : public CameraManager {
  public:
    Opal1kManager(const Src& src);
    ~Opal1kManager();

  private:
    Pds::Damage _handle    ();
    void        _register  ();
    void        _unregister();

  private:
    void _configure(char*);
    void _configure(InDatagram*);

    PdsLeutron::PicPortCL& camera();
    const TypeId& camConfigType();

  private:
    PdsLeutron::Opal1kCamera* _camera;
    const Opal1kConfigType*   _configdata;
    Xtc                       _configtc;
    //  buffer management
    bool            _outOfOrder;
    GenericPool*    _occPool;
  };
};

#endif
