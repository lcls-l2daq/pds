/*
 * Epix100aDestination.cc
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#include "pds/epix100a/Epix100aDestination.hh"
#include <stdio.h>

using namespace Pds::Epix100a;

const char* Epix100aDestination::name()
{
  // no abstraction possible because of hardware hacking
  static const char* _names[NumberOf + 1] = {
      "Data VC, invalid destination",
      "Registers",
      "Oscilloscope",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
