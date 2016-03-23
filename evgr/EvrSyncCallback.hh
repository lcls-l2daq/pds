#ifndef Pds_EvrSyncCallback_hh
#define Pds_EvrSyncCallback_hh

//
//  Abstract class to callback with the target fiducial and
//  EVR action to be taken for potential triggers following
//  the target fiducial.
//
namespace Pds {

  class EvrSyncCallback {
  public:
    virtual ~EvrSyncCallback() {}
  public:
    virtual void initialize(unsigned fiducial,
                            bool     enable) = 0;
  };
};

#endif
