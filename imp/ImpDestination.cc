/*
 * ImpDestination.cc
 *
 *  Created on: April 12, 2013
 *      Author: jackp
 */

#include "pds/imp/ImpDestination.hh"
#include <stdio.h>

using namespace Pds::Imp;

char* ImpDestination::name()
{
  // no abstraction possible because of hardware hacking
  static char* _names[NumberOf + 1] = {
      "Data VC, invalid destination",
      "Virtual Circuit 1, invalid destination",
      "Virtual Circuit 2, invalid destination",
      "Command Virtual Circuit 3",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
