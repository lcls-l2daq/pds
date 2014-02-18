#ifndef Pds_PimManager_hh
#define Pds_PimManager_hh

#include "pds/camera/TM6740Manager.hh"

namespace Pds {

  class CfgCache;

  class PimManager : public TM6740Manager {
  public:
    PimManager(const Src& src);
    ~PimManager();

  public:
    virtual void        allocate         (Transition* tr);
    virtual bool        doConfigure      (Transition* tr);
    virtual InDatagram* recordConfigure  (InDatagram* in);

  private:
    CfgCache* _fexConfig;
  };
};

#endif
