/*
 * Cspad2x2QuadRegisters.hh
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#ifndef CSPAD2x2QUADREGISTERS_HH_
#define CSPAD2x2QUADREGISTERS_HH_

#include "pds/cspad2x2/Cspad2x2Configurator.hh"
#include "pds/cspad2x2/Cspad2x2LinkCounters.hh"
#include "pds/cspad2x2/Cspad2x2RegisterAddrs.hh"
#include "pds/cspad2x2/Cspad2x2Destination.hh"

namespace Pds {
  namespace CsPad2x2 {
    class Cspad2x2Configurator;
    class Cspad2x2RegisterAddrs;

    class Cspad2x2QuadRegisters {

      public:
        Cspad2x2QuadRegisters();
        ~Cspad2x2QuadRegisters();
        void print();
        void printTemps();
        unsigned read();

      public:
        static unsigned size() {return sizeof(Cspad2x2QuadRegisters)/sizeof(uint32_t);}
        static void configurator(Cspad2x2Configurator* c) { cfgtr = c; }

      private:
        static Cspad2x2RegisterAddrs*  _regLoc;
        static unsigned             _foo[][2];
        static Cspad2x2Destination*         _d;

      private:
        static Cspad2x2Configurator* cfgtr;

      public:
        enum {numbTemps=4};
        uint32_t           acqTimer;
        uint32_t           acqCount;
        uint32_t           cmdCount;
        uint32_t           shiftTest;  // also read only config
        uint32_t           version;    // also read only config
        Cspad2x2LinkCounters  RxCounters;
        uint32_t           readTimer;
        uint32_t           bufferError;
        uint32_t           temperatures[numbTemps];
    };

  }
}

#endif /* CSPAD2x2QUADREGISTERS_HH_ */
