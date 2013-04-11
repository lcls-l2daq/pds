/*
 * XampsDestination.cc
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#include "pds/xamps/XampsDestination.hh"
#include <stdio.h>

using namespace Pds::Xamps;

     const char* XampsDestination::name()
     {
        static const char* _names[NumberOf + 1] = {
         "External Register",
         "Internal Register",
         "--INVALID--"
       };
       return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
     }
