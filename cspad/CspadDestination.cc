/*
 * CspadDestination.cc
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#include "pds/cspad/CspadDestination.hh"
#include <stdio.h>

using namespace Pds::CsPad;

     char* CspadDestination::name()
     {
        static char* _names[NumberOf + 1] = {
         "Quad_0",
         "Quad_1",
         "Quad_2",
         "Quad_3",
         "Concentrator",
         "--INVALID--"
       };
       return (_dest < NumberOf ? _names[_dest] : _names[NumberOf]);
     }
