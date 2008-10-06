#ifndef STATE_HH
#define STATE_HH

namespace Pds {
class State {
public:
  enum Id {
    Unknown,
    Standby,
    Partitioned,
    Configured,
    Ready,
    Enabled,
    Paused,
  };
};
}

#endif
