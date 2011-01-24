/*
 * CspadQuadRegisters.hh
 *
 *  Created on: Jan 19, 2011
 *      Author: jackp
 */

#ifndef CSPADQUADREGISTERS_HH_
#define CSPADQUADREGISTERS_HH_

#include "pds/csPad/CspadConfigurator.hh"
#include "pds/csPad/CspadLinkCounters.hh"
#include "pds/csPad/CspadRegisterAddrs.hh"
#include "pds/csPad/CspadDirectRegisterReader.hh"

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
        unsigned read(unsigned, CspadDirectRegisterReader*);

      public:
        static unsigned size() {return sizeof(CspadQuadRegisters)/sizeof(uint32_t);}

      private:
        static CspadRegisterAddrs*  _regLoc;
        static unsigned             _foo[][2];

      private:
        static unsigned foo[][2];

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
