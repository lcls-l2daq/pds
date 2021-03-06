/*
 * EpixStatusRegisters.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef EPIXINTERNALREGISTERS_HH_
#define EPIXINTERNALREGISTERS_HH_

//#include <stdint.h>

namespace Pds {

  namespace Epix {

//   <status>
//    <register> <name>regStatus</name> <address>2</address> <lane>0</lane> <vc>2</vc> <size>1</size>
//       <field> <bits>0</bits> <label>LocLinkReady</label> </field>
//       <field> <bits>1</bits> <label>RemLinkReady</label> </field>
//       <field> <bits>2</bits> <label>PibLinkReady</label> </field>
//       <field> <bits>7:4</bits> <label>CntCellError</label> </field>
//       <field> <bits>15:12</bits> <label>CntLinkError</label> </field>
//       <field> <bits>19:16</bits> <label>CntLinkDown</label> </field>
//       <field> <bits>27:24</bits> <label>CntOverFlow</label> </field>
//    </register>
//      <register> <name>txCount</name> <address>3</address> <lane>0</lane> <vc>2</vc> <size>1</size> </register>
//      <register> <name>AdcChipIdReg</name> <address>49153</address> <lane>0</lane> <vc>1</vc> <size>1</size>
//         <field> <bits>7:0</bits><label>AdcChipId</label></field>
//      </register>
//      <register> <name>AdcChipGradeReg</name> <address>49154</address> <lane>0</lane> <vc>1</vc> <size>1</size>
//         <field> <bits>6:4</bits><label>AdcChipGrade</label></field>
//      </register>
//  </status>

    class StatusLane {
      public:
        StatusLane() {};
        ~StatusLane() {};
        void print();
      public:
        unsigned locLinkReady:          1;  //0
        unsigned remLinkReady:          1;  //1
        unsigned PibLinkReady:          1;  //2
        unsigned unused1:               1;  //3
        unsigned cellErrorCount:        4;  //7:4
        unsigned unused2:               4;  //11:8
        unsigned linkErrorCount:        4;  //15:12
        unsigned linkDownCount:         4;  //19:16
        unsigned unused3:               4;  //23:20
        unsigned bufferOverflowCount:   4;  //27:24
        unsigned unused4:               4;  //28:31
        unsigned txCounter;
    };

    class EpixStatusRegisters {
      public:
        EpixStatusRegisters() {};
        ~EpixStatusRegisters() {};

        void     print();
        enum {NumberOfLanes=1};

      public:
        unsigned        version;
        unsigned        scratchPad;
        StatusLane    lanes[NumberOfLanes];
    };

  }

}

#endif /* EPIXINTERNALREGISTERS_HH_ */
