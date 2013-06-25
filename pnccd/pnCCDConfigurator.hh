/*
 * pnCCDConfigurator.hh
 *
 *  Created on: May 30, 2013
 *      Author: jackp
 */

#ifndef PNCCD_CONFIGURATOR_HH_
#define PNCCD_CONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>


namespace Pds {

  namespace pnCCD {



    class pnCCDConfigurator : public Pds::Pgp::Configurator {
      public:
        pnCCDConfigurator(int, unsigned);
        virtual ~pnCCDConfigurator();

        enum resultReturn {Success=0, Failure=1, Terminate=2};

        unsigned             configure(pnCCDConfigType*, unsigned mask=0);
        pnCCDConfigType&    configuration() { return *_config; };
        void                 print();
        void                 dumpFrontEnd();
        void                 printMe();
        void                 runTimeConfigName(char*);

      private:
        bool                 _flush(unsigned);


      private:
        typedef unsigned     LoopHisto[4][10000];
        enum {MicroSecondsSleepTime=50};
        pnCCDConfigType*           _config;
        char                     _runTimeConfigFileName[256];
        unsigned*                _rhisto;
        //      LoopHisto*                _lhisto;
    };

  }
}

#endif /* PNCCD_CONFIGURATOR_HH_ */
