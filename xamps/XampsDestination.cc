/*
 * XampsDestination.cc
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#include "pds/xamps/XampsDestination.hh"
#include <stdio.h>

using namespace Pds::Xamps;

     char* XampsDestination::name()
     {
        static char* _names[NumberOf + 1] = {
         "External Register",
         "Internal Register",
         "Internal Register of lane 1"
         "--INVALID--"
       };
       return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
     }
