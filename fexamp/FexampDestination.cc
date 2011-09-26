/*
 * FexampDestination.cc
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#include "pds/fexamp/FexampDestination.hh"
#include <stdio.h>

using namespace Pds::Fexamp;

char* FexampDestination::name()
{
  // no abstraction possible because of hardware hacking
  static char* _names[NumberOf + 1] = {
      "Data VC, invalid destination",
      "Virtual Circuit 1",
      "Virtual Circuit 2",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
