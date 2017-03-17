/*
 * PgpStatus.cc
 *
 *  Created: Mon Feb  1 11:08:11 PST 2016
 *      Author: jackp
 */

#include "pds/pgp/PgpStatus.hh"
#include "stdio.h"
#include "string.h"

namespace Pds {
  namespace Pgp {

	  int                PgpStatus::_fd = -1;
	  unsigned           PgpStatus::_debug = 0;
	  Pds::Pgp::Pgp*     PgpStatus::_pgp = 0;
	  PgpCardTx*         PgpStatus::p = (PgpCardTx*)calloc(1, sizeof(PgpCardTx));

	  void PgpStatus::errorStringAppend(char* s) {
	    sprintf(esp, "%s", s);
	    esp = es + strlen(es);
	  }
	  void PgpStatus::clearErrorString() {
	    for(int i=0; i<4096; i++) {
	      es[i] = 0;
	    }
	    esp = es;
	  }
  }
}
