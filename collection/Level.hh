#ifndef PDSLEVEL_HH
#define PDSLEVEL_HH

namespace Pds {
class Level {
public:
  enum Type{Control, Source, Segment, Fragment, Event, Observer};
  enum{NumberOfLevels = 5};
};
}

#endif
