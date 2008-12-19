#ifndef PDSEVGROPCODE_HH
#define PDSEVGROPCODE_HH

namespace Pds {

  class EvgrOpcode {
  public:
    enum Opcode {TsBit0=0x70, TsBit1=0x71, TsBitEnd=0x7d, L1Accept=0x8c, EndOfSequence=0x7f};
  };

}

#endif
