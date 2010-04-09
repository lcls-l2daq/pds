#ifndef PDSACQFINDER_HH
#define PDSACQFINDER_HH

#include "acqiris/aqdrv4/AcqirisD1Import.h"

namespace Pds {

  class AcqFinder {
  public:
    AcqFinder();
    ViSession id(unsigned i) {return _instrumentId[i];}
    long numInstruments() {return _numInstruments;}
  private:
    ViSession _instrumentId[10];
    ViInt32   _numInstruments;
  };

}

#endif
