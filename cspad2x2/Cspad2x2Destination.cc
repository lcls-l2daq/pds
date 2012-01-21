/*
 * Cspad2x2Destination.cc
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#include "pds/cspad2x2/Cspad2x2Destination.hh"
#include <stdio.h>
#include "pds/pgp/RegisterSlaveImportFrame.hh"


Pds::CsPad2x2::Cspad2x2Destination::FEdest Pds::CsPad2x2::Cspad2x2Destination::getDest(
    Pds::Pgp::RegisterSlaveImportFrame* rsif) {
  unsigned ret = 0;

  if (rsif->bits._vc==0) {
    ret = concentratorMask;
  }
  if ((rsif->bits._lane) || (rsif->bits._vc > CR)) {
    printf("ERROR ERROR bad dest lane(%u), vc(%u)\n", rsif->bits._lane, rsif->bits._vc);
    ret = 2;
  }
  return (Cspad2x2Destination::FEdest) ret;
}

char* Pds::CsPad2x2::Cspad2x2Destination::name()
{
  static char* _names[NumberOf + 1] = {
      "Quad_0",
      "Concentrator",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
