/*
 * PgpStatus.cc
 *
 *  Created: Mon Feb  1 11:08:11 PST 2016
 *      Author: jackp
 */

#include "pds/pgp/PgpStatus.hh"
#include "stdio.h"
#include "stdlib.h"

namespace Pds {
  namespace Pgp {

	  int                PgpStatus::_fd = -1;
	  unsigned           PgpStatus::_debug = 0;
	  Pds::Pgp::Pgp*     PgpStatus::_pgp = 0;
	  PgpCardTx*         PgpStatus::p = (PgpCardTx*)calloc(1, sizeof(PgpCardTx));

  }
}
