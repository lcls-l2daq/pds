#ifndef Pds_CameraManager_hh
#define Pds_CameraManager_hh

#include "pdsdata/xtc/Xtc.hh"

namespace PdsLeutron {
  class PicPortCL;
};

namespace Pds {

  class Appliance;

  class CfgCache;
  class Fsm;
  class DmaSplice;
  class Transition;
  class InDatagram;
  class FrameServer;
  class GenericPool;

  class CameraManager {
  public:
    CameraManager(const Src&,
		  CfgCache*);
    virtual ~CameraManager();

    Appliance&      appliance();

    virtual FrameServer& server() = 0;

    // Unix signal handler
    static void sigintHandler(int);
    
  public:
    virtual void allocate      (Transition* tr);
    virtual void doConfigure   (Transition* tr);
    virtual void nextConfigure (Transition* tr);
    virtual void unconfigure   (Transition* tr);

    virtual InDatagram* recordConfigure  (InDatagram* in);

  public:
    void handle();
  private:
    void register_ (int);
    void unregister();

  private:
    virtual void _configure   (const void* tc)=0;

    virtual Pds::Damage _handle() { return 0; }
    virtual void _register  () {}
    virtual void _unregister() {}

  public:
    virtual void attach_camera() = 0;
    virtual void detach_camera() = 0;
  private:
    virtual PdsLeutron::PicPortCL& camera() = 0;

  protected:
    DmaSplice*      _splice;
  private:
    const Src&      _src;
    Fsm*            _fsm;
    int             _sig;
    CfgCache*       _camConfig;
    bool            _configured;
    GenericPool*    _occPool;
  protected:
    unsigned        _nposts;
  };
};

#endif
