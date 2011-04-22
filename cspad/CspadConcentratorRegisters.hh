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
