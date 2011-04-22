/*
 * CspadQuadRegisters.hh
 *
 *  Created on: Jan 19, 2011
 *      Author: jackp
 */

#ifndef XAMPSQUADREGISTERS_HH_
#define XAMPSQUADREGISTERS_HH_

#include "pds/cspad/CspadConfigurator.hh"
#include "pds/cspad/CspadLinkCounters.hh"
#include "pds/cspad/CspadRegisterAddrs.hh"
#include "pds/cspad/CspadDestination.hh"

namespace Pds {
  namespace CsPad {
    class CspadConfigurator;
    class CspadRegisterAddrs;

    class CspadQuadRegisters {

      public:
        CspadQuadRegisters();
        ~CspadQuadRegisters();
        void print();
        void printTemps();
        unsigned read(unsigned);

      public:
        static unsigned size() {return sizeof(CspadQuadRegisters)/sizeof(uint32_t);}
        static void configurator(CspadConfigurator* c) { cfgtr = c; }

      private:
        static CspadRegisterAddrs*  _regLoc;
        static unsigned             _foo[][2];
        static CspadDestination*         _d;

      private:
        static CspadConfigurator* cfgtr;

      public:
        enum {numbTemps=4};
        uint32_t           acqTimer;
        uint32_t           acqCount;
        uint32_t           cmdCount;
        uint32_t           shiftTest;  // also read only config
        uint32_t           version;    // also read only config
        CspadLinkCounters  RxCounters;
        uint32_t           readTimer;
        uint32_t           bufferError;
        uint32_t           temperatures[numbTemps];
    };

  }
}

#endif /* CSPADQUADREGISTERS_HH_ */
