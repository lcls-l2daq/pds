/*
 * Configurator.hh
 *
 */

#ifndef GenericPgp_CONFIGURATOR_HH_
#define GenericPgp_CONFIGURATOR_HH_

#include "pds/pgp/Configurator.hh"
#include "pds/config/GenericPgpConfigType.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/genericpgp/Destination.hh"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <new>

namespace Pds {
  namespace GenericPgp {
    class Configurator : public Pds::Pgp::Configurator {
      public:
        Configurator(int, unsigned);
        virtual ~Configurator();

        enum resultReturn {Success=0, Failure=1, Terminate=2};

        unsigned             configure(GenericPgpConfigType*, unsigned first=0);
        GenericPgpConfigType& configuration() { return *_config; };
      //        void                 print();
        void                 dumpFrontEnd();
      //        void                 printMe();
        void                 runTimeConfigName(char*);
      //        uint32_t             testModeState() { return _testModeState; };
      //        void                 resetFrontEnd();
      //        void                 resetSequenceCount();
      //        uint32_t             sequenceCount();
      //        uint32_t             acquisitionCount();
      void                 start();
      void                 stop ();
      //        void                 enableRunTrigger(bool);

    private:
      unsigned             _execute_sequence(unsigned         len,
                                             const CRegister* seq,
                                             uint32_t*        payload);
      bool                 _flush(unsigned index=0);


    private:
      // typedef unsigned     LoopHisto[4][10000];
      enum {MicroSecondsSleepTime=50};
      uint32_t                    _testModeState;
      GenericPgpConfigType*       _config;
      Destination                 _d;
      unsigned*                   _rhisto;
      char                        _runTimeConfigFileName[256];
      //      LoopHisto*                _lhisto;
    };
  }
}

#endif /* EPIX_CONFIGURATOR_HH_ */
