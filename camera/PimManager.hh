#ifndef Pds_PimManager_hh
#define Pds_PimManager_hh

#include "pds/camera/TM6740Manager.hh"

namespace Pds {

  class CfgCache;

  class PimManager : public TM6740Manager {
  public:
    PimManager(const Src& src, unsigned grabberId=0);
    ~PimManager();

  public:
    virtual void        doConfigure      (Transition* tr);
    virtual InDatagram* recordConfigure  (InDatagram* in);

  private:
    CfgCache* _fexConfig;
  };
};

#endif
