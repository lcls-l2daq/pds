#ifndef Pds_ClockTime_hh
#define Pds_ClockTime_hh

namespace Pds {

  class ClockTime {
  public:
    ClockTime() {}
    ClockTime(const ClockTime& t) : low(t.low), high(t.high) {}
    ClockTime(unsigned sec,
	      unsigned nsec) : low(nsec), high(sec) {}

    unsigned seconds    () const { return high; }
    unsigned nanoseconds() const { return low; }

    ClockTime& operator=(const ClockTime&);

    unsigned low;
    unsigned high;
  };
}


inline Pds::ClockTime& Pds::ClockTime::operator=(const Pds::ClockTime& input)
{
  low  = input.low;
  high = input.high;
  return *this;
}

#endif
