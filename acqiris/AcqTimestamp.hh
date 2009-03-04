#ifndef PDSACQTIMESTAMP_HH
#define PDSACQTIMESTAMP_HH

namespace Pds {

  class AcqTimestamp {
  public:
    AqSegmentDescriptor& desc() {return _desc;}
    unsigned long long value() const {
      unsigned long long ts = _desc.timeStampHi;
      ts = (ts<<32) + (unsigned long)(_desc.timeStampLo);
      return ts;
    }
    unsigned long long operator-(const AcqTimestamp& ts) {
      return value()-ts.value();
    }
  private:
    AqSegmentDescriptor _desc;
  };

}

#endif
