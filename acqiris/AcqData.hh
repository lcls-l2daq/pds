#ifndef PDSACQDATA_HH
#define PDSACQDATA_HH

#include "AcqTimestamp.hh"

// the data structure here (PER CHANNEL) is
// 1 Data Descriptor
// N Segment Descriptors
// Waveforms for each segment

namespace Pds {

  class WaveformV1;

  class AcqData {
  public:
    AqDataDescriptor& desc() {
      return _desc;
    }
    unsigned nbrSamplesInSeg() {
      return _desc.returnedSamplesPerSeg;
    }
    unsigned nbrSegments() {
      return _desc.returnedSegments;
    }
    AcqTimestamp& timestamp(unsigned seg) {
      return _timestamp()[seg];
    }
    WaveformV1* waveforms(unsigned expectedSegments) {
      return (WaveformV1*)(&(_timestamp()[expectedSegments]));
    }
    AcqData* nextChannel(unsigned expectedSegments,
                         unsigned expectedSamplesInSeg) {
      return (AcqData*)((char*)(waveforms(expectedSegments))+
        expectedSamplesInSeg*expectedSegments*sizeof(short)+_extra);
    }
  private:
    static const unsigned _extra=32*sizeof(short);
    AcqTimestamp* _timestamp() {
      return (AcqTimestamp*)(this+1);
    }
    AqDataDescriptor _desc;
  };

}

#endif
