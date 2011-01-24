/*
 * CspadConcentratorRegisters.hh
 *
 *  Created on: Jan 21, 2011
 *      Author: jackp
 */

#ifndef CSPADCONCENTRATORREGISTERS_HH_
#define CSPADCONCENTRATORREGISTERS_HH_

#include "pds/csPad/CspadRegisterAddrs.hh"
#include "pds/csPad/CspadLinkRegisters.hh"
#include "pds/csPad/CspadDirectRegisterReader.hh"

namespace Pds {
  namespace CsPad {

    class CspadConcentratorRegisters {
      public:
        CspadConcentratorRegisters();
        ~CspadConcentratorRegisters();

        void print();
        unsigned read(CspadDirectRegisterReader*);

        static unsigned size() {return sizeof(CspadConcentratorRegisters)/sizeof(uint32_t);}

      private:
        static CspadRegisterAddrs* _regLoc; //one for each register
        static unsigned   foo[][2];
        static bool       hasBeenRead;

      public:
        enum {numDSLinks=4, numUSLinks=2};
        uint32_t               version;
        uint32_t               cmdSeqCount;
        uint32_t               cycleTime;
        CspadLinkRegisters     ds[numDSLinks];
        CspadLinkRegisters     us[numUSLinks];
        uint32_t               dsFlowCntrl;
        uint32_t               usFlowCntrl;
        uint32_t               ds00DataCount;
        uint32_t               ds01DataCount;
        uint32_t               us0DataCount;
        uint32_t               ds10DataCount;
        uint32_t               ds11DataCount;
        uint32_t               us1DataCount;
        uint32_t               evrErrors;
    };

  }
}
#endif /* CSPADCONCENTRATORREGISTERS_HH_ */
