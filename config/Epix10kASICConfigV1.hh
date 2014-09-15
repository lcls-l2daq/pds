/*
 * Epix10kASICConfigV1.hh
 *
 *  Created on: 2014.4.17
 *      Author: jackp
 */

#ifndef EPIX10KASICCONFIGV1_HH_
#define EPIX10KASICCONFIGV1_HH_

#include "pdsdata/psddl/epix.ddl.h"
#include <stdint.h>
#include <string.h>

namespace Pds {
  namespace Epix10kConfig {

    class ASIC_ConfigV1 {
      public:
        ASIC_ConfigV1(bool init=false);
        ~ASIC_ConfigV1();

         enum Registers {
            CompTH_DAC,
            CompEn_0,
            PulserSync,
            dummyTest,
            dummyMask,
            dummyG,
            dummyGA,
            dummyUpper12bits,
            pulser,
            pbit,
            atest,
            test,
            sabTest,
            hrTest,
            PulserR,
            digMon1,
            digMon2,
            pulserDac,
            monostPulser,
            CompEn_1,
            CompEn_2,
            Dm1En,
            Dm2En,
            emph_bd,
            emph_bc,
            VREFDac,
            VrefLow,
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
            Pixel_CB,
            Vld1_b,
            S2D_tcomp,
            FilterDac,
            testLVDTransmitter,
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
            RO_rst_en,
            SLVDSBit,
            FELmode,
            CompEnOn,
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
        void                        clear    ();
        static char*                name     (Registers);
        static void                 initNames();
        static uint32_t             rangeHigh(Registers);
        static uint32_t             rangeLow (Registers);
        static uint32_t             defaultValue(Registers);
        static unsigned             readOnly(Registers);
        static unsigned             address(unsigned);
        static unsigned             mask   (unsigned);
        static readOnlyStates       use    (unsigned);
        void                        operator=(ASIC_ConfigV1&);
        bool                        operator==(ASIC_ConfigV1&);
        bool                        operator!=(ASIC_ConfigV1& foo) { return !(*this==foo); }

      private:
        uint32_t              _values[NumberOfValues];
    };

  } /* namespace Epix10kConfig */
} /* namespace Pds */

#endif /* EPIX10KASICCONFIGV1_HH_ */
