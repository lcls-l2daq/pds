/*
 * CspadConcentratorRegisters.hh
 *
 *  Created on: Jan 21, 2011
 *      Author: jackp
 */

#ifndef CSPADCONCENTRATORREGISTERS_HH_
#define CSPADCONCENTRATORREGISTERS_HH_

#include "pds/cspad/CspadConfigurator.hh"
#include "pds/cspad/CspadRegisterAddrs.hh"
#include "pds/cspad/CspadLinkRegisters.hh"
#include "pds/cspad/CspadDestination.hh"


//The upstream status register bits:
//
//   0  - Upstream lane 0 VC0 local buffer almost full
//   1  - Upstream lane 0 VC0 local buffer full
//   2  - Upstream lane 0 VC0 remote buffer almost full
//   3  - Upstream lane 0 VC0 remote buffer full

//   4  - Upstream lane 0 VC1 local buffer almost full
//   5  - Upstream lane 0 VC1 local buffer full
//   6  - Upstream lane 0 VC1 remote buffer almost full
//   7  - Upstream lane 0 VC1 remote buffer full

//   8  - Upstream lane 0 VC2 local buffer almost full
//   9  - Upstream lane 0 VC2 local buffer full
//   10 - Upstream lane 0 VC2 remote buffer almost full
//   11 - Upstream lane 0 VC2 remote buffer full

//   12 - Upstream lane 0 VC3 local buffer almost full
//   13 - Upstream lane 0 VC3 local buffer full
//   14 - Upstream lane 0 VC3 remote buffer almost full
//   15 - Upstream lane 0 VC3 remote buffer full

//   16 - '0'
//   17 - '0'
//   18 - '0'
//   19 - '0'

//   20 - Upstream lane 1 VC1 local buffer almost full
//   21 - Upstream lane 1 VC1 local buffer full
//   22 - Upstream lane 1 VC1 remote buffer almost full
//   23 - Upstream lane 1 VC1 remote buffer full

//   24 - Upstream lane 1 VC2 local buffer almost full
//   25 - Upstream lane 1 VC2 local buffer full
//   26 - Upstream lane 1 VC2 remote buffer almost full
//   27 - Upstream lane 1 VC2 remote buffer full

//   28 - '0'
//   29 - '0'
//   30 - Upstream lane 1 VC3 remote buffer almost full
//   31 - Upstream lane 1 VC3 remote buffer full


namespace Pds {
  namespace CsPad {

    class CspadConcentratorRegisters {
      public:
        CspadConcentratorRegisters();
        ~CspadConcentratorRegisters();

        void print();
        unsigned read();

        static unsigned size() {return sizeof(CspadConcentratorRegisters)/sizeof(uint32_t);}
        static void configurator(CspadConfigurator* c) { cfgtr = c; }

      private:
        static CspadRegisterAddrs* _regLoc; //one for each register
        static unsigned          foo[][2];
        static bool              hasBeenRead;
        static CspadDestination* dest;

      private:
        static CspadConfigurator* cfgtr;

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
