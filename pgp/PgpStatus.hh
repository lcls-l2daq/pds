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
#include <stdlib.h>

namespace Pds {

  namespace Pgp {

  class Pgp;

  class PgpStatus {
      public:
        PgpStatus(int f, unsigned d, Pgp* pgp)   {
        	es = (char*)calloc(4096,1);
        	esp = es;
        	_fd = f;
        	_debug = d;
        	_pgp = pgp;
        	p = new PgpCardTx;
        	p->model = sizeof(p);
        };
        virtual ~PgpStatus() {
          delete(p);
          free(es);
        };

      public:
        virtual void              print() = 0; 
        virtual unsigned          checkPciNegotiatedBandwidth() = 0;
        virtual unsigned          getCurrentFiducial() {return 0;}
        virtual bool              getLatestLaneStatus() {return false;}
        virtual bool			        evrEnabled(bool printFlag = false) {return true;}
        virtual char*             errorString() {return es;}
        void                      errorStringAppend(char*);
        void                      clearErrorString();
        int                       fd() { return _fd; }
        Pds::Pgp::Pgp*            pgp() { return _pgp; }

      public:
        char*                     es;

      protected:
        virtual void                 read() = 0;
        char*                        esp;
        static int                   _fd;
        static unsigned              _debug;
        static Pds::Pgp::Pgp*        _pgp;
        static PgpCardTx*            p;
    };

  }

}

#endif /* PGPSTATUS_HH_ */
