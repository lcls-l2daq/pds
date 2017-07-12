#include "JitterCleaner.hh"

#include <unistd.h>
#include <stdio.h>

using namespace Pds::Xpm;
using Pds::Cphw::Reg;

JitterCleaner::JitterCleaner()
{ dump(); }

void JitterCleaner::dump()
{
  printf("JitterCleaner\n");
  for(unsigned i=0; i<22; i++)
    printf("reg[%d]   %x\n", i,unsigned(_reg[i]));
}

void JitterCleaner::init()
{
#if 0
  _reg[0] = 0x3f0;
  _reg[1] = 0x3fc;
  _reg[2] = 0xff03;
  _reg[3] = 0x60;
  _reg[4] = 0x204d;
  // //  _reg[4] = 0x6f;
  _reg[5] = 0x23;
  _reg[6] = 0;
  _reg[7] = 0x23;
  _reg[8] = 0;
  _reg[9] = 0;
  _reg[12] = 0;
  _reg[15] = 0;
  _reg[18] = 0;
#else
  _reg[0] = 0x381;
  //  _reg[0] = 0x3b1;
  _reg[1] = 0xc;
  _reg[2] = 0x0f;
  _reg[3] = 0xf0;
  _reg[4] = 0x208f;
  _reg[5] = 0x23;
  _reg[6] = 3;
  _reg[7] = 0x23;
  _reg[8] = 3;
  _reg[9] = 1;
  _reg[12] = 1;
  _reg[15] = 1;
  _reg[18] = 1;
#endif
}
