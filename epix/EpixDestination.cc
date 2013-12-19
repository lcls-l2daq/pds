/*
 * EpixDestination.cc
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#include "pds/epix/EpixDestination.hh"
#include <stdio.h>

using namespace Pds::Epix;

const char* EpixDestination::name()
{
  // no abstraction possible because of hardware hacking
  static const char* _names[NumberOf + 1] = {
      "Data VC, invalid destination",
      "Registers",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
