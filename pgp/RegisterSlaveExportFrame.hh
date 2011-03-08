/*
 * RegisterSlaveExportFrame.hh
 *
 *  Created on: Mar 29, 2010
 *      Author: jackp
 */

#ifndef PGPREGISTERSLAVEEXPORTFRAME_HH_
#define PGPREGISTERSLAVEEXPORTFRAME_HH_

#include <stdint.h>
#include <linux/types.h>
#include <fcntl.h>
#include "pds/pgp/PgpRSBits.hh"

namespace Pds {
  namespace Pgp {

    class RegisterSlaveExportFrame {
      public:
        enum {Success=0, Failure=1};
        enum FEdest {Q0, Q1, Q2, Q3, CR};
        enum masks {laneMask=2, concentratorMask=4, addrMask=(1<<24)-1};

        RegisterSlaveExportFrame(
            PgpRSBits::opcode,  //  opcode
            FEdest,             //  cspad destination
            unsigned,           //  address
            unsigned,           //  transaction ID
            uint32_t=0,         //  data
            PgpRSBits::waitState=PgpRSBits::notWaiting);

        RegisterSlaveExportFrame(
            PgpRSBits::opcode,   // opcode
            unsigned,            // lane
            unsigned,            // vc
            unsigned,            // address
            unsigned,            // transaction ID
            uint32_t=0,          // data
            PgpRSBits::waitState=PgpRSBits::notWaiting);

        ~RegisterSlaveExportFrame() {};

      public:
        static unsigned    count;
        static unsigned    errors;
        static void FileDescr(int i);

      private:
        static int             _fd;

      public:
        unsigned tid()                            {return bits._tid;}
        void waiting(PgpRSBits::waitState w)      {bits._waiting = w;}
        uint32_t* array()                         {return (uint32_t*)&_data;}
        unsigned lane()                           {return bits._lane;}
        void lane(unsigned l)                     {bits._lane = l & 3;}
        unsigned vc()                             {return bits._vc;}
        void vc(unsigned v)                       {bits._vc = v & 3;}
//        RegisterSlaveExportFrame::FEdest dest();
        unsigned post(__u32);  // the size of the entire header + payload in number of 32 bit words
        void print(unsigned = 0, unsigned = 4);


      public:
        PgpRSBits bits;
        uint32_t _data;
        uint32_t NotSupposedToCare;
    };

  }
}

#endif /* PGPREGISTERSLAVEEXPORTFRAME_HH_ */
