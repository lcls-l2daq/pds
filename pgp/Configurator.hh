/*
 * Configurator.hh
 *
 *  Created on: Apr 6, 2011
 *      Author: jackp
 */

#ifndef CONFIGURATOR_HH_
#define CONFIGURATOR_HH_

#include "pds/pgp/Pgp.hh"
#include <time.h>

namespace Pds {

  namespace Pgp {

#define rdtscll(val) do { \
     unsigned int __a,__d; \
     asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
     (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
} while(0)

   class AddressRange {
      public:
        AddressRange() {};
        AddressRange(uint32_t b, uint32_t l) : base(b), load(l) {};
        ~AddressRange() {};
      public:
        uint32_t base;
        uint32_t load;
    };

    class ConfigSynch;

    class Configurator {
      public:
        Configurator(int, unsigned);
        virtual ~Configurator();

      public:
        virtual void              print();
        virtual void              dumpFrontEnd() = 0;
        void                      dumpPgpCard();
        int                       fd() { return _fd; }
        void                      fd(int f) { _fd = f; }
        void                      microSpin(unsigned);
        long long int             timeDiff(timespec*, timespec*);
        Pds::Pgp::Pgp*            pgp() { return _pgp; }
        unsigned                  checkPciNegotiatedBandwidth();
        unsigned                  getCurrentFiducial(bool pr = false);
        bool                      getLatestLaneStatus() {return _pgp->getLatestLaneStatus();}
        void                      loadRunTimeConfigAdditions(char *);
        int                       fiducialTarget(unsigned);
        int                       waitForFiducialMode(bool);
        int                       evrRunCode(unsigned);
        int                       evrRunDelay(unsigned);
        int                       evrDaqCode(unsigned);
        int                       evrDaqDelay(unsigned);
        int                       evrEnable(bool);
        int                       evrLaneEnable(bool);
        int                       evrEnableHdrChk(unsigned, bool);
        bool                      G3Flag() { return _G3; }
        bool                      evrEnabled();

      protected:
        friend class ConfigSynch;
        int                         _fd;
        unsigned                    _debug;
        Pds::Pgp::Pgp*              _pgp;
        bool						            _G3;
    };

    class ConfigSynch {
      public:
        ConfigSynch(int fd, uint32_t d, Configurator* c, unsigned s) :
          _depth(d), _length(d), _size(s), _fd(fd), _cfgrt(c), _printFlag(true) {};
        bool take();
        bool clear();
        unsigned depth() { return _depth; }

      private:
        enum {Success=0, Failure=1};
        unsigned             _getOne();
        unsigned             _depth;
        unsigned             _length;
        unsigned             _size;
        int                  _fd;
        Configurator*        _cfgrt;
        bool                 _printFlag;
    };

  }

}

#endif /* CONFIGURATOR_HH_ */
