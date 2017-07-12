#ifndef Pds_QualifiedControl_hh
#define Pds_QualifiedControl_hh

#include "pds/management/PartitionControl.hh"

namespace Pds {
  class RunAllocator;
  class QualifiedControl : public PartitionControl {
  public:
    QualifiedControl(unsigned         platform,
                     ControlCallback& cb,
                     Routine*         tmo=0);
  public:
    void set_target_state(PartitionControl::State);
    void enable (PartitionControl::State,bool);
  public:
    bool enabled(PartitionControl::State) const;
  private:
    bool _enabled[PartitionControl::NumberOfStates];
    PartitionControl::State _unqualified_target_state;
  };
};

#endif
