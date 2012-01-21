/*
 * Cspad2x2ConcentratorRegisters.hh
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#ifndef CSPAD2x2CONCENTRATORREGISTERS_HH_
#define CSPAD2x2CONCENTRATORREGISTERS_HH_

#include "pds/cspad2x2/Cspad2x2Configurator.hh"
#include "pds/cspad2x2/Cspad2x2RegisterAddrs.hh"
#include "pds/cspad2x2/Cspad2x2LinkRegisters.hh"
#include "pds/cspad2x2/Cspad2x2Destination.hh"

namespace Pds {
  namespace CsPad2x2 {

    class Cspad2x2ConcentratorRegisters {
      public:
        Cspad2x2ConcentratorRegisters();
        ~Cspad2x2ConcentratorRegisters();

        void print();
        unsigned read();

        static unsigned size() {return sizeof(Cspad2x2ConcentratorRegisters)/sizeof(uint32_t);}
        static void configurator(Cspad2x2Configurator* c) { cfgtr = c; }

      private:
        static Cspad2x2RegisterAddrs* _regLoc; //one for each register
        static unsigned          foo[][2];
        static bool              hasBeenRead;
        static Cspad2x2Destination* dest;

      private:
        static Cspad2x2Configurator* cfgtr;

      public:
        enum {numDSLinks=1, numUSLinks=1};
        uint32_t               version;
        uint32_t               cmdSeqCount;
        uint32_t               cycleTime;
        Cspad2x2LinkRegisters     ds[numDSLinks];
        Cspad2x2LinkRegisters     us[numUSLinks];
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
