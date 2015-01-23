/*
 * EpixSDestination.cc
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#include "pds/epixS/EpixSDestination.hh"
#include <stdio.h>

using namespace Pds::EpixS;

const char* EpixSDestination::name()
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
