/*
 * Epix10kDestination.cc
 *
 *  Created on: 2014.4.15
 *      Author: jackp
 */

#include "pds/epix10k/Epix10kDestination.hh"
#include <stdio.h>

using namespace Pds::Epix10k;

const char* Epix10kDestination::name()
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
