#ifndef Pds_VmonEvr_hh
#define Pds_VmonEvr_hh

namespace Pds {

  class Src;
  class MonEntryScalar;

  class VmonEvr {
  public:
    VmonEvr(const Src&);
    ~VmonEvr();
  public:
    void readout   (unsigned group);
    void queue     (int q0, int q1,
                    int q2, int q3);
    void reset     ();
  private:
    MonEntryScalar*   _readouts;
    MonEntryScalar*   _queues;
  };
};

#endif
