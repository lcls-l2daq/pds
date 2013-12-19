/*
 * EpixASICConfigV1.hh
 *
 *  Created on: Nov 6, 2013
 *      Author: jackp
 */

#ifndef EPIXASICCONFIGV1_HH_
#define EPIXASICCONFIGV1_HH_

#include "pdsdata/psddl/epix.ddl.h"
#include <stdint.h>

namespace Pds {
  namespace EpixConfig {

    class ASIC_ConfigV1 {
      public:
        ASIC_ConfigV1(bool init=false);
        ~ASIC_ConfigV1();

         enum Registers {
          monostPulser,
          dummyTest,
          dummyMask,
          pulser,
          pbit,
          atest,
          test,
          sabTest,
          hrTest,
          digMon1,
          digMon2,
          pulserDac,
          Dm1En,
          Dm2En,
          slvdSBit,
          VRefDac,
          TpsTComp,
          TpsMux,
          RoMonost,
          TpsGr,
          S2dGr,
          PpOcbS2d,
          Ocb,
          Monost,
          FastppEnable,
          Preamp,
          PixelCb,
          S2dTComp,
          FilterDac,
          TC,
          S2d,
          S2dDacBias,
          TpsTcDac,
          TpsDac,
          S2dTcDac,
          S2dDac,
          testBE,
          IsEn,
          DelExec,
          DelCckReg,
          RowStartAddr,
          RowStopAddr,
          ColStartAddr,
          ColStopAddr,
          chipID,
          NumberOfRegisters
        };

        enum readOnlyStates { ReadWrite=0, ReadOnly=1, UseOnly=2, WriteOnly=3 };

        enum {
          NumberOfValues=21
        };

        unsigned                    get      (Registers) const;
        void                        set      (Registers, unsigned);
        static char*                name     (Registers);
        static void                 initNames();
        static uint32_t             rangeHigh(Registers);
        static uint32_t             rangeLow (Registers);
        static uint32_t             defaultValue(Registers);
        static unsigned             readOnly(Registers);
        void                        operator=(ASIC_ConfigV1&);
        bool                        operator==(ASIC_ConfigV1&);
        bool                        operator!=(ASIC_ConfigV1& foo) { return !(*this==foo); }

      private:
        uint32_t              _values[NumberOfValues];
    };

  } /* namespace EpixConfig */
} /* namespace Pds */

#endif /* EPIXASICCONFIGV1_HH_ */
