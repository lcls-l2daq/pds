/*
 * PgpStatus.hh
 *
 *  Created on: Jan 12, 2016
 *      Author: jackp
 */

#ifndef PGPSTATUS_HH_
#define PGPSTATUS_HH_

#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <iostream>
#include "pds/pgp/Destination.hh"
#include "pds/pgp/Pgp.hh"
#include "pgpcard/PgpCardMod.h"
#include <time.h>

namespace Pds {

  namespace Pgp {

  class Pgp;

  class PgpStatus {
      public:
        PgpStatus(int f, unsigned d, Pgp* pgp)   {
        	_fd = f;
        	_debug = d;
        	_pgp = pgp;
        	p = new PgpCardTx;
        	p->model = sizeof(p);
        };
        virtual ~PgpStatus() {};

      public:
        virtual void              print() = 0; 
        virtual unsigned          checkPciNegotiatedBandwidth() = 0;
        int                       fd() { return _fd; }
        Pds::Pgp::Pgp*            pgp() { return _pgp; }

      protected:
        virtual void                 read() = 0;
        static int                   _fd;
        static unsigned              _debug;
        static Pds::Pgp::Pgp*        _pgp;
        static PgpCardTx*            p;
    };

  }

}

#endif /* PGPSTATUS_HH_ */
