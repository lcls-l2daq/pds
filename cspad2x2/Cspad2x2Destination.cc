/*
 * Cspad2x2Destination.cc
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#include "pds/cspad2x2/Cspad2x2Destination.hh"
#include <stdio.h>
#include "pds/pgp/RegisterSlaveImportFrame.hh"

char* Pds::CsPad2x2::Cspad2x2Destination::name()
{
  static char* _names[NumberOf + 1] = {
      "Quad",
      "Concentrator",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
