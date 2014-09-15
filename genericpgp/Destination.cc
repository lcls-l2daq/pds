/*
 * Destination.cc
 *
 */

#include "pds/genericpgp/Destination.hh"
#include <stdio.h>

using namespace Pds::GenericPgp;

const char* Destination::name()
{
  // no abstraction possible because of hardware hacking
  static const char* _names[NumberOf + 1] = {
      "Data VC, invalid destination",
      "Registers",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
