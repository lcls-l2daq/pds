/*
 * FexampConfigurator.hh
 *
 *  Created on: Nov 6, 2011
 *      Author: jackp
 */

#ifndef FEXAMP_CONFIGURATOR_HH_
#define FEXAMP_CONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/FexampConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/fexamp/FexampStatusRegisters.hh"
#include "pds/fexamp/FexampDestination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

//   <control>
//      <register> <name>regControl</name> <address>10</address> <lane>0</lane> <vc>2</vc> <size>1</size>
//         <field> <bits>0</bits> <label>reset</label> </field>
//         <field> <bits>1</bits> <label>reLink</label> </field>
//         <field> <bits>2</bits> <label>countReset</label> </field>
//      </register>
//       <register> <name>RunControl</name> <address>1</address> <lane>0</lane> <vc>1</vc> <size>1</size>
//         <field> <bits>0</bits><label>SeqCntRst</label></field>
//         <field> <bits>1</bits><label>ExtEnable</label></field>
//      </register>
//      <register>
//         <name>SeqCount</name> <address>2</address> <lane>0</lane> <vc>1</vc> <size>1</size>
//         <field> <bits>31:0</bits><label>SeqCount</label></field>
//      </register>
//   </control>

namespace Pds {

  namespace Fexamp {


    enum resetMasks { MasterReset=1, Relink=2, CountReset=4 };
    enum runControlMasks { SequenceCountResetMask=1, ExternalTriggerEnableMask=2 };
    enum ASICaddrs { ASICbaseAddr=0x8040, ChannelBaseAddr=0x8000, AsicShiftReqAddr=0x8044};
    enum controlAddrs { ResetAddr=10, RunControlAddr=1, SequenceCountAddr=2 };

    class FexampConfigurator : public Pds::Pgp::Configurator {
      public:
        FexampConfigurator(int, unsigned);
        virtual ~FexampConfigurator();

        enum resultReturn {Success=0, Failure=1, Terminate=2};

        unsigned             configure(FexampConfigType*, unsigned mask=0);
        unsigned             configure(unsigned) { return Success; };
        FexampConfigType&    configuration() { return *_config; };
        void                 print();
        void                 dumpFrontEnd();
        void                 printMe();
        uint32_t             testModeState() { return _testModeState; };
        void                 resetFrontEnd(uint32_t);
        void                 resetSequenceCount();
        uint32_t             sequenceCount();
        void                 enableExternalTrigger(bool);


      private:
        unsigned             writeConfig();
        unsigned             checkWrittenConfig(bool writeBack=true);
        unsigned             writeASIC();
        unsigned             checkWrittenASIC(bool writeBack=true);
        bool                 _flush(unsigned);


      private:
        typedef unsigned     LoopHisto[4][10000];
        enum {MicroSecondsSleepTime=50};
        uint32_t                    _testModeState;
        FexampConfigType*           _config;
        FexampDestination           _d;
        FexampStatusRegisters       _statRegs;
        uint32_t                    _runControl;
        unsigned*                   _rhisto;
        //      LoopHisto*                _lhisto;
    };

  }
}

#endif /* FEXAMP_CONFIGURATOR_HH_ */
