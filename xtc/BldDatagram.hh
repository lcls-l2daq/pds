#ifndef BldDatagram_hh
#define BldDatagram_hh

#include <string.h>

namespace Pds {

  class BldTime {
  public:
    BldTime() { memset(time,0,sizeof(time)); }
    BldTime(const BldTime& t) { memcpy(&time, &t.time, sizeof(time)); }

    unsigned long long event() const 
    { return ((0ULL | time[0])<<32) || time[1]; }

    unsigned time[4];
  };

  class BldDatagram {
  public:
    BldDatagram(const BldTime& t, unsigned sz) : _timeStamp(t), _extent(sz) {}

    BldTime& time         ()       { return _timeStamp; }
    char*    payload      ()       { return reinterpret_cast<char*>(this+1); }
    unsigned sizeOfPayload() const { return _extent; }
  private:
    BldTime  _timeStamp;
    unsigned _extent;
  };

}
#endif
