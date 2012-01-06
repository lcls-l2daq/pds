/*
 * PhasicsConfigurator.hh
 *
 *  Created on: Nov 6, 2011
 *      Author: jackp
 */

#ifndef FEXAMP_CONFIGURATOR_HH_
#define FEXAMP_CONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/PhasicsConfigType.hh"
#include <dc1394/dc1394.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

namespace Pds {

  namespace Phasics {


    enum resetMasks { MasterReset=1, Relink=2, CountReset=4 };

    class PhasicsConfigurator  {
      public:
        PhasicsConfigurator();
        virtual ~PhasicsConfigurator();

        enum resultReturn {Success=0, Failure=1, Terminate=2};

        unsigned             configure(PhasicsConfigType*);
        long long int        timeDiff(timespec*, timespec*);
        PhasicsConfigType&   configuration() { return *_config; };
        void                 print();
        void                 printMe();
        dc1394_t*            getD() { return d; };

      private:
        void                       printError(char* m, unsigned* rp);
        dc1394camera_t*            _camera;
        dc1394error_t              err;
        dc1394_t*                  d;
        bool                       _flush(unsigned);


      private:
        typedef unsigned            LoopHisto[4][10000];
        PhasicsConfigType*           _config;
        unsigned*                   _rhisto;
        //      LoopHisto*                _lhisto;
    };

  }
}

#endif /* FEXAMP_CONFIGURATOR_HH_ */
