/*
 * ImpDestination.cc
 *
 *  Created on: April 12, 2013
 *      Author: jackp
 */

#include "pds/imp/ImpDestination.hh"
#include <stdio.h>

using namespace Pds::Imp;

const char* ImpDestination::name()
{
  // no abstraction possible because of hardware hacking
  static const char* _names[NumberOf + 1] = {
      "Data VC, INVALID destination",
      "Virtual Circuit 1, INVALID destination",
      "Virtual Circuit 2, INVALID destination",
      "Command Virtual Circuit 3",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
