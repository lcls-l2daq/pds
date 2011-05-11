/*
 * Configurator.hh
 *
 *  Created on: Apr 6, 2011
 *      Author: jackp
 */

#ifndef CONFIGURATOR_HH_
#define CONFIGURATOR_HH_

#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/Pgp.hh"
#include <time.h>

namespace Pds {

  namespace Pgp {

    class AddressRange {
      public:
        AddressRange() {};
        AddressRange(uint32_t b, uint32_t l) : base(b), load(l) {};
        ~AddressRange() {};

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
        virtual unsigned          configure(unsigned) = 0;
        virtual void              dumpFrontEnd() = 0;
        int                       fd() { return _fd; }
        void                      microSpin(unsigned);
        long long int             timeDiff(timespec*, timespec*);
        Pds::Pgp::Pgp*            pgp() { return _pgp; }

      protected:
        friend class ConfigSynch;
        int                         _fd;
        unsigned                    _debug;
        Pds::Pgp::Pgp*              _pgp;

    };

    class ConfigSynch {
      public:
        ConfigSynch(int fd, uint32_t d, Configurator* c, unsigned s) :
          _depth(d), _length(d), _size(s), _fd(fd), _cfgrt(c) {};
        bool take();
        bool clear();

      private:
        enum {Success=0, Failure=1};
        unsigned _getOne();
        unsigned _depth;
        unsigned _length;
        unsigned _size;
        int       _fd;
        Configurator* _cfgrt;
    };

  }

}

#endif /* CONFIGURATOR_HH_ */