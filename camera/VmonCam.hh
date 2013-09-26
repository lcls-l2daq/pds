#ifndef Pds_VmonCam_hh
#define Pds_VmonCam_hh

namespace Pds {
  class MonEntryTH1F;
  class VmonCam {
  public:
    VmonCam(unsigned nbuffers);
    ~VmonCam();
  public:
    void event(unsigned depth);
  private:
    MonEntryTH1F* _depth;
    MonEntryTH1F* _interval;
    MonEntryTH1F* _interval_w;
    unsigned      _sample_sec;
    unsigned      _sample_nsec;
  };
};

#endif
