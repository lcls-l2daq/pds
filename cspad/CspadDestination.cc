/*
 * CspadDestination.cc
 *
 *  Created on: Apr 5, 2011
 *      Author: jackp
 */

#include "pds/cspad/CspadDestination.hh"
#include <stdio.h>

using namespace Pds::CsPad;

     CspadDestination::FEdest Pds::CsPad::CspadDestination::getDest(
         Pds::Pgp::RegisterSlaveImportFrame* rsif) {
       unsigned ret = 0;
       if (rsif->bits._lane) ret |= laneMask;
       if (rsif->bits._vc & laneMask) ret |= 1;
       if (rsif->bits._vc==0) {
         ret = concentratorMask;
         if (rsif->bits._lane) {
           printf("ERROR ERROR bad dest lane(%u), vc(%u)\n", rsif->bits._lane, rsif->bits._vc);
         }
       }
       return (CspadDestination::FEdest) ret;
     }

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
