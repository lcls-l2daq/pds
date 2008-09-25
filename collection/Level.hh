#ifndef PDSLEVEL_HH
#define PDSLEVEL_HH

namespace Pds {
class Level {
public:
  enum Type{Control, Source, Segment, Event, Recorder, Observer};
  enum{NumberOfLevels = 5};
};
}

#endif
