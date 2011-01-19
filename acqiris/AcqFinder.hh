#ifndef PDSACQFINDER_HH
#define PDSACQFINDER_HH

#include "acqiris/aqdrv4/AcqirisImport.h"
#include "acqiris/aqdrv4/AcqirisD1Import.h"

namespace Pds {

  class AcqFinder {
  public:
    enum Method { All, MultiInstrumentsOnly };
    AcqFinder(Method);

    ViSession id  (int i) {return (i<numD1Instruments()) ? D1Id(i) : T3Id(i-numD1Instruments());}
    long numInstruments() { return numD1Instruments()+numT3Instruments(); }

    ViSession D1Id(int i) {return _D1Id[i];}
    long numD1Instruments() {return _numD1;}

    ViSession T3Id(int i) {return _T3Id[i];}
    long numT3Instruments() {return _numT3;}
  private:
    ViSession _D1Id[10];
    ViSession _T3Id[10];
    unsigned  _numD1;
    unsigned  _numT3;
  };

}

#endif
