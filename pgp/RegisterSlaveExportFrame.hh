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

namespace Pds {
  namespace Pgp {

    class SEbits {
      public:
        unsigned vc:      2;    // 1:0
        unsigned mbz:     4;    // 5:2
        unsigned lane:    2;    // 7:6
        unsigned waiting: 1;    // 8
        unsigned tid:    23;    //31:9

        unsigned addr:   24;    //23:0
        unsigned dnc:     6;    //29:24
        unsigned oc:      2;    //31:30
    };

    class RegisterSlaveExportFrame {
      public:
        enum {Success=0, Failure=1};
        enum opcode {read, write, set, clear, numberOfOpCodes};
        enum waitState {notWaiting, Waiting};
        enum FEdest {Q0, Q1, Q2, Q3, CR};
        enum masks {laneMask=2, concentratorMask=4, addrMask=(1<<24)-1};

        RegisterSlaveExportFrame(opcode, FEdest, unsigned, unsigned, uint32_t=0, waitState=notWaiting);
        ~RegisterSlaveExportFrame() {};

      public:
        static unsigned    count;
        static unsigned    errors;
        static void FileDescr(int i);

      private:
        static int             _fd;

      public:
        unsigned tid() {return bits.tid;}
        void waiting(waitState w) {bits.waiting = w;}
        uint32_t* array() {return (uint32_t*)&_data;}
        void print(unsigned = 0, unsigned = 4);
        RegisterSlaveExportFrame::FEdest dest();
        unsigned post(__u32);  // the size of the entire header + payload in number of 32 bit words


      public:
        SEbits   bits;
        uint32_t _data;
        uint32_t NotSupposedToCare;
    };

  }
}

#endif /* PGPREGISTERSLAVEEXPORTFRAME_HH_ */
