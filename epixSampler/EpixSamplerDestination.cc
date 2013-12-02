/*
 * EpixSamplerDestination.cc
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#include "pds/epixSampler/EpixSamplerDestination.hh"
#include <stdio.h>

using namespace Pds::EpixSampler;

const char* EpixSamplerDestination::name()
{
  static const char* _names[NumberOf + 1] = {
      "Data VC, invalid destination",
      "Registers",
      "--INVALID--"
  };
  return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
}
