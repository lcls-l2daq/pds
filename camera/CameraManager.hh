#ifndef Pds_CameraManager_hh
#define Pds_CameraManager_hh

#include "pdsdata/xtc/Xtc.hh"

namespace Pds {

  class Appliance;

  class CameraDriver;
  class CfgCache;
  class Fsm;
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

  private:
    virtual void _configure   (const void* tc)=0;

  public:
    void          attach(CameraDriver* d);
    void          detach();
    CameraDriver& driver();

  private:
    const Src&      _src;
    Fsm*            _fsm;
    CfgCache*       _camConfig;
    CameraDriver*   _driver;
    bool            _configured;
    GenericPool*    _occPool;
  };
};

#endif
